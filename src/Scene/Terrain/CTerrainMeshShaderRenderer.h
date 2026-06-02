#pragma once

#include "ITerrainRenderer.h"

namespace VKE
{
    namespace Scene
    {
        class CTerrainMeshShadingRenderer final : public ITerrainRenderer
        {
            struct STileData
            {
                ExtentF32 Position;
            };

            using BindingDataArray = Utils::TCDynamicArray< STerrainUpdateBindingData, 1 >;
            using TileDataArray    = Utils::TCDynamicArray< STileGPUBindingData, 1 >;

        public:
            CTerrainMeshShadingRenderer( CTerrain* );
            ~CTerrainMeshShadingRenderer();

            virtual void   Update( RenderSystem::CommandBufferPtr, CScene* ) override;
            virtual void   Render( RenderSystem::CommandBufferPtr, CScene* ) override;
            virtual Result UpdateBindings( RenderSystem::CommandBufferPtr, STerrainUpdateBindingData& ) override;
            virtual void   UpdateBindings( STerrainUpdateBindingData& ) override;

        protected:
            virtual Result _Create( const STerrainDesc& Desc, RenderSystem::CommandBufferPtr ) override;
            virtual void   _Destroy() override;

            Result _CreatePipeline( RenderSystem::CommandBufferPtr );
            Result _CreateVertexBuffer( RenderSystem::CommandBufferPtr );
            Result _CreateIndexBuffer( RenderSystem::CommandBufferPtr, uint8_t );
            Result _CreateTriangleBuffer( RenderSystem::CommandBufferPtr );
            Result _CreateMeshletBuffer( RenderSystem::CommandBufferPtr );
            Result _CreateVertexIndexBuffer( RenderSystem::CommandBufferPtr );
            Result _CreateInstancingBuffer( RenderSystem::CommandBufferPtr );
            Result _CreateResourceDescriptor( RenderSystem::CommandBufferPtr );
            void   _UploadInstancingBuffer( RenderSystem::CommandBufferPtr );

        protected:
            CTerrain*                         m_pTerrain;
            RenderSystem::BufferPtr           m_pVertexBuffer;
            RenderSystem::BufferPtr           m_pIndexBuffer;
            RenderSystem::BufferPtr           m_pMeshletBuffer;
            RenderSystem::BufferPtr           m_pVertexIndexBuffer;
            RenderSystem::BufferPtr           m_pTileBuffer;
            RenderSystem::BufferPtr           m_pTriangleBuffer;
            RenderSystem::BufferPtr           m_pDebugBuffer;
            RenderSystem::DescriptorSetHandle m_hTileDescSet  = INVALID_HANDLE;
            RenderSystem::DescriptorSetHandle m_hSceneDescSet = INVALID_HANDLE;

            RenderSystem::NativeAPI::RenderPass   m_hNativeRenderPass = RenderSystem::NativeAPI::Null;
            RenderSystem::PipelinePtr m_pColorPipeline;
            RenderSystem::PipelinePtr m_pDepthOnlyPipeline;
            RenderSystem::PipelinePtr m_pWireframePipeline;
            RenderSystem::PipelinePtr m_pCurrPipeline;
            RenderSystem::ShaderPtr   m_pMeshShader;
            RenderSystem::ShaderPtr   m_pPixelShader;
            RenderSystem::ShaderPtr   m_pTaskShader;

            BindingDataArray m_vBindingData;
            TileDataArray    m_vTileData;

            struct
            {
                uint32_t vertexCountInRow;
                uint32_t totalTriangleCount = 0;
                uint32_t threadgroupSize    = 32;
                float    vertexDistance;
            } m_MeshletDesc;

            /// <summary>
            /// This struct describes requirements to render single terrain sub-tile.
            /// The sub-tile size is specified in TerrainDesc::TileSize::min
            /// </summary>
            struct
            {
                /// <summary>
                /// Size of workgroup. dispatchSize * threadgroupSize == totalMeshletCount.
                /// </summary>
                uint32_t dispatchSize;
                float    meshletSize;
                uint32_t meshletCountInRow;
                uint32_t totalMeshletCount;
                float    meshletDistance;
            } m_SubTileDesc;

            /// <summary>
            /// This struct describes requirements to render one or more terrain tiles.
            /// The terrain tile is specified in TerrainDesc::TileSize::max.
            /// Total number of sub-tiles for tile is (TileSize::max / TileSize::min)^2.
            /// </summary>
            struct
            {
                uint32_t threadgroupSize = 32;
                uint32_t tileCountInRow;
                /// <summary>
                /// Size of workgroup. dispatchSize * threadgroupSize * totalTileCount.
                /// It is possible to use instancing to render more tiles by specify
                /// dispatchSize *= instanceCount;
                /// </summary>
                uint32_t dispatchSize;
                /// <summary>
                /// (TileSize::max / TileSize::min)^2.
                /// </summary>
                uint32_t totalSubTileCount;
                uint32_t instanceCount;
            } m_TaskDesc;
        };
    } // namespace Scene
} // namespace VKE