#pragma once

#include "Common.h"
#include "RenderSystem.h"
#include "Core/Math/CAABB.h"
#include "Core/Math/CMatrix.h"
#include "Core/Math/CFrustum.h"
#include "Scene/CCamera.h"
#include "Core/Threads/ITask.h"
#include "Core/Math/CBoundingSphere.h"
#include "RenderSystem/IFrameGraph.h"
#include "Scene/Terrain/CTerrain.h"

namespace VKE
{
    namespace RenderSystem
    {
        class CGraphicsContext;
        class IFrameGraph;
        class CForwardRenderer;
        class CDeviceContext;
    }

    namespace Scene
    {
        class CScene;
        class CCamera;
        class CTerrain;

        using TerrainPtr = Utils::TCWeakPtr< CTerrain >;

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

        struct DrawLayers
        {
            enum LAYER
            {
                DEFAULT_0,
                DEFAULT_1,
                DEFAULT_2,
                DEFAULT_3,
                DEFAULT_4,
                DEFAULT_5,
                DEFAULT_6,
                TERRAIN_0   = 22,
                TERRAIN_1   = 23,
                SKY_0       = 24,
                SKY_1       = 25,
                BLENDING_0  = 26,
                BLENDING_1  = 27,
                BLENDING_2  = 28,
                BLENDING_3  = 29,
                BLENDING_4  = 30,
                _MAX_COUNT  = 31
            };
        };
        using DRAW_LAYER = DrawLayers::LAYER;

        struct SDrawcallDataInfo
        {
            Math::CMatrix4x4        mtxTransform;
            Math::CAABB             AABB = Math::CAABB::ONE;
            Math::CBoundingSphere   Sphere;
            DRAW_LAYER              layer = DrawLayers::DEFAULT_0;
            bool                    canBeCulled = true;
            bool                    debugView = false;
        };

        class VKE_API CScene : public Core::CObject
        {
                friend class CWorld;
                friend class COctree;
                friend class CQuadTree;
                friend class CBVH;
                friend class RenderSystem::IFrameGraph;
                friend class RenderSystem::CForwardRenderer;
                friend class CTerrain;
                friend class ITerrainRenderer;
                friend class CTerrainVertexFetchRenderer;

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

                struct SDebugView
                {
                    struct InstancingTypes
                    {
                        enum TYPE
                        {
                            AABB,
                            SPHERE,
                            FRUSTUM,
                            _MAX_COUNT
                        };
                    };
                    using INSTANCING_TYPE = InstancingTypes::TYPE;

                    struct BatchTypes
                    {
                        enum TYPE
                        {
                            AABB,
                            SPHERE,
                            FRUSTUM,
                            _MAX_COUNT
                        };
                    };
                    using BATCH_TYPE = BatchTypes::TYPE;

                    enum
                    {
                        MAX_INSTANCING_DRAW_COUNT = 0xFFFFFFF,
                        MAX_INSTANCING_CONSTANT_BUFFER_COUNT = 0xF,
                        MAX_INSTANCING_DATA_PER_BUFFER = 1000
                    };

                    union UInstancingHandle
                    {
                        struct 
                        {
                            uint32_t    bufferIndex : 4; // max 15 buffers
                            uint32_t    index       : 28;
                        };
                        uint32_t        handle;
                    };

                    struct SConstantBuffer
                    {
                        Utils::TCDynamicArray< uint8_t, 1>  vData;
                        RenderSystem::BufferPtr             pConstantBuffer;
                        RenderSystem::BufferPtr             pStorageBuffer;
                        RenderSystem::DescriptorSetHandle   hDescSet; // constant buffer
                        uint32_t                            drawCount = 0;
                    };
                    using BufferArray = Utils::TCDynamicArray< SConstantBuffer, 4 >;

                    struct SPerFrameShaderData
                    {
                        Math::CMatrix4x4    mtxViewProj;
                    };

                    struct SInstancingShaderData
                    {
                        Math::CMatrix4x4    mtxTransform;
                        Math::CVector4      vecColor;
                    };

                    struct SInstancing
                    {
                        struct 
                        {
                            RenderSystem::SDrawParams       DrawParams;
                            RenderSystem::PipelinePtr       pPipeline;
                        } DrawData;
                        RenderSystem::DDIRenderPass         hDDIRenderPass = RenderSystem::DDI_NULL_HANDLE;
                        BufferArray                         vConstantBuffers;
                        Utils::TCBitset<uint16_t>           UpdateBufferMask;
                    };
                    
                    struct SBatch
                    {
                        struct SVertex
                        {
                            Math::CVector3  vecPosition;
                            //Math::CVector4  vecColor;
                        };

                        struct SBuffer
                        {
                            RenderSystem::BufferPtr     pBuffer;
                            RenderSystem::SDrawParams   DrawParams;
                        };
                        using BufferVec = Utils::TCDynamicArray< SBuffer >;
                        
                        BufferVec                           vBuffers;
                        RenderSystem::PipelineRefPtr        pPipeline;
                        uint32_t                            indexBufferOffset;
                        uint16_t                            objectVertexCount;
                        uint16_t                            objectIndexCount;
                    };

                    SBatch                              aBatches[BatchTypes::_MAX_COUNT];
                    SInstancing                         aInstancings[InstancingTypes::_MAX_COUNT];
                    RenderSystem::SPipelineCreateDesc   InstancingPipelineTemplate;
                    RenderSystem::SPipelineCreateDesc   BatchPipelineTemplate;
                    RenderSystem::DescriptorSetHandle   hPerFrameDescSet = INVALID_HANDLE;
                    RenderSystem::BufferPtr             pPerFrameConstantBuffer;
                    RenderSystem::VertexBufferHandle    hInstancingVB;
                    RenderSystem::IndexBufferHandle     hInstancingIB;
                    RenderSystem::CDeviceContext*       pDeviceCtx;

                    void        Render( RenderSystem::CGraphicsContext* pCtx );
                    uint32_t    AddInstancing( RenderSystem::CDeviceContext* pCtx, INSTANCING_TYPE type );
                    uint32_t    AddBatchData( RenderSystem::CDeviceContext* pCtx, BATCH_TYPE type );
                    void        UpdateBatchData( BATCH_TYPE type, const uint32_t& handle, const Math::CAABB& AABB );
                    void        UpdateBatchData( BATCH_TYPE type, const uint32_t& handle, const Math::CVector3* aCorners );
                    void        UpdateInstancing( INSTANCING_TYPE type, const uint32_t& handle, const Math::CMatrix4x4& mtxTransform );
                    void        UploadInstancingConstantData(RenderSystem::CGraphicsContext* pCtx, const CCamera* pCamera);
                    void        UploadBatchData( RenderSystem::CGraphicsContext* pCtx, const CCamera* pCamera );
                    void        CalcCorners( const Math::CAABB& AABB, SBatch::SVertex* pOut );

                    bool        CreateConstantBuffer( RenderSystem::CDeviceContext* pCtx, uint32_t elementCount,
                        SConstantBuffer* pOut );
                    bool        CreateDrawcallData( RenderSystem::CDeviceContext* pCtx, INSTANCING_TYPE type );
                };

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
                        vTransforms.PushBack( Info.mtxTransform );
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

                using DrawDataArray = Utils::TCDynamicArray< SDrawData, 31 >;
                using DrawcallArrayArray = Utils::TCDynamicArray< DrawcallArray, 31 >;

            public:

                CScene(CWorld* pWorld) :
                    m_pWorld{ pWorld } {}
                ~CScene() {}

                RenderSystem::DrawcallPtr   CreateDrawcall( const Scene::SDrawcallDesc& Desc );
                TerrainPtr                  CreateTerrain( const STerrainDesc& Desc, RenderSystem::CDeviceContext* );
                void                        DestroyTerrain( TerrainPtr* ppInOut );

                handle_t AddObject( RenderSystem::DrawcallPtr pDrawcall, const SDrawcallDataInfo& Info );
                void AddObject( const CModel* );

                CameraPtr CreateCamera(cstr_t dbgName);
                void SetRenderCamera( CameraPtr pCamera ) { m_pCurrentRenderCamera = pCamera; }
                CameraPtr GetRenderCamera() const { return m_pCurrentRenderCamera; }
                void SetCamera( CameraPtr pCamera ) { m_pCurrentCamera = pCamera; }
                CameraPtr GetCamera() const { return m_pCurrentCamera; }

                void Render( VKE::RenderSystem::CGraphicsContext* pCtx );

                void    UpdateDrawcallAABB( const handle_t& hDrawcall, const Math::CAABB& NewAABB );

                void    AddDebugView( CameraPtr* pCamera );

            protected:

                Result  _Create( const SSceneDesc& );
                void    _Destroy();

                void        _FrustumCullDrawcalls( const Math::CFrustum& Frustum );
                void        _SortDrawcalls(const Math::CFrustum& Frustum);
                void        _Draw( VKE::RenderSystem::CGraphicsContext* pCtx );

                void vke_force_inline   _SetObjectVisible( const UObjectHandle& hObj, const bool isVisible );

                handle_t    _CreateSceneNode(const uint32_t idx);

                Result      _CreateDebugView(RenderSystem::CDeviceContext* pCtx);
                void        _DestroyDebugView();
                SDebugView* _GetDebugView() { return m_pDebugView; }
                void        _RenderDebugView(RenderSystem::CGraphicsContext* pCtx);
                void        _UpdateDebugViews(RenderSystem::CGraphicsContext* pCtx);

            protected:

                CWorld*                         m_pWorld = nullptr;
                COctree*                        m_pOctree = nullptr;
                CQuadTree*                      m_pQuadTree = nullptr;
                CBVH*                           m_pBVH = nullptr;
                RenderSystem::IFrameGraph*      m_pFrameGraph = nullptr;
                RenderSystem::CDeviceContext*   m_pDeviceCtx = nullptr;

                CameraArray             m_vCameras;
                FrustumArray            m_vFrustums;
                DrawcallArray           m_vpDrawcalls;
                DrawcallArray           m_vpAlwaysVisibleDrawcalls;
                DrawcallArrayArray      m_vpVisibleLayerDrawcalls;
                DrawDataArray           m_vDrawLayers;
                CameraPtr               m_pCurrentRenderCamera = nullptr;
                CameraPtr               m_pCurrentCamera = nullptr;
                TerrainPtr              m_pTerrain;
                FrustumCullTaskArray    m_vFrustumCullTasks;
                DrawcallSortTaskArray   m_vDrawcallSortTasks;

                Threads::SyncObject     m_ObjectDataSyncObj;

                SDebugView*             m_pDebugView = nullptr;
        };

        using ScenePtr = Utils::TCWeakPtr<CScene>;
        using SceneRefPtr = Utils::TCObjectSmartPtr<CScene>;

        void vke_force_inline CScene::_SetObjectVisible( const UObjectHandle& hObj, const bool isVisible )
        {
            //m_DrawData.vVisibles[hObj.index] = (isVisible);
            m_vDrawLayers[hObj.layer].vVisibles[hObj.index] = (isVisible);
        }
    }
} // VKE
