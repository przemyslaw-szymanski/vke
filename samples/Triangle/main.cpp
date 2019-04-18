

#include "../CSampleFramework.h"

struct SGfxContextListener : public VKE::RenderSystem::EventListeners::IGraphicsContext
{
    VKE::RenderSystem::VertexBufferRefPtr pVb;
    VKE::RenderSystem::ShaderRefPtr pVS;
    VKE::RenderSystem::ShaderRefPtr pPS;
    VKE::RenderSystem::SVertexInputLayoutDesc Layout;
    VKE::Resources::STaskResult VsResult, PsResult;

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
        VsDesc.Create.pResult = &VsResult;
        VsDesc.Shader.Base.pFileName = "Data/Samples/Shaders/simple.vs";
        
        PsDesc = VsDesc;
        PsDesc.Create.pResult = &PsResult;
        PsDesc.Shader.Base.pFileName = "Data/Samples/shaders/simple.ps";

        pCtx->CreateShader( VsDesc );
        pCtx->CreateShader( PsDesc );
    }

    bool Init( VKE::RenderSystem::CDeviceContext* pCtx )
    {
        LoadShaders( pCtx );

        VKE::RenderSystem::SCreateBufferDesc BuffDesc;
        BuffDesc.Create.async = false;
        BuffDesc.Buffer.usage = VKE::RenderSystem::BufferUsages::VERTEX_BUFFER;
        BuffDesc.Buffer.memoryUsage = VKE::RenderSystem::MemoryUsages::CPU_ACCESS | VKE::RenderSystem::MemoryUsages::SEPARATE_ALLOCATION;
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
        Info.offset = 0;
        pVb->Update( Info );

        Layout.vAttributes =
        {
            { "Position", VKE::RenderSystem::VertexAttributeTypes::POSITION }
        };

        while( VsResult.result == VKE::VKE_ENOTREADY || PsResult.result == VKE::VKE_ENOTREADY ) {}
        if( VsResult.result != VKE::VKE_OK || PsResult.result != VKE::VKE_OK )
        {
            return false;
        }

        pVS = VKE::Resources::STaskResult::Cast<VKE::RenderSystem::ShaderRefPtr>( VsResult.pData );
        pPS = VKE::Resources::STaskResult::Cast<VKE::RenderSystem::ShaderRefPtr>( PsResult.pData );

        return pVb.IsValid();
    }

    bool OnRenderFrame(VKE::RenderSystem::CGraphicsContext* pCtx) override
    {
        /*auto pCb = pCtx->CreateCommandBuffer();
        pCb->Begin();
        pCb->Bind( pCtx->GetSwapChain() );
        pCb->Bind( pVb );
        pCb->SetState( Layout );
        pCb->Bind( pVS );
        pCb->Bind( pPS );
        pCb->Draw( 3 );
        pCb->End();*/
        pCtx->BeginFrame();
        pCtx->SetState( Layout );
        pCtx->Bind( pVb );
        pCtx->Bind( pVS );
        pCtx->Bind( pPS );
        pCtx->Draw( 3 );
        pCtx->EndFrame();
        return true;
    }
};

int main()
{   
    CSampleFramework Sample;
    SSampleCreateDesc Desc;
    VKE::RenderSystem::EventListeners::IGraphicsContext* apListeners[1] =
    {
        new SGfxContextListener()
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
    return 0;
}