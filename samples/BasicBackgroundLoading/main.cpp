

#include "../CSampleFramework.h"

struct SGfxContextListener : public VKE::RenderSystem::EventListeners::IGraphicsContext
{
    VKE::RenderSystem::VertexBufferRefPtr pVb;
    VKE::RenderSystem::ShaderRefPtr pVS;
    VKE::RenderSystem::ShaderRefPtr pPS;
    VKE::RenderSystem::SVertexInputLayoutDesc Layout;

    SGfxContextListener()
    {

    }

    virtual ~SGfxContextListener()
    {

    }

    bool Init( VKE::RenderSystem::CDeviceContext* pCtx )
    {
        LoadSimpleShaders( pCtx, pVS, pPS );

        return CreateSimpleTriangle( pCtx, pVb, &Layout );
    }

    bool OnRenderFrame(VKE::RenderSystem::CGraphicsContext* pCtx) override
    {
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