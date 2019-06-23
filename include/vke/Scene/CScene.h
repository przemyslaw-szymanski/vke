#pragma once

#include "Common.h"
#include "RenderSystem/Common.h"
#include "Core/Math/CAABB.h"
#include "Core/Math/CMatrix.h"
#include "Core/Math/CFrustum.h"
#include "Scene/CCamera.h"
#include "Core/Threads/ITask.h"
#include "Core/Math/CBoundingSphere.h"
#include "RenderSystem/IFrameGraph.h"

namespace VKE
{
    namespace RenderSystem
    {
        class CGraphicsContext;
        class IFrameGraph;
        class CForwardRenderer;
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
        

        class CModel
        {
            public:
        };

        struct SSceneTasks
        {
            struct SFrustumCull : public Threads::ITask
            {
                CScene*  pScene;

                TaskState _OnStart( uint32_t ) override { return TaskStateBits::OK; }
            };

            struct SDrawcallSort : public Threads::ITask
            {
                CScene* pScene;

                TaskState _OnStart( uint32_t ) override { return TaskStateBits::OK; }
            };
        };

        struct SDrawcallDataInfo
        {
            Math::CMatrix4x4        Transform;
            Math::CAABB             AABB = Math::CAABB::ONE;
            Math::CBoundingSphere   Sphere;
            bool                    canBeCulled = true;
        };

        class VKE_API CScene : public Core::CObject
        {
            friend class CWorld;
            friend class COctree;
            friend class CQuadTree;
            friend class CBVH;
            friend class RenderSystem::IFrameGraph;
            friend class RenderSystem::CForwardRenderer;

            using TypeBitsArray = Utils::TCDynamicArray< UObjectBits, 1 >;
            using DrawcallArray = Utils::TCDynamicArray< RenderSystem::DrawcallPtr, 1 >;
            using DrawcallDataArray = Utils::TCDynamicArray< RenderSystem::SDrawcallData, 1 >;
            using BoolArray = Utils::TCDynamicArray< bool, 1 >;
            using AABBArray = Utils::TCDynamicArray< Math::CAABB, 1 >;
            using MatrixArray = Utils::TCDynamicArray < Math::CMatrix4x4, 1 >;
            using UintArray = Utils::TCDynamicArray< uint32_t, 1 >;
            using FrustumArray = Utils::TCDynamicArray< Math::CFrustum >;
            using CameraArray = Utils::TCDynamicArray< CCamera >;
            using FrustumCullTaskArray = Utils::TCDynamicArray< SSceneTasks::SFrustumCull, 8 >;
            using DrawcallSortTaskArray = Utils::TCDynamicArray< SSceneTasks::SDrawcallSort, 8 >;
            using BoundingSphereArray = Utils::TCDynamicArray< Math::CBoundingSphere, 1 >;

            struct SDrawData
            {
                BoolArray           vVisibles;
                BoundingSphereArray vBoundingSpheres;
                AABBArray           vAABBs;
                MatrixArray         vTransforms;

                void Clear()
                {
                    vVisibles.Clear();
                    vBoundingSpheres.Clear();
                    vAABBs.Clear();
                    vTransforms.Clear();
                }

                void Resize( const uint32_t count )
                {
                    vVisibles.Resize( count );
                    vBoundingSpheres.Resize( count );
                    vAABBs.Resize( count );
                    vTransforms.Resize( count );
                }

                void Reserve( const uint32_t count )
                {
                    vVisibles.Reserve( count );
                    vBoundingSpheres.Reserve( count );
                    vAABBs.Reserve( count );
                    vTransforms.Reserve( count );
                }

                uint32_t Add()
                {
                    uint32_t idx = vVisibles.PushBack( false );
                    vBoundingSpheres.PushBack( Math::CBoundingSphere::ONE );
                    vAABBs.PushBack( Math::CAABB::ONE );
                    vTransforms.PushBack( Math::CMatrix4x4::IDENTITY );
                    VKE_ASSERT( vVisibles.GetCount() == vAABBs.GetCount(), "" );
                    VKE_ASSERT( vAABBs.GetCount() == vTransforms.GetCount(), "" );
                    VKE_ASSERT( vTransforms.GetCount() == vBoundingSpheres.GetCount(), "" );
                    return idx;
                }

                uint32_t Add( const SDrawcallDataInfo& Info )
                {
                    uint32_t idx = vVisibles.PushBack( false );
                    vBoundingSpheres.PushBack( Info.Sphere );
                    vAABBs.PushBack( Info.AABB );
                    vTransforms.PushBack( Info.Transform );
                    VKE_ASSERT( vVisibles.GetCount() == vAABBs.GetCount(), "" );
                    VKE_ASSERT( vAABBs.GetCount() == vTransforms.GetCount(), "" );
                    VKE_ASSERT( vTransforms.GetCount() == vBoundingSpheres.GetCount(), "" );
                    return idx;
                }

                void Update( const uint32_t idx, bool isVisible ) { vVisibles[idx] = isVisible; }
                void Update( const uint32_t idx, const Math::CBoundingSphere& Sphere ) { vBoundingSpheres[idx] = Sphere; }
                void Update( const uint32_t idx, const Math::CAABB& AABB ) { vAABBs[idx] = AABB; }
                void Update( const uint32_t idx, const Math::CMatrix4x4& Matrix ) { vTransforms[idx] = Matrix; }
                uint32_t GetCount() const { return vVisibles.GetCount(); }
                Math::CBoundingSphere& GetBoundingSphere( const uint32_t idx ) { return vBoundingSpheres[idx]; }
                Math::CAABB& GetAABB( const uint32_t idx ) { return vAABBs[idx]; }
                Math::CMatrix4x4& GetTransfrom( const uint32_t idx ) { return vTransforms[idx]; }
                //UObjectBits& GetBits( const uint32_t idx ) { return vVisibles[idx]; }
                bool& GetVisible( const uint32_t idx ) { return vVisibles[idx]; }
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

                CScene(CWorld* pWorld) :
                    m_pWorld{ pWorld } {}
                ~CScene() {}

                RenderSystem::DrawcallPtr CreateDrawcall( const Scene::SDrawcallDesc& Desc );

                handle_t AddObject( RenderSystem::DrawcallPtr pDrawcall, const SDrawcallDataInfo& Info );
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

                void vke_force_inline   _SetObjectVisible( const UObjectHandle& hObj, const bool isVisible );

                handle_t    _CreateSceneNode(const uint32_t idx);

            protected:

                CWorld*                     m_pWorld = nullptr;
                COctree*                    m_pOctree = nullptr;
                CQuadTree*                  m_pQuadTree = nullptr;
                CBVH*                       m_pBVH = nullptr;
                RenderSystem::IFrameGraph*  m_pFrameGraph = nullptr;

                CameraArray             m_vCameras;
                FrustumArray            m_vFrustums;
                DrawcallArray           m_vpDrawcalls;
                DrawcallArray           m_vpAlwaysVisibleDrawcalls;
                DrawcallArray           m_vpVisibleDrawcalls;
                SDrawData               m_DrawData;
                CameraPtr               m_pCurrentCamera;
                FrustumCullTaskArray    m_vFrustumCullTasks;
                DrawcallSortTaskArray   m_vDrawcallSortTasks;

                Threads::SyncObject     m_ObjectDataSyncObj;
        };

        using ScenePtr = Utils::TCWeakPtr<CScene>;
        using SceneRefPtr = Utils::TCObjectSmartPtr<CScene>;

        void vke_force_inline CScene::_SetObjectVisible( const UObjectHandle& hObj, const bool isVisible )
        {
            m_DrawData.vVisibles[hObj.index] = (isVisible);
        }
    }
} // VKE
