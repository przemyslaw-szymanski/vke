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

        struct STileData
        {
            uint32_t index;
        };
        struct STerrainSubTileDesc
        {
            ExtentI32 Position;
            ExtentF32 Size;
            STileData Data;
        };
        struct STerrainUpdateBindingData
        {
            uint32_t index; // binding index
            Utils::TCDynamicArray<STerrainSubTileDesc, 1> vSubTiles;
            RenderSystem::TextureViewHandle hHeightmap = INVALID_HANDLE;
            RenderSystem::TextureViewHandle hHeightmapNormal = INVALID_HANDLE;
            RenderSystem::SamplerHandle hBilinearSampler = INVALID_HANDLE;
        };

        class ITerrainRenderer
        {
            friend class CTerrain;
            friend class CScene;

            public:

                virtual ~ITerrainRenderer() { }

                virtual void    Update(RenderSystem::CommandBufferPtr, CScene* ) {}
                virtual void    Render(RenderSystem::CommandBufferPtr, CScene* ) {}

                virtual Result  UpdateBindings(RenderSystem::CommandBufferPtr, const STerrainUpdateBindingData&) { return VKE_OK; }
                virtual void UpdateBindings( const STerrainUpdateBindingData& ) {}

            protected:

                virtual Result  _Create( const STerrainDesc& Desc, RenderSystem::CommandBufferPtr ) { return VKE_OK; }
                virtual void    _Destroy() {}

                virtual RenderSystem::PipelinePtr _GetPipelineForLOD(uint8_t) { return {}; }
        };
    } // Scene
} // VKE