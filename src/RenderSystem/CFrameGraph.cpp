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
    RENDER_TARGET_RENDER_PASS_OP FrameGraphPassToColorRenderTargetOp( RENDER_PASS_OP op )
    {
        static const RENDER_TARGET_RENDER_PASS_OP scValues[ FrameGraphPassOperations::_MAX_COUNT ] = {
            RenderTargetRenderPassOperations::COLOR,
            RenderTargetRenderPassOperations::COLOR_STORE,
            RenderTargetRenderPassOperations::COLOR_CLEAR_STORE,
            RenderTargetRenderPassOperations::COLOR
        };
        return scValues[ op ];
    }

    RENDER_TARGET_RENDER_PASS_OP
    FrameGraphPassToDepthRenderTargetOp( RENDER_PASS_OP op )
    {
        static const RENDER_TARGET_RENDER_PASS_OP scValues[ FrameGraphPassOperations::_MAX_COUNT ] = {
            RenderTargetRenderPassOperations::DEPTH_STENCIL,
            RenderTargetRenderPassOperations::DEPTH_STENCIL_STORE,
            RenderTargetRenderPassOperations::DEPTH_STENCIL_CLEAR_STORE,
            RenderTargetRenderPassOperations::DEPTH_STENCIL
        };
        return scValues[ op ];
    }

    TEXTURE_STATE FrameGraphPassOpToColorTextureState( RENDER_PASS_OP op )
    {
        static const TEXTURE_STATE scValues[ FrameGraphPassOperations::_MAX_COUNT ] = {
            TextureStates::SHADER_READ,
            TextureStates::COLOR_RENDER_TARGET,
            TextureStates::COLOR_RENDER_TARGET,
            TextureStates::COLOR_RENDER_TARGET
        };
        return scValues[ op ];
    }

    TEXTURE_STATE FrameGraphPassOpToDepthTextureState( RENDER_PASS_OP op )
    {
        static const TEXTURE_STATE scValues[ FrameGraphPassOperations::_MAX_COUNT ] = {
            TextureStates::SHADER_READ,
            TextureStates::DEPTH_RENDER_TARGET,
            TextureStates::DEPTH_RENDER_TARGET,
            TextureStates::DEPTH_RENDER_TARGET
        };
        return scValues[ op ];
    }

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
                                                            .operation = FrameGraphPassOperations::OVERWRITE },
                                                          { .pName     = "Depth",
                                                            .format    = Formats::D32_SFLOAT,
                                                            .operation = FrameGraphPassOperations::OVERWRITE } } } );
                    auto pFinishRenderFramePass = CreatePass(
                        { .pName          = "FinishRenderFrame",
                          .vRenderTargets = { { .pName = "Diffuse", .operation = FrameGraphPassOperations::READ } } } );
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
            m_vpThreads[ i ]->join();
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
            if( Data.hFrameFence == NativeAPI::Null )
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
        VKE_ASSERT( pRet.IsValid() );
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
                return !ThreadData.qWorkloads.empty();
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

namespace VKE::RenderSystem
{
    const FrameGraphWorkload CFrameGraphNode::EmptyWorkload = []( CFrameGraphNode* pPass, uint8_t backBufferIndex ) {
        Result ret = pPass->OnWorkloadBegin( backBufferIndex );
        return pPass->OnWorkloadEnd( ret );
    };

    Result CFrameGraphNode::_Create( const SFrameGraphPassDesc& Desc )
    {
        Result ret          = VKE_OK;
        m_ctxType           = Desc.contextType;
        m_Name              = Desc.pName;
        m_ThreadName        = Desc.pThread;
        m_CommandBufferName = Desc.pCommandBuffer;
        m_ExecuteName       = Desc.pExecute;
        m_Workload          = EmptyWorkload;
        m_isAsync           = strcmp( Desc.pThread, "Main" ) != 0;
        auto pThis          = this;
        ret                 = m_pFrameGraph->_OnCreateNode( Desc, &pThis );
        return ret;
    }

    void CFrameGraphNode::_Destroy()
    {
    }

    Result CFrameGraphNode::OnWorkloadBegin( uint8_t backBufferIndex )
    {
        /// TODO: WaitForThreads can cause huge CPU overhead
        Result ret = _WaitForThreads();
        if( VKE_SUCCEEDED( ret ) )
        {
            if( HasCommandBuffer() )
            {
                m_pCommandBuffer = GetCommandBuffer( backBufferIndex );
                auto pDevice     = GetContext()->GetDeviceContext();
                for( auto& Pair: m_mTaskResults )
                {
                    if( pDevice->IsReadyToUse( Pair.first ) )
                    {
                    }
                }
                for( uint32_t i = 0; i < m_vpColorRenderTargets.GetCount(); ++i )
                {
                    auto pTex = m_vpColorRenderTargets[ i ];
                    // TEXTURE_STATE state = FrameGraphPassOpToColorTextureState();
                    m_pCommandBuffer->SetState( m_BeginRenderPassInfo.vColorRenderTargetInfos[ i ].state, &pTex );
                }
                if( m_pDepthStencilRenderTarget.IsValid() )
                {
                    m_pCommandBuffer->SetState( m_BeginRenderPassInfo.DepthRenderTargetInfo.state,
                                                &m_pDepthStencilRenderTarget );
                }
                if( HasRenderPass() )
                {
                    m_pCommandBuffer->BeginRenderPass( m_BeginRenderPassInfo );
                }
            }
        }
        VKE_ASSERT( VKE_SUCCEEDED( ret ) );
        m_vSyncObjects.Clear();
        m_finished = false;
        return ret;
    }

    CommandBufferPtr CFrameGraphNode::GetCommandBuffer( uint8_t backBufferIndex ) const
    {
        CommandBufferPtr pCommandBuffer;
        if( HasCommandBuffer() )
        {
            pCommandBuffer = m_pFrameGraph->_GetCommandBuffer( this, backBufferIndex );
            const auto state = pCommandBuffer->GetState();
            if( state != CommandBufferStates::FLUSH )
            {
                if( state == CommandBufferStates::EXECUTED )
                {
                    pCommandBuffer->Reset();
                }
                if( state == CommandBufferStates::RESET )
                {
                    pCommandBuffer->Begin();
                }
                VKE_ASSERT( pCommandBuffer->GetState() == CommandBufferStates::BEGIN );
            }
            pCommandBuffer->SetBackBufferIndex( backBufferIndex );
        }
        return pCommandBuffer;
    }

    Result CFrameGraphNode::OnWorkloadEnd( Result workloadResult )
    {
        if( VKE_SUCCEEDED( workloadResult ) )
        {
            m_pFrameGraph->_ExecuteSubpassNodes( this );

            if( HasRenderPass() )
            {
                m_pCommandBuffer->EndRenderPass();
            }
        }
        IncrementThreadFence();
        m_finished = true;
        m_CondVar.notify_all();
        return workloadResult;
    }

    Result CFrameGraphNode::_Run( CFrameGraphNode* pLastNode )
    {
        Result res = VKE_OK;
        if( pLastNode && !pLastNode->IsSubpass() )
        {
            this->m_pPrevNode = pLastNode;
        }
        if( m_isAsync )
        {
            auto& ThreadData = m_pFrameGraph->_GetThreadData( m_Index.thread );

            ThreadData.qWorkloads.push_back( { m_Workload, this, m_pFrameGraph->m_backBufferIndex } );
            ThreadData.CondVar.notify_one();
        }
        else
        {
            res = m_Workload( this, m_pFrameGraph->m_backBufferIndex );
        }
        return res;
    }

    Result CFrameGraphNode::_WaitForThreads()
    {
        Result ret = VKE_OK;
        // auto threadFenceValue = GetThreadFence().Load();
        for( uint32_t i = 0; i < m_vWaitForNodes.GetCount(); ++i )
        {
            auto& WaitInfo = m_vWaitForNodes[ i ];
            if( WaitInfo.WaitOn == WaitOnBits::CPU_WAITS_FOR_CPU )
            {
                CFrameGraphNode* pNode   = WaitInfo.pNode;
                uint64_t         timeout = 2 * 1000 * 1000; // 2 seconds
                ret                      = WaitForFrame( pNode->GetThreadFence(), WaitInfo.frame, timeout );
                if( !VKE_SUCCEEDED( ret ) )
                {
#if !defined( VKE_RENDER_SYSTEM_DEBUG )
                    VKE_LOG_ERR( "A node: '" << this->m_Name << "' reached timeout waiting for node: '"
                                             << m_vWaitForNodes[ i ].pNode->m_Name << "'"
                                             << " with value: " << WaitInfo.pNode->GetThreadFence().Load() );
#endif // VKE_RENDER_SYSTEM_DEBUG
                    break;
                }
            }
        }
        return ret;
    }

    uint64_t CFrameGraphNode::GetFenceValue() const
    {
        return (m_pFrameGraph->m_currentFrameIndex+1) * m_fenceValue;
    }

    uint64_t CFrameGraphNode::InitFenceValue( uint8_t backBufferIndex )
    {
        m_fenceValue = m_pFrameGraph->_AdvanceBackBufferFence(backBufferIndex);
        return m_fenceValue;
    }

    Result CFrameGraphNode::Wait( const Platform::ThreadFence& hFence, uint32_t value, uint64_t timeout )
    {
        Result ret = VKE_OK;
        // VKE_LOG_NO_SYNC( "Fence value: " << hFence.Load() );
        bool res = Platform::Thread::Wait( hFence, value, timeout );
        ret      = res ? VKE_TIMEOUT : VKE_OK;
        // VKE_LOG_NO_SYNC( "Wait for thread fence: " << value << " ret: " << ret );
        return ret;
    }

    Result CFrameGraphNode::WaitForFrame( const Platform::ThreadFence& hFence, WAIT_FOR_FRAME frame, uint64_t timeout )
    {
        auto fidx  = m_pFrameGraph->GetFrameIndex();
        auto value = fidx + frame;
        return Wait( hFence, value, timeout );
    }

    Result CFrameGraphNode::Wait( const NativeAPI::CPUFence& hFence, uint64_t timeout )
    {
        Result ret = VKE_OK;
        bool   res = m_pFrameGraph->m_Desc.pDevice->IsReadyToUse( hFence );
        if( !res )
        {
            ret = VKE_ENOTREADY;
            if( timeout == UINT64_MAX )
            {
                ret = m_pFrameGraph->_GetContext( this )->Wait( hFence );
            }
        }
        return ret;
    }

    CFrameGraphNode* CFrameGraphNode::SetNext( CFrameGraphNode* pNext )
    {
        CFrameGraphNode* pThis = this;
        return m_pFrameGraph->_SetNextNode( &pThis, pNext );
    }

    void CFrameGraphNode::WaitFor( const SWaitInfo& Info )
    {
        m_vWaitForNodes.PushBack( Info );
        if( Info.WaitOn == WaitOnBits::GPU_WAITS_FOR_GPU )
        {
            Info.pNode->_SignalGPUFence();
        }
    }

    const TexturePtr CFrameGraphNode::GetColorRenderTarget( uint32_t index ) const
    {
        TexturePtr pRet;
        if( m_vpColorRenderTargets.GetCount() > index )
        {
            pRet = m_vpColorRenderTargets[ index ];
        }
        return pRet;
    }

    FORMAT CFrameGraphNode::GetDepthRenderTargetFormat() const
    {
        FORMAT ret = Formats::UNDEFINED;
        if( m_pDepthStencilRenderTarget.IsValid() )
        {
            ret = m_pDepthStencilRenderTarget->GetDesc().format;
        }
        return ret;
    }

    

    CFrameGraphNode* CFrameGraphNode::AddSubpass( CFrameGraphNode* pNode, uint32_t index )
    {
        pNode->m_pParent   = this;
        pNode->m_isSubpass = true;
        auto ppCurr        = &m_pSubpassNode;

        for( uint32_t i = 0; i < index; ++i )
        {
            if( *ppCurr )
            {
                ppCurr = &( *ppCurr )->m_pNextNode;
            }
            else
            {
                break;
            }
        }
        pNode->m_fenceValue = m_pFrameGraph->_AcquireNodeIndex();
        pNode->m_pNextNode = *ppCurr;
        *ppCurr            = pNode;

        return this;
    }

    CFrameGraphNode* CFrameGraphNode::AddSubpass( cstr_t pName, FrameGraphWorkload&& Wl )
    {
        SFrameGraphNodeDesc Desc;
        Desc.pName = pName;
        auto pNode = m_pFrameGraph->CreatePass( Desc );
        auto pRet  = this;
        if( pNode )
        {
            pNode->m_fenceValue = m_pFrameGraph->_AcquireNodeIndex();
            pNode->SetWorkload( std::forward< FrameGraphWorkload >( Wl ) );
            pRet = AddSubpass( pNode );
        }
        return pRet;
    }

    Platform::ThreadFence& CFrameGraphNode::GetThreadFence()
    {
        // return m_pFrameGraph->_GetThreadFence( m_Index.threadFence );
        return m_hFence;
    }

    void CFrameGraphNode::SignalThreadFence( uint32_t value )
    {
        // std::unique_lock l( m_CondVarMtx );
        GetThreadFence().Store( value );
    }

    void CFrameGraphNode::IncrementThreadFence()
    {
        // std::unique_lock l( m_CondVarMtx );
        auto& Fence = GetThreadFence();
        ++Fence;
        // Platform::Debug::PrintOutput( "%s Signal fence: %d\n", m_Name.GetData(), Fence.Load() );
        // VKE_LOG_NO_SYNC( m_Name << " = " << Fence.Load() );
    }

    void CFrameGraphNode::_SignalGPUFence()
    {
        if( m_pExecuteNode != nullptr )
        {
            m_pExecuteNode->m_executeFlags |= ExecuteCommandBufferFlags::SIGNAL_GPU_FENCE;
        }
    }

    void CFrameGraphNode::AddTask( TaskFunc&& Func, CFrameGraphNode::STaskResult* pResult )
    {
        static STaskResult  DummyTaskResult;
        Threads::ScopedLock l( m_TaskSyncObj );
        // Do not use nullptr in order to avoid redundant if-checks
        pResult = pResult ? pResult : &DummyTaskResult;
        m_qTasks.push_back( { pResult, std::move( Func ) } );
    }

    void CFrameGraphNode::_ExecuteTasks( const SExecuteTaskDesc& Desc )
    {
        uint32_t taskExecutedCount = 0;
        for( auto Itr = m_qTasks.begin(); Itr != m_qTasks.end(); ++Itr )
        {
            bool taskExecuted            = Itr->Func( this, Desc.backBufferIndex );
            taskExecutedCount           += taskExecuted;
            bool removeTask              = taskExecuted || Desc.forceRemove;
            Itr->pResult->executedOnCPU  = true;
            if( removeTask )
            {
                Threads::ScopedLock l( m_TaskSyncObj );
                m_qTasks.erase( Itr );
            }
            if( Desc.executeTaskCount >= taskExecutedCount )
            {
                break;
            }
        }
    }

    void CFrameGraphNode::_CreateBeginRenderPassInfo( const SFrameGraphNodeDesc& Desc )
    {
        uint32_t writeCount = 0;
        for( uint32_t i = 0; i < Desc.vRenderTargets.GetCount(); ++i )
        {
            const SFrameGraphRenderTargetTextureDesc& RTDesc   = Desc.vRenderTargets[ i ];
            TexturePtr                                pTexture = m_pFrameGraph->_GetTexture( RTDesc );
            if( pTexture.IsValid() )
            {
                writeCount           += ( RTDesc.operation == FrameGraphPassOperations::WRITE ||
                                RTDesc.operation == FrameGraphPassOperations::OVERWRITE ||
                                RTDesc.operation == FrameGraphPassOperations::READ_WRITE );
                TextureViewPtr pView  = pTexture->GetView();
                if( pTexture->IsColor() )
                {
                    SRenderTargetInfo Info = { .hDDIView   = pView->GetDDIObject(),
                                               .format     = pView->GetDesc().format,
                                               .ClearColor = SClearValue( 0, 0, 0, 0 ),
                                               .state      = FrameGraphPassOpToColorTextureState( RTDesc.operation ),
                                               .renderPassOp =
                                                   FrameGraphPassToColorRenderTargetOp( RTDesc.operation ) };
                    m_BeginRenderPassInfo.vColorRenderTargetInfos.PushBack( Info );
                    m_vpColorRenderTargets.PushBack( pTexture );
                    m_vColorRenderTargetFormats.PushBack( RTDesc.format );
                }
                else
                {
                    m_BeginRenderPassInfo.DepthRenderTargetInfo.hDDIView   = pView->GetDDIObject();
                    m_BeginRenderPassInfo.DepthRenderTargetInfo.ClearColor = SClearValue( 1, 0 );
                    m_BeginRenderPassInfo.DepthRenderTargetInfo.format     = pView->GetDesc().format;
                    m_BeginRenderPassInfo.DepthRenderTargetInfo.renderPassOp =
                        FrameGraphPassToDepthRenderTargetOp( RTDesc.operation );
                    m_BeginRenderPassInfo.DepthRenderTargetInfo.state =
                        FrameGraphPassOpToDepthTextureState( RTDesc.operation );
                    m_pDepthStencilRenderTarget = pTexture;
                }
            }
        }
        m_BeginRenderPassInfo.SetDebugName( m_Name.GetData() );
        m_BeginRenderPassInfo.RenderArea = m_pFrameGraph->_GetRenderArea( Desc.size );
        m_hasRenderPass                  = writeCount > 0;
    }

    void CFrameGraphNode::_BeginRenderPass()
    {
        if( HasCommandBuffer() )
        {
        }
    }

    Scene::ScenePtr CFrameGraphNode::GetScene()
    {
        return m_pFrameGraph->GetScene();
    }

} // namespace VKE::RenderSystem

namespace VKE::RenderSystem
{
    CFrameGraphExecuteNode* CFrameGraphExecuteNode::AddToExecute( CFrameGraphNode* pNode )
    {
        VKE_ASSERT( pNode->m_pExecuteNode == nullptr );
        m_vpNodesToExecute.PushBackUnique( pNode );
        pNode->m_pExecuteNode = this;
        return this;
    }

    CFrameGraphExecuteNode* CFrameGraphExecuteNode::AddToExecute( CFrameGraphExecuteNode* pNode )
    {
        return static_cast< CFrameGraphExecuteNode* >( this->AddSubpass( pNode ) );
    }

    CFrameGraphExecuteNode::SExecuteData* CFrameGraphExecuteNode::_BuildDataToExecute( uint8_t backBufferIndex )
    {
        auto&  Exe = m_aExecutes[ backBufferIndex ];
        Exe.vpCommandBuffers.Clear();
        for( uint32_t n = 0; n < m_vWaitForNodes.GetCount(); ++n )
        {
            const auto& WaitInfo = m_vWaitForNodes[ n ];
            if( WaitInfo.pNode->IsEnabled() && WaitInfo.WaitOn != WaitOnBits::NONE )
            {
                // GPU waits on GPU
                if( WaitInfo.WaitOn == WaitOnBits::GPU_WAITS_FOR_GPU )
                {
                    auto waitForValue = WaitInfo.pNode->GetFenceValue();
                    VKE_ASSERT( waitForValue < GetFenceValue() );
                    // Wait for highest value
                    Exe.SubmitInfo.waitForFenceValue = Math::Max( waitForValue, Exe.SubmitInfo.waitForFenceValue );
                }
                // CPU waits for GPU
                if( WaitInfo.WaitOn == WaitOnBits::CPU_WAITS_FOR_GPU )
                {
                    auto waitForValue = WaitInfo.pNode->GetFenceValue();
                    VKE_ASSERT( waitForValue < GetFenceValue() );
                    // Wait for highest value
                    Exe.SubmitInfo.waitForFenceValue = Math::Max( waitForValue, Exe.SubmitInfo.waitForFenceValue );
                }
                if( WaitInfo.WaitOn == WaitOnBits::CPU_WAITS_FOR_GPU )
                {
                    auto waitForValue = WaitInfo.pNode->GetFenceValue();
                    VKE_ASSERT( waitForValue < GetFenceValue() );
                    // Wait for highest value
                    Exe.SubmitInfo.waitForFenceValue = Math::Max( waitForValue, Exe.SubmitInfo.waitForFenceValue );
                }
            }
        }
        for( uint32_t nodeIdx = 0; nodeIdx < m_vpNodesToExecute.GetCount(); ++nodeIdx )
        {
            auto& pNode = m_vpNodesToExecute[ nodeIdx ];
            if( pNode->IsEnabled() )
            {
                auto pCb = pNode->GetCommandBuffer( backBufferIndex );
                if( pCb.IsValid() && pCb->GetState() == CommandBufferStates::BEGIN )
                {
                    if( VKE_SUCCEEDED(pCb->End()))
                    {
                        if( VKE_SUCCEEDED( pCb->Flush() ) )
                        {
                            // VKE_LOG( "Execute batch: " << this->m_Name.GetData() << ", cb: " << pCb->GetDebugName()
                            // );
                            Exe.vpCommandBuffers.PushBackUnique( pCb->GetDDIObject() );
                        }
                        else
                        {
                            VKE_LOG_ERR( "Unable to flush cmd buffer." );
                        }
                    }
                    else
                    {
                        VKE_LOG_ERR( "Unable to end cmd buffer." );
                    }
                }
            }
        }
        //m_fenceValue = m_pFrameGraph->IncrementFrameFenceValue( backBufferIndex );
        //VKE_ASSERT( m_fenceValue > m_pFrameGraph->m_aFrameData[backBufferIndex].frameFenceValue );
        //m_fenceValue                      = m_pFrameGraph->m_aFrameData[ backBufferIndex ].frameFenceValue + 1;
        Exe.SubmitInfo.commandBufferCount = (uint16_t)Exe.vpCommandBuffers.GetCount();
        Exe.SubmitInfo.pDDICommandBuffers = Exe.vpCommandBuffers.GetData();
        Exe.SubmitInfo.signalFenceValue   = GetFenceValue();
        Exe.SubmitInfo.hSignalFence       = m_pFrameGraph->GetFrameFence( backBufferIndex );
        Exe.SubmitInfo.hDDIQueue          = this->GetContext()->GetNativeQueue();
     
        return &Exe;
    }
} // namespace VKE::RenderSystem
