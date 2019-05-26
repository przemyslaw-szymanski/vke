#pragma once

#include "Common.h"
#include "RenderSystem/Common.h"
#include "Core/Math/CAABB.h"
#include "Core/Math/CMatrix.h"
#include "Core/Math/CFrustum.h"
#include "Scene/CCamera.h"
#include "Core/Threads/ITask.h"

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
        using CameraPtr = Utils::TCWeakPtr<CCamera>;

        struct SceneObjectTypes
        {
            enum TYPE : uint8_t
            {
                SCENE_NODE,
                LIGHT,
                DRAWCALL,
                _MAX_COUNT
            };
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
            RenderSystem::DDIBuffer hDDIVertexBuffer;
            RenderSystem::DDIBuffer hDDIIndexBuffer;
            SDrawParams             Params;
        };

        class CDrawcall
        {
            friend class CScene;
            using LODArray = Utils::TCDynamicArray< SDrawcallData*, 4 >;

            public:


            protected:
       
                uint8_t             m_currLOD = 0;
                LODArray            m_vpLODs;
                uint32_t            m_handle;
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

                TaskState _OnStart( uint32_t threadId ) override;
            };

            struct SDrawcallSort : public Threads::ITask
            {
                CScene* pScene;

                TaskState _OnStart( uint32_t threadId ) override;
            };
        };

        class CScene : public Core::CObject
        {
            friend class CWorld;
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

            struct SDrawData
            {
                BoolArray           vVisibles;
                AABBArray           vAABBs;
                MatrixArray         vTransforms;
                DrawcallDataArray   vParams;

                void Clear()
                {
                    vVisibles.Clear();
                    vAABBs.Clear();
                    vTransforms.Clear();
                    vParams.Clear();
                }

                void Resize( const uint32_t count )
                {
                    vVisibles.Resize( count );
                    vAABBs.Resize( count );
                    vTransforms.Resize( count );
                    vParams.Resize( count );
                }

                void Reserve( const uint32_t count )
                {
                    vVisibles.Reserve( count );
                    vAABBs.Reserve( count );
                    vTransforms.Reserve( count );
                    vParams.Reserve( count );
                }

                uint32_t Add( const Math::CMatrix4x4& Transform, const Math::CAABB& AABB, const SDrawcallData& Params,
                    bool isVisible )
                {
                    uint32_t idx = vVisibles.PushBack( isVisible );
                    vAABBs.PushBack( AABB );
                    vTransforms.PushBack( Transform );
                    vParams.PushBack( Params );
                    VKE_ASSERT( vVisibles.GetCount() == vAABBs.GetCount() == vTransforms.GetCount() == vParams.GetCount(), "" );
                    return idx;
                }

                uint32_t Add()
                {
                    uint32_t idx = vVisibles.PushBack( false );
                    vAABBs.PushBack( Math::CAABB::ONE );
                    vTransforms.PushBack( Math::CMatrix4x4::IDENTITY );
                    vParams.PushBack( {} );
                    VKE_ASSERT( vVisibles.GetCount() == vAABBs.GetCount() == vTransforms.GetCount() == vParams.GetCount(), "" );
                    return idx;
                }

                void Update( const uint32_t idx, bool isVisible ) { vVisibles[idx] = isVisible; }
                void Update( const uint32_t idx, const Math::CAABB& AABB ) { vAABBs[idx] = AABB; }
                void Update( const uint32_t idx, const Math::CMatrix4x4& Matrix ) { vTransforms[idx] = Matrix; }
                void Update( const uint32_t idx, const SDrawcallData& Params ) { vParams[idx] = Params; }
                uint32_t GetCount() const { return vVisibles.GetCount(); }
                Math::CAABB& GetAABB( const uint32_t idx ) { return vAABBs[idx]; }
                Math::CMatrix4x4& GetTransfrom( const uint32_t idx ) { return vTransforms[idx]; }
                SDrawcallData& GetParams( const uint32_t idx ) { return vParams[idx]; }
                bool& GetVisible( const uint32_t idx ) { return vVisibles[idx]; }
            };

            public:

                uint32_t AddObject(CDrawcall*);
                void AddObject( const CModel* );

                CameraPtr CreateCamera(cstr_t dbgName);
                void SetCamera( CameraPtr pCamera );

                void Render( VKE::RenderSystem::CGraphicsContext* pCtx );

            protected:

                Result  _Create( const SSceneDesc& );
                void    _Destroy();

                void    _FrustumCullDrawcalls( const Math::CFrustum& Frustum );
                void    _SortDrawcalls(const Math::CFrustum& Frustum);
                void    _Draw( VKE::RenderSystem::CGraphicsContext* pCtx );

            protected:

                CameraArray             m_vCameras;
                FrustumArray            m_vFrustums;
                DrawcallArray           m_vDrawcalls;
                SDrawData               m_DrawData;
                CameraPtr               m_pCurrentCamera;
                FrustumCullTaskArray    m_vFrustumCullTasks;
                DrawcallSortTaskArray   m_vDrawcallSortTasks;
        };

        using ScenePtr = Utils::TCWeakPtr<CScene>;
        using SceneRefPtr = Utils::TCObjectSmartPtr<CScene>;
    }
} // VKE
