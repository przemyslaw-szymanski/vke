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

            struct SPerDrawConstantBufferData
            {
                //Math::CMatrix4x4    mtxTransform;
                Math::CVector4      vecPosition;
            };

            struct SPerFrameConstantBuffer
            {
                Math::CMatrix4x4    mtxViewProj;
                ExtentF32           TerrainSize;
                ExtentF32           Height;
                float               vertexDistance;
                uint32_t            tileRowVertexCount;
            };

            struct SConstantBuffer
            {
                using TileConstantBufferArray = Utils::TCDynamicArray< SPerDrawConstantBufferData, 1 >;
                
                SPerFrameConstantBuffer m_PerFrame;
                TileConstantBufferArray m_vPerDraw;
            };

            struct SDrawData
            {

            };

            using ConstantBufferData = Utils::TCDynamicArray< uint8_t, 1 >;
            using DescriptorSetArray = Utils::TCDynamicArray< RenderSystem::DescriptorSetHandle >;
            using DrawcallArray = Utils::TCDynamicArray< RenderSystem::CDrawcall*, 1 >;

            friend class CTerrain;
            public:

                CTerrainVertexFetchRenderer(CTerrain* pTerrain) :
                    m_pTerrain( pTerrain )
                {}

                void    Update(RenderSystem::CGraphicsContext*, CCamera* ) override;
                void    Render( RenderSystem::CGraphicsContext*, CCamera* ) override;

            protected:

                Result  _Create( const STerrainDesc& Desc, RenderSystem::CDeviceContext* ) override;
                void    _Destroy() override;

                Result   _CreateConstantBuffers(RenderSystem::CDeviceContext*);
                Result   _CreatePipeline( const STerrainDesc& Desc, uint8_t lod,
                    RenderSystem::CDeviceContext* pCtx, RenderSystem::CDrawcall::LOD* pInOut );
                Result  _CreateDrawcalls( const STerrainDesc& Desc );

                void    _UpdateConstantBuffers( RenderSystem::CGraphicsContext* pCtx, CCamera* pCamera );
                void    _UpdateTileConstantBufferData( const SPerDrawConstantBufferData& Data, uint32_t drawIdx );
                void    _UpdateDrawcalls( CCamera* pCamera );

            protected:

                CTerrain*                               m_pTerrain;
                RenderSystem::CDrawcall*                m_pDrawcall;
                RenderSystem::DescriptorSetHandle       m_hPerFrameDescSet;
                RenderSystem::DescriptorSetHandle       m_hPerTileDescSet;
                RenderSystem::BufferHandle              m_hIndexBuffer;
                RenderSystem::BufferHandle              m_hVertexBuffer;
                RenderSystem::BufferPtr                 m_pConstantBuffer;
                //RenderSystem::SBindDescriptorSetsInfo   m_BindingTables[2];
                RenderSystem::DDIDescriptorSet          m_hDDISets[2];
                DescriptorSetArray                      m_vTileDescSets;
                DrawcallArray                           m_vpDrawcalls;
                ConstantBufferData                      m_vConstantBufferData;
        };
    } // Scene
} // VKE