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

        struct SPackedUint
        {
            union
            {
                struct
                {
                    uint32_t index0 : 8;
                    uint32_t index1 : 8;
                    uint32_t index2 : 8;
                    uint32_t index3 : 8;
                };
                uint32_t indices;
            };
        };

        struct STileGPUBindingData
        {
            ExtentI32      Position;
            ExtentU32      HeightmapNormalOffset;
            SPackedUint    HeightmapNormalIndices;
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

                virtual Result  UpdateBindings(RenderSystem::CommandBufferPtr, STerrainUpdateBindingData&) { return VKE_OK; }
                virtual void UpdateBindings( STerrainUpdateBindingData& ) {}

            protected:

                virtual Result  _Create( const STerrainDesc& Desc, RenderSystem::CommandBufferPtr ) { return VKE_OK; }
                virtual void    _Destroy() {}

                virtual RenderSystem::PipelinePtr _GetPipelineForLOD(uint8_t) { return {}; }
        };
    } // Scene
} // VKE