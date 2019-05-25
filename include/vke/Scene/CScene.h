#pragma once

#include "Common.h"
#include "RenderSystem/Common.h"
#include "Core/Math/CAABB.h"

namespace VKE
{
    namespace RenderSystem
    {
        class CGraphicsContext;
    }

    namespace Scene
    {
        struct SDrawcallData
        {
            RenderSystem::DDIBuffer hDDIVertexBuffer;
            RenderSystem::DDIBuffer hDDIIndexBuffer;
            uint16_t                vertexCount;
            uint16_t                instanceCount;
            uint16_t                firstVertex;
            uint16_t                firstInstance;
        };

        class CDrawcall
        {
            public:

                struct SLOD
                {
                    uint32_t    indexOffset;
                    uint32_t    indexCount;
                    uint32_t    vertexOffset;
                    uint32_t    vertexCount;
                };

                using LODArray = Utils::TCDynamicArray< SLOD, 4 >;

            protected:

                uint8_t     m_currLOD = 0;
                LODArray    m_vLODs;
        };

        class CModel
        {
            public:
        };

        class CScene : public Core::CObject
        {
            friend class CWorld;
            using DrawcallArray = Utils::TCDynamicArray< SDrawcallData, 1 >;
            using BoolArray = Utils::TCDynamicArray< bool, 1 >;
            using AABBArray = Utils::TCDynamicArray< Math::CAABB, 1 >;
            using MatrixArray = Utils::TCDynamicArray < Math::CMatrix4x4, 1 >;

            struct SDrawData
            {
                BoolArray       vVisibles;
                AABBArray       vAABBs;
                MatrixArray     vTransforms;
                DrawcallArray   vDrawcallParams;

                void Clear()
                {
                    vVisibles.Clear();
                    vAABBs.Clear();
                    vTransforms.Clear();
                    vDrawcallParams.Clear();
                }

                void Resize( const uint32_t count )
                {
                    vVisibles.Resize( count );
                    vAABBs.Resize( count );
                    vTransforms.Resize( count );
                    vDrawcallParams.Resize( count );
                }

                void Reserve( const uint32_t count )
                {
                    vVisibles.Reserve( count );
                    vAABBs.Reserve( count );
                    vTransforms.Reserve( count );
                    vDrawcallParams.Reserve( count );
                }

                uint32_t Add( const Math::CMatrix4x4& Transform, const Math::CAABB& AABB, const SDrawcallData& Params,
                    bool isVisible )
                {
                    uint32_t idx = vVisibles.PushBack( isVisible );
                    vAABBs.PushBack( AABB );
                    vTransforms.PushBack( Transform );
                    vDrawcallParams.PushBack( Params );
                    VKE_ASSERT( vVisibles.GetCount() == vAABBs.GetCount() == vTransforms.GetCount() == vDrawcallParams.GetCount(), "" );
                    return idx;
                }

                void Update( const uint32_t idx, bool isVisible ) { vVisibles[idx] = isVisible; }
                void Update( const uint32_t idx, const Math::CAABB& AABB ) { vAABBs[idx] = AABB; }
                void Update( const uint32_t idx, const Math::CMatrix4x4& Matrix ) { vTransforms[idx] = Matrix; }
                void Update( const uint32_t idx, const SDrawcallData& Params ) { vDrawcallParams[idx] = Params; }
                uint32_t GetCount() const { return vVisibles.GetCount(); }
                Math::CAABB& GetAABB( const uint32_t idx ) { return vAABBs[idx]; }
                Math::CMatrix4x4& GetTransfrom( const uint32_t idx ) { return vTransforms[idx]; }
                SDrawcallData& GetParams( const uint32_t idx ) { return vDrawcallParams[idx]; }
                bool& GetVisible( const uint32_t idx ) { return vVisibles[idx]; }
            };

            public:

                uint32_t AddObject(const RenderSystem::CDrawcall*);

                void Render( VKE::RenderSystem::CGraphicsContext* pCtx );

            protected:

                Result  _Create( const SSceneDesc& );
                void    _Destroy();

            protected:

                DrawcallArray   m_vDrawcalls;
        };

        using ScenePtr = Utils::TCWeakPtr<CScene>;
        using SceneRefPtr = Utils::TCObjectSmartPtr<CScene>;
    }
} // VKE
