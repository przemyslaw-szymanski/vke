#pragma once

#include "ITerrainRenderer.h"
#include "Core/Math/Math.h"

namespace VKE
{
    namespace Scene
    {
        static cstr_t const TERRAIN_VERTEX_FETCH_RENDERER_NAME = "VKE_TERRAIN_VERTEX_FETCH_RENDERER";
        class CTerrain;

        class CTerrainVertexFetchRenderer final : public ITerrainRenderer
        {
            public:
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
                Math::CVector4      vecLodColor;
                uint32_t            vertexDiff; // packed top, bottom, left, right values for vertex shift
                float               tileSize;
                uint32_t            topVertexDiff; // vertex move to highest lod to create stitches
                uint32_t            bottomVertexDiff;
                uint32_t            leftVertexDiff;
                uint32_t            rightVertexDiff;
                uint32_t            heightmapIndex;
            };

            struct SPerInstanceBufferData
            {
                Math::CVector4      vecPosition;
                Math::CVector4      vecLodColor;
                uint32_t            vertexDiff; // packed top, bottom, left, right values for vertex shift
                float               tileSize;
                uint32_t            topVertexDiff; // vertex move to highest lod to create stitches
                uint32_t            bottomVertexDiff;
                uint32_t            leftVertexDiff;
                uint32_t            rightVertexDiff;
            };

            struct SVertex
            {
                Math::CVector3  vecPosition;
                ExtentF32       Texcoords;
            };

            struct SPerFrameConstantBuffer
            {
                Math::CMatrix4x4    mtxViewProj;
                ExtentU32           TerrainSize;
                ExtentF32           Height;
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
                RenderSystem::PipelinePtr   pPipeline;
                uint32_t                    vertexBufferOffset;
            };

            using ConstantBufferData = Utils::TCDynamicArray< uint8_t, 1 >;
            using DescriptorSetArray = Utils::TCDynamicArray< RenderSystem::DescriptorSetHandle >;
            using DrawcallArray = Utils::TCDynamicArray< RenderSystem::CDrawcall*, 1 >;
            using LODArray = Utils::TCDynamicArray< SDrawData, 16 >;

            friend class CTerrain;
            public:

                CTerrainVertexFetchRenderer(CTerrain* pTerrain) :
                    m_pTerrain( pTerrain )
                {}

                void    Update(RenderSystem::CGraphicsContext*, CCamera* ) override;
                void    Render( RenderSystem::CGraphicsContext*, CCamera* ) override;

                Result  UpdateBindings(RenderSystem::CDeviceContext*, const STerrainUpdateBindingData&) override;

            protected:

                Result  _Create( const STerrainDesc& Desc, RenderSystem::CDeviceContext* ) override;
                void    _Destroy() override;

                RenderSystem::PipelinePtr    _GetPipelineForLOD( uint8_t lod ) override;

                Result   _CreateVertexBuffer( const STerrainDesc& Desc,
                                              RenderSystem::CDeviceContext* pCtx );

                Result  _CreateConstantBuffers(RenderSystem::CDeviceContext*);
                Result  _CreateBindings(RenderSystem::CDeviceContext*);
                RenderSystem::PipelinePtr   _CreatePipeline( const STerrainDesc& Desc, uint8_t lod,
                    RenderSystem::CDeviceContext* pCtx );
                // Binding per draw / root node
                uint32_t    _CreateTileBindings(RenderSystem::CDeviceContext*);
                Result      _UpdateTileBindings(const uint32_t& idx);

                void        _UpdateConstantBuffers( RenderSystem::CGraphicsContext* pCtx, CCamera* pCamera );
                void        _UpdateDrawcalls( CCamera* pCamera );

            protected:

                CTerrain*                               m_pTerrain;
                //RenderSystem::CDrawcall*                m_pDrawcall;
                LODArray                                m_vDrawLODs;
                RenderSystem::DescriptorSetHandle       m_hPerFrameDescSet;
                RenderSystem::DescriptorSetHandle       m_hPerTileDescSet;
                RenderSystem::IndexBufferHandle         m_hIndexBuffer;
                RenderSystem::VertexBufferHandle        m_hVertexBuffer;
                // A buffer containing per frame data and per each tile data
                // Separate fragments of this buffer are bound to separate bindings
                RenderSystem::BufferPtr                 m_pConstantBuffer;
                //RenderSystem::SBindDescriptorSetsInfo   m_BindingTables[2];
                RenderSystem::DDIDescriptorSet          m_hDDISets[2];
                uint32_t                                m_indexCount;
                RenderSystem::SDrawParams               m_DrawParams;
                DescriptorSetArray                      m_vTileBindings;
                DrawcallArray                           m_vpDrawcalls;
        };
    } // Scene
} // VKE