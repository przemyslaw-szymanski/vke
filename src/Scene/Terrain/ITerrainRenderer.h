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

        struct STerrainUpdateBindingData
        {
            uint32_t    index; // binding index
            RenderSystem::TextureViewHandle     hHeightmap = INVALID_HANDLE;
            RenderSystem::TextureViewHandle     hHeightmapNormal = INVALID_HANDLE;
            RenderSystem::SamplerHandle         hDiffuseSampler = INVALID_HANDLE;
            RenderSystem::TextureViewHandle*    ahDiffuses = nullptr;
            RenderSystem::TextureViewHandle*    ahDiffuseNormals = nullptr;
            uint16_t                            diffuseTextureCount = 0;
        };

        class ITerrainRenderer
        {
            friend class CTerrain;
            friend class CScene;

            public:

                virtual void    Update(RenderSystem::CGraphicsContext*, CCamera* ) {}
                virtual void    Render(RenderSystem::CGraphicsContext*, CCamera* ) {}

                virtual Result  UpdateBindings(RenderSystem::CDeviceContext*, const STerrainUpdateBindingData&) { return VKE_OK; }

            protected:

                virtual Result  _Create( const STerrainDesc& Desc, RenderSystem::CDeviceContext* ) { return VKE_OK; }
                virtual void    _Destroy() {}

                virtual RenderSystem::PipelinePtr _GetPipelineForLOD(uint8_t) { return {}; }
        };
    } // Scene
} // VKE