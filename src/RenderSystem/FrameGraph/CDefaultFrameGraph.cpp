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

namespace VKE::RenderSystem
{
    Result CFrameGraph::_CreateDefaultFrameGraph( const SFrameGraphDesc& Desc )
    {
        auto pSwapBufferPass        = CreatePass( {
                   .pName = "SwapBuffers",
        } );
        auto pBeginFramePass        = CreatePass( {
                   .pName = "BeginFrame",
        } );
        auto pRenderFramePass       = CreatePass( {
                  .pName          = "RenderFrame",
                  .vRenderTargets = {{ .pName     = "Diffuse",
.format    = Formats::R8G8B8A8_UNORM,
.operation = FrameGraphPassOperations::OVERWRITE },
                                     { .pName     = "Depth",
                                     .format    = Formats::D32_SFLOAT,
                                     .operation = FrameGraphPassOperations::OVERWRITE }}
        } );
        auto pFinishRenderFramePass = CreatePass(
            { .pName          = "FinishRenderFrame",
              .vRenderTargets = { { .pName = "Diffuse", .operation = FrameGraphPassOperations::READ } } } );
        auto pEndFramePass     = CreatePass( {
                .pName = "EndFrame",
        } );
        auto pExecuteFrame     = CreateExecutePass( { .pName               = "ExecuteFrame",
                                                      //.pThread = "ExecuteFrame",
                                                      .pCommandBufferName  = nullptr,
                                                      .pFenceName          = "Main",
                                                      .executeIndex        = 3,
                                                      .signalEndFrameFence = true } );
        auto pPresent          = CreatePresentPass( { .pName              = "PresentFrame",
                                                      //.pThread = "PresentFrame",
                                                      .pCommandBufferName = nullptr } );
        auto pTextureLoadPass  = CreatePass( { .pName = "LoadTextures", .pCommandBufferName = nullptr } );
        auto pBufferLoadPass   = CreatePass( { .pName = "LoadBuffers", .pCommandBufferName = nullptr } );
        auto pBufferUploadPass = CreatePass( { .pName = "BufferUpload", .pCommandBufferName = "Upload" } );
        auto pCompileShaderPass
            = CreatePass( { .pName = "CompileShaders", .pThread = "CompileShaders", .pCommandBufferName = nullptr } );
        auto pTextureUploadPass    = CreatePass( { .pName = "UploadTextures", .pCommandBufferName = "Upload" } );
        auto pTextureGenMipmapPass = CreatePass( { .pName = "GenMipmaps" } );
        auto pLoadDataPass         = CreatePass( { .pName = "Load", .pCommandBufferName = nullptr } );
        auto pUploadDataPass       = CreatePass( { .pName = "Upload", .pCommandBufferName = "Upload" } );
        auto pSceneUpdatePass      = CreatePass( { .pName = "SceneUpdate", .pCommandBufferName = nullptr } );
        auto pUpdatePass           = CreatePass( { .pName = "Update", .pCommandBufferName = "Update" } );
        auto pExecuteUploadPass    = CreateExecutePass(
            { .pName = "ExecuteUpload", .pCommandBufferName = nullptr, .pFenceName = "Main", .executeIndex = 1 } );
        auto pExecuteUpdatePass = CreateExecutePass(
            { .pName = "ExecuteUpdate", .pCommandBufferName = nullptr, .pFenceName = "Main", .executeIndex = 2 } );
        auto pFinishFramePass   = CreatePass( { .pName = "FinishFrame" } );
        auto pCheckResourcePass = CreatePass(
            { .pName = "CheckResourceState", .pThread = "CheckResourceState", .pCommandBufferName = nullptr } );
        // auto pCreateResourcePass
        //   = CreateCustomPass<VKE::RenderSystem::CFrameGraphMultiWorkloadNode>( { .pName =
        //   "CreateResource" }, nullptr );
        SetRootNode( pSwapBufferPass )
            ->SetNext( pCheckResourcePass )
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
        pRenderFramePass->WaitFor( { .pNode = pUpdatePass, .WaitOn = VKE::RenderSystem::WaitOnBits::THREAD } );
        pExecuteFrame->WaitFor( { .pNode = pSwapBufferPass, .WaitOn = VKE::RenderSystem::WaitOnBits::GPU } );
        pExecuteFrame->WaitFor( { .pNode = pExecuteUpdatePass, .WaitOn = VKE::RenderSystem::WaitOnBits::GPU } );
        pExecuteFrame->AddToExecute( pBeginFramePass );
        pExecuteFrame->AddToExecute( pTextureGenMipmapPass );
        pExecuteFrame->AddToExecute( pRenderFramePass );
        pExecuteFrame->AddToExecute( pEndFramePass );
        pPresent->WaitFor( { .pNode  = pExecuteFrame,
                             .frame  = VKE::RenderSystem::WaitForFrames::CURRENT,
                             .WaitOn = VKE::RenderSystem::WaitOnBits::GPU | VKE::RenderSystem::WaitOnBits::THREAD } );
        pBeginFramePass->WaitFor( { .pNode  = pFinishFramePass,
                                    .frame  = VKE::RenderSystem::WaitForFrames::LAST,
                                    .WaitOn = VKE::RenderSystem::WaitOnBits::THREAD } );
        pFinishFramePass->WaitFor( { .pNode = pEndFramePass, .WaitOn = VKE::RenderSystem::WaitOnBits::THREAD } );
        pSwapBufferPass->SetWorkload(
            [ & ]( VKE::RenderSystem::CFrameGraphNode* const pPass, uint8_t backBufferIdx )
            {
                VKE::Result ret = pPass->OnWorkloadBegin( backBufferIdx );
                if( VKE_SUCCEEDED( ret ) )
                {
                    auto pCtx      = pPass->GetContext()->Reinterpret<CGraphicsContext>();
                    auto pSwpChain = pCtx->GetSwapChain();
                    /*ret = pSwpChain->SwapBuffers( pPass->GetGPUFence( backBufferIdx ),
                                                  VKE::RenderSystem::NativeAPI::Null );*/
                    auto hFence    = pPass->GetFrameGraph()->GetSwapChainFence( backBufferIdx );
                    ret            = pSwpChain->SwapBuffers( hFence, NativeAPI::Null );
                    if( VKE_SUCCEEDED( ret ) )
                    {
                        //pPass->_SetCustomFence( hFence, backBufferIdx );
                    }
                }
                ret = pPass->OnWorkloadEnd( ret );
                return ret;
            } );
        pLoadDataPass->SetWorkload(
            [ & ]( CFrameGraphNode* const pPass, uint8_t backBufferIndex )
            {
                Result ret = pPass->OnWorkloadBegin( backBufferIndex );
                if( VKE_SUCCEEDED( ret ) )
                {
                }
                ret = pPass->OnWorkloadEnd( ret );
                return ret;
            } );
        pUpdatePass->SetWorkload(
            [ & ]( CFrameGraphNode* const pPass, uint8_t backBufferIndex )
            {
                Result ret = pPass->OnWorkloadBegin( backBufferIndex );
                if( VKE_SUCCEEDED( ret ) )
                {
                    pPass->GetScene()->Update( { .pCommandBuffer = pPass->GetCommandBuffer( backBufferIndex ) } );
                }
                ret = pPass->OnWorkloadEnd( ret );
                return ret;
            } );
        pCompileShaderPass->SetWorkload(
            [ & ]( CFrameGraphNode* const pPass, uint8_t backBufferIndex )
            {
                Result ret = pPass->OnWorkloadBegin( backBufferIndex );
                if( VKE_SUCCEEDED( ret ) )
                {
                    pPass->_ExecuteTasks(
                        { .executeTaskCount = 1, .backBufferIndex = backBufferIndex, .forceRemove = false } );
                    auto pResMgr
                        = pPass->GetContext()->GetDeviceContext()->GetRenderSystem()->GetEngine()->GetResourceManager();
                    while( VKE_SUCCEEDED( pResMgr->LoadDeferredShader() ) )
                    {
                    }
                    while( VKE_SUCCEEDED( pResMgr->CreateDeferredPipeline() ) )
                    {
                    }
                }
                ret = pPass->OnWorkloadEnd( ret );
                return ret;
            } );
        pBeginFramePass->SetWorkload(
            [ & ]( VKE::RenderSystem::CFrameGraphNode* const pPass, uint8_t backBufferIdx )
            {
                VKE::Result ret = pPass->OnWorkloadBegin( backBufferIdx );
                if( VKE_SUCCEEDED( ret ) )
                {
                    auto pCtx       = pPass->GetContext()->Reinterpret<VKE::RenderSystem::CGraphicsContext>();
                    auto pCmdBuffer = pPass->GetCommandBuffer( backBufferIdx );
                    auto pSwpChain  = pCtx->GetSwapChain();
                    pSwpChain->BeginFrame( pCmdBuffer );
                    VKE::RenderSystem::STextureBarrierInfo Barrier;
                    auto                                   pBackBufferTex = pSwpChain->GetBackBufferTexture();
                    pBackBufferTex->SetState( VKE::RenderSystem::TextureStates::COLOR_RENDER_TARGET, &Barrier );
                    pCmdBuffer->Barrier( Barrier );
#if VKE_LOG_FRAMEGRAPH
                    VKE_LOG( "BeginFrame SwapChain fence: " << pSwpChain->GetBackBufferGPUFence() );
#endif
                    // pPass->AddSynchronization( SFence{ pSwpChain->GetBackBufferGPUFence(), 0u } );
                    pPass->AddSynchronization(
                        SFence{ pPass->GetFrameGraph()->GetSwapChainFence( backBufferIdx ), 0u } );
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
            [ & ]( RenderSystem::CFrameGraphNode* pPass, uint8_t backBufferIdx )
            {
                Result ret = pPass->OnWorkloadBegin( backBufferIdx );
                if( VKE_SUCCEEDED( ret ) )
                {
                    pPass->GetScene()->Render( pPass->GetCommandBuffer( backBufferIdx ) );
                }
                return pPass->OnWorkloadEnd( ret );
            } );
        pFinishRenderFramePass->SetWorkload(
            [ & ]( RenderSystem::CFrameGraphNode* pPass, uint8_t backBufferIndex )
            {
                VKE::Result ret = pPass->OnWorkloadBegin( backBufferIndex );
                if( VKE_SUCCEEDED( ret ) )
                {
                    auto pCtx       = pPass->GetContext()->Reinterpret<VKE::RenderSystem::CGraphicsContext>();
                    auto pSwapChain = pCtx->GetSwapChain();
                    auto pTex       = pSwapChain->GetBackBufferTexture();
                    auto pCmdBuffer = pPass->GetCommandBuffer( backBufferIndex );
                    pCmdBuffer->SetState( TextureStates::TRANSFER_DST, &pTex );
                    auto pLastPass        = pPass->GetPrev();
                    auto pRenderTargetTex = pLastPass->GetColorRenderTarget( 0 );
                    pCmdBuffer->SetState( TextureStates::TRANSFER_SRC, &pRenderTargetTex );
                    SCopyTextureInfo CopyInfo = {
                        .pSrcTexture = pRenderTargetTex,
                        .pDstTexture = pTex,
                        .Size        = pTex->GetDesc().Size,
                        .depth       = 0,
                        .SrcOffset   = {0, 0},
                        .DstOffset   = {0, 0}
                    };
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
            [ & ]( VKE::RenderSystem::CFrameGraphNode* const pPass, uint8_t backBufferIdx )
            {
                VKE::Result ret = pPass->OnWorkloadBegin( backBufferIdx );
                if( VKE_SUCCEEDED( ret ) )
                {
                    auto pCtx            = pPass->GetContext()->Reinterpret<VKE::RenderSystem::CGraphicsContext>();
                    auto pCmdBuffer      = pPass->GetCommandBuffer( backBufferIdx );
                    // pCmdBuffer->EndRenderPass();
                    auto       pSwpChain = pCtx->GetSwapChain();
                    TexturePtr pTex      = pSwpChain->GetBackBufferTexture();
                    /*VKE_LOG(
                        "Change state: " << pTex->GetDesc().Name << ", " << ( int )pTex->GetState() << " -> PRESENT, "
                                         << pSwpChain->GetNativeBackBufferIndex() << ", "
                                         << pSwpChain->GetBackBufferIndex() );*/
                    pCmdBuffer->SetState( TextureStates::PRESENT, &pTex );
                    ret = EndFrame();
                    // VKE_LOG_NO_SYNC( "end frame " << pCmdBuffer.Get() );
                    /*Platform::Debug::PrintOutput( "end %llx, %d, %d\n",
                        pCmdBuffer.Get(), pCmdBuffer->GetState(), Barrier.hNativeAPITexture );*/
                }
                ret = pPass->OnWorkloadEnd( ret );
                return ret;
            } );
        pFinishFramePass->SetWorkload(
            [ & ]( VKE::RenderSystem::CFrameGraphNode* const pPass, uint8_t backBufferIdx )
            {
                VKE::Result ret = pPass->OnWorkloadBegin( backBufferIdx );
                pPass->GetFrameGraph()->UpdateCounters();
                ret = pPass->OnWorkloadEnd( ret );
                return ret;
            } );
        const auto ResourceDefaultFunc = [ & ]( CFrameGraphNode* const pPass, uint8_t backBufferIndex )
        {
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
            [ & ]( const RenderSystem::CFrameGraphNode* pNode, uint8_t backBufferIndex )
            {
                // One time initializations
                bool ret        = false;
                auto pCmdBuffer = pNode->GetCommandBuffer( backBufferIndex );
                /// TODO: RenderSystem should know nothing about Scene.
                /// Below code is a design flaw.
                /// There should be some callback system where other engine
                /// systems could run their functions.
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
        
        return Build();
    }
} // VKE::RenderSystem