#include "RenderSystem/CGraphicsContext.h"
#include "RenderSystem/CRenderSystem.h"

#include "CVkEngine.h"
#include "RenderSystem/Vulkan/CSwapChain.h"
#include "RenderSystem/CDeviceContext.h"
#include "RenderSystem/Vulkan/PrivateDescs.h"
#include "RenderSystem/Vulkan/Vulkan.h"
#include "Core/Utils/CLogger.h"

#include "Core/Memory/Memory.h"

#include "Core/Platform/CWindow.h"

#include "RenderSystem/Vulkan/CRenderQueue.h"

#include "Core/Utils/TCDynamicArray.h"

#include "Core/Threads/CThreadPool.h"

namespace VKE
{
    namespace RenderSystem
    {
        struct SDefaultGraphicsContextEventListener final : public EventListeners::IGraphicsContext
        {

        };
        static SDefaultGraphicsContextEventListener g_sDefaultGCListener;

        struct CGraphicsContext::SPrivate
        {
            SGraphicsContextPrivateDesc PrivateDesc;
            CSwapChain*				    pSwapChain = nullptr;                       
            bool					    needRenderFrame = false;
        };

        CGraphicsContext::CGraphicsContext(CDeviceContext* pCtx) :
            m_pDeviceCtx(pCtx)
            , m_VkDevice(pCtx->_GetDevice())
            , m_pEventListener(&g_sDefaultGCListener)
        {

        }

        CGraphicsContext::~CGraphicsContext()
        {
            Destroy();
        }

        void CGraphicsContext::Destroy()
        {
            if( m_pDeviceCtx )
            {
                m_pDeviceCtx->_NotifyDestroy(this);
                {
                    auto& vFences = m_Fences.vFences;
                    for( auto& vkFence : vFences )
                    {
                        m_VkDevice.DestroyObject( nullptr, &vkFence );
                    }

                    m_Fences.vFences.Clear<false>();
                    m_Fences.vFreeFences.Clear<false>();
                }

                m_pQueue = nullptr;
                m_pDeviceCtx = nullptr;
                m_presentDone = false;
                m_readyToPresent = false;
                Memory::DestroyObject( &HeapAllocator, &m_pPrivate->pSwapChain );
                Memory::DestroyObject( &HeapAllocator, &m_pPrivate );
            }
        }

        Result CGraphicsContext::Create(const SGraphicsContextDesc& Desc)
        {
            auto pPrivate = reinterpret_cast<SGraphicsContextPrivateDesc*>(Desc.pPrivate);
            VKE_RETURN_IF_FAILED(Memory::CreateObject(&HeapAllocator, &m_pPrivate));
            m_pPrivate->PrivateDesc = *pPrivate;
            auto& ICD = pPrivate->pICD->Device;
            m_pQueue = pPrivate->pQueue;

            {
                VkCommandPoolCreateInfo ci;
                Vulkan::InitInfo(&ci, VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO);
                ci.queueFamilyIndex = m_pPrivate->PrivateDesc.pQueue->familyIndex;
                ci.flags = 0;
                VK_ERR(m_VkDevice.CreateObject(ci, nullptr, &m_vkCommandPool));
            }
            {
                for( uint32_t i = 0; i < RenderQueueUsages::_MAX_COUNT; ++i )
                {
                    SCommnadBuffers& CBs = m_avCmdBuffers[ i ];
                    auto res = CBs.vCmdBuffers.Resize(DEFAULT_CMD_BUFFER_COUNT);

                    if( VKE_FAILED(_AllocateCommandBuffers(&CBs.vCmdBuffers)) )
                    {
                        return VKE_FAIL;
                    }
                    CBs.vFreeCmdBuffers = CBs.vCmdBuffers;
                }
            }
            {
                m_pCurrSubmit = _GetNextSubmit();
            }
            {
                CSwapChain* pSwpChain;
                if( VKE_FAILED(Memory::CreateObject(&HeapAllocator, &pSwpChain, this)) )
                {
                    return VKE_FAIL;
                }
                SSwapChainDesc SwpDesc = Desc.SwapChainDesc;
                SwpDesc.pPrivate = &m_pPrivate->PrivateDesc;
                if( VKE_FAILED(pSwpChain->Create(SwpDesc)) )
                {
                    return VKE_FAIL;
                }
            }
            // Tasks
            {
                auto pThreadPool = m_pDeviceCtx->GetRenderSystem()->GetEngine()->GetThreadPool();
                m_Tasks.Present.pCtx = this;
                pThreadPool->AddConstantTask(Constants::Threads::ID_BALANCED, &m_Tasks.Present);
            }
            // Create dummy queue
            //CreateGraphicsQueue({});
            return VKE_OK;
        }

        const Vulkan::ICD::Device& CGraphicsContext::_GetICD() const
        {
            return *m_pPrivate->PrivateDesc.pICD;
        }

        VkCommandBuffer CGraphicsContext::_CreateCommandBuffer(RENDER_QUEUE_USAGE usage)
        {
            auto& CBs = m_avCmdBuffers[ usage ];
            if( CBs.vFreeCmdBuffers.GetCount() )
            {
                VkCommandBuffer vkCb = CBs.vFreeCmdBuffers.Back();
                CBs.vFreeCmdBuffers.PopBack();
                return vkCb;
            }
            return VK_NULL_HANDLE;
        }

        void CGraphicsContext::_FreeCommandBuffer(RENDER_QUEUE_USAGE usage, const VkCommandBuffer& Cb)
        {
            m_avCmdBuffers[ usage ].vFreeCmdBuffers.PushBack(Cb);
        }

        Result CGraphicsContext::CreateSwapChain(const SSwapChainDesc& Desc)
        {
            CSwapChain* pSwapChain;
            if (VKE_FAILED(Memory::CreateObject(&HeapAllocator, &pSwapChain, this)))
            {
                VKE_LOG_ERR("No memory to create swap chain object");
                return VKE_ENOMEMORY;
            }
            Result err;
            SSwapChainDesc SwDesc = Desc;
            SwDesc.pPrivate = &m_pPrivate->PrivateDesc;
            if (VKE_FAILED((err = pSwapChain->Create(SwDesc))))
            {
                Memory::DestroyObject(&HeapAllocator, &pSwapChain);
                return err;
            }

            m_pPrivate->pSwapChain = pSwapChain;
            auto pWnd = m_pDeviceCtx->GetRenderSystem()->GetEngine()->FindWindow(Desc.hWnd);
            ////pWnd->SetRenderingContext(this);
            //pWnd->AddUpdateCallback([&](CWindow* pWnd)
            //{
            //    (void)pWnd;
            //    if( this->m_pPrivate->needRenderFrame )
            //    {
            //        if( this->_BeginFrame() )
            //        {
            //            this->_EndFrame();
            //        }
            //        this->m_pPrivate->needRenderFrame = false;
            //    }
            //});
            return VKE_OK;
        }

        void CGraphicsContext::RenderFrame()
        {
            m_pPrivate->needRenderFrame = true;
        }

        bool CGraphicsContext::_BeginFrame()
        {
            return true;
        }

        void CGraphicsContext::_EndFrame()
        {
            
        }

        bool CGraphicsContext::PresentFrame()
        {
            auto pSwapChain = m_pPrivate->pSwapChain;
            if( m_readyToPresent && pSwapChain )
            {
                /*auto presentCount = m_PresentData.presentCount;

                VkPresentInfoKHR pi;
                Vulkan::InitInfo(&pi, VK_STRUCTURE_TYPE_PRESENT_INFO_KHR);
                {
                    Threads::ScopedLock l(m_PresentData.m_SyncObj);
                    pi.pSwapchains = &m_PresentData.vSwapChains[ 0 ];
                    pi.swapchainCount = m_PresentData.vSwapChains.GetCount();
                    pi.pWaitSemaphores = &m_PresentData.vWaitSemaphores[ 0 ];
                    pi.waitSemaphoreCount = m_PresentData.vWaitSemaphores.GetCount();
                    pi.pImageIndices = &m_PresentData.vImageIndices[ 0 ];
                    pi.pResults = nullptr;
                    m_PresentData.Clear();
                }
                m_pQueue->Lock();
                VkQueue vkQueue = m_pQueue->vkQueue;
                VK_ERR(m_VkDevice.QueuePresentKHR(vkQueue, pi));
                m_pQueue->Unlock();*/
                auto& BackBuffer = pSwapChain->_GetCurrentBackBuffer();
                assert(m_pEventListener);
                if( m_pQueue->WillNextSwapchainDoPresent() )
                {
                    m_pEventListener->OnBeforePresent(this);
                }
                auto res = m_pQueue->Present(m_VkDevice.GetICD(), pSwapChain->_GetCurrentImageIndex(),
                                             pSwapChain->_GetSwapChain(), BackBuffer.vkCmdBufferSemaphore);
                m_presentDone = res == VKE_OK;
                if( m_presentDone )
                {
                    m_readyToPresent = false;
                    m_pEventListener->OnAfterPresent(this);
                    pSwapChain->SwapBuffers();
                }
                             
                m_pEventListener->OnBeginFrame(this);
            }
            return m_presentDone;
        }

        void CGraphicsContext::Resize(uint32_t width, uint32_t height)
        {
            m_pPrivate->pSwapChain->Resize(width, height);
        }

        void CGraphicsContext::_AddToPresent(CSwapChain* pSwapChain)
        {
           
        }

        CRenderQueue* CGraphicsContext::CreateRenderQueue(const SRenderQueueDesc& Desc)
        {
            // Add command buffer to the current batch
            CRenderQueue* pRQ;
            if( VKE_FAILED(Memory::CreateObject(&HeapAllocator, &pRQ, this)) )
            {
                VKE_LOG_ERR("Unable to create CRenderQueue object. No memory.");
                return nullptr;
            }
            if( VKE_FAILED(pRQ->Create(Desc)) )
            {
                Memory::DestroyObject(&HeapAllocator, &pRQ);
                return nullptr;
            }
            _EnableRenderQueue(pRQ, true);
            m_vpRenderQueues.PushBack(pRQ);
            return pRQ;
        }

        void CGraphicsContext::_EnableRenderQueue(CRenderQueue* pRQ, bool enable)
        {
            assert(m_pCurrSubmit);
            if( enable )
            {
                m_enabledRenderQueueCount++;
            }
            else
            {
                m_enabledRenderQueueCount--;
            }
        }

        CGraphicsContext::SSubmit* CGraphicsContext::_GetNextSubmit()
        {
            if( !m_lSubmits.empty() )
            {
                auto& FirstSubmit = m_lSubmits.front();
                if( m_VkDevice.IsFenceReady(FirstSubmit.vkFence) )
                {
                    VK_ERR(m_VkDevice.ResetFences(1, &FirstSubmit.vkFence));
                    _FreeCommandBuffers(RenderQueueUsages::DYNAMIC, FirstSubmit.vCmdBuffers);
                    FirstSubmit.Reset();
                    m_lSubmits.push_back(FirstSubmit);
                    m_lSubmits.pop_front();
                    return &FirstSubmit;
                }
                else
                {
                    goto ADD_NEW;
                }
            }
            else
            {
                goto ADD_NEW;
            }
         
            ADD_NEW:
            SSubmit Submit;
            Submit.vkFence = _CreateFence();
            m_lSubmits.push_back(Submit);
            return &m_lSubmits.back();
        }

        VkCommandBuffer CGraphicsContext::_GetNextCommandBuffer(RENDER_QUEUE_USAGE usage)
        {
            auto& vCmdBuffers = m_avCmdBuffers[ usage ];
            VkCommandBuffer vkCb;
            if( !vCmdBuffers.vFreeCmdBuffers.PopBack(&vkCb) )
            {
                CommandBufferArray vTmp;
                vTmp.Resize(DEFAULT_CMD_BUFFER_COUNT);
                if( VKE_SUCCEEDED( _AllocateCommandBuffers(&vTmp) ) )
                {
                    vCmdBuffers.vFreeCmdBuffers = vTmp;
                    vCmdBuffers.vCmdBuffers.Append(vTmp);
                    return _GetNextCommandBuffer(usage);
                }
                return VK_NULL_HANDLE;
            }
            return vkCb;
        }

        Result CGraphicsContext::_AllocateCommandBuffers(CommandBufferArray* pOut)
        {
            auto& vTmp = *pOut;
            VkCommandBufferAllocateInfo ai;
            Vulkan::InitInfo(&ai, VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO);
            ai.commandBufferCount = vTmp.GetCount();
            ai.commandPool = m_vkCommandPool;
            ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            VK_ERR( m_VkDevice.AllocateObjects( ai, &vTmp[0] ) );
            return VKE_OK;
        }

        void CGraphicsContext::_FreeCommandBuffers(RENDER_QUEUE_USAGE usage, const CommandBufferArray& vCbs)
        {
            auto& vFree = m_avCmdBuffers[ usage ].vFreeCmdBuffers;
            for( uint32_t i = 0; i < vCbs.GetCount(); ++i )
            {
                vFree.PushBack(vCbs[ i ]);
            }
        }

        Result CGraphicsContext::ExecuteRenderQueue(CRenderQueue* pRQ)
        {
            Threads::ScopedLock l(m_SyncObj);
            assert(m_pCurrSubmit);
            if( pRQ->IsEnabled() )
            {
                m_pCurrSubmit->vCmdBuffers.PushBack(pRQ->_GetCommandBuffer());
                bool readyToExecute = m_pCurrSubmit->vCmdBuffers.GetCount() == m_enabledRenderQueueCount;
                m_pCurrSubmit->readyToExecute = readyToExecute;
                if( readyToExecute )
                {
                    _ExecuteSubmit(m_pCurrSubmit);
                    m_pCurrSubmit = _GetNextSubmit();
                    // Add swapchain transition static buffer
                    // present src -> color attachment
                    //m_pCurrSubmit->vCmdBuffers.PushBack(m_psw)
                }
            }
            return VKE_OK;
        }

        void CGraphicsContext::_ExecuteSubmit(SSubmit* pSubmit)
        {
            //m_pQueue->Lock();
            // Call end frame event
            assert(m_pEventListener);
            m_pEventListener->OnEndFrame(this);
            // Submit command buffers
            assert(pSubmit);
            VkQueue vkQueue = m_pQueue->vkQueue;
            /*auto& vCbs = pSubmit->vCmdBuffers;
            VkSubmitInfo si;
            Vulkan::InitInfo(&si, VK_STRUCTURE_TYPE_SUBMIT_INFO);
            si.commandBufferCount = vCbs.GetCount();
            si.pCommandBuffers = &vCbs[ 0 ];
            si.pSignalSemaphores = nullptr;
            si.pWaitDstStageMask = nullptr;
            si.pWaitSemaphores = nullptr;
            si.signalSemaphoreCount = 0;
            si.waitSemaphoreCount = 0;
            VK_ERR(m_VkDevice.GetICD().vkQueueSubmit(vkQueue, 1, &si, pSubmit->vkFence));*/
            // Add swap chain transition static command buffer
            // color attachment -> present src
            m_pEventListener->OnBeforeExecute(this);
            _SubmitCommandBuffers(pSubmit->vCmdBuffers, pSubmit->vkFence);
            m_readyToPresent = true;
            m_pEventListener->OnAfterExecute(this);
            //m_pQueue->Unlock();
        }

        void CGraphicsContext::_SubmitCommandBuffers(const CommandBufferArray& vCmdBuffers, VkFence vkFence)
        {
            VkQueue vkQueue = m_pQueue->vkQueue;
            VkSubmitInfo si;
            Vulkan::InitInfo(&si, VK_STRUCTURE_TYPE_SUBMIT_INFO);
            si.commandBufferCount = vCmdBuffers.GetCount();
            si.pCommandBuffers = &vCmdBuffers[0];
            si.pSignalSemaphores = nullptr;
            si.pWaitDstStageMask = nullptr;
            si.pWaitSemaphores = nullptr;
            si.signalSemaphoreCount = 0;
            si.waitSemaphoreCount = 0;
            //VK_ERR(m_VkDevice.GetICD().vkQueueSubmit(vkQueue, 1, &si, vkFence));
            m_pQueue->Submit(m_VkDevice.GetICD(), si, vkFence);
        }

        VkFence CGraphicsContext::_CreateFence()
        {
            VkFence vkFence;
            if( !m_Fences.vFreeFences.PopBack( &vkFence ) )
            {
                const auto count = m_Fences.vFences.GetMaxCount();
                for( uint32_t i = 0; i < count; ++i )
                {
                    VkFence vkFence;
                    VkFenceCreateInfo ci;
                    Vulkan::InitInfo(&ci, VK_STRUCTURE_TYPE_FENCE_CREATE_INFO);
                    ci.flags = 0;
                    VK_ERR( m_VkDevice.CreateObject(ci, nullptr, &vkFence) );
                    m_Fences.vFences.PushBack(vkFence);
                    m_Fences.vFreeFences.PushBack(vkFence);
                }
                
                return _CreateFence();
            }
            return vkFence;
        }

        VkInstance CGraphicsContext::_GetInstance() const
        {
            return m_pDeviceCtx->_GetInstance();
        }

        void CGraphicsContext::SetEventListener(EventListeners::IGraphicsContext* pListener)
        {
            m_pEventListener = pListener;
        }

        void CGraphicsContext::Wait()
        {
            m_pQueue->Lock();
            m_VkDevice.GetICD().vkQueueWaitIdle(m_pQueue->vkQueue);
            m_pQueue->Unlock();
        }     

    } // RenderSystem
} // VKE