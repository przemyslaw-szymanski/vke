

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

    bool LoadShaders(VKE::RenderSystem::CDeviceContext* pDevice)
    {
        auto pFrameGraph = pDevice->GetRenderSystem()->GetFrameGraph();

        auto pResMgr = VKEGetEngine()->GetResourceManager();
        {
            VKE::RenderSystem::SCreateShaderDesc VsDesc, PsDesc;
            VsDesc.Create.flags = VKE::Core::CreateResourceFlags::DEFAULT;
            VsDesc.Create.stages = VKE::Core::ResourceStages::FULL_LOAD;
            VsDesc.Shader.FileInfo.FileName = "Data/Samples/Shaders/simple.vs.hlsl";
            VsDesc.Shader.type = VKE::RenderSystem::ShaderTypes::VERTEX;
            PsDesc.Create.flags = VKE::Core::CreateResourceFlags::DEFAULT;
            PsDesc.Create.stages = VKE::Core::ResourceStages::FULL_LOAD;
            PsDesc.Shader.FileInfo.FileName = "Data/Samples/shaders/simple.ps.hlsl";
            PsDesc.Shader.type = VKE::RenderSystem::ShaderTypes::PIXEL;
            pVS = pResMgr->LoadShader( VsDesc );
            pPS = pResMgr->LoadShader( PsDesc );
        }
        {
            auto pRenderPass = pFrameGraph->GetPass( "RenderFrame" );
            const auto& vColorFormats = pRenderPass->GetColorRenderTargetFormats();
            const auto& depthFormat = pRenderPass->GetDepthRenderTargetFormat();
            VKE::RenderSystem::SCreateBindingDesc BindingDesc;
            BindingDesc.SetDebugName( "BasicBackgroundLoading" );
            auto hBindings = pDevice->CreateResourceBindings( BindingDesc );
            auto hDescLayout = pDevice->GetDescriptorSetLayout( hBindings );
            VKE::RenderSystem::SPipelineLayoutDesc PipelineLayoutDesc;
            PipelineLayoutDesc.vDescriptorSetLayouts.PushBack( hDescLayout );
            PipelineLayoutDesc.SetDebugName( "BasicLayout" );
            auto hPipelineLayout = pDevice->CreatePipelineLayout( PipelineLayoutDesc );
            VKE::RenderSystem::SPipelineCreateDesc PipelineDesc;
            VKE::RenderSystem::SPipelineDesc& Pipeline = PipelineDesc.Pipeline;
            Pipeline.Rasterization.Polygon.cullMode = VKE::RenderSystem::CullModes::NONE;
            Pipeline.InputLayout.enable = true;
            Pipeline.InputLayout.vVertexAttributes
                = { { "POSITION", VKE::RenderSystem::Formats::R32G32B32_SFLOAT, 0u } };
            Pipeline.InputLayout.topology = VKE::RenderSystem::PRIMITIVE_TOPOLOGY::TRIANGLE_LIST;
            Pipeline.Shaders.apShaders[ VKE::RenderSystem::ShaderTypes::VERTEX ] = pVS;
            Pipeline.Shaders.apShaders[ VKE::RenderSystem::ShaderTypes::PIXEL ] = pPS;
            Pipeline.hLayout = hPipelineLayout->GetHandle();
            Pipeline.vColorRenderTargetFormats = vColorFormats;
            Pipeline.depthRenderTargetFormat = depthFormat;
            Pipeline.SetDebugName( "BasicBackgroundLoading" );
            pPSO = pResMgr->CreatePipeline( PipelineDesc );
        }
        {
            auto pPass = pFrameGraph->GetPass( "CompileShaders" );
            pPass->AddTask(
                [ & ]( const VKE::RenderSystem::CFrameGraphNode* pNode, uint8_t ) {
                    auto pDevice = pNode->GetContext()->GetDeviceContext();

                    VKE::RenderSystem::SCreateShaderDesc VsDesc, PsDesc;

                    VsDesc.Create.flags = VKE::Core::CreateResourceFlags::DEFAULT;
                    VsDesc.Create.stages = VKE::Core::ResourceStages::FULL_LOAD;
                    VsDesc.Shader.FileInfo.FileName = "Data/Samples/Shaders/simple.vs.hlsl";
                    VsDesc.Shader.type = VKE::RenderSystem::ShaderTypes::VERTEX;

                    PsDesc.Create.flags = VKE::Core::CreateResourceFlags::DEFAULT;
                    PsDesc.Create.stages = VKE::Core::ResourceStages::FULL_LOAD;
                    PsDesc.Shader.FileInfo.FileName = "Data/Samples/shaders/simple.ps.hlsl";
                    PsDesc.Shader.type = VKE::RenderSystem::ShaderTypes::PIXEL;

                    pVS = pDevice->CreateShader( VsDesc );
                    pPS = pDevice->CreateShader( PsDesc );

                    return true;
                },
                &ShaderCompiledResult );
        }

        {
            auto pPass = pFrameGraph->GetPass( "Upload" );
            pPass->AddTask(
                [ & ]( const VKE::RenderSystem::CFrameGraphNode* pNode, uint8_t bbidx ) {
                    auto pDevice = pNode->GetContext()->GetDeviceContext();
                    auto pCmdBuffer = pNode->GetCommandBuffer( bbidx );
                    VKE::RenderSystem::SCreateBufferDesc BuffDesc;
                    const float vertexData[] = { 0.0f, 0.5f, 0.0f, -0.5f, -0.5f, 0.0f, 0.5f, -0.5f, 0.0f };
                    BuffDesc.Create.flags = VKE::Core::CreateResourceFlags::DEFAULT;
                    BuffDesc.Buffer.usage = VKE::RenderSystem::BufferUsages::VERTEX_BUFFER;
                    BuffDesc.Buffer.memoryUsage
                        = VKE::RenderSystem::MemoryUsages::GPU_ACCESS | VKE::RenderSystem::MemoryUsages::BUFFER;
                    BuffDesc.Buffer.size = sizeof( vertexData );
                    BuffDesc.Buffer.SetDebugName( "VKE_SimpleTriangle_DebugView" );
                    auto hVb = pDevice->CreateBuffer( BuffDesc );
                    pVb = pDevice->GetBuffer( hVb );
                    VKE::RenderSystem::SUpdateMemoryInfo Info;
                    Info.pData = ( const void* )vertexData;
                    Info.dataSize = sizeof( vertexData );
                    pCmdBuffer->GetContext()->UpdateBuffer( pCmdBuffer, Info, &hVb );
                    return pVb.IsValid();
                },
                &UploadVertexDataResult );

            pPass->AddTask(
                [ & ]( const VKE::RenderSystem::CFrameGraphNode* pNode, uint8_t backBufferIdx ) {
                    if( ShaderCompiledResult.executedOnCPU )
                    {
                        /*auto pDevice = pNode->GetContext()->GetDeviceContext();
                        auto pFrameGraph = pNode->GetFrameGraph();
                        auto pRenderPass = pFrameGraph->GetPass( "RenderFrame" );

                        const auto& vColorFormats = pRenderPass->GetColorRenderTargetFormats();
                        const auto& depthFormat = pRenderPass->GetDepthRenderTargetFormat();

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

                        pPSO = pDevice->CreatePipeline( PipelineDesc );*/

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

        auto pPass = pFrameGraph->GetPass( "RenderFrame" );
        pPass->AddSubpass( pRenderFrame );
        pFrameGraph->Build();

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