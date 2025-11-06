#include "VKE.h"

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
        return true;
    }

};

int main()
{   
    VKE_DETECT_MEMORY_LEAKS();

    // Bootstrap engine
    {
        // Create engine instance
        VKE::CVkEngine* pEngine = VKECreate();

        // Create Window
        VKE::SWindowDesc WndDesc;
        VKE::WindowPtr pWindow = pEngine->CreateRenderWindow( WndDesc );

        // Initialize RenderSystem
        VKE::RenderSystem::SRenderSystemDesc RenderSysDesc;
        VKE::RenderSystem::CRenderSystem*    pRenderSystem = pEngine->CreateRenderSystem( RenderSysDesc );
    }

    return 0;
}