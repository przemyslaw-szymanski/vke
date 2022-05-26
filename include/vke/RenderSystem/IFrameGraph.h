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
            using PipelineArray = Utils::TCDynamicArray< RenderSystem::PipelinePtr, 16 >;

            RenderSystem::VertexBufferHandle        hVertexBuffer;
            RenderSystem::IndexBufferHandle         hIndexBuffer;
            RenderSystem::DescriptorSetHandle       hDescSet;
            uint32_t                                descSetOffset = 0;
            /*RenderSystem::ShaderPtr*                ppVertexShader;
            RenderSystem::ShaderPtr*                ppPixelShader;
            RenderSystem::SVertexInputLayoutDesc    InputLayout;*/
            PipelineArray                           vpPipelines;
            uint32_t                                vertexBufferOffset = 0;
            uint32_t                                indexBufferOffset = 0;
            RenderSystem::SDrawParams               DrawParams;
        };

        union UObjectHandle
        {
            VKE_ALIGN( 8 ) struct
            {
                handle_t    layer           : 5;
                handle_t    index           : 20;
                handle_t    reserved        : 64 - 20 - 5;
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

                vke_force_inline LOD& GetLOD( const uint32_t& idx )
                {
                    return m_vLODs[ idx ];
                }

                vke_force_inline LOD& GetLOD() { return GetLOD( m_currLOD ); }

                vke_force_inline handle_t GetHandle() const { return m_hObj.handle; }

            protected:

                using LODArray = Utils::TCDynamicArray< LOD, 6 >;

            public:

                void AddLOD( const LOD& LOD )
                {
                    m_vLODs.PushBack( LOD );
                }

                void DisableFrameGraphRendering( bool disable ) { m_isFrameGraphRendering = !disable; }
                bool IsFrameGrpahRenderingEnabled() const { return m_isFrameGraphRendering; }

            protected:


            protected:

                bool                m_isFrameGraphRendering = true;
                uint8_t             m_currLOD = 0;
                LODArray            m_vLODs;
                UObjectHandle       m_hObj; // a handle in frame and Scene buffer
                handle_t            m_hSceneGraph; // a handle in scene graph
                uint32_t            m_hDbgView; // a handle to debug view object
        };
        using DrawcallPtr = CDrawcall*;

        struct SFrameGraphDesc
        {
            cstr_t  pName = "";
            void*   pDesc = nullptr;
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

                virtual void        SetScene(Scene::CScene* pScene) { m_pScene = pScene; }

                virtual void        Render(CommandBufferPtr) = 0;

            protected:

                virtual Result      _Create( const SFrameGraphDesc& ) = 0;
                virtual void        _Destroy() = 0;

            protected:

                Scene::CScene*      m_pScene = nullptr;
        };
    }
}