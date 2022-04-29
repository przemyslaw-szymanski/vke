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
        class CScene;

        class ITerrainRenderer
        {
            friend class CTerrain;
            friend class CScene;

            public:

                virtual ~ITerrainRenderer() { }

                virtual void    Update(RenderSystem::CommandBufferPtr, CScene* ) {}
                virtual void    Render(RenderSystem::CommandBufferPtr, CScene* ) {}

                virtual Result  UpdateBindings(RenderSystem::CommandBufferPtr, const STerrainUpdateBindingData&) { return VKE_OK; }

            protected:

                virtual Result  _Create( const STerrainDesc& Desc, RenderSystem::CommandBufferPtr ) { return VKE_OK; }
                virtual void    _Destroy() {}

                virtual RenderSystem::PipelinePtr _GetPipelineForLOD(uint8_t) { return {}; }
        };
    } // Scene
} // VKE