#include "RenderSystem/CGraphicsContext.h"
#include "RenderSystem/CRenderSystem.h"
#include "RenderSystem/Vulkan/CDevice.h"
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

namespace VKE
{
    namespace RenderSystem
    {
        struct CGraphicsContext::SPrivate
        {
            SGraphicsContextPrivateDesc PrivateDesc;
            CSwapChain*				    pSwapChain = nullptr;                       
            bool					    needRenderFrame = false;
        };

        CGraphicsContext::CGraphicsContext(CDeviceContext* pCtx) :
            m_pDeviceCtx(pCtx),
            m_VkDevice(pCtx->_GetDevice())
            //, m_pDeviceCtxCtx(pDevice->_GetDeviceContext())
        {

        }

        CGraphicsContext::~CGraphicsContext()
        {
            Destroy();
        }

        void CGraphicsContext::Destroy()
        {
            {
                auto& vFences = m_Fences.vFences;
                for( auto& vkFence : vFences )
                {
                    m_VkDevice.DestroyObject(nullptr, &vkFence);
                }

                m_Fences.vFences.Clear<false>();
                m_Fences.vFreeFences.Clear<false>();
            }

            Memory::DestroyObject(&HeapAllocator, &m_pPrivate->pSwapChain);
            Memory::DestroyObject(&HeapAllocator, &m_pPrivate);
        }

        Result CGraphicsContext::Create(const SGraphicsContextDesc& Desc)
        {
            auto pPrivate = reinterpret_cast<SGraphicsContextPrivateDesc*>(Desc.pPrivate);
            VKE_RETURN_IF_FAILED(Memory::CreateObject(&HeapAllocator, &m_pPrivate));
            m_pPrivate->PrivateDesc = *pPrivate;
            auto& ICD = pPrivate->pICD->Device;

            {
                VkCommandPoolCreateInfo ci;
                Vulkan::InitInfo(&ci, VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO);
                ci.queueFamilyIndex = m_pPrivate->PrivateDesc.Queue.familyIndex;
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
                const auto& SwapChains = Desc.SwapChains;
                if( SwapChains.count )
                {
                    for( uint32_t i = 0; i < SwapChains.count; ++i )
                    {
                        SSwapChainDesc Desc = SwapChains[ i ];
                        Desc.pPrivate = &m_pPrivate->PrivateDesc;
                        VKE_RETURN_IF_FAILED(CreateSwapChain(Desc));
                    }
                }
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
            //pWnd->SetRenderingContext(this);
            pWnd->AddUpdateCallback([&](CWindow* pWnd)
            {
                (void)pWnd;
                if( this->m_pPrivate->needRenderFrame )
                {
                    if( this->_BeginFrame() )
                    {
                        this->_EndFrame();
                    }
                    this->m_pPrivate->needRenderFrame = false;
                }
            });
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

        void CGraphicsContext::PresentFrame()
        {
            auto presentCount = m_PresentData.presentCount;

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
            VkQueue vkQueue = m_pPrivate->PrivateDesc.Queue.vkQueue;

            VK_ERR(m_VkDevice.QueuePresentKHR(vkQueue, pi));
        }

        void CGraphicsContext::Resize(uint32_t width, uint32_t height)
        {
            (void)width;
            (void)height;
        }

        void CGraphicsContext::_AddToPresent(CSwapChain* pSwapChain)
        {
            Threads::ScopedLock l(m_PresentData.m_SyncObj);
            m_PresentData.vImageIndices.PushBack(pSwapChain->_GetCurrentImageIndex());
            m_PresentData.vSwapChains.PushBack(pSwapChain->_GetSwapChain());
            m_PresentData.vWaitSemaphores.PushBack(pSwapChain->_GetCurrentBackBuffer().vkCmdBufferSemaphore);
            m_PresentData.presentCount++;
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
            return nullptr;
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
                }
            }
            return VKE_OK;
        }

        void CGraphicsContext::_ExecuteSubmit(SSubmit* pSubmit)
        {
            assert(pSubmit);
            VkQueue vkQueue = m_pPrivate->PrivateDesc.Queue.vkQueue;
            auto& vCbs = pSubmit->vCmdBuffers;
            VkSubmitInfo si;
            Vulkan::InitInfo(&si, VK_STRUCTURE_TYPE_SUBMIT_INFO);
            si.commandBufferCount = vCbs.GetCount();
            si.pCommandBuffers = &vCbs[ 0 ];
            si.pSignalSemaphores = nullptr;
            si.pWaitDstStageMask = nullptr;
            si.pWaitSemaphores = nullptr;
            si.signalSemaphoreCount = 0;
            si.waitSemaphoreCount = 0;
            VK_ERR(m_VkDevice.GetICD().vkQueueSubmit(vkQueue, vCbs.GetCount(), &si, pSubmit->vkFence));
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

    } // RenderSystem
} // VKE