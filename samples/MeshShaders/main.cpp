

#include "../CSampleFramework.h"

struct SGfxContextListener : public VKE::RenderSystem::EventListeners::IGraphicsContext
{
    VKE::RenderSystem::VertexBufferRefPtr pVb;
    VKE::RenderSystem::ShaderRefPtr pTs;
    VKE::RenderSystem::ShaderRefPtr pMs;
    VKE::RenderSystem::ShaderRefPtr pPs;
    VKE::RenderSystem::SVertexInputLayoutDesc Layout;
    VKE::RenderSystem::CFrameGraphNode::STaskResult ShaderCompiledResult;
    VKE::RenderSystem::CFrameGraphNode::STaskResult UploadResult;
    VKE::RenderSystem::PipelineRefPtr pPSO;

    SGfxContextListener()
    {

    }

    virtual ~SGfxContextListener()
    {

    }

    void LoadShaders( VKE::RenderSystem::CDeviceContext* pCtx )
    {
        VKE::RenderSystem::SCreateShaderDesc TsDesc, MsDesc;
        TsDesc.Create.flags = VKE::Core::CreateResourceFlags::DEFAULT;
        TsDesc.Create.stages = VKE::Core::ResourceStages::FULL_LOAD;
        TsDesc.Shader.FileInfo.FileName = "Data/Samples/Shaders/simple.ts";
        MsDesc = TsDesc;
        MsDesc.Shader.FileInfo.FileName = "Data/Samples/shaders/simple.ms";
        //pCtx->CreateShader( TsDesc );
        //pCtx->CreateShader( MsDesc );
        auto pFrameGraph = pCtx->GetRenderSystem()->GetFrameGraph();
        {
            auto pPass = pFrameGraph->GetPass( "CompileShaders" );
            pPass->AddTask(
                [&]( const VKE::RenderSystem::CFrameGraphNode* pNode, uint8_t ) {
                    auto pDevice = pNode->GetContext()->GetDeviceContext();
                    VKE::RenderSystem::SCreateShaderDesc TsDesc, MsDesc, PsDesc;
                    TsDesc.Create.flags = VKE::Core::CreateResourceFlags::DEFAULT;
                    TsDesc.Create.stages = VKE::Core::ResourceStages::FULL_LOAD;
                    TsDesc.Shader.FileInfo.FileName = "Data/Samples/Shaders/simple.ts.hlsl";
                    MsDesc = TsDesc;
                    MsDesc.Shader.FileInfo.FileName = "Data/Samples/shaders/simple.ms.hlsl";
                    PsDesc = TsDesc;
                    PsDesc.Shader.FileInfo.FileName = "Data/Samples/shaders/simple.ps.hlsl";
                    pTs = pDevice->CreateShader( TsDesc );
                    pMs = pDevice->CreateShader( MsDesc );
                    pPs = pDevice->CreateShader( PsDesc );
                    return true;
                },
                &ShaderCompiledResult );
        }
        {
            auto pPass = pFrameGraph->GetPass( "UploadData" );
            pPass->AddTask( [&]( const VKE::RenderSystem::CFrameGraphNode* pNode, uint8_t )
            {
                if( ShaderCompiledResult.executedOnCPU )
                {
                    auto pFrameGraph = pNode->GetFrameGraph();
                    auto pRenderPass = pFrameGraph->GetPass( "RenderFrame" );
                    const auto& vColorFormaats = pRenderPass->GetColorRenderTargetFormats();
                    const auto& depthFormat = pRenderPass->GetDepthRenderTargetFormat();

                    auto pDevice = pNode->GetContext()->GetDeviceContext();

                    VKE::RenderSystem::SCreateBindingDesc BindingDesc;
                    BindingDesc.SetDebugName( "SimpleMS" );
                    auto hBindings = pDevice->CreateResourceBindings( BindingDesc );
                    auto hDescLayout = pDevice->GetDescriptorSetLayout( hBindings );
                    VKE::RenderSystem::SPipelineLayoutDesc PipelineLayoutDesc;
                    PipelineLayoutDesc.vDescriptorSetLayouts.PushBack( hDescLayout );
                    auto hPipelineLayout = pDevice->CreatePipelineLayout( PipelineLayoutDesc );
                    VKE::RenderSystem::SPipelineCreateDesc PipelineDesc;
                    VKE::RenderSystem::SPipelineDesc& Pipeline = PipelineDesc.Pipeline;
                    Pipeline.Rasterization.Polygon.cullMode = VKE::RenderSystem::CullModes::NONE;
                    Pipeline.InputLayout.enable = false;
                    Pipeline.Shaders.apShaders[ VKE::RenderSystem::ShaderTypes::MESH ] = pMs;
                    Pipeline.Shaders.apShaders[ VKE::RenderSystem::ShaderTypes::PIXEL ] = pPs;
                    Pipeline.hLayout = hPipelineLayout->GetHandle();
                    Pipeline.vColorRenderTargetFormats = vColorFormaats;
                    Pipeline.depthRenderTargetFormat = depthFormat;
                    Pipeline.SetDebugName( "SimpleMS" );
                    pPSO = pDevice->CreatePipeline( PipelineDesc );
                    return true;
                }
                return false;
            },
            &UploadResult );
        }

    }

    bool Init( VKE::RenderSystem::CDeviceContext* pCtx )
    {
        LoadShaders( pCtx );

        auto pFrameGraph = pCtx->GetRenderSystem()->GetFrameGraph();
        auto pRenderFrame = pFrameGraph->CreatePass( { .pName = "RenderMS" } );
        pRenderFrame->SetWorkload( [&](
            VKE::RenderSystem::CFrameGraphNode* const pPass,
            uint8_t backBufferIdx )
        {
            if( pPSO.IsValid() && pPSO->IsResourceReady() )
            {
                auto pCmdBuffer = pPass->GetCommandBuffer( backBufferIdx );

                pCmdBuffer->Bind( pPSO );
                pCmdBuffer->DrawMesh( 1, 1, 1 );
            }
            return VKE::VKE_OK;
        });
        auto pPass = pFrameGraph->GetPass( "RenderFrame" );
        pPass->AddSubpass( pRenderFrame );
        pFrameGraph->Build();
        return true;
    }

    bool OnRenderFrame(VKE::RenderSystem::CGraphicsContext* pCtx) override
    {
        /*auto pCmdbuffer = pCtx->BeginFrame();
        pCmdbuffer->SetState( Layout );
        pCmdbuffer->Bind( pVb );
        pCmdbuffer->SetState( pVS );
        pCmdbuffer->SetState( pPS );
        pCmdbuffer->Draw( 3 );
        pCtx->EndFrame();*/
        return true;
    }
};

int main()
{
    VKE_DETECT_MEMORY_LEAKS();
    //VKE::Platform::Debug::BreakAtAllocation( 3307 );
    {
        CSampleFramework Sample;
        SSampleCreateDesc Desc;
        VKE::RenderSystem::EventListeners::IGraphicsContext* apListeners[1] =
        {
            VKE_NEW SGfxContextListener()
        };
        Desc.ppGfxListeners = apListeners;
        Desc.gfxListenerCount = 1;
        Desc.featureLevel = VKE::RenderSystem::FeatureLevels::LEVEL_ULTIMATE;

        if( Sample.Create( Desc ) )
        {
            SGfxContextListener* pListener = reinterpret_cast<SGfxContextListener*>(apListeners[0]);
            if( pListener->Init( Sample.m_vpDeviceContexts[0] ) )
            {
                Sample.Start();
            }
        }
        Sample.Destroy();
    }

    return 0;
}