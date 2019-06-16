#pragma once

#include "Common.h"

namespace VKE
{
    namespace Scene
    {
        class CScene;
    }

    namespace RenderSystem
    {
        using LayerHandle = uint8_t;

        struct DrawcallTypes
        {
            enum TYPE
            {
                STATIC_OPAQUE,
                DYNAMIC_OPAQUE,
                STATIC_ALPHA,
                DYNAMIC_ALPHA,
                _MAX_COUNT
            };
        };
        using DRAWCALL_TYPE = uint8_t;

        union UObjectBits
        {
            VKE_ALIGN( 1 ) struct
            {
                uint8_t invisible : 1;
                uint8_t culled : 1;
                uint8_t invalid : 1;
                uint8_t reserved : 5;
            };
            uint8_t bits;
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

        union UObjectHandle
        {
            VKE_ALIGN( 8 ) struct
            {
                handle_t    type            : 4;
                handle_t    index           : 20;
                handle_t    reserved        : 64 - 20 - 4;
            };
            handle_t handle;
        };

        union UDrawcallHandle
        {
            VKE_ALIGN( 4 ) struct
            {
                uint32_t    reserved1 : 32;
            };
            uint32_t handle;
        };

        class CDrawcall
        {
            friend class Scene::CScene;
            friend class CRenderSystem;
            friend class IFrameGraph;

            using Bitset = Utils::TCBitset< uint16_t >;

            public:

                using LOD = SDrawcallData;

            public:

                LOD & GetLOD( const uint32_t& idx )
                {
                    return m_vLODs[ idx ];
                }

            protected:

                using LODArray = Utils::TCDynamicArray< LOD, 6 >;

            public:

                void AddLOD( const LOD& LOD )
                {
                    m_vLODs.PushBack( LOD );
                }

            protected:


            protected:

            uint8_t             m_currLOD = 0;
            LODArray            m_vLODs;
            UObjectHandle       m_hObj; // a handle in frame and Scene buffer
            handle_t            m_hSceneGraph; // a handle in scene graph
        };
        using DrawcallPtr = CDrawcall*;

        struct SFrameGraphDesc
        {
            cstr_t  pName = "VKE_FORWARD";
            void*   pDesc;
        };

        struct SForwardRendererDesc
        {

        };

        struct SForwardRendererDrawcallInfo
        {
            DrawcallPtr     pDrawcall;
            DRAWCALL_TYPE   type;
        };

        class VKE_API IFrameGraph
        {
            friend class CRenderSystem;
            friend class Scene::CScene;
          
            public:

                virtual ~IFrameGraph() {}

                virtual void        SetScene(CScene* pScene) { m_pScene = pScene; }

                virtual void        Render(CGraphicsContext*) = 0;

            protected:

                virtual Result      _Create( const SFrameGraphDesc& ) = 0;
                virtual void        _Destroy() = 0;

            protected:

                CScene*  m_pScene = nullptr;
        };
    }
}