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
        class CCamera;

        class ITerrainRenderer
        {
            friend class CTerrain;
            friend class CScene;

            public:
            
                virtual void    Update(RenderSystem::CGraphicsContext*, CCamera* ) {}
                virtual void    Render(RenderSystem::CGraphicsContext*, CCamera* ) {}

            protected:

                virtual Result  _Create( const STerrainDesc& Desc, RenderSystem::CDeviceContext* ) { return VKE_OK; }
                virtual void    _Destroy() {}

                virtual RenderSystem::PipelinePtr _GetPipelineForLOD(uint8_t) { return {}; }
        };
    } // Scene
} // VKE