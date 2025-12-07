#include "VKE.h"

#include <directx/d3d12.h>

extern "C"
{
    _declspec( dllexport ) extern const UINT D3D12SDKVersion = D3D12_SDK_VERSION;
}
extern "C"
{
    _declspec( dllexport ) extern const char* D3D12SDKPath = ".\\D3D12\\";
}

struct SGfxContextListener : public VKE::RenderSystem::EventListeners::IGraphicsContext
{

    SGfxContextListener()
    {
    }

    virtual ~SGfxContextListener()
    {
    }

    bool OnRenderFrame( VKE::RenderSystem::CGraphicsContext* pCtx ) override
    {
        return true;
    }
};

int main()
{
    VKE_DETECT_MEMORY_LEAKS();

    // Bootstrap engine
    {
        VKE::Result Result = VKE::VKE_FAIL;

        // Configure engine
        VKE::CVkEngine*  pEngine = VKECreate();
        VKE::SEngineInfo EngineInfo;

        // Initialize engine
        Result = pEngine->Init( EngineInfo );
        if( VKE_FAILED( Result ) )
        {
            VKEDestroy();
            return -1;
        }

        // Create Window
        VKE::SWindowDesc WndDesc;
        VKE::WindowPtr   pWindow = pEngine->CreateRenderWindow( WndDesc );

        // Initialize RenderSystem
        VKE::RenderSystem::SRenderSystemDesc RenderSystemDesc;
        RenderSystemDesc.debugMode = VKE_RENDER_SYSTEM_DEBUG;

        VKE::RenderSystem::CRenderSystem* pRenderSystem = pEngine->CreateRenderSystem( RenderSystemDesc );

        // Get adapter
        auto& adapters = pRenderSystem->GetAdapters();
        if( adapters.IsEmpty() )
        {
            VKEDestroy();
            return -1;
        }

        VKE::RenderSystem::SDeviceContextDesc DeviceContextDesc = {};

        // First adapter should be the best.
        DeviceContextDesc.pAdapterInfo = &adapters[ 0 ];
        
        VKE::RenderSystem::CDeviceContext* pDeviceContext = pRenderSystem->CreateDeviceContext( DeviceContextDesc );
        if( !pDeviceContext )
        {
            VKEDestroy();
            return -1;
        }

    }

    VKEDestroy();
    return 0;
}