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
            CContextBase( pCtx, "Graphics" )
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

                //this->m_pQueue->_RemoveSwapChainRef();

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

            //const bool waitForFinish = true;
            //m_Tasks.RenderFrame.Remove< waitForFinish, THREAD_SAFE >();
            //m_Tasks.Present.Remove< waitForFinish, THREAD_SAFE >();
            //m_Tasks.SwapBuffers.Remove< waitForFinish, THREAD_SAFE >();
            //m_Tasks.Execute.Remove< waitForFinish, THREAD_SAFE >();

            /*m_BaseCtx.*/m_pQueue->Lock();
            /*m_BaseCtx.*/m_pQueue->Wait();
            /*m_BaseCtx.*/m_pQueue->Unlock();
        }

        Result CGraphicsContext::Create( const SGraphicsContextDesc& Desc )
        {
            Result res = VKE_OK;

            CommandBufferPtr pCmdBuffer;
            SExecuteBatch* pExecute = nullptr;
            auto pPrivate = reinterpret_cast<SGraphicsContextPrivateDesc*>(Desc.pPrivate);
            ///*m_BaseCtx.*/m_pQueue = pPrivate->m_pQueue;

            {
                SContextBaseDesc BaseDesc;
                BaseDesc.pQueue = pPrivate->pQueue;
                //BaseDesc.hCommandBufferPool = pPrivate->hCmdPool;
                this->m_initGraphicsShaders = true;
                res = CContextBase::Create( BaseDesc );
                if( VKE_SUCCEEDED( res ) )
                {
                    pCmdBuffer = this->GetCommandBuffer();
                    pExecute = this->_GetExecuteBatch( pCmdBuffer );
                    pExecute->executeFlags |= ExecuteCommandBufferFlags::WAIT;
                }
                
            }
            VKE_RETURN_IF_FAILED( Memory::CreateObject( &HeapAllocator, &m_pPrivate ) );
            VKE_ASSERT( pCmdBuffer.IsValid() && pExecute != nullptr );
            if( VKE_SUCCEEDED(res) )
            {
                SSwapChainDesc SwpDesc = Desc.SwapChainDesc;
                if( VKE_FAILED( Memory::CreateObject( &HeapAllocator, &m_pSwapChain, this ) ) )
                {
                    goto ERR;
                }

                //SwpDesc.pPrivate = &m_pPrivate->PrivateDesc;
                SwpDesc = Desc.SwapChainDesc;
                //SwpDesc.pCtx = this;
                SwpDesc.enableVSync = SwpDesc.pWindow->GetDesc().vSync;
                SwpDesc.Size = SwpDesc.pWindow->GetSize();
                if( VKE_FAILED( m_pSwapChain->Create( SwpDesc, pCmdBuffer ) ) )
                {
                    goto ERR;
                }
                //this->m_pQueue->_AddSwapChainRef();

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
                        //this->m_Tasks.RenderFrame.IsActive( true );
                        //this->m_Tasks.Present.IsActive( true );
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
                PipelineDesc.SetDebugName( "Default" );
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

            
            res = this->_ExecuteBatch( pExecute );
            if( VKE_FAILED(res) )
            {
                goto ERR;
            }

            // Tasks
            {
                
                
              
                Threads::TSSimpleTask<void> RenderFrameTask =
                { 
                    .Task = [ this ]( void* )
                    {
                        return  _RenderFrameTask();
                    },
                    .pData = nullptr
                };

                Threads::TSSimpleTask<void> PresentFrameTask =
                { 
                    .Task = [ this ]( void* )
                    {
                        return _PresentFrameTask();
                    },
                    .pData = nullptr
                };

                auto pRS = m_pDeviceCtx->GetRenderSystem();
                auto pEngine = pRS->GetEngine();
                auto pThreadPool = pEngine->GetThreadPool();
                pThreadPool->AddTask(
                    Threads::ThreadUsageBits::GRAPHICS | Threads::ThreadUsageBits::MAIN_THREAD,
                                      "Render Frame",
                                      RenderFrameTask.Task );
                pThreadPool->AddTask(
                    Threads::ThreadUsageBits::GRAPHICS | Threads::ThreadUsageBits::ANY_EXCEPT_MAIN,
                                      "Present Frame",
                                      PresentFrameTask.Task );

                /*g_TaskGrp.m_Group.Pause();
                pThreadPool->AddConstantTaskGroup(&g_TaskGrp.m_Group);
                g_TaskGrp.m_Group.Restart();*/
            }

            if( VKE_FAILED( res ) )
            {
                goto ERR;
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

        static const TASK_RESULT g_aTaskResults[] =
        {
            TaskResults::WAIT,
            TaskResults::OK, // if m_needQuit == true
            TaskResults::WAIT
        };

        TASK_RESULT CGraphicsContext::_RenderFrameTask()
        {
            TASK_RESULT res = g_aTaskResults[ m_needQuit ];
            if( m_needRenderFrame && !m_needQuit && !m_stopRendering )
            {
                //m_pDeviceCtx->_OnFrameStart(this);
                //const SBackBuffer* pBackBuffer = m_pSwapChain->SwapBuffers( true /*waitForPresent*/ );
                //if( pBackBuffer /*&& pBackBuffer->IsReady()*/ )
                //{
                //    m_frameEnded = false;
                //    m_backBufferIdx = static_cast< uint8_t >( pBackBuffer->ddiBackBufferIdx );

                //    auto pBatch = this->_AcquireExecuteBatch();

                //    m_renderState = RenderState::END;
                //    m_pEventListener->OnRenderFrame( this );

                //    pBatch->vDDIWaitGPUFences.PushBack( pBackBuffer->hDDIPresentImageReadySemaphore );
                //    VKE_LOG( "Batch: " << pBatch
                //                       << " waits on present gpu fence: " << pBackBuffer->hDDIPresentImageReadySemaphore );
                //    pBatch->swapchainElementIndex = m_backBufferIdx;
                //    this->_PushCurrentBatchToExecuteQueue();
                //    //VKE_LOG( "Push batch: " << pBatch );
                //    m_readyToExecute = true;
                //    m_frameEnded = true;

                //    res = TaskResults::WAIT;
                //}
                m_pEventListener->OnRenderFrame( this );
                res = TaskResults::WAIT;
            }
            return res;
        }

        TASK_RESULT CGraphicsContext::_ExecuteCommandBuffersTask()
        {
            TASK_RESULT ret = g_aTaskResults[ m_needQuit ];
            if( !m_needQuit /*&& m_readyToExecute*/ )
            {
                SExecuteBatch* pBatch = this->_PopExecuteBatch();
                if(pBatch != nullptr)
                {
                    //VKE_LOG( "Pop batch: " << pBatch );
                    m_submitEnded = false;
                    //Result res = this->_ExecuteDependenciesForBatch(pBatch);
                    //VKE_ASSERT( VKE_SUCCEEDED( res ) );
                    pBatch->executeFlags |= ExecuteCommandBufferFlags::SIGNAL_GPU_FENCE;
                    Result res = this->_ExecuteBatch( pBatch );
                    VKE_LOG( "Signal gpu fence: " << pBatch->hSignalGPUFence );
                    if( VKE_SUCCEEDED( res ) )
                    {
                        //VKE_LOG( "Execute batch: " << pBatch << " swpchain idx: " << pBatch->swapchainElementIndex );
                        m_PresentInfo.pSwapChain = m_pSwapChain;
                        m_PresentInfo.hDDIWaitSemaphore = pBatch->hSignalGPUFence;
                        m_PresentInfo.imageIndex = pBatch->swapchainElementIndex;
                        m_readyToPresent = true;
                    }
                    VKE_ASSERT2( VKE_SUCCEEDED( res ), "" );
                    m_submitEnded = true;
                    m_readyToExecute = false;
                    ret = TaskResults::WAIT;
                }
            }
            return ret;
        }

        Result CGraphicsContext::Present(const SPresentInfo& Info)
        {
            return m_pQueue->Present( Info );
        }

        TASK_RESULT CGraphicsContext::_PresentFrameTask()
        {
            _ExecuteCommandBuffersTask();
            TASK_RESULT ret = g_aTaskResults[ m_needQuit ];
            if( !m_needQuit /*&& CurrTask == ContextTasks::PRESENT*/ )
            {
                if( m_readyToPresent )
                {
                    // Debug Swapchain
                    //static uint32_t frame = 0; VKE_LOG("present frame: " << frame++);
                    m_presentEnded = false;
                    m_renderState = RenderState::PRESENT;
                    //printf( "present frame: %s\n", m_pSwapChain->m_Desc.pWindow->GetDesc().pTitle );
                    VKE_ASSERT2( m_pEventListener, "Event listener must be set." );
                    //if( /*m_BaseCtx.*/m_pQueue->WillNextSwapchainDoPresent() )
                    {
                        m_pEventListener->OnBeforePresent( this );
                    }

                    //Result res = m_pQueue->Present( m_PresentInfo );
                    Result res = Present( m_PresentInfo );
                    VKE_LOG( "Present wait on gpu fence: " << m_PresentInfo.hDDIWaitSemaphore );
                    //VKE_LOG( "Present: " << res << " wait on: " << m_PresentInfo.hDDIWaitSemaphore );
                    if( res != VKE_OK )
                    {
                    }
                    m_readyToPresent = false;

                    if( /*m_BaseCtx.*/m_pQueue->IsPresentDone() )
                    {
                        m_pEventListener->OnAfterPresent( this );
                        //_SetCurrentTask( ContextTasks::SWAP_BUFFERS );
                        ret = TaskResults::WAIT;
                    }
                    m_presentEnded = true;
                    m_pDeviceCtx->_OnFrameEnd(this);
                    //m_pSwapChain->GetWindow()->Update();
                }
            }
            return ret;
        }

        CommandBufferPtr CGraphicsContext::BeginFrame()
        {
            this->m_pDeviceCtx->FreeUnusedAllocations();
            auto pCmdBuffer = this->_GetCurrentCommandBuffer();
            //m_pQueue->_GetSubmitManager()->GetCurrentBatch()
#if 0
            VKE_LOG( "BEGIN FRAME: " << pCmdBuffer->m_hDDIObject );
#endif
            return CommandBufferPtr{ pCmdBuffer };
        }

        void CGraphicsContext::EndFrame()
        {

        }

        void CGraphicsContext::Resize( uint32_t width, uint32_t height )
        {
            m_pSwapChain->Resize( width, height );
        }

        void CGraphicsContext::_AddToPresent( CSwapChain* /*pSwapChain*/ )
        {

        }

        //Result CGraphicsContext::ExecuteCommandBuffers( DDISemaphore* phDDISignalSemaphore )
        //{
        //    CCommandBufferBatch* pBatch;
        //    Threads::ScopedLock l( m_SyncObj );
        //    /*m_BaseCtx.*/m_pQueue->_GetSubmitManager()->SignalSemaphore( phDDISignalSemaphore );
        //    Result ret = m_pQueue->_GetSubmitManager()->ExecuteCurrentBatch( this, this->m_pQueue, &pBatch );
        //    return ret;
        //}

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

        void CGraphicsContext::SetTextureState( CommandBufferPtr pCmdBuffer, CSwapChain* pSwapChain,
            const TEXTURE_STATE& state )
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
            //_GetCurrentCommandBuffer()->Barrier( Info );
            pCmdBuffer->Barrier( Info );
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
            //while( !m_qExecuteData.IsEmpty() )
            while(!m_qExecuteData.empty())
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
            this->Execute(ExecuteCommandBufferFlags::END);
            this->_GetCurrentCommandBuffer();
            this->_GetQueue()->Wait();
            GetSwapChain()->Resize( width, height );
            this->Execute(ExecuteCommandBufferFlags::END);
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