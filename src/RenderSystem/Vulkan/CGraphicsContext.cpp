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
#include "RenderSystem/CTransferContext.h"

namespace VKE
{
    namespace RenderSystem
    {

        struct SDefaultGraphicsContextEventListener final : public EventListeners::IGraphicsContext
        {
            bool AutoDestroy() override { return false; }

            bool OnRenderFrame( CGraphicsContext* ) override
            {
                //CSwapChain* pSwapChain = pCtx->GetSwapChain();
                //auto& BackBuffer = pSwapChain->GetCurrentBackBuffer();
                //auto pCb = pCtx->CreateCommandBuffer();
                //
                ////auto pSubmit = pCtx->GetNextCommandBufferBatch( 1, BackBuffer.vkAcquireSemaphore );

                //pCb->Begin();

                ////pSwapChain->BeginFrame( pCb );
                //pCb->Bind( pSwapChain );
                //pCb->Draw( 3 );
                //pCb->Draw( 3 );
                //pCb->Draw( 3 );
                ////pSwapChain->EndFrame( pCb );

                //pCb->End();

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
            //m_BaseCtx { pCtx->DDI(), pCtx }
            CContextBase( pCtx )
            ///*m_BaseCtx.*/m_pDeviceCtx( pCtx )
            //, /*m_BaseCtx.*/DDI( pCtx->_GetDDI() )
            , m_pEventListener( &g_sDefaultGCListener )
            //, m_CmdBuffMgr( pCtx )
            , m_PipelineMgr( pCtx )
            //, /*m_BaseCtx.*/SubmitMgr( pCtx )
        {
            static uint32_t instanceId = 0;
            m_instnceId = ++instanceId;
        }

        CGraphicsContext::~CGraphicsContext()
        {
            _Destroy();
        }

        //void CGraphicsContext::Destroy()
        //{
        //    assert( /*m_BaseCtx.*/m_pDeviceCtx );
        //    if( /*m_BaseCtx.*/m_pQueue.IsValid() )
        //    {
        //        CGraphicsContext* pCtx = this;
        //        /*m_BaseCtx.*/m_pDeviceCtx->DestroyGraphicsContext( &pCtx );
        //    }
        //}

        void CGraphicsContext::_Destroy()
        {
            if( /*m_BaseCtx.*/m_pDeviceCtx && /*m_BaseCtx.*/m_pQueue.IsValid() )
            {
                Threads::ScopedLock l( m_SyncObj );

                /*if( m_pEventListener && m_pEventListener->AutoDestroy() )
                {
                    VKE_DELETE( m_pEventListener );
                    m_pEventListener = nullptr;
                }*/

                this->m_pQueue->_RemoveSwapChainRef();

                FinishRendering();

                Memory::DestroyObject( &HeapAllocator, &m_pDefaultRenderingPipeline );
                Memory::DestroyObject( &HeapAllocator, &m_pSwapChain );

                m_PipelineMgr.Destroy();
                ///*m_BaseCtx.*/SubmitMgr.Destroy();
                //m_CmdBuffMgr.Destroy();

                //printf( "destroy graphics context\n" );
                m_readyToPresent = false;

                /*_DestroyObjects( &m_Fences );
                _DestroyObjects( &m_Semaphores );*/

                /*m_BaseCtx.*/CContextBase::Destroy();

                Memory::DestroyObject( &HeapAllocator, &m_pPrivate );
                /*m_BaseCtx.*/m_pDeviceCtx->_NotifyDestroy( this );
            }
        }

        void CGraphicsContext::FinishRendering()
        {
            m_needQuit = true;
            m_needRenderFrame = false;

            const bool waitForFinish = true;
            m_Tasks.RenderFrame.Remove< waitForFinish, THREAD_SAFE >();
            m_Tasks.Present.Remove< waitForFinish, THREAD_SAFE >();
            //m_Tasks.SwapBuffers.Remove< waitForFinish, THREAD_SAFE >();
            //m_Tasks.Execute.Remove< waitForFinish, THREAD_SAFE >();

            /*m_BaseCtx.*/m_pQueue->Lock();
            /*m_BaseCtx.*/m_pQueue->Wait();
            /*m_BaseCtx.*/m_pQueue->Unlock();
        }

        Result CGraphicsContext::Create( const SGraphicsContextDesc& Desc )
        {
            Result res = VKE_OK;
            auto pPrivate = reinterpret_cast<SGraphicsContextPrivateDesc*>(Desc.pPrivate);
            VKE_RETURN_IF_FAILED( Memory::CreateObject( &HeapAllocator, &m_pPrivate ) );

            ///*m_BaseCtx.*/m_pQueue = pPrivate->m_pQueue;

            {
                SContextBaseDesc BaseDesc;
                BaseDesc.pQueue = pPrivate->pQueue;
                BaseDesc.hCommandBufferPool = pPrivate->hCmdPool;
                this->m_initGraphicsShaders = true;
                if( VKE_FAILED( /*m_BaseCtx.*/CContextBase::Create( BaseDesc ) ) )
                {
                    goto ERR;
                }
            }
            // Create temporary command buffer
            {
                this->m_pCurrentCommandBuffer = this->_GetCurrentCommandBuffer();
            }
            {
                SSwapChainDesc SwpDesc = Desc.SwapChainDesc;
                if( VKE_FAILED( Memory::CreateObject( &HeapAllocator, &m_pSwapChain, this ) ) )
                {
                    goto ERR;
                }

                //SwpDesc.pPrivate = &m_pPrivate->PrivateDesc;
                SwpDesc.pCtx = this;
                SwpDesc.enableVSync = SwpDesc.pWindow->GetDesc().vSync;
                if( VKE_FAILED( m_pSwapChain->Create( SwpDesc ) ) )
                {
                    goto ERR;
                }
                this->m_pQueue->_AddSwapChainRef();

                SwpDesc.pWindow->AddDestroyCallback( [ & ]( CWindow* )
                {
                    auto pThis = this;
                    m_pDeviceCtx->DestroyGraphicsContext( &pThis );
                } );
                SwpDesc.pWindow->AddShowCallback( [ this ]( CWindow* pWnd )
                {
                    if( pWnd->IsVisible() )
                    {
                        //this->m_Tasks.SwapBuffers.IsActive( true );
                        //this->m_Tasks.Present.IsActive( true );
                        this->m_Tasks.RenderFrame.IsActive( true );
                        this->m_Tasks.Present.IsActive( true );
                    }
                } );
                SwpDesc.pWindow->AddResizeCallback( [ this ]( CWindow* pWnd, uint32_t width, uint32_t height )
                {
                    if( this != nullptr &&
                        !this->m_needQuit &&
                        pWnd->IsVisible() )
                    {
                        this->_ResizeSwapChainTask( width, height );
                    }
                } );
                SwpDesc.pWindow->SetSwapChain( m_pSwapChain );
            }
            {
                SRenderingPipelineDesc PipelineDesc;
                VKE_RENDER_SYSTEM_DEBUG_CODE( PipelineDesc.pDebugName = "Default" );
                SRenderingPipelineDesc::SPassDesc PassDesc;
                PassDesc.OnRender = [ & ]( const SRenderingPipelineDesc::SPassDesc& /*PassDesc*/ )
                {

                };
                m_pDefaultRenderingPipeline = _CreateRenderingPipeline( PipelineDesc );
                m_pCurrRenderingPipeline = m_pDefaultRenderingPipeline;
            }
            {
                SPipelineManagerDesc MgrDesc;
                MgrDesc.maxPipelineCount = Config::RenderSystem::Pipeline::MAX_PIPELINE_COUNT;

                if( VKE_FAILED( m_PipelineMgr.Create( MgrDesc ) ) )
                {
                    goto ERR;
                }
            }

            // Wait for all pending submits and reset submit data
            //this->_FlushCurrentCommandBuffer( nullptr );


            // Tasks
            {
                static uint32_t taskIdx = 123;
                auto pThreadPool = /*m_BaseCtx.*/m_pDeviceCtx->GetRenderSystem()->GetEngine()->GetThreadPool();
                m_Tasks.Present.pCtx = this;
                m_Tasks.Present.SetTaskWeight( UINT8_MAX );
                m_Tasks.Present.SetTaskPriority( 1 );
                m_Tasks.RenderFrame.pCtx = this;
                m_Tasks.RenderFrame.SetTaskWeight( UINT8_MAX );
                m_Tasks.RenderFrame.SetTaskPriority( 0 );
                m_Tasks.SwapBuffers.pCtx = this;
                m_Tasks.Execute.pCtx = this;

                //m_Tasks.SwapBuffers.SetNextTask( &m_Tasks.RenderFrame );
                //m_Tasks.RenderFrame.SetNextTask( &m_Tasks.Execute );
                //m_Tasks.Execute.SetNextTask( &m_Tasks.Present );
                //m_Tasks.Present.SetNextTask( &m_Tasks.SwapBuffers );

                //m_Tasks.RenderFrame.SetNextTask( &m_Tasks.Present );
                //m_Tasks.Present.SetNextTask( &m_Tasks.RenderFrame );
                
                //pThreadPool->AddConstantTask( &m_Tasks.SwapBuffers, TaskStateBits::NOT_ACTIVE );
                pThreadPool->AddConstantTask( &m_Tasks.RenderFrame, TaskStateBits::NOT_ACTIVE );
                //pThreadPool->AddConstantTask( &m_Tasks.Execute, TaskStateBits::NOT_ACTIVE );
                pThreadPool->AddConstantTask( &m_Tasks.Present, TaskStateBits::NOT_ACTIVE );
                

                /*g_TaskGrp.m_Group.Pause();
                pThreadPool->AddConstantTaskGroup(&g_TaskGrp.m_Group);
                g_TaskGrp.m_Group.Restart();*/
            }


            return res;
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
            return /*m_BaseCtx.*/m_DDI.GetDeviceICD();
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
            if( !m_needQuit /*&& CurrTask == ContextTasks::SWAP_BUFFERS*/ )
            {
                //if( m_pSwapChain && m_presentDone && /*m_BaseCtx.*/m_pQueue->IsPresentDone() )
                {
                    m_renderState = RenderState::SWAP_BUFFERS;
                    //printf( "swap buffers: %s\n", m_pSwapChain->m_Desc.pWindow->GetDesc().pTitle );
                    //m_currentBackBufferIdx = m_pSwapChain->SwapBuffers()->ddiBackBufferIdx;
                    const SBackBuffer* pBackBuffer = m_pSwapChain->SwapBuffers(true);
                    //m_currentBackBufferIdx = pBackBuffer->ddiBackBufferIdx;
                    /*m_BaseCtx.*/m_backBufferIdx = static_cast< uint8_t >( pBackBuffer->ddiBackBufferIdx );

                    if( /*m_BaseCtx.*/m_pQueue->GetSubmitCount() == 0 )
                    {
                        DDISemaphore hDDISemaphore = m_pSwapChain->GetCurrentBackBuffer().hDDIPresentImageReadySemaphore;
                        /*m_BaseCtx.*/m_pQueue->_GetSubmitManager()->SetWaitOnSemaphore( hDDISemaphore );
                    }

                    //_SetCurrentTask( ContextTasks::BEGIN_FRAME );
                    res |= TaskStateBits::NEXT_TASK;
                }
            }
            return res;
        }

        TaskState CGraphicsContext::_RenderFrameTask()
        {
            TaskState res = g_aTaskResults[m_needQuit];
            if( m_needRenderFrame && !m_needQuit && !m_stopRendering )
            {
                //_SwapBuffersTask();
                const SBackBuffer* pBackBuffer = m_pSwapChain->SwapBuffers( true /*waitForPresent*/ );
                if( pBackBuffer && pBackBuffer->IsReady() )
                {   
                    m_frameEnded = false;
                    //m_currentBackBufferIdx = pBackBuffer->ddiBackBufferIdx;
                    /*m_BaseCtx.*/m_backBufferIdx = static_cast< uint8_t >( pBackBuffer->ddiBackBufferIdx );

                    m_renderState = RenderState::END;
                    m_pEventListener->OnRenderFrame( this );
                    
                    SExecuteData Data;
                    //Data.ddiImageIndex = m_currentBackBufferIdx;
                    Data.ddiImageIndex = /*m_BaseCtx.*/m_backBufferIdx;
                    //Data.hDDISemaphoreBackBufferReady = pBackBuffer->hDDIPresentImageReadySemaphore;
                    DDISemaphore hTransferSemaphore = this->m_pDeviceCtx->GetTransferContext()->GetSignaledSemaphore();
                    if( hTransferSemaphore )
                    {
                        Data.vWaitSemaphores.PushBack( hTransferSemaphore );
                    }
                    Data.vWaitSemaphores.PushBack( pBackBuffer->hDDIPresentImageReadySemaphore );
                    Data.pBatch = m_pQueue->_GetSubmitManager()->FlushCurrentBatch( this->m_pDeviceCtx, this->m_hCommandPool );
                    {
                        //Threads::ScopedLock l( m_ExecuteQueueSyncObj );
                        m_qExecuteData.PushBack( Data );
                    }
                    m_readyToExecute = true;
                    m_frameEnded = true;
                    //_SetCurrentTask( ContextTasks::PRESENT );
                    res |= TaskStateBits::NEXT_TASK;
                }
            }
            return res;
        }

        TaskState CGraphicsContext::_ExecuteCommandBuffersTask()
        {
            TaskState ret = g_aTaskResults[ m_needQuit ];
            if( !m_needQuit /*&& m_readyToExecute*/ )
            {
                SExecuteData Data;
                bool dataReady;
                {
                    //Threads::ScopedLock l( m_ExecuteQueueSyncObj );
                    m_qExecuteData.GetCount();
                    dataReady = m_qExecuteData.PopFront( &Data );
                }
                if( dataReady )
                {
                    m_submitEnded = false;
                    //CCommandBufferBatch* pBatch;
                    //Data.pBatch->WaitOnSemaphore( Data.hDDISemaphoreBackBufferReady );
                    Data.pBatch->WaitOnSemaphores( Data.vWaitSemaphores );
                    if( VKE_SUCCEEDED( m_pQueue->_GetSubmitManager()->ExecuteBatch( this->m_pDeviceCtx, m_pQueue, &Data.pBatch ) ) )
                    {
                        //m_PresentInfo.hDDISwapChain = m_pSwapChain->GetDDIObject();
                        m_PresentInfo.pSwapChain = m_pSwapChain;
                        m_PresentInfo.hDDIWaitSemaphore = Data.pBatch->GetSignaledSemaphore();
                        m_PresentInfo.imageIndex = Data.ddiImageIndex;
                        m_readyToPresent = true;
                    }
                    m_submitEnded = true;
                    m_readyToExecute = false;
                    ret |= TaskStateBits::NEXT_TASK;
                }
            }
            return ret;
        }

        TaskState CGraphicsContext::_PresentFrameTask()
        {
            _ExecuteCommandBuffersTask();
            TaskState ret = g_aTaskResults[m_needQuit];
            if( !m_needQuit /*&& CurrTask == ContextTasks::PRESENT*/ )
            {
                if( m_readyToPresent )
                {
                    m_presentEnded = false;
                    m_renderState = RenderState::PRESENT;
                    //printf( "present frame: %s\n", m_pSwapChain->m_Desc.pWindow->GetDesc().pTitle );
                    assert( m_pEventListener );
                    //if( /*m_BaseCtx.*/m_pQueue->WillNextSwapchainDoPresent() )
                    {
                        m_pEventListener->OnBeforePresent( this );
                    }

                    Result res = m_pQueue->Present( m_PresentInfo );
                    if( res != VKE_OK )
                    {
                    }
                    m_readyToPresent = false;

                    if( /*m_BaseCtx.*/m_pQueue->IsPresentDone() )
                    {
                        m_pEventListener->OnAfterPresent( this );
                        //_SetCurrentTask( ContextTasks::SWAP_BUFFERS );
                        ret |= TaskStateBits::NEXT_TASK;
                    }
                    m_presentEnded = true;
                }
                
            }
            return ret;
        }

        CCommandBuffer* CGraphicsContext::BeginFrame()
        {
            this->m_pDeviceCtx->FreeUnusedAllocations();
            auto pCmdBuffer = this->_GetCurrentCommandBuffer();
            return pCmdBuffer;
        }

        void CGraphicsContext::EndFrame()
        {
            //VKE_ASSERT(this->m_pCurrentCommandBuffer.IsValid(), "" );
            //this->_FlushCurrentCommandBuffer();
            this->_EndCurrentCommandBuffer( CommandBufferEndFlags::END, nullptr );
        }

        void CGraphicsContext::Resize( uint32_t width, uint32_t height )
        {
            m_pSwapChain->Resize( width, height );
        }

        void CGraphicsContext::_AddToPresent( CSwapChain* /*pSwapChain*/ )
        {

        }

        Result CGraphicsContext::ExecuteCommandBuffers( DDISemaphore* phDDISignalSemaphore )
        {
            CCommandBufferBatch* pBatch;
            Threads::ScopedLock l( m_SyncObj );
            /*m_BaseCtx.*/m_pQueue->_GetSubmitManager()->SignalSemaphore( phDDISignalSemaphore );
            Result ret = m_pQueue->_GetSubmitManager()->ExecuteCurrentBatch( this->m_pDeviceCtx, this->m_pQueue, &pBatch );
            return ret;
        }

        VkInstance CGraphicsContext::_GetInstance() const
        {
            return /*m_BaseCtx.*/m_pDeviceCtx->_GetInstance();
        }

        void CGraphicsContext::SetEventListener( EventListeners::IGraphicsContext* pListener )
        {
            m_pEventListener = pListener;
        }

        void CGraphicsContext::Wait()
        {
            /*m_BaseCtx.*/m_pQueue->Lock();
            /*m_BaseCtx.*/m_DDI.GetICD().vkQueueWaitIdle( /*m_BaseCtx.*/m_pQueue->GetDDIObject() );
            /*m_BaseCtx.*/m_pQueue->Unlock();
        }

        void CGraphicsContext::SetTextureState( CSwapChain* pSwapChain, const TEXTURE_STATE& state )
        {
            auto pCurrEl = pSwapChain->GetCurrentBackBuffer().pAcquiredElement;
            STextureBarrierInfo Info;
            Info.currentState = pCurrEl->currentState;
            Info.newState = state;
            Info.SubresourceRange.aspect = TextureAspects::COLOR;
            Info.SubresourceRange.beginMipmapLevel = 0;
            Info.SubresourceRange.beginArrayLayer = 0;
            Info.SubresourceRange.layerCount = 1;
            Info.SubresourceRange.mipmapLevelCount = 1;
            Info.hDDITexture = pCurrEl->hDDITexture;
            Info.srcMemoryAccess = CTexture::ConvertStateToSrcMemoryAccess( Info.currentState, Info.newState );
            Info.dstMemoryAccess = CTexture::ConvertStateToDstMemoryAccess( Info.currentState, Info.newState );
            _GetCurrentCommandBuffer()->Barrier( Info );
        }

        void CGraphicsContext::_WaitForFrameToFinish()
        {
            while( m_frameEnded == false )
            {
                Platform::ThisThread::Pause();
            }
            while( m_submitEnded == false )
            {
                Platform::ThisThread::Pause();
            }
            while( !m_qExecuteData.IsEmpty() )
            {
                Platform::ThisThread::Pause();
            }
            while( m_presentEnded == false )
            {
                Platform::ThisThread::Pause();
            }
        }

        void CGraphicsContext::_ResizeSwapChainTask( uint32_t width, uint32_t height )
        {
            m_stopRendering = true;
            _WaitForFrameToFinish();
            this->_GetQueue()->m_SyncObj.Lock();
            this->_FlushCurrentCommandBuffer( nullptr );
            this->_GetCurrentCommandBuffer();
            this->_GetQueue()->Wait();
            GetSwapChain()->Resize( width, height );
            this->_FlushCurrentCommandBuffer( nullptr );
            this->_GetQueue()->Wait();
            this->_GetQueue()->m_SyncObj.Unlock();
            m_stopRendering = false;
        }

        void CGraphicsContext::BindDefaultRenderPass()
        {
            _GetCurrentCommandBuffer()->Bind( GetSwapChain() );
        }

    } // RenderSystem
} // VKE