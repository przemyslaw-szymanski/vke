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
    namespace RenderSystem
    {

        struct SDefaultGraphicsContextEventListener final : public EventListeners::IGraphicsContext
        {
            bool OnRenderFrame( CGraphicsContext* pCtx ) override
            {
                CSwapChain* pSwapChain = pCtx->GetSwapChain();
                auto& BackBuffer = pSwapChain->GetCurrentBackBuffer();
                auto pCb = pCtx->CreateCommandBuffer( BackBuffer.hDDIPresentImageReadySemaphore );
                
                //auto pSubmit = pCtx->GetNextCommandBufferBatch( 1, BackBuffer.vkAcquireSemaphore );

                pCb->Begin();

                //pSwapChain->BeginFrame( pCb );
                pCb->Bind( pSwapChain );
                pCb->Draw( 3 );
                pCb->Draw( 3 );
                pCb->Draw( 3 );
                //pSwapChain->EndFrame( pCb );

                pCb->End();
                pCb->Flush();

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

                TaskState _OnStart( uint32_t ) override
                {
                    Platform::ThisThread::Sleep( 1 );
                    return TaskStateBits::NOT_ACTIVE;
                }
            };

            Threads::CTaskGroup m_Group;
            STask m_aTasks[7];

            STaskGroup()
            {
                for( uint32_t i = 0; i < 7; ++i )
                {
                    m_Group.AddTask( &m_aTasks[i] );
                }
            }
        };
        STaskGroup g_TaskGrp;

        CGraphicsContext::CGraphicsContext( CDeviceContext* pCtx ) :
            m_pDeviceCtx( pCtx )
            , m_DDI( pCtx->_GetDDI() )
            , m_pEventListener( &g_sDefaultGCListener )
            //, m_CmdBuffMgr( pCtx )
            , m_PipelineMgr( pCtx )
            , m_SubmitMgr( pCtx )
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
            assert( m_pDeviceCtx );
            if( m_pQueue.IsValid() )
            {
                CGraphicsContext* pCtx = this;
                m_pDeviceCtx->DestroyGraphicsContext( &pCtx );
            }
        }

        void CGraphicsContext::_Destroy()
        {
            if( m_pDeviceCtx && m_pQueue.IsValid() )
            {
                Threads::ScopedLock l( m_SyncObj );

                m_needQuit = true;
                FinishRendering();

                Memory::DestroyObject( &HeapAllocator, &m_pDefaultRenderingPipeline );
                Memory::DestroyObject( &HeapAllocator, &m_pSwapChain );

                m_PipelineMgr.Destroy();
                m_SubmitMgr.Destroy();
                //m_CmdBuffMgr.Destroy();

                //printf( "destroy graphics context\n" );
                m_readyToPresent = false;

                /*_DestroyObjects( &m_Fences );
                _DestroyObjects( &m_Semaphores );*/

                Memory::DestroyObject( &HeapAllocator, &m_pPrivate );
                m_pDeviceCtx->_NotifyDestroy( this );
            }
        }

        void CGraphicsContext::FinishRendering()
        {
            const bool waitForFinish = true;
            m_Tasks.RenderFrame.Remove< waitForFinish, THREAD_SAFE >();
            m_Tasks.Present.Remove< waitForFinish, THREAD_SAFE >();
            m_Tasks.SwapBuffers.Remove< waitForFinish, THREAD_SAFE >();
            m_Tasks.Execute.Remove< waitForFinish, THREAD_SAFE >();

            m_pQueue->Lock();
            m_pQueue->Wait();
            m_pQueue->Unlock();
        }

        Result CGraphicsContext::Create( const SGraphicsContextDesc& Desc )
        {
            Result res = VKE_FAIL;
            auto pPrivate = reinterpret_cast<SGraphicsContextPrivateDesc*>(Desc.pPrivate);
            VKE_RETURN_IF_FAILED( Memory::CreateObject( &HeapAllocator, &m_pPrivate ) );

            m_pQueue = pPrivate->pQueue;
            m_hCommandPool = pPrivate->hCmdPool;

            m_Tasks.Present.IsActive( false );
            m_Tasks.RenderFrame.IsActive( false );
            m_Tasks.SwapBuffers.IsActive( false );

            

            //{
            //    VkCommandPoolCreateInfo ci;
            //    Vulkan::InitInfo( &ci, VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO );
            //    ci.queueFamilyIndex = m_pPrivate->PrivateDesc.pQueue->familyIndex;
            //    ci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            //    VK_ERR( m_VkDevice.CreateObject( ci, nullptr, &m_vkCommandPool ) );
            //}
            //{
            //    for( uint32_t i = 0; i < RenderQueueUsages::_MAX_COUNT; ++i )
            //    {
            //        SCommandBuffers& CBs = m_avCmdBuffers[i];
            //        auto& vCmdBuffers = CBs.vPool;
            //        // This should not fail as it is allocated on stack
            //        vCmdBuffers.Resize( vCmdBuffers.GetMaxCount() );

            //        if( VKE_FAILED( _AllocateCommandBuffers( &CBs.vPool[0], CBs.vPool.GetCount() ) ) )
            //        {
            //            goto ERR;
            //        }
            //        CBs.vFreeElements = CBs.vPool;
            //    }
            //}
            /*{
                SCommandBufferManagerDesc Desc;
                if( VKE_SUCCEEDED( m_CmdBuffMgr.Create( Desc ) ) )
                {
                    SCommandPoolDesc Desc;
                    Desc.commandBufferCount = CCommandBufferManager::DEFAULT_COMMAND_BUFFER_COUNT; /// @todo hardcode...
                    /// @todo store command pool handle
                    if( m_CmdBuffMgr.CreatePool( Desc ) == NULL_HANDLE )
                    {
                        goto ERR;
                    }
                }
            }*/
            {
                SSubmitManagerDesc Desc;
                Desc.pQueue = m_pQueue;
                Desc.hCmdBufferPool = m_hCommandPool;
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

                //SwpDesc.pPrivate = &m_pPrivate->PrivateDesc;
                SwpDesc.pCtx = this;
                if( VKE_FAILED( m_pSwapChain->Create( SwpDesc ) ) )
                {
                    goto ERR;
                }

                SwpDesc.pWindow->AddDestroyCallback( [ & ]( CWindow* )
                {
                    this->Destroy();
                } );
                SwpDesc.pWindow->AddShowCallback( [ this ]( CWindow* pWnd )
                {
                    if( pWnd->IsVisible() )
                    {
                        this->m_Tasks.SwapBuffers.IsActive( true );
                        this->m_Tasks.Present.IsActive( true );
                        this->m_Tasks.Execute.IsActive( true );
                        this->m_Tasks.RenderFrame.IsActive( true );
                    }
                } );
                SwpDesc.pWindow->SetSwapChain( m_pSwapChain );
            }
            {
                SRenderingPipelineDesc Desc;
                VKE_RENDER_SYSTEM_DEBUG_CODE( Desc.pDebugName = "Default" );
                SRenderingPipelineDesc::SPassDesc PassDesc;
                PassDesc.OnRender = [ & ]( const SRenderingPipelineDesc::SPassDesc& /*PassDesc*/ )
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

            // Wait for all pending submits and reset submit data
            m_pQueue->Wait();
            m_pQueue->Reset();
            // Swap buffers due to get first presentation image before first present
            //_SwapBuffersTask();

            // Tasks
            {
                static uint32_t taskIdx = 123;
                auto pThreadPool = m_pDeviceCtx->GetRenderSystem()->GetEngine()->GetThreadPool();
                m_Tasks.Present.pCtx = this;
                m_Tasks.RenderFrame.pCtx = this;
                m_Tasks.SwapBuffers.pCtx = this;
                m_Tasks.Execute.pCtx = this;

                /*m_Tasks.SwapBuffers.SetNextTask( &m_Tasks.RenderFrame );
                m_Tasks.RenderFrame.SetNextTask( &m_Tasks.Execute );
                m_Tasks.Execute.SetNextTask( &m_Tasks.Present );
                m_Tasks.Present.SetNextTask( &m_Tasks.SwapBuffers );*/
                
                pThreadPool->AddConstantTask( &m_Tasks.SwapBuffers, TaskStateBits::NOT_ACTIVE );
                pThreadPool->AddConstantTask( &m_Tasks.RenderFrame, TaskStateBits::NOT_ACTIVE );
                pThreadPool->AddConstantTask( &m_Tasks.Execute, TaskStateBits::NOT_ACTIVE );
                pThreadPool->AddConstantTask( &m_Tasks.Present, TaskStateBits::NOT_ACTIVE );
                

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

        CRenderingPipeline* CGraphicsContext::_CreateRenderingPipeline( const SRenderingPipelineDesc& Desc )
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

        const VkICD::Device& CGraphicsContext::_GetICD() const
        {
            //return *m_pPrivate->PrivateDesc.pICD;
            return m_DDI.GetDeviceICD();
        }

        CommandBufferPtr CGraphicsContext::CreateCommandBuffer(const DDISemaphore& hDDIWaitSemaphore)
        {
            CommandBufferPtr pCb;
            //m_CmdBuffMgr.CreateCommandBuffers< VKE_THREAD_SAFE >( 1, &pCb );
            Result res = m_pDeviceCtx->_CreateCommandBuffers( m_hCommandPool, 1, &pCb );
            if( VKE_SUCCEEDED( res ) )
            {
                pCb->m_pBatch = m_SubmitMgr.GetCurrentBatch();
                pCb->m_currBackBufferIdx = GetBackBufferIndex();
            }
            return pCb;
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

        TaskState CGraphicsContext::_SwapBuffersTask()
        {
            //Threads::ScopedLock l( m_SyncObj );
            //CurrentTask CurrTask = _GetCurrentTask();
            TaskState res = g_aTaskResults[ m_needQuit ];
            if( !m_needQuit && m_readyToSwap /*&& m_acquireCount < 1*/ )
            {
                //m_acquireCount++;
                m_readyToSwap = false;
                //if( m_pSwapChain && m_presentDone && m_pQueue->IsPresentDone() )
                {
                    m_renderState = RenderState::SWAP_BUFFERS;
                    //printf( "swap buffers: %s\n", m_pSwapChain->m_Desc.pWindow->GetDesc().pTitle );
                    //m_currentBackBufferIdx = m_pSwapChain->SwapBuffers()->ddiBackBufferIdx;
                    const SBackBuffer* pBackBuffer = m_pSwapChain->SwapBuffers();
                    if( pBackBuffer->IsReady() )
                    {
                        m_currentBackBufferIdx = pBackBuffer->ddiBackBufferIdx;
                        if( m_pQueue->GetSubmitCount() == 0 )
                        {
                            DDISemaphore hDDISemaphore = m_pSwapChain->GetCurrentBackBuffer().hDDIPresentImageReadySemaphore;
                            m_SubmitMgr.SetWaitOnSemaphore( hDDISemaphore );
                        }
                        m_readyToRender = true;
                        //_SetCurrentTask( ContextTasks::BEGIN_FRAME );
                        res |= TaskStateBits::NEXT_TASK;
                    }
                }
            }
            return res;
        }

        TaskState CGraphicsContext::_RenderFrameTask()
        {

            TaskState res = g_aTaskResults[m_needQuit];
            if( m_needRenderFrame && !m_needQuit && m_readyToRender )
            {
                m_readyToRender = false;
                //if( m_pSwapChain /*&& m_presentDone*/ )
                {
                    m_renderState = RenderState::END;
                    m_pEventListener->OnRenderFrame( this );
                    //m_pCurrRenderingPipeline->Render();
                    //m_readyToPresent = true;
                    m_readyToExecute = true;

                    //_SetCurrentTask( ContextTasks::PRESENT );
                    res |= TaskStateBits::NEXT_TASK;
                }
            }
            return res;
        }

        TaskState CGraphicsContext::_ExecuteCommandBuffersTask()
        {
            TaskState ret = g_aTaskResults[ m_needQuit ];
            if( !m_needQuit && m_readyToExecute )
            {
                m_readyToSwap = true;

                CCommandBufferBatch* pBatch;
                if( VKE_SUCCEEDED( m_SubmitMgr.ExecuteCurrentBatch( &pBatch ) ) )
                {
                    m_PresentInfo.hDDISwapChain = m_pSwapChain->GetDDIObject();
                    m_PresentInfo.hDDIWaitSemaphore = pBatch->GetSignaledSemaphore();
                    m_PresentInfo.imageIndex = m_currentBackBufferIdx;
                    VKE_ASSERT( m_PresentInfo.imageIndex != UINT32_MAX, "" );
                    m_readyToPresent = true;
                }
                m_readyToExecute = false;
                ret |= TaskStateBits::NEXT_TASK;
            }
            return ret;
        }

        TaskState CGraphicsContext::_PresentFrameTask()
        {
            //Threads::ScopedLock l( m_SyncObj );
            //CurrentTask CurrTask = _GetCurrentTask();
            TaskState ret = g_aTaskResults[m_needQuit];
            if( !m_needQuit /*&& CurrTask == ContextTasks::PRESENT*/ )
            {
                if( m_readyToPresent )
                {
                    m_renderState = RenderState::PRESENT;
                    //printf( "present frame: %s\n", m_pSwapChain->m_Desc.pWindow->GetDesc().pTitle );
                    assert( m_pEventListener );
                    //if( m_pQueue->WillNextSwapchainDoPresent() )
                    {
                        m_pEventListener->OnBeforePresent( this );
                    }

                    m_pQueue->Present( m_PresentInfo );
                    
                    m_readyToPresent = false;
                    
                    if( m_pQueue->IsPresentDone() )
                    {
                        m_pSwapChain->NotifyPresent();
                        /*if( m_acquireCount > 0 )
                        {
                            m_acquireCount--;
                        }*/
                        //m_readyToSwap = true;
                        m_pEventListener->OnAfterPresent( this );
                        //_SetCurrentTask( ContextTasks::SWAP_BUFFERS );
                        ret |= TaskStateBits::NEXT_TASK;
                    }
                }
            }
            return ret;
        }

        

        void CGraphicsContext::Resize( uint32_t width, uint32_t height )
        {
            m_pSwapChain->Resize( width, height );
        }

        void CGraphicsContext::_AddToPresent( CSwapChain* /*pSwapChain*/ )
        {

        }

        /*Result CGraphicsContext::_AllocateCommandBuffers( VkCommandBuffer* pBuffers, uint32_t count )
        {
            VkCommandBufferAllocateInfo ai;
            Vulkan::InitInfo( &ai, VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO );
            ai.commandBufferCount = count;
            ai.commandPool = m_vkCommandPool;
            ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            VK_ERR( m_VkDevice.AllocateObjects( ai, pBuffers ) );
            return VKE_OK;
        }*/

        Result CGraphicsContext::ExecuteCommandBuffers( DDISemaphore* phDDISignalSemaphore )
        {
            CCommandBufferBatch* pBatch;
            Threads::ScopedLock l( m_SyncObj );
            m_SubmitMgr.SignalSemaphore( phDDISignalSemaphore );
            Result ret = m_SubmitMgr.ExecuteCurrentBatch( &pBatch );
            return ret;
        }

        //VkFence CGraphicsContext::_CreateFence( VkFenceCreateFlags flags )
        //{
        //    //VkFence vkFence;
        //    //VkFenceCreateInfo ci;
        //    //Vulkan::InitInfo( &ci, VK_STRUCTURE_TYPE_FENCE_CREATE_INFO );
        //    //ci.flags = flags;
        //    /*if( !m_Fences.vFreeObjects.PopBack( &vkFence ) )
        //    {
        //        const auto count = m_Fences.vObjects.GetMaxCount();
        //        for( uint32_t i = 0; i < count; ++i )
        //        {
        //            VK_ERR( m_VkDevice.CreateObject(ci, nullptr, &vkFence) );
        //            m_Fences.vObjects.PushBack(vkFence);
        //            m_Fences.vFreeObjects.PushBack(vkFence);
        //        }

        //        return _CreateFence(flags);
        //    }
        //    return vkFence;*/
        //    //VkFence vkFence = _CreateObject( ci, &m_Fences );
        //    return vkFence;
        //}

        //void CGraphicsContext::_DestroyFence( VkFence* pVkFence )
        //{
        //    //m_VkDevice.DestroyObject(nullptr, pVkFence);
        //    m_Fences.vFreeElements.PushBack( *pVkFence );
        //    *pVkFence = VK_NULL_HANDLE;
        //}

        //VkSemaphore CGraphicsContext::_CreateSemaphore()
        //{
        //    VkSemaphoreCreateInfo ci;
        //    Vulkan::InitInfo( &ci, VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO );
        //    ci.flags = 0;
        //    //VkSemaphore vkSemaphore;
        //    //VK_ERR(m_VkDevice.CreateObject(ci, nullptr, &vkSemaphore));
        //    //return vkSemaphore;
        //    VkSemaphore vkSemaphore = _CreateObject( ci, &m_Semaphores );
        //    return vkSemaphore;
        //}

        //void CGraphicsContext::_DestroySemaphore( VkSemaphore* pVkSemaphore )
        //{
        //    //m_VkDevice.DestroyObject(nullptr, pVkSemaphore);
        //    m_Semaphores.vFreeElements.PushBack( *pVkSemaphore );
        //    *pVkSemaphore = VK_NULL_HANDLE;
        //}

        VkInstance CGraphicsContext::_GetInstance() const
        {
            return m_pDeviceCtx->_GetInstance();
        }

        void CGraphicsContext::SetEventListener( EventListeners::IGraphicsContext* pListener )
        {
            m_pEventListener = pListener;
        }

        void CGraphicsContext::Wait()
        {
            m_pQueue->Lock();
            m_DDI.GetICD().vkQueueWaitIdle( m_pQueue->GetDDIObject() );
            m_pQueue->Unlock();
        }

    } // RenderSystem
} // VKE