

#include "../CSampleFramework.h"

struct SGfxContextListener : public VKE::RenderSystem::EventListeners::IGraphicsContext
{
    VKE::RenderSystem::VertexBufferRefPtr pVb;
    VKE::RenderSystem::ShaderRefPtr pVS;
    VKE::RenderSystem::ShaderRefPtr pPS;
    VKE::RenderSystem::SVertexInputLayoutDesc Layout;

    VKE::RenderSystem::CFrameGraphNode::STaskResult ShaderCompiledResult;
    VKE::RenderSystem::CFrameGraphNode::STaskResult UploadResult;
    VKE::RenderSystem::CFrameGraphNode::STaskResult UploadVertexDataResult;
    VKE::RenderSystem::PipelineRefPtr pPSO;

    SFpsCounter Fps;

    SGfxContextListener()
    {

    }

    virtual ~SGfxContextListener()
    {

    }

    bool LoadShaders(VKE::RenderSystem::CDeviceContext* pCtx)
    {
        auto pFrameGraph = pCtx->GetRenderSystem()->GetFrameGraph();

        {
            auto pPass = pFrameGraph->GetPass( "CompileShaders" );
            pPass->AddTask(
                [ & ]( const VKE::RenderSystem::CFrameGraphNode* pNode, uint8_t ) {
                    auto pDevice = pNode->GetContext()->GetDeviceContext();

                    VKE::RenderSystem::SCreateShaderDesc VsDesc, PsDesc;

                    VsDesc.Create.flags = VKE::Core::CreateResourceFlags::DEFAULT;
                    VsDesc.Create.stages = VKE::Core::ResourceStages::FULL_LOAD;
                    VsDesc.Shader.FileInfo.FileName = "Data/Samples/Shaders/simple.vs";

                    PsDesc.Create = VsDesc.Create;
                    PsDesc.Shader.FileInfo.FileName = "Data/Samples/shaders/simple.ps";

                    pVS = pDevice->CreateShader( VsDesc );
                    pPS = pDevice->CreateShader( PsDesc );

                    return true;
                },
                &ShaderCompiledResult );
        }

        {
            auto pPass = pFrameGraph->GetPass( "UploadVertexData" );
            pPass->AddTask(
                [ & ]( const VKE::RenderSystem::CFrameGraphNode* pNode, uint8_t backBufferIdx ) {
                    auto pCmdBuffer = pPass->GetCommandBuffer( backBufferIdx );
                    VKE::RenderSystem::SCreateBufferDesc BuffDesc;

                    BuffDesc.Create.flags = VKE::Core::CreateResourceFlags::DEFAULT;
                    BuffDesc.Buffer.usage = VKE::RenderSystem::BufferUsages::VERTEX_BUFFER;
                    BuffDesc.Buffer.memoryUsage = VKE::RenderSystem::MemoryUsages::GPU_ACCESS;
                    BuffDesc.Buffer.size = ( sizeof( float ) * 4 ) * 3;

                    auto hVb = pCtx->CreateBuffer( BuffDesc );
                    pVb = pCtx->GetBuffer( hVb );
                    const float vb[ 4 * 3 ]
                        = { 0.0f, 0.5f, 0.0f, 1.0f, -0.5f, -0.5f, 0.0f, 1.0f, 0.5f, -0.5f, 0.0f, 1.0f };

                    VKE::RenderSystem::SUpdateMemoryInfo Info;
                    Info.pData = vb;
                    Info.dataSize = sizeof( vb );
                    Info.dstDataOffset = 0;
                    pCtx->UpdateBuffer( pCmdBuffer, Info, &hVb );
                    return true;
                },
                &UploadVertexDataResult );
        }

        {
            auto pPass = pFrameGraph->GetPass( "UploadData" );
            pPass->AddTask(
                [ & ]( const VKE::RenderSystem::CFrameGraphNode* pNode, uint8_t ) {
                    if( ShaderCompiledResult.executedOnCPU )
                    {
                        auto pFrameGraph = pNode->GetFrameGraph();
                        auto pRenderPass = pFrameGraph->GetPass( "RenderFrame" );

                        const auto& vColorFormats = pRenderPass->GetColorRenderTargetFormats();
                        const auto& depthFormat = pRenderPass->GetDepthRenderTargetFormat();

                        auto pDevice = pNode->GetContext()->GetDeviceContext();

                        VKE::RenderSystem::SCreateBindingDesc BindingDesc;
                        BindingDesc.SetDebugName( "BasicBackgroundLoading" );
                        auto hBindings = pDevice->CreateResourceBindings( BindingDesc );

                        auto hDescLayout = pDevice->GetDescriptorSetLayout( hBindings );

                        VKE::RenderSystem::SPipelineLayoutDesc PipelineLayoutDesc;
                        PipelineLayoutDesc.vDescriptorSetLayouts.PushBack( hDescLayout );
                        auto hPipelineLayout = pDevice->CreatePipelineLayout( PipelineLayoutDesc );

                        VKE::RenderSystem::SPipelineCreateDesc PipelineDesc;
                        VKE::RenderSystem::SPipelineDesc& Pipeline = PipelineDesc.Pipeline;

                        Pipeline.Rasterization.Polygon.cullMode = VKE::RenderSystem::CullModes::NONE;

                        Pipeline.InputLayout.enable = true;
                        Pipeline.InputLayout.vVertexAttributes = { { "POSITION", VKE::RenderSystem::Formats::R32G32B32_SFLOAT, 0u } };
                        Pipeline.InputLayout.topology = VKE::RenderSystem::PRIMITIVE_TOPOLOGY::TRIANGLE_LIST;

                        Pipeline.Shaders.apShaders[ VKE::RenderSystem::ShaderTypes::VERTEX ] = pVS;
                        Pipeline.Shaders.apShaders[ VKE::RenderSystem::ShaderTypes::PIXEL ] = pPS;
                        Pipeline.hLayout = hPipelineLayout->GetHandle();
                        Pipeline.vColorRenderTargetFormats = vColorFormats;
                        Pipeline.depthRenderTargetFormat = depthFormat;
                        Pipeline.SetDebugName( "BasicBackgroundLoading" );

                        pPSO = pDevice->CreatePipeline( PipelineDesc );

                        return true;
                    }
                    return false;
                },
                &UploadResult );
        }

        return true;

    }

    bool Init( VKE::RenderSystem::CDeviceContext* pCtx )
    {
        LoadShaders( pCtx );

        auto pFrameGraph = pCtx->GetRenderSystem()->GetFrameGraph();
        auto pRenderFrame = pFrameGraph->CreatePass( { .pName = "BasicBackgroundLoading" } );

        pRenderFrame->SetWorkload( [ & ]( VKE::RenderSystem::CFrameGraphNode* const pPass, uint8_t backBufferIdx ) {
            if( pPSO.IsValid() && pPSO->IsResourceReady() &&
                pVb.IsValid() && pVb->IsResourceReady() )
            {
                auto pCmdBuffer = pPass->GetCommandBuffer( backBufferIdx );
                pCmdBuffer->Bind( pPSO );
                pCmdBuffer->Bind( pVb );
                pCmdBuffer->Draw( 3 );
            }
            return VKE::VKE_OK;
        } );

        return true;
    }

    bool OnRenderFrame(VKE::RenderSystem::CGraphicsContext* pCtx) override
    {
        char buff[128];
        vke_sprintf( buff, 128, "%d fps", Fps.GetFps() );
        pCtx->GetSwapChain()->GetWindow()->SetText( buff );

        /*
        SSimpleDrawData Data;
        Data.pLayout = &Layout;
        Data.pVertexBuffer = pVb;
        Data.pPixelShader = pPS;
        Data.pVertexShader = pVS;
        DrawSimpleFrame( pCtx, Data );
        */
        return true;
    }
};

int main()
{   
    VKE_DETECT_MEMORY_LEAKS();

    CSampleFramework Sample;
    SSampleCreateDesc Desc;
    VKE::RenderSystem::EventListeners::IGraphicsContext* apListeners[1] =
    {
        VKE_NEW SGfxContextListener()
    };
    Desc.ppGfxListeners = apListeners;
    Desc.gfxListenerCount = 1;

    if( Sample.Create( Desc ) )
    {
        SGfxContextListener* pListener = reinterpret_cast<SGfxContextListener*>(apListeners[0]);
        if( pListener->Init( Sample.m_vpDeviceContexts[0] ) )
        {
            Sample.Start();
        }
    }
    VKE_DELETE( apListeners[0] );
    Sample.Destroy();
    return 0;
}