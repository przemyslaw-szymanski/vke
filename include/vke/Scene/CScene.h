#pragma once
#include "Common.h"
#include "Core/Math/CAABB.h"
#include "Core/Math/CBoundingSphere.h"
#include "Core/Math/CFrustum.h"
#include "Core/Math/CMatrix.h"
#include "Core/Threads/ITask.h"
#include "RenderSystem.h"
#include "RenderSystem/IFrameGraph.h"
#include "Scene/CCamera.h"
#include "Scene/Terrain/CTerrain.h"
#include "Scene/CLight.h"
namespace VKE
{
    namespace RenderSystem
    {
        class CGraphicsContext;
        class IFrameGraph;
        class CForwardRenderer;
        class CDeviceContext;
    } // namespace RenderSystem
    namespace Scene
    {
        class CScene;
        class CCamera;
        class CTerrain;
        using TerrainPtr = Utils::TCWeakPtr<CTerrain>;
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
                CScene* pScene;
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
                TERRAIN_0 = 22,
                TERRAIN_1 = 23,
                SKY_0 = 24,
                SKY_1 = 25,
                BLENDING_0 = 26,
                BLENDING_1 = 27,
                BLENDING_2 = 28,
                BLENDING_3 = 29,
                BLENDING_4 = 30,
                _MAX_COUNT = 31
            };
        };
        using DRAW_LAYER = DrawLayers::LAYER;
        struct SDrawcallDataInfo
        {
            Math::CMatrix4x4 mtxTransform;
            Math::CAABB AABB = Math::CAABB::ONE;
            Math::CBoundingSphere Sphere;
            DRAW_LAYER layer = DrawLayers::DEFAULT_0;
            bool canBeCulled = true;
            bool debugView = false;
        };

        struct SLights
        {
            using Vec3Array = Utils::TCDynamicArray<Math::CVector3, 1>;
            using FloatArray = Utils::TCDynamicArray<float, 1>;
            using NameArray = Utils::TCDynamicArray<ResourceName, 1>;
            using ColorArray = Utils::TCDynamicArray<RenderSystem::SColor, 1>;
            using LightArray = Utils::TCDynamicArray<LightRefPtr, 1>;
            using IndexArray = Utils::TCDynamicArray<uint32_t, 1>;
            using LightDataArray = Utils::TCDynamicArray<CLight::SData, 1>;
            Vec3Array vPositions;
            Vec3Array vDirections;
            FloatArray vStrengths;
            FloatArray vRadiuses;
            ColorArray vColors;
            IndexArray vDbgViews;
            vke_bool_vector vNeedUpdates;
            vke_bool_vector vEnableds;
            LightArray vpLights;
            LightDataArray vSortedLightData;
            IndexArray vFreeIndices;
            uint32_t enabledCount = 0;
            uint32_t needUpdateCount = 0;
        };

        struct ConstantBufferLayoutElements
        {
            enum ELEMENT : uint8_t
            {
                VIEW_CAMERA_VIEW_MTX4,
                VIEW_CAMERA_PROJ_MTX4,
                VIEW_CAMERA_VIEWPROJ_MTX4,
                MAIN_LIGHT_POSITION_FLOAT3,
                MAIN_LIGHT_RADIUS_FLOAT,
                MAIN_LIGHT_DIRECTION_FLOAT3,
                MAIN_LIGHT_ATTENUATION_FLOAT,
                MAIN_LIGHT_COLOR_FLOAT3,
                _MAX_COUNT
            };
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
            using TypeBitsArray = Utils::TCDynamicArray<UObjectBits, 1>;
            using DrawcallArray = Utils::TCDynamicArray<RenderSystem::DrawcallPtr, 1>;
            using DrawcallDataArray = Utils::TCDynamicArray<RenderSystem::SDrawcallData, 1>;
            using BoolArray = Utils::TCDynamicArray<bool, 1>;
            using AABBArray = Utils::TCDynamicArray<Math::CAABB, 1>;
            using MatrixArray = Utils::TCDynamicArray<Math::CMatrix4x4, 1>;
            using UintArray = Utils::TCDynamicArray<uint32_t, 1>;
            using FrustumArray = Utils::TCDynamicArray<Math::CFrustum>;
            using CameraArray = Utils::TCDynamicArray<CCamera>;
            using FrustumCullTaskArray = Utils::TCDynamicArray<SSceneTasks::SFrustumCull, 8>;
            using DrawcallSortTaskArray = Utils::TCDynamicArray<SSceneTasks::SDrawcallSort, 8>;
            using BoundingSphereArray = Utils::TCDynamicArray<Math::CBoundingSphere, 1>;
            
            using LightsArray = SLights[LightTypes::_MAX_COUNT];
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
                struct DynamicTypes
                {
                    enum TYPE
                    {
                        AABB,
                        SPHERE,
                        FRUSTUM,
                        _MAX_COUNT
                    };
                };
                using DYNAMIC_TYPE = DynamicTypes::TYPE;
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
                        uint32_t bufferIndex : 4; // max 15 buffers
                        uint32_t index : 28;
                    };
                    uint32_t handle;
                };
                struct SConstantBuffer
                {
                    Utils::TCDynamicArray<uint8_t, 1> vData;
                    RenderSystem::BufferPtr pConstantBuffer;
                    RenderSystem::BufferPtr pStorageBuffer;
                    RenderSystem::DescriptorSetHandle hDescSet; // constant buffer
                    uint32_t drawCount = 0;
                };
                using BufferArray = Utils::TCDynamicArray<SConstantBuffer, 4>;
                struct SPerFrameShaderData
                {
                    Math::CMatrix4x4 mtxViewProj;
                };
                struct SInstancingShaderData
                {
                    Math::CMatrix4x4 mtxTransform;
                    Math::CVector4 vecColor;
                };
                struct SInstancing
                {
                    struct
                    {
                        RenderSystem::SDrawParams DrawParams;
                        RenderSystem::PipelinePtr pPipeline;
                    } DrawData;
                    RenderSystem::DDIRenderPass hDDIRenderPass = DDI_NULL_HANDLE;
                    BufferArray vConstantBuffers;
                    Utils::TCBitset<uint16_t> UpdateBufferMask;
                };
                struct SBatch
                {
                    struct SVertex
                    {
                        Math::CVector3 vecPosition;
                        // Math::CVector4  vecColor;
                    };
                    struct SBuffer
                    {
                        RenderSystem::BufferPtr pBuffer;
                        RenderSystem::SDrawParams DrawParams;
                    };
                    using BufferVec = Utils::TCDynamicArray<SBuffer>;
                    BufferVec vBuffers;
                    RenderSystem::PipelineRefPtr pPipeline;
                    uint32_t indexBufferOffset;
                    uint16_t objectVertexCount;
                    uint16_t objectIndexCount;
                };
                SBatch aBatches[ BatchTypes::_MAX_COUNT ];
                SInstancing aInstancings[ InstancingTypes::_MAX_COUNT ];
                RenderSystem::SPipelineCreateDesc InstancingPipelineTemplate;
                RenderSystem::SPipelineCreateDesc BatchPipelineTemplate;
                RenderSystem::DescriptorSetHandle hPerFrameDescSet = INVALID_HANDLE;
                RenderSystem::BufferPtr pPerFrameConstantBuffer;
                RenderSystem::VertexBufferHandle hInstancingVB;
                RenderSystem::IndexBufferHandle hInstancingIB;
                RenderSystem::CDeviceContext* pDeviceCtx;
                void Render( RenderSystem::CommandBufferPtr );
                uint32_t AddDynamic( RenderSystem::CommandBufferPtr, DYNAMIC_TYPE type );
                uint32_t AddInstancing( RenderSystem::CommandBufferPtr, INSTANCING_TYPE type );
                uint32_t AddBatchData( RenderSystem::CommandBufferPtr pCmdBuffer, BATCH_TYPE type );
                void UpdateBatchData( RenderSystem::CommandBufferPtr, BATCH_TYPE type, const uint32_t& handle, const Math::CAABB& AABB );
                void UpdateBatchData( RenderSystem::CommandBufferPtr, BATCH_TYPE type, const uint32_t& handle,
                                      const Math::CVector3* aCorners );
                void UpdateInstancing( RenderSystem::CommandBufferPtr, INSTANCING_TYPE type, const uint32_t handle,
                                       const SInstancingShaderData& );
                void UploadInstancingConstantData( RenderSystem::CommandBufferPtr pCmdBuffer, const CCamera* pCamera );
                void UploadBatchData( RenderSystem::CommandBufferPtr pCmdbuffer, const CCamera* pCamera );
                void CalcCorners( const Math::CAABB& AABB, SBatch::SVertex* pOut );
                Result CreateBatch( BATCH_TYPE type, RenderSystem::CommandBufferPtr );
                bool CreateConstantBuffer( RenderSystem::CDeviceContext* pCtx, uint32_t elementCount,
                                           SConstantBuffer* pOut );
                bool CreateDrawcallData( RenderSystem::CDeviceContext* pCtx, INSTANCING_TYPE type );
            };
            struct SDrawData
            {
                BoolArray vVisibles;
                BoundingSphereArray vBoundingSpheres;
                AABBArray vAABBs;
                MatrixArray vTransforms;
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
                void Update( const uint32_t idx, bool isVisible ) { vVisibles[ idx ] = isVisible; }
                void Update( const uint32_t idx, const Math::CBoundingSphere& Sphere )
                {
                    vBoundingSpheres[ idx ] = Sphere;
                }
                void Update( const uint32_t idx, const Math::CAABB& AABB ) { vAABBs[ idx ] = AABB; }
                void Update( const uint32_t idx, const Math::CMatrix4x4& Matrix ) { vTransforms[ idx ] = Matrix; }
                uint32_t GetCount() const { return vVisibles.GetCount(); }
                Math::CBoundingSphere& GetBoundingSphere( const uint32_t idx ) { return vBoundingSpheres[ idx ]; }
                Math::CAABB& GetAABB( const uint32_t idx ) { return vAABBs[ idx ]; }
                Math::CMatrix4x4& GetTransfrom( const uint32_t idx ) { return vTransforms[ idx ]; }
                // UObjectBits& GetBits( const uint32_t idx ) { return vVisibles[idx]; }
                bool& GetVisible( const uint32_t idx ) { return vVisibles[ idx ]; }
            };
            struct SLightData
            {
                BoundingSphereArray vBoundingSpheres;
                FrustumArray vFrustums;
            };
            struct SSceneNodeData
            {
                BoundingSphereArray vBoundingSpheres;
                AABBArray vAABBs;
            };
            using DrawDataArray = Utils::TCDynamicArray<SDrawData, 31>;
            using DrawcallArrayArray = Utils::TCDynamicArray<DrawcallArray, 31>;

            struct SConstantBuffer
            {
                CCamera::SData Camera;
                CLight::SData MainLight;
            };

          public:
            CScene( CWorld* pWorld )
                : m_pWorld{ pWorld }
            {
            }
            ~CScene() {}
            RenderSystem::DrawcallPtr CreateDrawcall( const Scene::SDrawcallDesc& Desc );
            TerrainPtr CreateTerrain( const STerrainDesc& Desc, RenderSystem::CommandBufferPtr );
            void DestroyTerrain( TerrainPtr* ppInOut );
            handle_t AddObject( RenderSystem::CommandBufferPtr, RenderSystem::DrawcallPtr pDrawcall, const SDrawcallDataInfo& Info );
            void AddObject( const CModel* );
            CameraPtr CreateCamera( const SCameraDesc& );
            void SetViewCamera( CameraPtr pCamera ) { m_pViewCamera = pCamera; }
            CameraPtr GetViewCamera() const { return m_pViewCamera; }
            void SetCamera( CameraPtr pCamera );
            CameraPtr GetCamera() const { return m_pCurrentCamera; }
            void Render( VKE::RenderSystem::CommandBufferPtr );
            void RenderDebug( RenderSystem::CommandBufferPtr );
            void UpdateDrawcallAABB( RenderSystem::CommandBufferPtr, const handle_t& hDrawcall, const Math::CAABB& NewAABB );
            void AddDebugView( RenderSystem::CommandBufferPtr, CameraPtr* pCamera );
            void AddDebugView( RenderSystem::CommandBufferPtr, LightPtr* );
            RenderSystem::CDeviceContext* GetDeviceContext() const { return m_pDeviceCtx; }
            void Update( const SUpdateSceneInfo& );

            LightRefPtr CreateLight( const SLightDesc& Desc );
            void DestroyLight( LightPtr );
            LightRefPtr GetLight( LIGHT_TYPE type, uint32_t idx ) const
            {
                return m_Lights[ type ].vpLights[ idx ];
            }

          protected:
            Result _Create( const SSceneDesc& );
            void _Destroy();
            void _FrustumCullDrawcalls( const Math::CFrustum& Frustum );
            void _SortDrawcalls( const Math::CFrustum& Frustum );
            void _Draw( VKE::RenderSystem::CommandBufferPtr );
            void vke_force_inline _SetObjectVisible( const UObjectHandle& hObj, const bool isVisible );
            handle_t _CreateSceneNode( const uint32_t idx );
            Result _CreateDebugView( RenderSystem::CommandBufferPtr );
            void _DestroyDebugView();
            SDebugView* _GetDebugView() { return m_pDebugView; }
            void _RenderDebugView( RenderSystem::CommandBufferPtr );
            void _UpdateDebugViews( RenderSystem::CommandBufferPtr );
            Result _CreateConstantBuffers();
            void _UpdateConstantBuffers(RenderSystem::CommandBufferPtr);

            void _SortLights(LIGHT_TYPE type);
            void _SortLights();

          protected:
            CWorld* m_pWorld = nullptr;
            COctree* m_pOctree = nullptr;
            CQuadTree* m_pQuadTree = nullptr;
            CBVH* m_pBVH = nullptr;
            RenderSystem::IFrameGraph* m_pFrameGraph = nullptr;
            RenderSystem::CDeviceContext* m_pDeviceCtx = nullptr;
            CameraArray m_vCameras;
            FrustumArray m_vFrustums;
            LightsArray m_Lights;
            LightsArray m_SortedLights;
            DrawcallArray m_vpDrawcalls;
            DrawcallArray m_vpAlwaysVisibleDrawcalls;
            DrawcallArrayArray m_vpVisibleLayerDrawcalls;
            DrawDataArray m_vDrawLayers;
            CameraPtr m_pViewCamera = nullptr;
            CameraPtr m_pCurrentCamera = nullptr;
            TerrainPtr m_pTerrain;
            FrustumCullTaskArray m_vFrustumCullTasks;
            DrawcallSortTaskArray m_vDrawcallSortTasks;
            Threads::SyncObject m_ObjectDataSyncObj;
            SDebugView* m_pDebugView = nullptr;
            RenderSystem::BufferRefPtr m_pConstantBufferCPU;
            RenderSystem::BufferRefPtr m_pConstantBufferGPU;
            RenderSystem::DescriptorSetHandle m_ahBindings[ Config::RenderSystem::SwapChain::MAX_ELEMENT_COUNT+1 ];
        };
        using ScenePtr = Utils::TCWeakPtr<CScene>;
        using SceneRefPtr = Utils::TCObjectSmartPtr<CScene>;
        void vke_force_inline CScene::_SetObjectVisible( const UObjectHandle& hObj, const bool isVisible )
        {
            // m_DrawData.vVisibles[hObj.index] = (isVisible);
            m_vDrawLayers[ hObj.layer ].vVisibles[ hObj.index ] = ( isVisible );
        }
    } // namespace Scene
} // namespace VKE
