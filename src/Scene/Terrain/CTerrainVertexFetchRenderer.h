#pragma once

#include "ITerrainRenderer.h"
#include "Core/Math/CMatrix.h"

namespace VKE
{
    namespace Scene
    {
        static cstr_t const TERRAIN_VERTEX_FETCH_RENDERER_NAME = "VKE_TERRAIN_VERTEX_FETCH_RENDERER";
        class CTerrain;

        class CTerrainVertexFetchRenderer final : public ITerrainRenderer
        {
            struct SPerDrawVertexConstantBuffer
            {
                Math::CMatrix4x4    mtxTransform;
            };

            struct SPerDrawPixelConstantBuffer
            {

            };

            struct SPerDrawConstantBuffer
            {
                Math::CMatrix4x4    mtxTransform;
            };

            struct SPerFrameConstantBuffer
            {
                Math::CMatrix4x4    mtxViewProj;
                ExtentF32           TerrainSize;
                ExtentF32           TileSize;
                ExtentF32           Height;
            };

            struct SConstantBuffer
            {
                using TileConstantBufferArray = Utils::TCDynamicArray< SPerDrawConstantBuffer, 1 >;
                

                SPerFrameConstantBuffer m_PerFrame;
                TileConstantBufferArray m_vPerDraw;
            };

            friend class CTerrain;
            public:

                CTerrainVertexFetchRenderer(CTerrain* pTerrain) :
                    m_pTerrain( pTerrain )
                {}

                void    Render( RenderSystem::CGraphicsContext* ) override;

            protected:

                Result  _Create( const STerrainDesc& Desc, RenderSystem::CDeviceContext* ) override;
                void    _Destroy() override;

                Result   _CreateConstantBuffers(RenderSystem::CDeviceContext*);
                Result   _CreatePipeline( const STerrainDesc& Desc, uint8_t lod,
                    RenderSystem::CDeviceContext* pCtx, RenderSystem::CDrawcall::LOD* pInOut );

            protected:

                CTerrain*                               m_pTerrain;
                RenderSystem::CDrawcall*                m_pDrawcall;
                RenderSystem::DescriptorSetHandle       m_hPerFrameDescSet;
                RenderSystem::DescriptorSetHandle       m_hPerTileDescSet;
                RenderSystem::BufferHandle              m_hIndexBuffer;
                RenderSystem::BufferHandle              m_hVertexBuffer;
                RenderSystem::BufferPtr                 m_pConstantBuffer;
                RenderSystem::SBindDescriptorSetsInfo   m_BindingTables[2];
                RenderSystem::DDIDescriptorSet          m_hDDISets[2];
                SConstantBuffer                         m_ConstantBuffer;
                uint32_t                                m_maxVisibleTiles;
        };
    } // Scene
} // VKE