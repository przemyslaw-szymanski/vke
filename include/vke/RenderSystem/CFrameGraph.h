#pragma once

#include "RenderSystem/Common.h"

namespace VKE
{
    namespace RenderSystem
    {
        struct SDrawParams
        {
            uint32_t    indexOffset;
            uint32_t    indexCount;
            uint32_t    vertexOffset;
            uint32_t    vertexCount;
        };

        union UDrawcallHandle
        {
            struct
            {
                handle_t    layerIndex : 6;
                handle_t    bufferIndex : 18;
            };
            handle_t    handle;
        };

        struct SDrawcallData
        {
            RenderSystem::VertexBufferHandle        hVertexBuffer;
            RenderSystem::IndexBufferHandle         hIndexBuffer;
            RenderSystem::DescriptorSetHandle       hDescSet;
            uint32_t                                descSetOffset = 0;
            /*RenderSystem::ShaderPtr*                ppVertexShader;
            RenderSystem::ShaderPtr*                ppPixelShader;
            RenderSystem::SVertexInputLayoutDesc    InputLayout;*/
            RenderSystem::PipelinePtr*              ppPipeline;
            uint32_t                                vertexBufferOffset = 0;
            uint32_t                                indexBufferOffset = 0;
            RenderSystem::SDrawParams               DrawParams;
        };

        class CDrawcall
        {
            friend class CScene;
            public:

                using LOD = SDrawcallData;

            public:

                LOD & GetLOD( const uint32_t& idx ) { return m_vLODs[idx]; }

            protected:

                using LODArray = Utils::TCDynamicArray< LOD, 6 >;

            public:

                void AddLOD( const LOD& LOD )
                {
                    m_vLODs.PushBack( LOD );
                }

            protected:
       
                uint8_t             m_currLOD = 0;
                LODArray            m_vLODs;
                UObjectHandle       m_handle; // a handle in Scene buffer
                uint8_t             m_layer; // order of rendering
        };
        using DrawcallPtr = CDrawcall*;

        struct SRenderLayerDesc
        {
            uint16_t    level;
        };

        class CRenderLayer
        {
            using DrawcallArray = Utils::TCDynamicArray< DrawcallPtr, 1 >;

            public:

                void            AddDrawcall( DrawcallPtr pDrawcall );

                void            Render();

            protected:

                DrawcallArray   m_vpDrawcalls;
                uint16_t        m_handle;
                uint16_t        m_level; // order of rendering, 0 is first
                uint16_t        m_nextLayer = UNDEFINED_U16;
        };
        using RenderLayerPtr = CRenderLayer*;

        struct SFrameGraphNodeDesc
        {
            RenderSystem::SRenderPassDesc   RenderPassDesc;
            uint32_t                        id;
        };

        class CFrameGraphNode
        {
            using LayerArray = Utils::TCDynamicArray< RenderLayerPtr >;
            public:

                void            AddLayer( RenderLayerPtr pLayer ) { m_vpLayers.PushBack( pLayer ); }

            protected:

                LayerArray      m_vpLayers;
                uint16_t        m_handle;
                uint16_t        m_nextPass = UNDEFINED_U16;
        };
        using FrameGraphNodePtr = CFrameGraphNode *;

        struct SFrameGraphDesc
        {};

        class CFrameGraph
        {
            using NodeArray = Utils::TCDynamicArray< CFrameGraphNode, 32 >;
            using LayerArray = Utils::TCDynamicArray< CRenderLayer, 32 >;

            public:

                uint16_t                CreateLayer( const SRenderLayerDesc& Desc );
                RenderLayerPtr  GetLayer(const uint16_t& idx) { return &m_vLayers[idx]; }
                uint16_t                CreateNode( const SFrameGraphNodeDesc& Desc );
                FrameGraphNodePtr       GetNode( const uint16_t& idx ) { return &m_vNodes[idx]; }

            protected:

                NodeArray   m_vNodes;
                LayerArray  m_vLayers;
        };
    } // RenderSystem
} // VKE