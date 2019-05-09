

#include "../CSampleFramework.h"

struct SGfxContextListener : public VKE::RenderSystem::EventListeners::IGraphicsContext
{
    VKE::RenderSystem::VertexBufferRefPtr pVb;
    VKE::RenderSystem::ShaderRefPtr pVS;
    VKE::RenderSystem::ShaderRefPtr pPS;
    VKE::RenderSystem::SVertexInputLayoutDesc Layout;

    SFpsCounter Fps;

    SGfxContextListener()
    {

    }

    virtual ~SGfxContextListener()
    {

    }

    bool Init( VKE::RenderSystem::CDeviceContext* pCtx )
    {
        SLoadShaderData ShaderData;
        ShaderData.apShaderFiles[VKE::RenderSystem::ShaderTypes::VERTEX] = "data/samples/shaders/simple.vs";
        ShaderData.apShaderFiles[VKE::RenderSystem::ShaderTypes::PIXEL] = "data/samples/shaders/simple.ps";
        LoadSimpleShaders( pCtx, ShaderData, pVS, pPS );

        return CreateSimpleTriangle( pCtx, pVb, &Layout );
    }

    bool OnRenderFrame(VKE::RenderSystem::CGraphicsContext* pCtx) override
    {
        char buff[128];
        vke_sprintf( buff, 128, "%d fps", Fps.GetFps() );
        pCtx->GetSwapChain()->GetWindow()->SetText( buff );

        SSimpleDrawData Data;
        Data.pLayout = &Layout;
        Data.pVertexBuffer = pVb;
        Data.pPixelShader = pPS;
        Data.pVertexShader = pVS;
        DrawSimpleFrame( pCtx, Data );
        return true;
    }
};

int main()
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
    VKE_DELETE( apListeners[0] );
    Sample.Destroy();
    return 0;
}