#include "RenderSystem/CGraphicsContext.h"
#include "RenderSystem/CRenderSystem.h"

#include "CVkEngine.h"



#include "Core/Utils/CLogger.h"

#include "Core/Memory/Memory.h"

#include "Core/Platform/CWindow.h"

#include "Core/Utils/TCDynamicArray.h"
#include "Core/Utils/CTimer.h"

#include "Core/Threads/CThreadPool.h"

#include "RenderSystem/CDeviceContext.h"
#include "RenderSystem/Vulkan/CRenderPass.h"
#include "RenderSystem/Vulkan/Wrappers/CCommandBuffer.h"
#include "RenderSystem/Vulkan/CSwapChain.h"
#include "RenderSystem/Vulkan/PrivateDescs.h"
#include "RenderSystem/Vulkan/Vulkan.h"
#include "RenderSystem/Vulkan/CRenderQueue.h"
#include "Core/Threads/CTaskGroup.h"
#include "RenderSystem/Vulkan/CRenderingPipeline.h"
#include "RenderSystem/Managers/CBackBufferManager.h"
#include "RenderSystem/Vulkan/CResourceBarrierManager.h"
#include "RenderSystem/Managers/CShaderManager.h"

namespace VKE
{
#define VKE_THREAD_SAFE true
#define VKE_NOT_THREAD_SAFE false

    namespace RenderSystem
    {

        struct SDefaultGraphicsContextEventListener final : public EventListeners::IGraphicsContext
        {
            bool OnRenderFrame(CGraphicsContext* pCtx) override
            {
                auto pCb = pCtx->CreateCommandBuffer();
                auto pSwapChain = pCtx->GetSwapChain();
                auto& BackBuffer = pSwapChain->GetCurrentBackBuffer();
                auto pSubmit = pCtx->GetNextSubmit( 1, BackBuffer.vkAcquireSemaphore );

                pCb->Begin();

                pSwapChain->BeginFrame( pCb->GetNative() );
                pSwapChain->GetRenderPass()->Begin( pCb->GetNative() );
                pSwapChain->GetRenderPass()->End( pCb->GetNative() );
                pSwapChain->EndFrame( pCb->GetNative() );
                
                pCb->End();
                pSubmit->Submit( pCb );
                return true;
            }
        };
        static SDefaultGraphicsContextEventListener g_sDefaultGCListener;

        struct CGraphicsContext::SPrivate
        {
            SGraphicsContextPrivateDesc PrivateDesc;
            bool					    needRenderFrame = false;
        };

        struct STaskGroup
        {
            struct STask : public Threads::ITask
            {
                STask()
                {
                    VKE_DEBUG_CODE( this->m_dbgType = 123 );
                }

                TaskState _OnStart(uint32_t) override
                {
                    Platform::ThisThread::Sleep(1);
                    return TaskStateBits::NOT_ACTIVE;
                }
            };

            Threads::CTaskGroup m_Group;
            STask m_aTasks[ 7 ];

            STaskGroup()
            {
                for( uint32_t i = 0; i < 7; ++i )
                {
                    m_Group.AddTask(&m_aTasks[ i ]);
                }
            }
        };
        STaskGroup g_TaskGrp;

        CGraphicsContext::CGraphicsContext(CDeviceContext* pCtx) :
            m_pDeviceCtx( pCtx )
            , m_VkDevice( pCtx->_GetDevice() )
            , m_pEventListener( &g_sDefaultGCListener )
            , m_CmdBuffMgr( this )
            , m_PipelineMgr( pCtx )
            , m_SubmitMgr( this )
        {
            static uint32_t instanceId = 0;
            m_instnceId = ++instanceId;
        }

        CGraphicsContext::~CGraphicsContext()
        {
            _Destroy();
        }

        void CGraphicsContext::Destroy()
        {
            assert(m_pDeviceCtx);
            if( m_pQueue )
            {
                CGraphicsContext* pCtx = this;
                m_pDeviceCtx->DestroyGraphicsContext(&pCtx);
            }
        }

        void CGraphicsContext::_Destroy()
        {
            if( m_pDeviceCtx && m_pQueue )
            {
                Threads::ScopedLock l(m_SyncObj);

                m_needQuit = true;
                FinishRendering();

                Memory::DestroyObject( &HeapAllocator, &m_pDefaultRenderingPipeline );
                Memory::DestroyObject( &HeapAllocator, &m_pSwapChain );

                m_PipelineMgr.Destroy();
                m_SubmitMgr.Destroy();
                m_CmdBuffMgr.Destroy();

                //printf( "destroy graphics context\n" );
                m_readyToPresent = false;

                _DestroyObjects( &m_Fences );
                _DestroyObjects( &m_Semaphores );

                Memory::DestroyObject(&HeapAllocator, &m_pPrivate);
                m_pDeviceCtx->_NotifyDestroy(this);
            }
        }

        void CGraphicsContext::FinishRendering()
        {
            const bool waitForFinish = true;
            m_Tasks.RenderFrame.Remove< waitForFinish, THREAD_SAFE >();
            m_Tasks.Present.Remove< waitForFinish, THREAD_SAFE >();
            m_Tasks.SwapBuffers.Remove< waitForFinish, THREAD_SAFE >();

            m_pQueue->Lock();
            m_pQueue->Wait( m_VkDevice.GetICD() );
            m_pQueue->Unlock();
        }

        Result CGraphicsContext::Create(const SGraphicsContextDesc& Desc)
        {
            Result res = VKE_FAIL;
            auto pPrivate = reinterpret_cast<SGraphicsContextPrivateDesc*>(Desc.pPrivate);
            VKE_RETURN_IF_FAILED( Memory::CreateObject( &HeapAllocator, &m_pPrivate ) );
            m_pPrivate->PrivateDesc = *pPrivate;
            //auto& ICD = pPrivate->pICD->Device;
            m_pQueue = pPrivate->pQueue;

            {
                VkCommandPoolCreateInfo ci;
                Vulkan::InitInfo( &ci, VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO );
                ci.queueFamilyIndex = m_pPrivate->PrivateDesc.pQueue->familyIndex;
                ci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
                VK_ERR( m_VkDevice.CreateObject( ci, nullptr, &m_vkCommandPool ) );
            }
            {
                for( uint32_t i = 0; i < RenderQueueUsages::_MAX_COUNT; ++i )
                {
                    SCommandBuffers& CBs = m_avCmdBuffers[ i ];
                    auto& vCmdBuffers = CBs.vPool;
                    // This should not fail as it is allocated on stack
                    vCmdBuffers.Resize( vCmdBuffers.GetMaxCount() );

                    if( VKE_FAILED( _AllocateCommandBuffers( &CBs.vPool[ 0 ], CBs.vPool.GetCount() ) ) )
                    {
                        goto ERR;
                    }
                    CBs.vFreeElements = CBs.vPool;
                }
            }
            {
                SCommandBufferManagerDesc Desc;
                if( VKE_SUCCEEDED( m_CmdBuffMgr.Create( Desc ) ) )
                {
                    SCommandPoolDesc Desc;
                    Desc.commandBufferCount = CCommandBufferManager::DEFAULT_COMMAND_BUFFER_COUNT; /// @todo hardcode...
                    /// @todo store command pool handle
                    if( m_CmdBuffMgr.CreatePool(Desc) == NULL_HANDLE )
                    {
                        goto ERR;
                    }
                }
            }
            {
                SSubmitManagerDesc Desc;
                if( VKE_FAILED( m_SubmitMgr.Create( Desc ) ) )
                {
                    goto ERR;
                }
            }
            {
                SSwapChainDesc SwpDesc = Desc.SwapChainDesc;
                if( VKE_FAILED( Memory::CreateObject( &HeapAllocator, &m_pSwapChain, this ) ) )
                {
                    goto ERR;
                }
                
                SwpDesc.pPrivate = &m_pPrivate->PrivateDesc;
                if( VKE_FAILED( m_pSwapChain->Create( SwpDesc ) ) )
                {
                    goto ERR;
                }

                SwpDesc.pWindow->AddDestroyCallback( [ & ](CWindow*)
                {
                    this->Destroy();
                });
                SwpDesc.pWindow->AddShowCallback( [ this ](CWindow* pWnd)
                {
                    if( pWnd->IsVisible() )
                    {
                        this->m_Tasks.RenderFrame.IsActive(true);
                    }
                });
				SwpDesc.pWindow->SetSwapChain( m_pSwapChain );
            }
            {
                SRenderingPipelineDesc Desc;
                VKE_RENDER_SYSTEM_DEBUG_CODE( Desc.pDebugName = "Default" );
                SRenderingPipelineDesc::SPassDesc PassDesc;
                PassDesc.OnRender = [&](const SRenderingPipelineDesc::SPassDesc& /*PassDesc*/)
                {
                    
                };
                m_pDefaultRenderingPipeline = _CreateRenderingPipeline( Desc );
                m_pCurrRenderingPipeline = m_pDefaultRenderingPipeline;
            }
            {
                SPipelineManagerDesc Desc;
                Desc.maxPipelineCount = Config::RenderSystem::Pipeline::MAX_PIPELINE_COUNT;

                if( VKE_FAILED( m_PipelineMgr.Create( Desc ) ) )
                {
                    goto ERR;
                }
            }
            
            // Tasks
            {
                static uint32_t taskIdx = 123;
                auto pThreadPool = m_pDeviceCtx->GetRenderSystem()->GetEngine()->GetThreadPool();
                m_Tasks.Present.pCtx = this;
                m_Tasks.RenderFrame.pCtx = this;
                m_Tasks.RenderFrame.SetTaskWeight(255);
                m_Tasks.SwapBuffers.pCtx = this;
                m_Tasks.RenderFrame.SetDbgType(taskIdx++);
                m_Tasks.RenderFrame.SetNextTask(&m_Tasks.Present);
                m_Tasks.Present.SetNextTask(&m_Tasks.SwapBuffers);
                m_Tasks.SwapBuffers.SetNextTask(&m_Tasks.RenderFrame);
                pThreadPool->AddConstantTask(&m_Tasks.RenderFrame, TaskStateBits::NOT_ACTIVE);
                pThreadPool->AddConstantTask(&m_Tasks.Present, TaskStateBits::NOT_ACTIVE );
                pThreadPool->AddConstantTask(&m_Tasks.SwapBuffers, TaskStateBits::NOT_ACTIVE );

                /*g_TaskGrp.m_Group.Pause();
                pThreadPool->AddConstantTaskGroup(&g_TaskGrp.m_Group);
                g_TaskGrp.m_Group.Restart();*/
            }
            // Create dummy queue
            //CreateGraphicsQueue({});
            {
                m_pDeviceCtx->m_pShaderMgr->Compile();
            }

            return VKE_OK;
ERR:
            Destroy();
            return VKE_FAIL;
        }

        CRenderingPipeline* CGraphicsContext::_CreateRenderingPipeline(const SRenderingPipelineDesc& Desc)
        {
            CRenderingPipeline* pPipeline = nullptr;
            if( VKE_SUCCEEDED( Memory::CreateObject( &HeapAllocator, &pPipeline, this ) ) )
            {
                if( VKE_SUCCEEDED( pPipeline->Create( Desc ) ) )
                {

                }
                else
                {
                    Memory::DestroyObject( &HeapAllocator, &pPipeline );
                }
            }
            return pPipeline;
        }

        const Vulkan::ICD::Device& CGraphicsContext::_GetICD() const
        {
            return *m_pPrivate->PrivateDesc.pICD;
        }

        CommandBufferPtr CGraphicsContext::CreateCommandBuffer()
        {
            CommandBufferPtr pCb;
            m_CmdBuffMgr.CreateCommandBuffers< VKE_THREAD_SAFE >(1, &pCb);
            return pCb;
        }

        void CGraphicsContext::_FreeCommandBuffer(CommandBufferPtr pCb)
        {
            m_CmdBuffMgr.FreeCommandBuffers< VKE_THREAD_SAFE >(1, &pCb);
        }

        void CGraphicsContext::RenderFrame()
        {
            m_needRenderFrame = !m_needQuit;
        }

        static const TaskState g_aTaskResults[] =
        {
            TaskStateBits::OK,
            TaskStateBits::REMOVE, // if m_needQuit == true
            TaskStateBits::NEXT_TASK
        };

        TaskState CGraphicsContext::_RenderFrameTask()
        {

            TaskState res = g_aTaskResults[ m_needQuit ];
            if( m_needRenderFrame && !m_needQuit )
            {
                //if( m_pSwapChain /*&& m_presentDone*/ )
                {
                    m_renderState = RenderState::END;
                    m_pEventListener->OnRenderFrame( this );
                    //m_pCurrRenderingPipeline->Render();
                    m_readyToPresent = true;
                    
                    _SetCurrentTask(ContextTasks::PRESENT);
                    res |= TaskStateBits::NEXT_TASK;
                }
            }
            return res;
        }

        TaskState CGraphicsContext::_PresentFrameTask()
        {
            //Threads::ScopedLock l( m_SyncObj );
            //CurrentTask CurrTask = _GetCurrentTask();
            TaskState ret = g_aTaskResults[ m_needQuit ];
            if( !m_needQuit /*&& CurrTask == ContextTasks::PRESENT*/ )
            {
                if( m_readyToPresent )
                {
                    m_renderState = RenderState::PRESENT;
                    //printf( "present frame: %s\n", m_pSwapChain->m_Desc.pWindow->GetDesc().pTitle );
                    assert(m_pEventListener);
                    //if( m_pQueue->WillNextSwapchainDoPresent() )
                    {
                        m_pEventListener->OnBeforePresent(this);
                    }
                    //auto& BackBuffer = m_pSwapChain->_GetCurrentBackBuffer();
                    CSubmit* pSubmit = m_SubmitMgr.GetCurrentSubmit();
                    
                    const auto res = m_pQueue->Present(m_VkDevice.GetICD(), m_pSwapChain->_GetCurrentImageIndex(),
                        m_pSwapChain->_GetSwapChain(), pSubmit->GetSignaledSemaphore());
                    // $TID Present: sc={(void*)m_pSwapChain}, imgIdx={m_pSwapChain->_GetCurrentImageIndex()}
                    m_readyToPresent = false;
                    
                }
                if(m_pQueue->IsPresentDone())
                {
                    m_pEventListener->OnAfterPresent(this);
                    _SetCurrentTask(ContextTasks::SWAP_BUFFERS);
                    ret |= TaskStateBits::NEXT_TASK;
                }
            }
            return ret;
        }

        TaskState CGraphicsContext::_SwapBuffersTask()
        {
            //Threads::ScopedLock l( m_SyncObj );
            //CurrentTask CurrTask = _GetCurrentTask();
            TaskState res = g_aTaskResults[ m_needQuit ];
            if( !m_needQuit /*&& CurrTask == ContextTasks::SWAP_BUFFERS*/ )
            {
                //if( m_pSwapChain && m_presentDone && m_pQueue->IsPresentDone() )
                {
                    m_renderState = RenderState::SWAP_BUFFERS;
                    //printf( "swap buffers: %s\n", m_pSwapChain->m_Desc.pWindow->GetDesc().pTitle );
                    m_pSwapChain->SwapBuffers();
                    _SetCurrentTask(ContextTasks::BEGIN_FRAME);
                    res |= TaskStateBits::NEXT_TASK;
                }
            }
            return res;
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

        Result CGraphicsContext::_AllocateCommandBuffers(VkCommandBuffer* pBuffers, uint32_t count)
        {
            VkCommandBufferAllocateInfo ai;
            Vulkan::InitInfo(&ai, VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO);
            ai.commandBufferCount = count;
            ai.commandPool = m_vkCommandPool;
            ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            VK_ERR( m_VkDevice.AllocateObjects( ai, pBuffers ) );
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
            VkCommandBufferArray vVkCmdBuffers;
            for( uint32_t i = 0; i < vCmdBuffers.GetCount(); ++i )
            {
                vCmdBuffers[ i ]->Flush();
                vVkCmdBuffers.PushBack( vCmdBuffers[ i ]->GetNative() );
            }
            //VkQueue vkQueue = m_pQueue->vkQueue;
            VkSubmitInfo si;
            Vulkan::InitInfo(&si, VK_STRUCTURE_TYPE_SUBMIT_INFO);
            si.commandBufferCount = vVkCmdBuffers.GetCount();
            si.pCommandBuffers = &vVkCmdBuffers[0];
            si.pSignalSemaphores = nullptr;
            si.pWaitDstStageMask = nullptr;
            si.pWaitSemaphores = nullptr;
            si.signalSemaphoreCount = 0;
            si.waitSemaphoreCount = 0;
            //VK_ERR(m_VkDevice.GetICD().vkQueueSubmit(vkQueue, 1, &si, vkFence));
            m_pQueue->Submit( m_VkDevice.GetICD(), si, vkFence );
        }

        VkFence CGraphicsContext::_CreateFence(VkFenceCreateFlags flags)
        {
            //VkFence vkFence;
            VkFenceCreateInfo ci;
            Vulkan::InitInfo(&ci, VK_STRUCTURE_TYPE_FENCE_CREATE_INFO);
            ci.flags = flags;
            /*if( !m_Fences.vFreeObjects.PopBack( &vkFence ) )
            {
                const auto count = m_Fences.vObjects.GetMaxCount();
                for( uint32_t i = 0; i < count; ++i )
                {                         
                    VK_ERR( m_VkDevice.CreateObject(ci, nullptr, &vkFence) );
                    m_Fences.vObjects.PushBack(vkFence);
                    m_Fences.vFreeObjects.PushBack(vkFence);
                }
                
                return _CreateFence(flags);
            }
            return vkFence;*/
            VkFence vkFence = _CreateObject( ci, &m_Fences );
            return vkFence;
        }

        void CGraphicsContext::_DestroyFence(VkFence* pVkFence)
        {
            //m_VkDevice.DestroyObject(nullptr, pVkFence);
            m_Fences.vFreeElements.PushBack(*pVkFence);
            *pVkFence = VK_NULL_HANDLE;
        }

        VkSemaphore CGraphicsContext::_CreateSemaphore()
        {
            VkSemaphoreCreateInfo ci;
            Vulkan::InitInfo(&ci, VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO);
            ci.flags = 0;
            //VkSemaphore vkSemaphore;
            //VK_ERR(m_VkDevice.CreateObject(ci, nullptr, &vkSemaphore));
            //return vkSemaphore;
            VkSemaphore vkSemaphore = _CreateObject( ci, &m_Semaphores );
            return vkSemaphore;
        }

        void CGraphicsContext::_DestroySemaphore(VkSemaphore* pVkSemaphore)
        {
            //m_VkDevice.DestroyObject(nullptr, pVkSemaphore);
            m_Semaphores.vFreeElements.PushBack( *pVkSemaphore );
            *pVkSemaphore = VK_NULL_HANDLE;
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