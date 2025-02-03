#pragma once

#include "ITerrainRenderer.h"

namespace VKE
{
    namespace Scene
    {
        class CTerrainMeshShadingRenderer final : public ITerrainRenderer
        {
            struct SInstancingData
            {
                ExtentF32 Position;
            };
            using BindingDataArray = Utils::TCDynamicArray< STerrainUpdateBindingData, 1 >;
          public:
            CTerrainMeshShadingRenderer( CTerrain* );
            ~CTerrainMeshShadingRenderer();

            virtual void Update( RenderSystem::CommandBufferPtr, CScene* ) override;
            virtual void Render( RenderSystem::CommandBufferPtr, CScene* ) override;
            virtual Result UpdateBindings( RenderSystem::CommandBufferPtr, const STerrainUpdateBindingData& ) override;
            virtual void UpdateBindings( const STerrainUpdateBindingData& ) override;

          protected:
            virtual Result _Create( const STerrainDesc& Desc, RenderSystem::CommandBufferPtr ) override;
            virtual void _Destroy() override;

            Result _CreatePipeline( RenderSystem::CommandBufferPtr );
            Result _CreateVertexBuffer(RenderSystem::CommandBufferPtr);
            Result _CreateIndexBuffer(RenderSystem::CommandBufferPtr, uint8_t);
            Result _CreateTriangleBuffer( RenderSystem::CommandBufferPtr );
            Result _CreateMeshletBuffer( RenderSystem::CommandBufferPtr );
            Result _CreateVertexIndexBuffer( RenderSystem::CommandBufferPtr );
            Result _CreateInstancingBuffer( RenderSystem::CommandBufferPtr );
            Result _CreateResourceDescriptor( RenderSystem::CommandBufferPtr );
            void   _UploadInstancingBuffer( RenderSystem::CommandBufferPtr );

          protected:
              CTerrain*                 m_pTerrain;
              RenderSystem::BufferPtr           m_pVertexBuffer;
              RenderSystem::BufferPtr           m_pIndexBuffer;
              RenderSystem::BufferPtr           m_pMeshletBuffer;
              RenderSystem::BufferPtr           m_pVertexIndexBuffer;
              RenderSystem::BufferPtr           m_pTileBuffer;
              RenderSystem::BufferPtr           m_pTriangleBuffer;
              RenderSystem::DescriptorSetHandle m_hTileDescSet = INVALID_HANDLE;

              hash_t                    m_renderPassHash = 0;
              RenderSystem::PipelinePtr m_pColorPipeline;
              RenderSystem::PipelinePtr m_pDepthOnlyPipeline;
              RenderSystem::PipelinePtr m_pWireframePipeline;
              RenderSystem::PipelinePtr m_pCurrPipeline;
              RenderSystem::ShaderPtr   m_pMeshShader;
              RenderSystem::ShaderPtr   m_pPixelShader;
              RenderSystem::ShaderPtr   m_pTaskShader;

              BindingDataArray          m_vBindingData;
        };
    } // Scene
} // VKE