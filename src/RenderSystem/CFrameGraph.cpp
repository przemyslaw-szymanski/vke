#include "RenderSystem/CFrameGraph.h"
#include "RenderSystem/CDeviceContext.h"
#include "RenderSystem/CSwapChain.h"
#include "RenderSystem/CGraphicsContext.h"
#include "RenderSystem/Helper.h"

#include "Scene/CScene.h"
#include "CVkEngine.h"
#include "Scene/CWorld.h"

#include "CVkEngine.h"
#include "Core/Managers/CResourceManager.h"

#define VKE_LOG_FRAMEGRAPH 0

namespace VKE::RenderSystem
{
    Result CFrameGraph::_Create( const SFrameGraphDesc& Desc )
    {
        Result ret = VKE_FAIL;
        m_Desc     = Desc;
        // VKE_ASSERT( Desc.Size != TextureSize{ 0, 0 } );
        if( Desc.Size != TextureSize{ 0, 0 } )
        {
            if( VKE_SUCCEEDED( Memory::CreateObject( &HeapAllocator, &m_pLoadMgr ) ) )
            {
                VKE_ASSERT( m_Desc.pDevice != nullptr );
                if( ( Desc.flags & FrameGraphFlagBits::BASIC_MULTITHREADED ) != 0 )
                {
                    auto pSwapBufferPass = CreatePass( {
                        .pName = "SwapBuffers",
                    } );
                    auto pBeginFramePass = CreatePass( {
                        .pName = "BeginFrame",
                    } );
                    auto pRenderFramePass =
                        CreatePass( { .pName          = "RenderFrame",
                                      .vRenderTargets = { { .pName     = "Diffuse",
                                                            .format    = Formats::R8G8B8A8_UNORM,
                                                            .operation = FrameGraphPassOperations::RENDER_PASS_OVERWRITE },
                                                          { .pName     = "Depth",
                                                            .format    = Formats::D32_SFLOAT,
                                                            .operation = FrameGraphPassOperations::RENDER_PASS_OVERWRITE } } } );
                    auto pFinishRenderFramePass = CreatePass(
                        { .pName          = "FinishRenderFrame",
                          .vRenderTargets = { { .pName = "Diffuse", .operation = FrameGraphPassOperations::SHADER_READ } } } );
                    auto pEndFramePass      = CreatePass( {
                             .pName = "EndFrame",
                    } );
                    auto pExecuteFrame      = CreateExecutePass( { .pName = "ExecuteFrame",
                                                                   //.pThread = "ExecuteFrame",
                                                                   .pCommandBuffer = nullptr,
                        .gpuFenceValue = 2 } );
                    auto pPresent           = CreatePresentPass( { .pName = "PresentFrame",
                                                                   //.pThread = "PresentFrame",
                                                                   .pCommandBuffer = nullptr,
                        .gpuFenceValue = 3} );
                    auto pTextureLoadPass   = CreatePass( { .pName = "LoadTextures", .pCommandBuffer = nullptr } );
                    auto pBufferLoadPass    = CreatePass( { .pName = "LoadBuffers", .pCommandBuffer = nullptr } );
                    auto pBufferUploadPass  = CreatePass( { .pName = "BufferUpload", .pCommandBuffer = "Upload" } );
                    auto pCompileShaderPass = CreatePass( { .pName = "CompileShaders",
                                                            //.pThread = "CompileShaders",
                                                            .pCommandBuffer = nullptr } );
                    auto pTextureUploadPass = CreatePass( { .pName = "UploadTextures", .pCommandBuffer = "Upload" } );
                    auto pTextureGenMipmapPass = CreatePass( { .pName = "GenMipmaps" } );
                    auto pLoadDataPass         = CreatePass( { .pName = "Load", .pCommandBuffer = nullptr } );
                    auto pUploadDataPass       = CreatePass( { .pName = "Upload", .pCommandBuffer = "Upload" } );
                    auto pSceneUpdatePass      = CreatePass( { .pName = "SceneUpdate", .pCommandBuffer = nullptr } );
                    auto pUpdatePass           = CreatePass( { .pName = "Update", .pCommandBuffer = "Update" } );
                    auto pExecuteUploadPass =
                        CreateExecutePass( { .pName = "ExecuteUpload", .pCommandBuffer = nullptr } );
                    auto pExecuteUpdatePass =
                        CreateExecutePass( { .pName = "ExecuteUpdate", .pCommandBuffer = nullptr, .gpuFenceValue = 1 } );
                    auto pFinishFramePass = CreatePass( { .pName = "FinishFrame" } );
                    // auto pCreateResourcePass
                    //   = CreateCustomPass<VKE::RenderSystem::CFrameGraphMultiWorkloadNode>( { .pName =
                    //   "CreateResource" }, nullptr );
                    SetRootNode( pSwapBufferPass )
                        ->SetNext( pBeginFramePass )
                        ->SetNext( pLoadDataPass )
                        ->AddSubpass( pTextureLoadPass )
                        ->AddSubpass( pBufferLoadPass )
                        ->AddSubpass( pCompileShaderPass )
                        ->SetNext( pUploadDataPass )
                        ->AddSubpass( pTextureUploadPass )
                        ->AddSubpass( pBufferUploadPass )
                        ->AddSubpass( pTextureGenMipmapPass )
                        ->SetNext( pUpdatePass )
                        ->AddSubpass( pSceneUpdatePass )
                        ->SetNext( pRenderFramePass )
                        ->SetNext( pFinishRenderFramePass )
                        ->SetNext( pEndFramePass )
                        ->AddSubpass( pExecuteUploadPass )
                        ->AddSubpass( pExecuteUpdatePass )
                        ->AddSubpass( pExecuteFrame )
                        ->SetNext( pPresent )
                        ->SetNext( pFinishFramePass );

                    pExecuteUploadPass->AddToExecute( pUploadDataPass )
                        ->AddToExecute( pBufferUploadPass )
                        ->AddToExecute( pTextureUploadPass );

                    pExecuteUpdatePass->AddToExecute( pUpdatePass );

                    pRenderFramePass->WaitFor(
                        { .pNode = pUpdatePass, .WaitOn = VKE::RenderSystem::WaitOnBits::CPU_WAITS_FOR_CPU } );
                    pExecuteFrame->WaitFor(
                        { .pNode = pSwapBufferPass, .WaitOn = VKE::RenderSystem::WaitOnBits::GPU_WAITS_FOR_GPU } );
                    pExecuteFrame->WaitFor(
                        { .pNode = pExecuteUpdatePass, .WaitOn = VKE::RenderSystem::WaitOnBits::GPU_WAITS_FOR_GPU } );

                    pExecuteFrame->AddToExecute( pBeginFramePass );
                    pExecuteFrame->AddToExecute( pTextureGenMipmapPass );
                    pExecuteFrame->AddToExecute( pRenderFramePass );
                    pExecuteFrame->AddToExecute( pEndFramePass );

                    pPresent->WaitFor(
                        { .pNode  = pExecuteFrame,
                          .frame  = VKE::RenderSystem::WaitForFrames::CURRENT,
                          .WaitOn = VKE::RenderSystem::WaitOnBits::GPU_WAITS_FOR_GPU | VKE::RenderSystem::WaitOnBits::CPU_WAITS_FOR_CPU } );
                    pBeginFramePass->WaitFor( { .pNode  = pFinishFramePass,
                                                .frame  = VKE::RenderSystem::WaitForFrames::LAST,
                                                .WaitOn = VKE::RenderSystem::WaitOnBits::CPU_WAITS_FOR_CPU } );
                    pFinishFramePass->WaitFor(
                        { .pNode = pEndFramePass, .WaitOn = VKE::RenderSystem::WaitOnBits::CPU_WAITS_FOR_CPU } );
                    pSwapBufferPass->SetWorkload(
                        [ & ]( VKE::RenderSystem::CFrameGraphNode* const pPass, uint8_t backBufferIdx ) {
                            VKE::Result ret = pPass->OnWorkloadBegin( backBufferIdx );
                            if( VKE_SUCCEEDED( ret ) )
                            {
                                auto pCtx = pPass->GetContext()->Reinterpret< VKE::RenderSystem::CGraphicsContext >();
                                auto pSwpChain = pCtx->GetSwapChain();
                                ret            = pSwpChain->SwapBuffers();
                            }
                            ret = pPass->OnWorkloadEnd( ret );
                            return ret;
                        } );

                    pLoadDataPass->SetWorkload( [ & ]( CFrameGraphNode* const pPass, uint8_t backBufferIndex ) {
                        Result ret = pPass->OnWorkloadBegin( backBufferIndex );
                        if( VKE_SUCCEEDED( ret ) )
                        {
                        }
                        ret = pPass->OnWorkloadEnd( ret );
                        return ret;
                    } );

                    pUpdatePass->SetWorkload( [ & ]( CFrameGraphNode* const pPass, uint8_t backBufferIndex ) {
                        Result ret = pPass->OnWorkloadBegin( backBufferIndex );
                        if( VKE_SUCCEEDED( ret ) )
                        {
                            pPass->GetScene()->Update(
                                { .pCommandBuffer = pPass->GetCommandBuffer( backBufferIndex ) } );
                        }
                        ret = pPass->OnWorkloadEnd( ret );
                        return ret;
                    } );

                    pCompileShaderPass->SetWorkload( [ & ]( CFrameGraphNode* const pPass, uint8_t backBufferIndex ) {
                        Result ret = pPass->OnWorkloadBegin( backBufferIndex );
                        if( VKE_SUCCEEDED( ret ) )
                        {
                            auto pResMgr = pPass->GetContext()
                                               ->GetDeviceContext()
                                               ->GetRenderSystem()
                                               ->GetEngine()
                                               ->GetResourceManager();
                            while( VKE_SUCCEEDED( pResMgr->LoadDeferredShader() ) )
                            {
                            }
                            while( VKE_SUCCEEDED( pResMgr->CreateDeferredPipeline() ) )
                            {
                            }
                        }
                        if( VKE_SUCCEEDED( ret ) )
                        {
                            pPass->_ExecuteTasks(
                                { .executeTaskCount = 1, .backBufferIndex = backBufferIndex, .forceRemove = false } );
                        }
                        ret = pPass->OnWorkloadEnd( ret );
                        return ret;
                    } );

                    pBeginFramePass->SetWorkload( [ & ]( VKE::RenderSystem::CFrameGraphNode* const pPass,
                                                         uint8_t                                   backBufferIdx ) {
                        VKE::Result ret = pPass->OnWorkloadBegin( backBufferIdx );
                        if( VKE_SUCCEEDED( ret ) )
                        {
                            auto pCtx       = pPass->GetContext()->Reinterpret< VKE::RenderSystem::CGraphicsContext >();
                            auto pCmdBuffer = pPass->GetCommandBuffer( backBufferIdx );
                            auto pSwpChain  = pCtx->GetSwapChain();
                            // pCmdBuffer->Begin();
                            VKE::RenderSystem::STextureBarrierInfo Barrier;
                            auto                                   pBackBufferTex = pSwpChain->GetBackBufferTexture();
                            pBackBufferTex->SetState( VKE::RenderSystem::TextureStates::COLOR_RENDER_TARGET, &Barrier );
                            pCmdBuffer->Barrier( Barrier );
                            // pCmdBuffer->BeginRenderPass( { pBackBufferTex }, {} );
                            pPass->AddSynchronization( pSwpChain->GetBackBufferGPUFence() );
                            // VKE_LOG_NO_SYNC( "begin frame " << pCmdBuffer.Get() );
                            /*Platform::Debug::PrintOutput( "begin %llx, %d\n",
                                pCmdBuffer.Get(), pCmdBuffer->GetState() );*/
                        }
                        if( VKE_SUCCEEDED( ret ) )
                        {
                            pPass->_ExecuteTasks(
                                { .executeTaskCount = 1, .backBufferIndex = backBufferIdx, .forceRemove = false } );
                        }
                        ret = pPass->OnWorkloadEnd( ret );
                        return ret;
                    } );
                    pRenderFramePass->SetWorkload(
                        [ & ]( RenderSystem::CFrameGraphNode* pPass, uint8_t backBufferIdx ) {
                            Result ret = pPass->OnWorkloadBegin( backBufferIdx );
                            if( VKE_SUCCEEDED( ret ) )
                            {
                                pPass->GetScene()->Render( pPass->GetCommandBuffer( backBufferIdx ) );
                            }
                            return pPass->OnWorkloadEnd( ret );
                        } );
                    pFinishRenderFramePass->SetWorkload( [ & ]( RenderSystem::CFrameGraphNode* pPass,
                                                                uint8_t                        backBufferIndex ) {
                        VKE::Result ret = pPass->OnWorkloadBegin( backBufferIndex );
                        if( VKE_SUCCEEDED( ret ) )
                        {
                            auto pCtx       = pPass->GetContext()->Reinterpret< VKE::RenderSystem::CGraphicsContext >();
                            auto pSwapChain = pCtx->GetSwapChain();
                            auto pTex       = pSwapChain->GetBackBufferTexture();
                            auto pCmdBuffer = pPass->GetCommandBuffer( backBufferIndex );
                            STextureBarrierInfo Barrier;
                            pTex->SetState( TextureStates::TRANSFER_DST, &Barrier );
                            pCmdBuffer->Barrier( Barrier );
                            auto pLastPass        = pPass->GetPrev();
                            auto pRenderTargetTex = pLastPass->GetColorRenderTarget( 0 );
                            pRenderTargetTex->SetState( TextureStates::TRANSFER_SRC, &Barrier );
                            pCmdBuffer->Barrier( Barrier );
                            SCopyTextureInfo   CopyInfo   = { .hDDISrcTexture = pRenderTargetTex->GetDDIObject(),
                                                              .hDDIDstTexture = pTex->GetDDIObject(),
                                                              .Size           = pTex->GetDesc().Size,
                                                              .depth          = 0,
                                                              .SrcOffset      = { 0, 0 },
                                                              .DstOffset      = { 0, 0 } };
                            SCopyTextureInfoEx CopyInfoEx = { .pBaseInfo       = &CopyInfo,
                                                              .srcTextureState = pRenderTargetTex->GetState(),
                                                              .dstTextureState = pTex->GetState(),
                                                              .SrcSubresource  = { .aspect = TextureAspects::COLOR },
                                                              .DstSubresource  = { .aspect = TextureAspects::COLOR } };
                            pCmdBuffer->Copy( CopyInfoEx );
                        }
                        ret = pPass->OnWorkloadEnd( ret );
                        return ret;
                    } );
                    pEndFramePass->SetWorkload(
                        [ & ]( VKE::RenderSystem::CFrameGraphNode* const pPass, uint8_t backBufferIdx ) {
                            VKE::Result ret = pPass->OnWorkloadBegin( backBufferIdx );
                            if( VKE_SUCCEEDED( ret ) )
                            {
                                auto pCtx = pPass->GetContext()->Reinterpret< VKE::RenderSystem::CGraphicsContext >();
                                auto pCmdBuffer = pPass->GetCommandBuffer( backBufferIdx );
                                // pCmdBuffer->EndRenderPass();
                                auto                                   pSwpChain = pCtx->GetSwapChain();
                                VKE::RenderSystem::STextureBarrierInfo Barrier;
                                pSwpChain->GetBackBufferTexture()->SetState( VKE::RenderSystem::TextureStates::PRESENT,
                                                                             &Barrier );
                                pCmdBuffer->Barrier( Barrier );
                                //ret = EndFrame();
                                // VKE_LOG_NO_SYNC( "end frame " << pCmdBuffer.Get() );
                                /*Platform::Debug::PrintOutput( "end %llx, %d, %d\n",
                                    pCmdBuffer.Get(), pCmdBuffer->GetState(), Barrier.hDDITexture );*/
                            }
                            ret = pPass->OnWorkloadEnd( ret );
                            return ret;
                        } );
                    pFinishFramePass->SetWorkload(
                        [ & ]( VKE::RenderSystem::CFrameGraphNode* const pPass, uint8_t backBufferIdx ) {
                            VKE::Result ret = pPass->OnWorkloadBegin( backBufferIdx );
                            pPass->GetFrameGraph()->UpdateCounters();
                            ret = pPass->OnWorkloadEnd( ret );
                            return ret;
                        } );
                    const auto ResourceDefaultFunc = [ & ]( CFrameGraphNode* const pPass, uint8_t backBufferIndex ) {
                        VKE::Result ret = pPass->OnWorkloadBegin( backBufferIndex );
                        if( VKE_SUCCEEDED( ret ) )
                        {
                            pPass->_ExecuteTasks(
                                { .executeTaskCount = 1, .backBufferIndex = backBufferIndex, .forceRemove = false } );
                        }
                        ret = pPass->OnWorkloadEnd( ret );
                        return ret;
                    };

                    pBeginFramePass->AddTask(
                        [ & ]( const RenderSystem::CFrameGraphNode* pNode, uint8_t backBufferIndex ) {
                            // One time initializations
                            bool ret        = false;
                            auto pCmdBuffer = pNode->GetCommandBuffer( backBufferIndex );
                            VKEGetEngine()->GetWorld()->Init( pCmdBuffer );
                            return ret;
                        },
                        nullptr );
                    // pCompileShaderPass->SetWorkload( ResourceDefaultFunc );
                    // pLoadDataPass->SetWorkload( ResourceDefaultFunc );
                    pTextureGenMipmapPass->SetWorkload( ResourceDefaultFunc );
                    pTextureLoadPass->SetWorkload( ResourceDefaultFunc );
                    pUploadDataPass->SetWorkload( ResourceDefaultFunc );
                    pTextureUploadPass->SetWorkload( ResourceDefaultFunc );
                    ret = Build();
                }
            }
        }
        else
        {
            VKE_LOG_ERR( "FrameGraph Size must be set" );
        }
        return ret;
    }

    void CFrameGraph::_Destroy()
    {
        // Notify all threads to stop
        for( uint32_t i = 0; i < m_vpThreads.GetCount(); ++i )
        {
            auto pData      = m_vpThreadData[ i ];
            pData->needExit = true;
            pData->qWorkloads.clear();
            pData->CondVar.notify_all();
        }
        // Wait for threads and destroy them
        for( uint32_t i = 0; i < m_vpThreads.GetCount(); ++i )
        {
            if( m_vpThreads[ i ]->joinable() )
            {
                m_vpThreads[ i ]->join();
            }
            Memory::DestroyObject( &HeapAllocator, &m_vpThreads[ i ] );
            Memory::DestroyObject( &HeapAllocator, &m_vpThreadData[ i ] );
        }
        m_vpThreads.Clear();
        m_vpThreadData.Clear();

        for( auto& Pair: m_mNodes )
        {
            Pair.second->_Destroy();
            Memory::DestroyObject( &HeapAllocator, &Pair.second );
        }
        m_mNodes.clear();
        Memory::DestroyObject( &HeapAllocator, &m_pLoadMgr );
    }

    bool ExecuteLoopWithTimeout( std::function< bool() >&& Fun, uint64_t timeoutUS )
    {
        Utils::CTimer Timer;
        Timer.Start();
        bool ret = true;
        while( !Fun() )
        {
            if( Timer.GetElapsedTime() >= timeoutUS )
            {
                ret = false;
                break;
            }
            
            Platform::ThisThread::Pause();
        }
        return ret;
    }

    Result CFrameGraph::_GetNextFrame()
    {
        Result ret = VKE_OK;
        ++m_currentFrameIndex;
        m_backBufferIndex = ( m_backBufferIndex + 1 ) % MAX_BACKBUFFER_COUNT;
        SFrameData& FrameData = m_aFrameData[ m_backBufferIndex ];
        // Get first free frame
        bool needWait = true;
        for( uint32_t i = 0; i < MAX_BACKBUFFER_COUNT; ++i )
        {
            if( m_Desc.pDevice->IsReadyToUse( FrameData.hFrameFence, FrameData.frameFenceValue ) )
            {
                needWait = false;
                break;
            }
            else
            {
                m_backBufferIndex = ( m_backBufferIndex + 1 ) % MAX_BACKBUFFER_COUNT;
            }
        }
        if( needWait )
        {
            // If no frame is executed, wait for first one
            // Find context to wait on
            // Do active wait...
            // Wait 2 sec
            bool res = ExecuteLoopWithTimeout( [ this ]() {
                return this->m_Desc.pDevice->IsReadyToUse( m_aFrameData[ m_backBufferIndex ].hFrameFence,
                                                           m_aFrameData[ m_backBufferIndex ].frameFenceValue );
            }, 2000*1000 );
            if( !res )
            {
                ret = VKE_TIMEOUT;
            }
        }

        _ResetFrameData( &m_aFrameData[ m_backBufferIndex ] );
        m_aFrameData[ m_backBufferIndex ].localUseIndex++;

        return ret;
    }

    Result CFrameGraph::_ResetFrameData( SFrameData* pFrameData )
    {
        Result ret = VKE_FAIL;
        m_Desc.pDevice->Reset( &pFrameData->hFrameFence );
        for( uint32_t i = 0; i < ContextTypes::_MAX_COUNT; ++i )
        {
            auto& vCmdBuffers = pFrameData->avpCommandBuffers[ i ];
            for( uint32_t c = 0; c < vCmdBuffers.GetCount(); ++c )
            {
                vCmdBuffers[ c ]->Reset();
            }
        }
        return ret;
    }

    Result CFrameGraph::_BeginFrame()
    {
        Result res = VKE_OK;

        
        return res;
    }

    void CFrameGraph::_Reset( SExecuteBatch* pBatch )
    {
        // auto pContext = pBatch->pContext;
        // auto pDevice = pContext->m_pDeviceCtx;
        // auto& API = pDevice->DDI();
        // bool signaled = API.IsSignaled( pBatch->hSignalCPUFence );
        // bool executed = pBatch->executionResult == Results::OK;
        // bool hasCmdBuffers = !pBatch->vpCommandBuffers.IsEmpty();
        pBatch->executionResult = Results::NOT_READY;
        pBatch->executeFlags    = 0;
        pBatch->vDependencies.Clear();
        pBatch->refCount = 0;

        for( uint32_t c = 0; c < pBatch->vpCommandBuffers.GetCount(); ++c )
        {
            auto pCb     = pBatch->vpCommandBuffers[ c ];
            auto pDevice = pCb->GetContext()->GetDeviceContext();
            VKE_ASSERT( pDevice->IsReadyToUse( pBatch->hSignalCPUFence ) );
            while( !pDevice->IsReadyToUse( pBatch->hSignalCPUFence ) )
            {
                Platform::ThisThread::Pause();
            }
            pBatch->vpCommandBuffers[ c ]->Reset();
        }
    }

    Result CFrameGraph::_EndFrame()
    {
        Result ret = VKE_OK;
        if( VKE_SUCCEEDED( ret ) )
        {
            ret = _GetNextFrame();
        }
        return ret;
    }

    void CFrameGraph::_AcquireCommandBuffers()
    {
    }

    Result CFrameGraph::_OnCreateNode( const SFrameGraphNodeDesc& Desc, CFrameGraphNode** ppNode )
    {
        Result ret   = VKE_OK;
        auto   pNode = *ppNode;
        // By default use parent command buffer
        // auto& ParentNode = m_mNodes[ pNode->m_Desc.ParentName ];
        // const auto& Desc = pNode->m_Desc;
        pNode->m_pContext = m_Desc.apContexts[ pNode->m_ctxType ];

        if( VKE_SUCCEEDED( ret ) )
        {
            pNode->m_Index.threadFence = _CreateThreadFence( pNode );
            pNode->m_Index.thread      = _CreateThreadIndex( Desc.pThread );
            pNode->_CreateBeginRenderPassInfo( Desc );

            pNode->m_RenderArea = _GetRenderArea( Desc.size );

            if( !pNode->m_CommandBufferName.IsEmpty() )
            {
                pNode->m_Index.commandBuffer = _CreateCommandBuffer( pNode );
                ret                          = pNode->m_Index.commandBuffer != INVALID_INDEX ? VKE_OK : VKE_FAIL;
            }
        }
        // Get prev node
        // If root skip it

        return ret;
    }

    CFrameGraphNode* CFrameGraph::CreatePass( const SFrameGraphPassDesc& Desc )
    {
        CFrameGraphNode* pRet = _CreateNode< CFrameGraphNode >( Desc );
        return pRet;
    }

    CFrameGraphExecuteNode* CFrameGraph::CreateExecutePass( const SFrameGraphNodeDesc& Desc )
    {
        auto idx = m_avExecuteNames[ Desc.contextType ].Find( Desc.pName );
        if( idx != INVALID_POSITION )
        {
            VKE_LOG_ERR( "FrameGraph: " << Desc.pName << ", Node: " << Desc.pName << ": Execution: " << Desc.pExecute
                                        << " already exists." );
        }
        SFrameGraphNodeDesc NewDesc = Desc;

        CFrameGraphExecuteNode* pPass = _CreateNode< CFrameGraphExecuteNode >( Desc );
        if( pPass != nullptr )
        {
            pPass->m_doExecute     = true;
            pPass->m_Index.execute = _CreateExecute( static_cast< CFrameGraphNode* >( pPass ) );
            pPass->m_pExecuteNode  = pPass;
            pPass->SetWorkload( [ this ]( CFrameGraphNode* const pPass, uint8_t backBufferIndex ) {
                Result ret = pPass->OnWorkloadBegin( backBufferIndex );
                if( VKE_SUCCEEDED( ret ) )
                {
                    CFrameGraphExecuteNode* pNode = static_cast< CFrameGraphExecuteNode* >( pPass );
                    auto pExecuteData             = pNode->_BuildDataToExecute( backBufferIndex );
                    if( pExecuteData )
                    {
#if VKE_LOG_FRAMEGRAPH
                        auto& Exe = _GetExecute( pNode, backBufferIndex );
                        VKE_LOG( pPass->m_Name << ", bbidx: " << (uint32_t)this->m_backBufferIndex << " "
                                               << pPass->GetThreadFence().Load()
                                               << " signal gpufence: " << Exe.hSignalGPUFence );
#endif
                        VKE_ASSERT( pExecuteData->SubmitInfo.commandBufferCount );
                        ret = pNode->m_pContext->Execute( pExecuteData->SubmitInfo );
                    }
                }
                ret = pPass->OnWorkloadEnd( ret );
                return ret;
            } );
        }
        return pPass;
    }

    void CFrameGraph::UpdateCounters()
    {
        ++m_CounterMgr.aCounters[ FrameGraphCounterTypes::FRAME_COUNT ].Avg.u32;
        float elapsedCPUTime  = (float)( ( m_CounterMgr.FrameTimer.GetElapsedTime() ) * 0.001f );
        float elapsedCPUTime2 = (float)( ( m_CounterMgr.FPSTimer.GetElapsedTime() ) * 0.001f );
        auto& CPUFrameTime    = m_CounterMgr.aCounters[ FrameGraphCounterTypes::CPU_FRAME_TIME ];

        auto& CPUFps = m_CounterMgr.aCounters[ FrameGraphCounterTypes::CPU_FPS ];
        CPUFps.Total.u32++;
        if( elapsedCPUTime2 >= 1000 )
        {
            CPUFrameTime.Set( elapsedCPUTime );
            CPUFps.Avg.u32   = CPUFps.Total.u32;
            CPUFps.Total.u32 = 0;
            m_CounterMgr.FPSTimer.Start();
        }
        CPUFps.Avg.f32 = 1000.0f / CPUFrameTime.CalcAvg< float >();
        m_CounterMgr.FrameTimer.Start();
    }

    CFrameGraphNode* CFrameGraph::CreatePresentPass( const SFrameGraphNodeDesc& Desc )
    {
        auto pRet = CreatePass( Desc );
        if( pRet != nullptr )
        {
            // Present executes as well but via Present api call
            pRet->m_doExecute = true;
            pRet->SetWorkload( [ & ]( CFrameGraphNode* const pPass, uint8_t backBufferIndex ) {
                Result ret = VKE_OK;
                {
                    ret = pPass->OnWorkloadBegin( backBufferIndex );
                }
                if( VKE_SUCCEEDED( ret ) )
                {
                    if( backBufferIndex != UNDEFINED_U32 )
                    {
                        // VKE_LOG( "present" );
                        auto pCtx      = pPass->GetContext()->Reinterpret< CGraphicsContext >();
                        auto pSwpChain = pCtx->GetSwapChain();
                        //auto hGPUFence = pPass->m_vWaitForNodes.Back().pNode->GetGPUFence( backBufferIdx );
#if VKE_LOG_FRAMEGRAPH
                        VKE_LOG( "bbidx: "
                                 << backBufferIdx << " frame " << m_currentFrameIndex << " wait for thread fence "
                                 << pPass->m_vWaitForNodes.Back().pNode->GetThreadFence().Load() << " present fence "
                                 << pPass->GetThreadFence().Load() << " wait on gpufence: " << hGPUFence );
#endif
                        //auto hFrameFence = m_ahFrameCPUFences[ backBufferIdx ];
                        SPresentInfo PresentInfo;
                        PresentInfo.hSignalFence = m_aFrameData[ backBufferIndex ].hFrameFence;
                        PresentInfo.signalFenceValue = pPass->GetFenceValue();
                    
                        ret              = pSwpChain->Present( PresentInfo );
                    }
                }
                {
                    ret = pPass->OnWorkloadEnd( ret );
                }
                return ret;
            } );
        }
        return pRet;
    }

    bool CFrameGraph::_Validate( CFrameGraphNode* pNode )
    {
        bool ret = true;

        return ret;
    }

    Result CFrameGraph::Build()
    {
        Result ret = VKE_OK;
        // Create back buffer data
        for( uint32_t i = 0; i < MAX_BACKBUFFER_COUNT; ++i )
        {
            auto& Data = m_aFrameData[ i ];
            SFenceDesc Desc;
            Desc.startValue = 0;
            if( Data.hFrameFence == NativeTypes::Null )
            {
                Desc.SetDebugName( "FrameData%d", i );
                Data.hFrameFence = m_Desc.pDevice->CreateFence( Desc );
            }
        }
        if( !m_isValidated )
        {
            m_isValidated = _Validate( m_pRootNode );
        }
        if( m_isValidated )
        {
            if( m_needBuild )
            {
                ret = _Build( m_pRootNode );
                if( VKE_SUCCEEDED( ret ) )
                {
                }
            }
        }
        return ret;
    }

    Result CFrameGraph::_Build( CFrameGraphNode* pNode )
    {
        Result ret = VKE_OK;
        if( pNode->IsEnabled() )
        {
            
        }
        else
        {
            pNode->m_pCommandBuffer = nullptr;
        }
        return ret;
    }

    Result CFrameGraph::Run()
    {
        Result ret = VKE_FAIL;
        m_pScene = m_Desc.pDevice->GetRenderSystem()->GetEngine()->GetWorld()->GetScene().Get();
        if( VKE_SUCCEEDED( _BeginFrame() ) )
        {
            if( VKE_SUCCEEDED( Build() ) )
            {
                _ExecuteNode( m_pRootNode );
                ret = _EndFrame();
            }
        }
        return ret;
    }

    void CFrameGraph::_ExecuteNode( CFrameGraphNode* pNode )
    {
        pNode->InitFenceValue(m_backBufferIndex);
        if( pNode->IsEnabled() )
        {
            pNode->_Run( m_pLastNode );
            if( !pNode->IsSubpass() )
            {
                m_pLastNode = pNode;
            }
        }
        if( pNode->m_pNextNode )
        {
            _ExecuteNode( pNode->m_pNextNode );
        }
    }

    void CFrameGraph::_ExecuteSubpassNodes( CFrameGraphNode* pNode )
    {
        auto pCurrSubpass = pNode->m_pSubpassNode;
        if( pCurrSubpass )
        {
            _ExecuteNode( pCurrSubpass );
        }
    }

    static const cstr_t g_aContextNames[ ContextTypes::_MAX_COUNT ] = { "General", //
                                                                        "Compute",
                                                                        "Transfer",
                                                                        "Sparse",
                                                                        "Present" };

    CFrameGraph::INDEX_TYPE CFrameGraph::_CreateCommandBuffer( const CFrameGraphNode* const pNode )
    {
        INDEX_TYPE ret = INVALID_INDEX;

        // Find required command buffer
        const auto   ctxType = pNode->m_ctxType;
        ResourceName CmdBufferName =
            std::format( "{}_{}_{}", (int)ctxType, pNode->m_CommandBufferName.GetData(), pNode->m_Index.thread )
                .c_str();
        auto idx = m_avCommandBufferNames[ ctxType ].Find( CmdBufferName );
        if( idx == INVALID_POSITION )
        {
            auto threadIndex = pNode->m_Index.thread;
            VKE_ASSERT( threadIndex != INVALID_INDEX );
            SCreateCommandBufferInfo                                       CreateInfo = { .count = MAX_BACKBUFFER_COUNT,
                                                                                          .threadIndex = (uint8_t)threadIndex };
            Utils::TCDynamicArray< CCommandBuffer*, MAX_BACKBUFFER_COUNT > vCbs( CreateInfo.count );
            VKE_ASSERT( m_Desc.apContexts[ ctxType ] != nullptr );
            Result res = m_Desc.apContexts[ ctxType ]->_CreateCommandBuffers( CreateInfo, &vCbs[ 0 ] );
            if( VKE_SUCCEEDED( res ) )
            {
                ResourceName DbgName;
                for( uint32_t i = 0; i < vCbs.GetCount(); ++i )
                {
                    DbgName.Format( "%s_backBuffer%d_%s_%s",
                                    g_aContextNames[ ctxType ],
                                    i,
                                    pNode->m_ExecuteName.GetData(),
                                    pNode->m_CommandBufferName.GetData() );
                    vCbs[ i ]->SetDebugName( DbgName.GetData() );
                    // m_Desc.apContexts[ ctxType ]->Reset( vCbs[ i ] );
                    vCbs[ i ]->Reset();
                    auto& FrameData = m_aFrameData[ i ];
                    ret = (INDEX_TYPE)FrameData.avpCommandBuffers[ ctxType ].PushBack( CommandBufferPtr{ vCbs[ i ] } );
                }
                idx = (INDEX_TYPE)m_avCommandBufferNames[ pNode->m_ctxType ].PushBack( CmdBufferName );
                VKE_ASSERT( idx == ret );
            }
        }
        else
        {
            ret = (INDEX_TYPE)idx;
        }
        return ret;
    }

    /*CommandBufferPtr CFrameGraph::_GetCommandBuffer( const SGetCommandBufferInfo& Info )
    {
        CommandBufferPtr pRet = m_pCurrentFrameData->avpCommandBuffers[ Info.contextType ][ Info.commandBufferIndex ];
        VKE_ASSERT( pRet!= nullptr );
        return pRet;
    }*/
    CommandBufferRefPtr CFrameGraph::_GetCommandBuffer( const CFrameGraphNode* const pNode, uint8_t backBufferIdx )
    {
        return CommandBufferRefPtr{
            m_aFrameData[ backBufferIdx ].avpCommandBuffers[ pNode->m_ctxType ][ pNode->m_Index.commandBuffer ]
        };
    }

    CFrameGraph::INDEX_TYPE CFrameGraph::_CreateExecute( const CFrameGraphNode* const pNode )
    {
        INDEX_TYPE ret     = INVALID_INDEX;
        const auto ctxType = pNode->m_ctxType;
        auto&      vNames  = m_avExecuteNames[ ctxType ];
        auto       idx     = vNames.Find( ResourceName{ pNode->m_Name } );
        if( idx == INVALID_POSITION )
        {
            Result res = VKE_OK;

            if( VKE_SUCCEEDED( res ) )
            {
                ret = (INDEX_TYPE)m_avExecuteNames[ ctxType ].PushBack( ResourceName{ pNode->m_ExecuteName } );
            }
        }
        else
        {
            ret = (INDEX_TYPE)idx;
        }
        return ret;
    }

    CFrameGraph::INDEX_TYPE CFrameGraph::_CreateThreadFence( const CFrameGraphNode* const )
    {
        INDEX_TYPE ret = INVALID_INDEX;
        for( uint32_t backBufferIdex = 0; backBufferIdex < MAX_BACKBUFFER_COUNT; ++backBufferIdex )
        {
            ret = (INDEX_TYPE)m_aFrameData[ backBufferIdex ].vThreadFences.PushBack( { 0 } );
        }
        return ret;
    }

    CFrameGraph::SThreadData& CFrameGraph::_GetThreadData( uint32_t threadindex ) const
    {
        return *( m_vpThreadData[ threadindex ] );
    }

    void CFrameGraph::_ThreadFunc( const CFrameGraph* pFrameGraph, uint32_t index )
    {
        using namespace std::chrono_literals;
        CFrameGraph::SThreadData& ThreadData = pFrameGraph->_GetThreadData( index );
        while( !ThreadData.needExit )
        {
            std::unique_lock< std::mutex > l( ThreadData.Mutex );
            /*if( ThreadData.CondVar.wait_for( l, 2s,
                [&] { return !ThreadData.qWorkloads.empty(); } ) )*/
            ThreadData.CondVar.wait( l, [ & ] {
                return !ThreadData.qWorkloads.empty() || ThreadData.needExit;
            } );
            {
                if( !ThreadData.qWorkloads.empty() )
                {
                    auto Workload = ThreadData.qWorkloads.front();
                    ThreadData.qWorkloads.pop_front();
                    if( Workload.Func )
                    {
                        Workload.Func( Workload.pNode, Workload.backBufferIndex );
                    }
                }
            }
        }
        return;
    }

    CFrameGraph::INDEX_TYPE CFrameGraph::_CreateThreadIndex( const std::string_view& ThreadName )
    {
        INDEX_TYPE ret = INVALID_INDEX;
        auto       idx = m_vThreadNames.Find( ResourceName{ ThreadName } );
        if( idx == INVALID_POSITION ) // such thread name is not present
        {
            // VKE_ASSERT( m_vThreadNames.GetCount() < MAX_GRAPHICS_THREAD_COUNT );
            ret = (INDEX_TYPE)( m_vThreadNames.PushBack( ResourceName{ ThreadName } ) );
            std::thread* pThread;
            if( VKE_SUCCEEDED( Memory::CreateObject( &HeapAllocator, &pThread, CFrameGraph::_ThreadFunc, this, ret ) ) )
            {
                INDEX_TYPE   ret2 = (INDEX_TYPE)( m_vpThreads.PushBack( ( pThread ) ) );
                INDEX_TYPE   ret3 = INVALID_INDEX;
                SThreadData* pData;
                if( VKE_SUCCEEDED( Memory::CreateObject( &HeapAllocator, &pData ) ) )
                {
                    ret3 = (INDEX_TYPE)( m_vpThreadData.PushBack( pData ) );
                }
                else
                {
                    ret = INVALID_INDEX;
                }
                VKE_ASSERT( ret == ret2 && ret2 == ret3 );
            }
        }
        else
        {
            ret = (INDEX_TYPE)idx;
        }
        return ret;
    }

    TextureRefPtr CFrameGraph::_GetTexture( const SFrameGraphRenderTargetTextureDesc& Desc )
    {
        TextureRefPtr pRet;
        auto          Itr = m_mRenderTargets.find( Desc.pName );
        if( Itr == m_mRenderTargets.end() )
        {
            TEXTURE_USAGE      usage = Helper::HasDepth( Desc.format ) ? TextureUsages::DEPTH_STENCIL_RENDER_TARGET
                                                                       : TextureUsages::COLOR_RENDER_TARGET;
            SCreateTextureDesc TexDesc;
            TexDesc.Texture = { .Size        = TextureSize( _GetRenderArea( Desc.size ).Size ),
                                .format      = Desc.format,
                                .usage       = usage,
                                .memoryUsage = MemoryUsages::GPU_ACCESS | MemoryUsages::TEXTURE,
                                .Name        = Desc.pName };
            auto hTex       = m_Desc.pDevice->CreateTexture( TexDesc );
            pRet            = m_Desc.pDevice->GetTexture( hTex );
        }
        else
        {
            pRet = Itr->second;
        }

        return pRet;
    }

    Rect2DI32 CFrameGraph::_GetRenderArea( RENDER_PASS_SIZE size )
    {
        /// TODO: Handle position!
        Rect2DI32 Ret = { .Position = { 0, 0 }, .Size = ExtentU32( m_Desc.Size / TextureSize{ size, size } ) };
        return Ret;
    }

    Result CFrameGraph::SetupPresent( CSwapChain* const pSwapChain, uint8_t backBufferIdx )
    {
        VKE_ASSERT( false );
        return VKE_FAIL;
    }

    CFrameGraphNode* CFrameGraph::_SetNextNode( CFrameGraphNode** ppCurrNode, CFrameGraphNode* pNext )
    {
        auto pCurrNode         = *ppCurrNode;
        pCurrNode->m_pNextNode = pNext;
        pNext->m_fenceValue    = _AcquireNodeIndex();
        // For every execute node, try to figure out the last one
        // Last executed command buffer's fence should be frame's fence
        if( pNext->m_doExecute )
        {
            for( uint32_t i = 0; i < CFrameGraph::MAX_BACKBUFFER_COUNT; ++i )
            {
                m_aFrameData[ i ].cpuFenceIndex = pNext->m_Index.cpuFence;
            }
        }
        if( pCurrNode->HasCommandBuffer() )
        {
            // Use the same command buffer if both passes use the same
            // command buffer name, context and thread
            if( pCurrNode->m_CommandBufferName == pNext->m_CommandBufferName &&
                pCurrNode->m_ThreadName == pNext->m_ThreadName && pCurrNode->m_ctxType == pNext->m_ctxType )
            {
                pNext->m_Index.commandBuffer = pCurrNode->m_Index.commandBuffer;
            }
        }
        m_vpNextNodes.PushBackUnique( pNext );
        return pNext;
    }

    void CFrameGraph::_IsNodeEnabled( CFrameGraphNode** ppCurrNode, bool isEnabled )
    {
        // auto pCurrNode = *ppCurrNode;
    }
} // namespace VKE::RenderSystem
