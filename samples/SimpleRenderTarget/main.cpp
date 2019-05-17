

#include "../CSampleFramework.h"

struct SGfxContextListener : public VKE::RenderSystem::EventListeners::IGraphicsContext
{
    VKE::RenderSystem::VertexBufferRefPtr pVb;
    VKE::RenderSystem::ShaderRefPtr pVS;
    VKE::RenderSystem::ShaderRefPtr pPS;
    VKE::RenderSystem::SVertexInputLayoutDesc Layout;
    VKE::Resources::STaskResult VsResult, PsResult;
    VKE::RenderSystem::PipelinePtr pPipeline;
    VKE::RenderSystem::TextureHandle hRenderTargetTex;
    VKE::RenderSystem::TextureViewHandle hRenderTargetView;
    VKE::RenderSystem::RenderTargetHandle hRenderTarget;

    VKE::RenderSystem::VertexBufferRefPtr pRtVb;
    VKE::RenderSystem::ShaderRefPtr pRtVS;
    VKE::RenderSystem::ShaderRefPtr pRtPS;
    VKE::RenderSystem::SVertexInputLayoutDesc RtLayout;

    SFpsCounter m_Fps;

    SGfxContextListener()
    {

    }

    virtual ~SGfxContextListener()
    {

    }

    void LoadShaders( VKE::RenderSystem::CDeviceContext* pCtx )
    {
        VKE::RenderSystem::SCreateShaderDesc VsDesc, PsDesc;

        VsDesc.Create.async = true;
        VsDesc.Create.stages = VKE::Resources::StageBits::FULL_LOAD;
        VsDesc.Create.pOutput = &pVS;
        VsDesc.Shader.Base.pFileName = "Data/Samples/Shaders/simple.vs";
        pCtx->CreateShader( VsDesc );
        
        PsDesc = VsDesc;
        PsDesc.Create.pOutput = &pPS;
        PsDesc.Shader.Base.pFileName = "Data/Samples/shaders/simple.ps";
        pCtx->CreateShader( PsDesc );

        VsDesc.Create.async = true;
        VsDesc.Create.stages = VKE::Resources::StageBits::FULL_LOAD;
        VsDesc.Create.pOutput = &pRtVS;
        VsDesc.Shader.Base.pFileName = "Data/Samples/Shaders/simple-fullscreen-quad.vs";
        pCtx->CreateShader( VsDesc );

        PsDesc = VsDesc;
        PsDesc.Create.pOutput = &pRtPS;
        PsDesc.Shader.Base.pFileName = "Data/Samples/shaders/simple-copy.ps";
        pCtx->CreateShader( PsDesc );
    }

    bool Init( VKE::RenderSystem::CDeviceContext* pCtx )
    {
        LoadShaders( pCtx );

        VKE::RenderSystem::SCreateBufferDesc BuffDesc;
        BuffDesc.Create.async = false;
        BuffDesc.Buffer.usage = VKE::RenderSystem::BufferUsages::VERTEX_BUFFER;
        BuffDesc.Buffer.memoryUsage = VKE::RenderSystem::MemoryUsages::GPU_ACCESS;
        BuffDesc.Buffer.size = ( sizeof( float ) * 4 ) * 3;
        pVb = pCtx->CreateBuffer( BuffDesc );
        const float vb[4 * 3] =
        {
            0.0f,   0.5f,   0.0f,   1.0f,
            -0.5f, -0.5f,   0.0f,   1.0f,
            0.5f,  -0.5f,   0.0f,   1.0f
        };
        VKE::RenderSystem::SUpdateMemoryInfo Info;
        Info.pData = vb;
        Info.dataSize = sizeof( vb );
        Info.dstDataOffset = 0;
        pCtx->UpdateBuffer( Info, &pVb );

        BuffDesc.Buffer.size = ( sizeof( float ) * 6 ) * 4;
        pRtVb = pCtx->CreateBuffer( BuffDesc );
        const float vb2[4*6] =
        {
            -1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f,
            1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f,
            -1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
            1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f
        };
        Info.pData = vb2;
        Info.dataSize = sizeof( vb2 );
        pCtx->UpdateBuffer( Info, &pRtVb );

        Layout.vAttributes =
        {
            { "Position", VKE::RenderSystem::VertexAttributeTypes::POSITION }
        };

        RtLayout.vAttributes =
        {
            { "Position", VKE::RenderSystem::VertexAttributeTypes::POSITION },
            { "Texcoord0", VKE::RenderSystem::VertexAttributeTypes::TEXCOORD }
        };



        VKE::RenderSystem::SRenderTargetDesc RtDesc;
        RtDesc.beginState = VKE::RenderSystem::TextureStates::COLOR_RENDER_TARGET;
        RtDesc.endState = VKE::RenderSystem::TextureStates::COLOR_RENDER_TARGET;
        RtDesc.clearStoreUsage = VKE::RenderSystem::RenderPassAttachmentUsages::COLOR_CLEAR_STORE;
        RtDesc.format = VKE::RenderSystem::Formats::R16G16B16A16_SFLOAT;
        RtDesc.memoryUsage = VKE::RenderSystem::MemoryUsages::GPU_ACCESS;
        RtDesc.mipLevelCount = 1;
        RtDesc.multisampling = VKE::RenderSystem::SampleCounts::SAMPLE_1;
        RtDesc.pDebugName = "Sample RenderTarget";
        RtDesc.Size = { 800, 600 };
        RtDesc.type = VKE::RenderSystem::TextureTypes::TEXTURE_2D;
        RtDesc.usage = VKE::RenderSystem::TextureUsages::COLOR_RENDER_TARGET;
        hRenderTarget = pCtx->CreateRenderTarget( RtDesc );

        pCtx->SetTextureState( hRenderTarget, RtDesc.beginState );

        return pVb.IsValid();
    }

    bool OnRenderFrame(VKE::RenderSystem::CGraphicsContext* pCtx) override
    {
        char buff[128];
        vke_sprintf( buff, 128, "Simple RenderTarget - %d fps", m_Fps.GetFps() );
        pCtx->GetSwapChain()->GetWindow()->SetText( buff );

        pCtx->BeginFrame();
        pCtx->SetState( Layout );
        pCtx->Bind( pVb );
        pCtx->SetState( pVS );
        pCtx->SetState( pPS );
        pCtx->Bind( hRenderTarget );

        for( uint32_t i = 0; i < 1; ++i )
        {
            pCtx->Draw( 3 );
        }
        pCtx->SetTextureState( hRenderTarget, VKE::RenderSystem::TextureStates::SHADER_READ );
        pCtx->Bind( pCtx->GetSwapChain() );
        pCtx->Bind( pRtVb );
        pCtx->SetState( RtLayout );
        pCtx->SetState( pRtVS );
        pCtx->SetState( pRtPS );
        pCtx->Draw( 4 );
        pCtx->SetTextureState( hRenderTarget, VKE::RenderSystem::TextureStates::COLOR_RENDER_TARGET );
        pCtx->EndFrame();
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
        Sample.Destroy();
    }
    
    return 0;
}