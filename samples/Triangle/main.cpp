

#include "../CSampleFramework.h"

struct SGfxContextListener : public VKE::RenderSystem::EventListeners::IGraphicsContext
{
    VKE::RenderSystem::VertexBufferRefPtr pVb;
    VKE::RenderSystem::ShaderRefPtr pVS;
    VKE::RenderSystem::ShaderRefPtr pPS;
    //VKE::RenderSystem::SVertexInputLayoutDesc Layout;
    VKE::RenderSystem::PipelineRefPtr pPipeline;

    SGfxContextListener()
    {

    }

    virtual ~SGfxContextListener()
    {

    }

    void LoadShaders( VKE::RenderSystem::CDeviceContext* pCtx )
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

        pVS = pCtx->CreateShader( VsDesc );
        pPS = pCtx->CreateShader( PsDesc );
    }

    bool Init( VKE::RenderSystem::CDeviceContext* pCtx )
    {
        LoadShaders( pCtx );

        auto pGfxCtx = pCtx->GetGraphicsContext( 0 );
        auto pCmdBuffer = pGfxCtx->GetCommandBuffer();

        auto pFrameGraph = pCtx->GetRenderSystem()->GetFrameGraph();
        auto pPass = pFrameGraph->GetPass( "RenderFrame" );

        VKE::RenderSystem::SPipelineLayoutDesc LayoutDesc;
        LayoutDesc.SetDebugName( "Test" );

        auto pLayout = pCtx->CreatePipelineLayout( LayoutDesc );

        VKE::RenderSystem::SPipelineCreateDesc PipelineTemplate;
        auto& Pipeline = PipelineTemplate.Pipeline;
        Pipeline.hLayout = pLayout->GetHandle();
        Pipeline.InputLayout.topology = VKE::RenderSystem::PrimitiveTopologies::TRIANGLE_LIST;
        Pipeline.InputLayout.vVertexAttributes = {
            VKE::RenderSystem::SPipelineDesc::SInputLayout::SVertexAttribute( "POSITION", VKE::RenderSystem::Formats::R32G32B32_SFLOAT, 0 ),
        };
        Pipeline.Shaders.apShaders[ VKE::RenderSystem::ShaderTypes::VERTEX ] = pVS;
        Pipeline.Shaders.apShaders[ VKE::RenderSystem::ShaderTypes::PIXEL ] = pPS;
        // VKE_RENDER_SYSTEM_SET_DEBUG_NAME( Pipeline, "VKE_DebugView_Batch" );
        Pipeline.SetDebugName( "VKE_Triangle_Simple" );
        Pipeline.hDDIRenderPass = pPass->GetRHIRenderPass();

        pPipeline = pCtx->CreatePipeline( PipelineTemplate );
        
        auto pUploadPass = pFrameGraph->GetPass( "Upload" );
        pUploadPass->AddTask( [this, pCtx](const VKE::RenderSystem::CFrameGraphNode* pPass, uint8_t backBufferIndex) -> VKE::Threads::TASK_RESULT
        {
            auto        pCmdBuffer = pPass->GetCommandBuffer( backBufferIndex );
            static bool uploaded = false;
            if( !uploaded )
            {
                VKE::RenderSystem::SCreateBufferDesc BuffDesc;

                const float vertexData[] = { 0.0f, 0.5f, 0.0f, -0.5f, -0.5f, 0.0f, 0.5f, -0.5f, 0.0f };

                BuffDesc.Create.flags = VKE::Core::CreateResourceFlags::DEFAULT;
                BuffDesc.Buffer.usage = VKE::RenderSystem::BufferUsages::VERTEX_BUFFER;
                BuffDesc.Buffer.memoryUsage =
                    VKE::RenderSystem::MemoryUsages::GPU_ACCESS | VKE::RenderSystem::MemoryUsages::BUFFER;
                BuffDesc.Buffer.vRegions = { { sizeof( vertexData ), 1u } };
                BuffDesc.Buffer.SetDebugName( "VKE_SimpleTriangle_DebugView" );

                auto hVb = pCtx->CreateBuffer( BuffDesc );
                this->pVb      = pCtx->GetBuffer( hVb );

                VKE::RenderSystem::SUpdateMemoryInfo Info;
                Info.pData    = (const void*)vertexData;
                Info.dataSize = sizeof( vertexData );
                pCmdBuffer->GetContext()->UpdateBuffer( pCmdBuffer, Info, &hVb );
                uploaded = true;
            }
            return VKE::Threads::TaskResults::OK;
        }, nullptr );

        auto pRenderFrame = pFrameGraph->CreatePass( [&]( VKE::RenderSystem::CFrameGraphNode** ppNode )
        {
            auto                                   pNode = ( *ppNode );
            VKE::RenderSystem::SFrameGraphNodeDesc Desc;
            Desc.pName = "Triangle";
            VKE::Result ret = pNode->Create( Desc );
            if( VKE_SUCCEEDED( ret ) )
            {
                pNode->SetWorkload(
                    [ & ]( VKE::RenderSystem::CFrameGraphNode* const pPass, uint8_t backBufferIdx ) {
                        auto pCmdBuffer = pPass->GetCommandBuffer( backBufferIdx );

                        if( pPipeline != nullptr && pPipeline->IsResourceReady() )
                        {
                            pCmdBuffer->Bind( pPipeline );
                            pCmdBuffer->Bind( pVb );
                            pCmdBuffer->Draw( 3 );
                        }
                        return VKE::VKE_OK;
                    } );
            }
            return ret;
        } );
        
        
        pPass->AddSubpass( pRenderFrame );
        pFrameGraph->Build();

        return true;
    }

    bool OnRenderFrame(VKE::RenderSystem::CGraphicsContext* pCtx) override
    {
        /*
        auto pCmdBuffer = pCtx->BeginFrame();

        if( pPipeline!= nullptr && pPipeline->IsResourceReady() )
        {
            pCmdBuffer->Bind( pPipeline );
            pCmdBuffer->Bind( pVb );
            pCmdBuffer->Draw( 3 );
        }

        pCtx->EndFrame();
        */
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

        if( Sample.Create( Desc ) )
        {
            SGfxContextListener* pListener = reinterpret_cast<SGfxContextListener*>(apListeners[0]);
            if( pListener->Init( Sample.m_vpDeviceContexts[0] ) )
            {
                Sample.Start();
            }
        }
        VKE_DELETE( apListeners[ 0 ] );
        Sample.Destroy();
    }

    return 0;
}