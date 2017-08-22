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

#include "RenderSystem/Vulkan/Wrappers/CCommandBuffer.h"
#include "Core/Utils/CTimer.h"

namespace VKE
{
#define VKE_THREAD_SAFE true
#define VKE_NOT_THREAD_SAFE false

    namespace RenderSystem
    {
        struct SDefaultGraphicsContextEventListener final : public EventListeners::IGraphicsContext
        {

        };
        static SDefaultGraphicsContextEventListener g_sDefaultGCListener;

        struct CGraphicsContext::SPrivate
        {
            SGraphicsContextPrivateDesc PrivateDesc;
            bool					    needRenderFrame = false;
        };

        CGraphicsContext::CGraphicsContext(CDeviceContext* pCtx) :
            m_pDeviceCtx(pCtx)
            , m_VkDevice(pCtx->_GetDevice())
            , m_pEventListener(&g_sDefaultGCListener)
            , m_CmdBuffMgr(this)
            , m_SubmitMgr(this)
        {

        }

        CGraphicsContext::~CGraphicsContext()
        {
            Destroy();
        }

        void CGraphicsContext::Destroy()
        {
            m_VkDevice.Wait();

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
                Memory::DestroyObject( &HeapAllocator, &m_pSwapChain );
                Memory::DestroyObject( &HeapAllocator, &m_pPrivate );
            }
        }

        Result CGraphicsContext::Create(const SGraphicsContextDesc& Desc)
        {
            auto pPrivate = reinterpret_cast<SGraphicsContextPrivateDesc*>(Desc.pPrivate);
            VKE_RETURN_IF_FAILED(Memory::CreateObject(&HeapAllocator, &m_pPrivate));
            m_pPrivate->PrivateDesc = *pPrivate;
            //auto& ICD = pPrivate->pICD->Device;
            m_pQueue = pPrivate->pQueue;

            {
                VkCommandPoolCreateInfo ci;
                Vulkan::InitInfo(&ci, VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO);
                ci.queueFamilyIndex = m_pPrivate->PrivateDesc.pQueue->familyIndex;
                ci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
                VK_ERR(m_VkDevice.CreateObject(ci, nullptr, &m_vkCommandPool));
            }
            {
                for( uint32_t i = 0; i < RenderQueueUsages::_MAX_COUNT; ++i )
                {
                    SCommnadBuffers& CBs = m_avCmdBuffers[ i ];
                    auto& vCmdBuffers = CBs.vCmdBuffers;
                    // This should not fail as it is allocated on stack
                    vCmdBuffers.Resize(vCmdBuffers.GetMaxCount());

                    if( VKE_FAILED(_AllocateCommandBuffers(&CBs.vCmdBuffers)) )
                    {
                        return VKE_FAIL;
                    }
                    CBs.vFreeCmdBuffers = CBs.vCmdBuffers;
                }
            }
            {
                SCommandBufferManagerDesc Desc;
                VKE_RETURN_IF_FAILED(m_CmdBuffMgr.Create(Desc));
                {
                    SCommandPoolDesc Desc;
                    Desc.commandBufferCount = CCommandBufferManager::DEFAULT_COMMAND_BUFFER_COUNT; /// @todo hardcode...
                    /// @todo store command pool handle
                    if( m_CmdBuffMgr.CreatePool(Desc) == NULL_HANDLE )
                    {
                        return VKE_FAIL;
                    }
                }
            }
            {
                SSubmitManagerDesc Desc;
                VKE_RETURN_IF_FAILED(m_SubmitMgr.Create(Desc));
            }
            {
                if( VKE_FAILED(Memory::CreateObject(&HeapAllocator, &m_pSwapChain, this)) )
                {
                    return VKE_FAIL;
                }
                SSwapChainDesc SwpDesc = Desc.SwapChainDesc;
                SwpDesc.pPrivate = &m_pPrivate->PrivateDesc;
                if( VKE_FAILED(m_pSwapChain->Create(SwpDesc)) )
                {
                    return VKE_FAIL;
                }
                
            }
            // Tasks
            {
                auto pThreadPool = m_pDeviceCtx->GetRenderSystem()->GetEngine()->GetThreadPool();
                m_Tasks.Present.pCtx = this;
                m_Tasks.BeginFrame.pCtx = this;
                m_Tasks.EndFrame.pCtx = this;
                m_Tasks.BeginFrame.SetNextTask(&m_Tasks.EndFrame, true);
                m_Tasks.EndFrame.SetNextTask(&m_Tasks.Present);
                m_Tasks.Present.SetNextTask(&m_Tasks.BeginFrame);
                pThreadPool->AddConstantTask(Constants::Threads::ID_BALANCED, &m_Tasks.BeginFrame);
                pThreadPool->AddConstantTask(Constants::Threads::ID_BALANCED, &m_Tasks.EndFrame);
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

        VkCommandBuffer CGraphicsContext::_CreateCommandBuffer()
        {
            VkCommandBuffer vkCb;
            m_CmdBuffMgr.CreateCommandBuffers< VKE_THREAD_SAFE >(1, &vkCb);
            return vkCb;
        }

        void CGraphicsContext::_FreeCommandBuffer(const VkCommandBuffer& vkCb)
        {
            m_CmdBuffMgr.FreeCommandBuffers< VKE_THREAD_SAFE >(1, &vkCb);
        }

       

        void CGraphicsContext::RenderFrame()
        {
            /*if(_BeginFrame())
            {
                _EndFrame();
                m_pPrivate->needRenderFrame = true;
            }*/
        }

        bool CGraphicsContext::_BeginFrame()
        {
            CurrentTask CurrTask = _GetCurrentTask();
            //if(CurrTask == ContextTasks::BEGIN_FRAME)
            {
                if( m_pSwapChain && m_presentDone )
                {
                    auto& BackBuffer = m_pSwapChain->_GetCurrentBackBuffer();
                    CSubmit* pSubmit = _GetNextSubmit(3, BackBuffer.vkAcquireSemaphore);
                    pSubmit->SubmitStatic(m_pSwapChain->m_pCurrAcquireElement->vkCbPresentToAttachment);
                }
                if( m_pEventListener->OnBeginFrame(this) )
                {
                    _SetCurrentTask(ContextTasks::END_FRAME);
                    return true;
                }
                return false;
            }
            return true;
        }

        void CGraphicsContext::_EndFrame()
        {
            CurrentTask CurrTask = _GetCurrentTask();
            //if(CurrTask == ContextTasks::END_FRAME)
            {
                m_pEventListener->OnEndFrame(this);
                if( m_pSwapChain && m_presentDone )
                {
                    auto& BackBuffer = m_pSwapChain->_GetCurrentBackBuffer();
                    CSubmit* pSubmit = m_SubmitMgr.GetCurrentSubmit();
                    
                    VkCommandBuffer vkCb = _CreateCommandBuffer();
                    Vulkan::Wrapper::CCommandBuffer Cb(m_VkDevice.GetICD(), vkCb);
                    Cb.Reset(VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
                    Cb.Begin();
                    //m_pSwapChain->BeginPass(vkCb);
                    //m_pSwapChain->EndPass(vkCb);
                    m_pSwapChain->GetRenderTarget()->Begin(vkCb);
                    m_pSwapChain->GetRenderTarget()->End(vkCb);
                    Cb.End();
                    pSubmit->Submit(vkCb);

                    pSubmit->SubmitStatic(m_pSwapChain->m_pCurrAcquireElement->vkCbAttachmentToPresent);
                }
                _SetCurrentTask(ContextTasks::PRESENT);
            }
        }

        bool CGraphicsContext::PresentFrame()
        {
            CurrentTask CurrTask = _GetCurrentTask();
            bool ret = false;
            if( !m_needQuit )
            {
                if( m_readyToPresent && m_pSwapChain )
                {
                    assert(m_pEventListener);
                    if( m_pQueue->WillNextSwapchainDoPresent() )
                    {
                        m_pEventListener->OnBeforePresent(this);
                    }
                    auto& BackBuffer = m_pSwapChain->_GetCurrentBackBuffer();
                    CSubmit* pSubmit = m_SubmitMgr.GetCurrentSubmit();
                    const auto res = m_pQueue->Present(m_VkDevice.GetICD(), m_pSwapChain->_GetCurrentImageIndex(),
                        m_pSwapChain->_GetSwapChain(), pSubmit->GetSignaledSemaphore());
                    m_presentDone = res == VKE_OK;
                    if (m_presentDone)
                    {
                        m_readyToPresent = false;
                        m_pEventListener->OnAfterPresent(this);
                        m_pSwapChain->SwapBuffers();
                    }
                }
                
                ret = true;
            }
 
            return ret;
        }

        void CGraphicsContext::Resize(uint32_t width, uint32_t height)
        {
            m_pSwapChain->Resize(width, height);
        }

        void CGraphicsContext::_AddToPresent(CSwapChain* /*pSwapChain*/)
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

        void CGraphicsContext::_EnableRenderQueue(CRenderQueue* /*pRQ*/, bool enable)
        {
            if( enable )
            {
                m_enabledRenderQueueCount++;
            }
            else
            {
                m_enabledRenderQueueCount--;
            }
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

        Result CGraphicsContext::ExecuteRenderQueue(CRenderQueue* pRQ)
        {
            Threads::ScopedLock l(m_SyncObj);
            if( pRQ->IsEnabled() )
            {
                
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
            //VkQueue vkQueue = m_pQueue->vkQueue;
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
            //VkQueue vkQueue = m_pQueue->vkQueue;
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

        VkFence CGraphicsContext::_CreateFence(VkFenceCreateFlags flags)
        {
            VkFence vkFence;
            VkFenceCreateInfo ci;
            Vulkan::InitInfo(&ci, VK_STRUCTURE_TYPE_FENCE_CREATE_INFO);
            ci.flags = flags;
            if( !m_Fences.vFreeFences.PopBack( &vkFence ) )
            {
                const auto count = m_Fences.vFences.GetMaxCount();
                for( uint32_t i = 0; i < count; ++i )
                {                         
                    VK_ERR( m_VkDevice.CreateObject(ci, nullptr, &vkFence) );
                    m_Fences.vFences.PushBack(vkFence);
                    m_Fences.vFreeFences.PushBack(vkFence);
                }
                
                return _CreateFence(flags);
            }
            return vkFence;
        }

        void CGraphicsContext::_DestroyFence(VkFence* pVkFence)
        {
            m_VkDevice.DestroyObject(nullptr, pVkFence);
        }

        VkSemaphore CGraphicsContext::_CreateSemaphore()
        {
            VkSemaphoreCreateInfo ci;
            Vulkan::InitInfo(&ci, VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO);
            ci.flags = 0;
            VkSemaphore vkSemaphore;
            VK_ERR(m_VkDevice.CreateObject(ci, nullptr, &vkSemaphore));
            return vkSemaphore;
        }

        void CGraphicsContext::_DestroySemaphore(VkSemaphore* pVkSemaphore)
        {
            m_VkDevice.DestroyObject(nullptr, pVkSemaphore);
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