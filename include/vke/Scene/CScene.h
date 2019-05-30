#pragma once

#include "Common.h"
#include "RenderSystem/Common.h"
#include "Core/Math/CAABB.h"
#include "Core/Math/CMatrix.h"
#include "Core/Math/CFrustum.h"
#include "Scene/CCamera.h"
#include "Core/Threads/ITask.h"
#include "Core/Math/CBoundingSphere.h"

namespace VKE
{
    namespace RenderSystem
    {
        class CGraphicsContext;
    }

    namespace Scene
    {
        class CScene;
        class CCamera;

        struct ObjectTypes
        {
            enum TYPE : uint8_t
            {
                SCENE_NODE,
                LIGHT,
                DRAWCALL,
                _MAX_COUNT
            };
        };
        using OBJECT_TYPE = ObjectTypes::TYPE;

        class CMaterial : public Core::CObject
        {
            friend class CScene;
            friend class CModel;
            friend class CDrawcall;

            public:

            protected:
        };

        struct SDrawParams
        {
            uint32_t    indexOffset;
            uint32_t    indexCount;
            uint32_t    vertexOffset;
            uint32_t    vertexCount;
        };

        struct SDrawcallData
        {
            RenderSystem::VertexBufferHandle        hVertexBuffer;
            RenderSystem::IndexBufferHandle         hIndexBuffer;
            RenderSystem::DescriptorSetHandle       hDescSet;
            RenderSystem::ShaderPtr                 pVertexShader;
            RenderSystem::ShaderPtr                 pPixelShader;
            RenderSystem::SVertexInputLayoutDesc    InputLayout;
            RenderSystem::PRIMITIVE_TOPOLOGY        topology;
            uint32_t                                vertexBufferOffset = 0;
            uint32_t                                indexBufferOffset = 0;
            RenderSystem::SDrawParams               DrawParams;
        };

        class CDrawcall
        {
            friend class CScene;
            public:

                using LOD = SDrawcallData;

            protected:

                using LODArray = Utils::TCDynamicArray< LOD, 4 >;

            public:

                void AddLOD( const LOD& LOD )
                {
                    m_vLODs.PushBack( LOD );
                }

            protected:
       
                uint8_t             m_currLOD = 0;
                LODArray            m_vLODs;
                handle_t            m_handle;
        };

        class CModel
        {
            public:
        };

        struct SSceneTasks
        {
            struct SFrustumCull : public Threads::ITask
            {
                CScene*  pScene;

                TaskState _OnStart( uint32_t threadId ) override { return TaskStateBits::OK; }
            };

            struct SDrawcallSort : public Threads::ITask
            {
                CScene* pScene;

                TaskState _OnStart( uint32_t threadId ) override { return TaskStateBits::OK; }
            };
        };

        class VKE_API CScene : public Core::CObject
        {
            friend class CWorld;
            friend class COctree;
            friend class CQuadTree;
            friend class CBVH;

            using TypeBitsArray = Utils::TCDynamicArray< UObjectBits, 1 >;
            using DrawcallArray = Utils::TCDynamicArray< CDrawcall, 1 >;
            using DrawcallDataArray = Utils::TCDynamicArray< SDrawcallData, 1 >;
            using BoolArray = Utils::TCDynamicArray< bool, 1 >;
            using AABBArray = Utils::TCDynamicArray< Math::CAABB, 1 >;
            using MatrixArray = Utils::TCDynamicArray < Math::CMatrix4x4, 1 >;
            using UintArray = Utils::TCDynamicArray< uint32_t, 1 >;
            using FrustumArray = Utils::TCDynamicArray< Math::CFrustum >;
            using CameraArray = Utils::TCDynamicArray< CCamera >;
            using FrustumCullTaskArray = Utils::TCDynamicArray< SSceneTasks::SFrustumCull, 8 >;
            using DrawcallSortTaskArray = Utils::TCDynamicArray< SSceneTasks::SDrawcallSort, 8 >;
            using BoundingSphereArray = Utils::TCDynamicArray< Math::CBoundingSphere, 1 >;

            union UObjectHandle
            {
                struct  
                {
                    uint64_t    type            : 3;
                    uint64_t    reserved        : 21;
                    uint64_t    objDataIndex    : 20;
                    uint64_t    objTypeIndex    : 20;
                };
                handle_t    handle;
            };

            struct SDrawData
            {
                TypeBitsArray       vBits;
                BoundingSphereArray vBoundingSpheres;
                AABBArray           vAABBs;
                MatrixArray         vTransforms;

                void Clear()
                {
                    vBits.Clear();
                    vBoundingSpheres.Clear();
                    vAABBs.Clear();
                    vTransforms.Clear();
                }

                void Resize( const uint32_t count )
                {
                    vBits.Resize( count );
                    vBoundingSpheres.Resize( count );
                    vAABBs.Resize( count );
                    vTransforms.Resize( count );
                }

                void Reserve( const uint32_t count )
                {
                    vBits.Reserve( count );
                    vBoundingSpheres.Reserve( count );
                    vAABBs.Reserve( count );
                    vTransforms.Reserve( count );
                }

                uint32_t Add(const uint32_t& index)
                {
                    uint32_t idx = vBits.PushBack( UObjectBits( false, index ) );
                    vBoundingSpheres.PushBack( Math::CBoundingSphere::ONE );
                    vAABBs.PushBack( Math::CAABB::ONE );
                    vTransforms.PushBack( Math::CMatrix4x4::IDENTITY );
                    VKE_ASSERT( vBits.GetCount() == vAABBs.GetCount() == vTransforms.GetCount() == vBoundingSpheres.GetCount(), "" );
                    return idx;
                }

                void Update( const uint32_t idx, bool isVisible ) { vBits[idx].visible = isVisible; }
                void Update( const uint32_t idx, const Math::CBoundingSphere& Sphere ) { vBoundingSpheres[idx] = Sphere; }
                void Update( const uint32_t idx, const Math::CAABB& AABB ) { vAABBs[idx] = AABB; }
                void Update( const uint32_t idx, const Math::CMatrix4x4& Matrix ) { vTransforms[idx] = Matrix; }
                uint32_t GetCount() const { return vBits.GetCount(); }
                Math::CBoundingSphere& GetBoundingSphere( const uint32_t idx ) { return vBoundingSpheres[idx]; }
                Math::CAABB& GetAABB( const uint32_t idx ) { return vAABBs[idx]; }
                Math::CMatrix4x4& GetTransfrom( const uint32_t idx ) { return vTransforms[idx]; }
                UObjectBits& GetBits( const uint32_t idx ) { return vBits[idx]; }
            };

            struct SLightData
            {
                BoundingSphereArray vBoundingSpheres;
                FrustumArray        vFrustums;
            };

            struct SSceneNodeData
            {
                BoundingSphereArray vBoundingSpheres;
                AABBArray           vAABBs;
            };

            public:

                CScene(CWorld* pWorld) {}
                ~CScene() {}

                handle_t AddObject(CDrawcall*);
                void AddObject( const CModel* );

                CameraPtr CreateCamera(cstr_t dbgName);
                void SetCamera( CameraPtr pCamera ) { m_pCurrentCamera = pCamera; }

                void Render( VKE::RenderSystem::CGraphicsContext* pCtx );

            protected:

                Result  _Create( const SSceneDesc& );
                void    _Destroy();

                void        _FrustumCullDrawcalls( const Math::CFrustum& Frustum );
                void        _SortDrawcalls(const Math::CFrustum& Frustum);
                void        _Draw( VKE::RenderSystem::CGraphicsContext* pCtx );

                handle_t    _CreateSceneNode(const uint32_t idx);

            protected:

                COctree*                m_pOctree = nullptr;
                CQuadTree*              m_pQuadTree = nullptr;
                CBVH*                   m_pBVH = nullptr;

                CameraArray             m_vCameras;
                FrustumArray            m_vFrustums;
                DrawcallArray           m_vDrawcalls;
                SDrawData               m_DrawData;
                CameraPtr               m_pCurrentCamera;
                FrustumCullTaskArray    m_vFrustumCullTasks;
                DrawcallSortTaskArray   m_vDrawcallSortTasks;

                Threads::SyncObject     m_ObjectDataSyncObj;
        };

        using ScenePtr = Utils::TCWeakPtr<CScene>;
        using SceneRefPtr = Utils::TCObjectSmartPtr<CScene>;
    }
} // VKE
