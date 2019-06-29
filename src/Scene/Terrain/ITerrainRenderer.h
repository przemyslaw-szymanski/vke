#pragma once

#include "Scene/Common.h"

namespace VKE
{
    namespace RenderSystem
    {
        class CDeviceContext;
    }

    namespace Scene
    {
        class ITerrainRenderer
        {
            friend class CTerrain;
            friend class CScene;

            public:
            
                virtual void    Render(RenderSystem::CGraphicsContext*) {}

            protected:

                virtual Result  _Create( const STerrainDesc& Desc, RenderSystem::CDeviceContext* ) { return VKE_OK; }
                virtual void    _Destroy() {}
        };
    } // Scene
} // VKE