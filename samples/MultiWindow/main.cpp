

#include "../CSampleFramework.h"

struct SGfxContextListener : public VKE::RenderSystem::EventListeners::IGraphicsContext
{

    SGfxContextListener()
    {

    }

    virtual ~SGfxContextListener()
    {

    }

    bool OnRenderFrame(VKE::RenderSystem::CGraphicsContext* pCtx) override
    {
        static 
        return true;
    }
};

int main()
{
    VKE_DETECT_MEMORY_LEAKS();
    CSampleFramework Sample;
    SSampleCreateDesc Desc;
    VKE::RenderSystem::EventListeners::IGraphicsContext* apListeners[2] =
    {
        VKE_NEW SGfxContextListener(),
        VKE_NEW SGfxContextListener()
    };

    VKE::SWindowDesc aWndDescs[ 2 ] = {};
    aWndDescs[ 0 ].mode = VKE::WindowModes::WINDOW;
    aWndDescs[ 0 ].Size = { 800, 600 };
    aWndDescs[ 0 ].pTitle = "Window 1";
    aWndDescs[ 1 ] = aWndDescs[ 0 ];
    aWndDescs[ 1 ].pTitle = "Window 2";

    Desc.ppGfxListeners = apListeners;
    Desc.gfxListenerCount = 2;
    Desc.customWindowCount = 2;
    Desc.pCustomWindows = aWndDescs;

    if( Sample.Create( Desc ) )
    {
        Sample.Start();
    }
    Sample.Destroy();
    return 0;
}