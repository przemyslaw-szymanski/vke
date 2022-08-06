#pragma once
#include "Core/Math/Math.h"
#include "Core/Resources/CImage.h"
#include "Scene/Common.h"
namespace VKE
{
    namespace RenderSystem
    {
        class CDeviceContext;
    }
    namespace Scene
    {
        class ITerrainRenderer;
        class CTerrainVertexFetchRenderer;
        class CCamera;
        class CTerrain;
        struct STerrainRootNodeDesc
        {
            Math::CVector3 vecPosition;
        };
        class VKE_API CTerrainQuadTree
        {
            friend class CTerrain;
            friend class ITerrainRenderer;

          public:
            using RootNodeCount = ExtentU16;
            struct ChildNodeIndices
            {
                enum INDEX : uint8_t
                {
                    LEFT_TOP,
                    RIGHT_TOP,
                    LEFT_BOTTOM,
                    RIGHT_BOTTOM,
                    _MAX_COUNT
                };
            };
            using CHILD_NODE_INDEX = ChildNodeIndices::INDEX;
            union UNodeHandle
            {
                static const uint32_t MAX_NODE_INDEX = 0x3FFFFFF; // 26 bit max
                struct
                {
                    uint32_t level : 4;          // up to 15 lods
                    uint32_t childIdx : 2;       // index in parent's node
                    uint32_t index : 32 - 4 - 2; // index in array, 67108863 nodes max
                };
                uint32_t handle = UNDEFINED_U32;
            };
            struct SChildNodeHandles
            {
                UNodeHandle aHandles[ 4 ];
                SChildNodeHandles& operator=( const SChildNodeHandles& Other )
                {
                    for( uint32_t i = 0; i < 4; ++i )
                    {
                        aHandles[ i ] = Other.aHandles[ i ];
                    }
                    return *this;
                }
                UNodeHandle& operator[]( const uint32_t idx )
                {
                    return aHandles[ idx ];
                }
            };
            struct SStitch
            {
                uint8_t lod : 4;
                uint8_t corner : 4;
            };
            static const uint8_t MAX_LOD_COUNT = 13; // 4 pow 13 == 67108864, fits to 26 bit index
            static const uint8_t LAST_LOD = MAX_LOD_COUNT - 1u;
            static const uint32_t MAIN_ROOT_COUNT = 4;
            struct SDrawData
            {
                RenderSystem::PipelinePtr pPipeline;
                Math::CVector3 vecPosition; // left top corner
                uint8_t topVertexDiff = 0;  // num of vertices different to lower lod
                uint8_t bottomVertexDiff = 0;
                uint8_t leftVertexDiff = 0;
                uint8_t rightVertexDiff = 0;
                float tileSize = 0;
                ExtentU16 TextureOffset; // texel position offset, unnormalized
                                         // [0, tex size]
                uint32_t bindingIndex;
#if VKE_RENDERER_DEBUG
                uint32_t rootIdx;
#endif
            };
            struct SNode
            {
                UNodeHandle ahChildren[ 4 ];
                UNodeHandle hParent;
                UNodeHandle Handle;
                Math::CAABB AABB = { Math::CVector3::ZERO, Math::CVector3::ZERO };
                Math::CVector3 vec3Position;
                float boundingSphereRadius;
                uint32_t childLevelIndex = UNDEFINED_U32;
                SDrawData DrawData;
            };
            using NodeArray = Utils::TCDynamicArray<SNode, 1>;
            using MainRootNodeArray = Utils::TCDynamicArray<SNode, 4>;
            using Uint32Array = Utils::TCDynamicArray<uint32_t, 1>;
            using AABBArray = Utils::TCDynamicArray<Math::CAABB, 1>;
            using SphereArray = Utils::TCDynamicArray<Math::CBoundingSphere, 1>;
            using HandleNode = Utils::TCDynamicArray<SChildNodeHandles, 1>;
            using BoolArray = Utils::TCDynamicArray<bool, 1>;
            using HandleArray = Utils::TCDynamicArray<UNodeHandle, 1>;
            using Vec4Array = Utils::TCDynamicArray<Math::CVector4, 1>;
            // Stores data for 4 neighbouring nodes
            struct SNodeLevel
            {
                enum
                {
                    CENTER_X = 0,
                    CENTER_Y = 2,
                    CENTER_Z = 1,
                    EXTENTS_X = 0,
                    EXTENTS_Y = 1,
                    EXTENTS_Z = 0,
                };
                Math::CVector4 aAABBCenters[ 3 ]; // Stores xxxx, zzzz, yyyy for 4 nodes
                Math::CVector4 aAABBExtents[ 2 ]; // Stores xxxx/zzzz, yyyy for 4 nodes
                //uint32_t aChildLevelIndices[ 4 ]; // node index in node buffer (m_vNodes)
                UNodeHandle aChildLevelNodeHandles[ 4 ]; // node level handle
                Math::CVector4 vecVisibility;     // stores visible info for each node
                uint32_t rootNodeIndex = UNDEFINED_U32;
                float boundingSphereRadius; // common for all nodes at the same
                                            // level
                //ExtentU8 CoordsInRootNode; // x,y index in root node grid
                uint8_t level;
                // uint32_t        drawDataIndex; // index to draw data for
                // these 4 nodes
                // TMP
                SDrawData DrawData; // tmp, use separate buffer
            };
            using NodeLevelArray = Utils::TCDynamicArray<SNodeLevel, 1>;
            struct SInitChildNodeLevel
            {
                Math::CVector3 vecParentSizes; // x,z,width/depth
                float parentBoundingSphereRadius;
                uint32_t rootNodeIndex = UNDEFINED_U32;
                uint32_t childLevelIndex;
                uint32_t bindingIndex;
                uint8_t parentLevel;
                uint8_t maxLODCount;
                ExtentU8 CoordsInRootNode; // x,y index in root node grid
            };
            struct SLODData
            {
                SDrawData DrawData;
                uint32_t lod : 4;
                uint32_t idx : 28; // chunk index in the array
            };
            struct STerrainInfo
            {
                // uint8_t     rootLOD; // LOD for root (not always 0 or 1)
                uint8_t maxLODCount;
                RootNodeCount RootCount;         // number of top level nodes
                uint32_t tileCountForRoot;       // tile count for a single root with
                                                 // all LODs
                uint32_t childLevelCountForRoot; // calc child node level count
                                                 // for root for all LODs
                uint32_t maxVisibleRootCount;    // max visible top level (0) tiles
                uint32_t maxNodeCount;           // max node count shared by all roots
            };
            struct SCalcTerrainInfo
            {
                const STerrainDesc* pDesc;
                uint32_t maxRootSize;
                uint8_t maxLODCount;
            };
            struct SAABBHeightsSIMD
            {
                Math::CVector4 vec4Mins; // min for 4 nodes in a node level
                Math::CVector4 vec4Maxs; // max for 4 nodes in a node level
            };
            using LODDataArray = Utils::TCDynamicArray<SLODData, 1>;
            using LODDataArrays = Utils::TCDynamicArray<LODDataArray, 8>;
            using TextureIndexArray = Utils::TCDynamicArray<uint8_t>;
            using LODMap = Utils::TCDynamicArray<uint8_t, 1>;
            using NodeHeightArray = Utils::TCDynamicArray<ExtentF32, 1>;
            using RootNodeHeightArray = Utils::TCDynamicArray<NodeHeightArray, 1>;
            struct SViewData
            {
                float fovRadians;
                float halfFOV;
                float screenWidth;
                float screenHeight;
                float frustumWidth;
                Math::CFrustum Frustum;
                Math::CVector3 vecPosition;
            };
            struct SCreateNodeData
            {
                Math::CVector4 vec4ParentCenter;
                Math::CVector4 vec4Extents;
                Math::CVector3 vecExtents;
                UNodeHandle hParent;
                float boundingSphereRadius;
                uint8_t level;
            };
            struct SInitChildNodesInfo
            {
                UNodeHandle hParent;
                uint8_t maxLODCount;
                uint32_t childNodeStartIndex; // start index in node buffer,
                                              // startIndex + 3 for next 4 children
                Math::CVector4 vecRootCenter;
                Math::CVector4 vec4ParentCenter;
                Math::CVector4 vec4Extents;
                Math::CVector3 vecExtents;
                float boundingSphereRadius;
            };
            struct SLODInfo
            {
                Math::CVector4 vec4Center;
                Math::CVector3 vec3RootPosition;
                float nodeExtents;
                uint32_t bindingIndex;
                uint8_t nodeLevel;
#if VKE_SCENE_DEBUG
                uint32_t rootIndex = UNDEFINED_U32;
#endif
            };
            struct SUpdateRootNodeData
            {
                uint32_t heightmapTextureIndex;
            };

          public:
            const LODDataArray& GetLODData() const
            {
                return m_vLODData;
            }
            const LODDataArray& GetSortedLODData() const
            {
                return m_vSortedLODData;
            }
            // Calculates min and max lod count
            static ExtentU8 CalcLODCount( const STerrainDesc& Desc, uint16_t maxHeightmapSize, uint8_t maxLODCount );

          protected:
            Result _Create( const STerrainDesc& Desc );
            void _Destroy();
            void _Update();
            void _UpdateWorldSize();
            void _CalcLODs( const SViewData& View );
            void _CalcLODsSIMD( const SNode& Root, const SViewData& View, float lodTreshold );
            void _CalcLODsSIMD( const SNodeLevel& NodeLevel, const SNode& Root, const SViewData& View, float lodTreshold );
            void _CalcLODsSIMD( uint32_t levelIndex, const SNode& Root, const SViewData& View );
            void _CalcErrorLODs( const SNode& Node, const uint32_t& textureIdx, const SViewData& View );
            void _CalcDistanceLODs( const SNode& Node, const uint32_t& textureIdx, const SViewData& View );
            void _CalcError( const Math::CVector4& vecPoint, const uint8_t nodeLevel, const SViewData& View,
                             float* pErrOut, float* pDistanceOut ) const;
            UNodeHandle _FindNode( const SNode& Node, const Math::CVector4& vecPosition ) const;
            float _CalcDistanceToCenter( const Math::CVector3& vecPoint, const SViewData& View );
            static void _CalcTerrainInfo( const SCalcTerrainInfo& Info, STerrainInfo* pOut );
            Result _CreateChildNodes( UNodeHandle hParent, const SCreateNodeData& Data, const uint8_t lodCount );
            Result _CreateChildNodes( const SCreateNodeData& Data );
            void _InitChildNodes( const SInitChildNodesInfo& Info );
            void _InitChildNodesSIMD( const SInitChildNodeLevel& Info );
            void _GetAABBHeightSIMD( const SNodeLevel&, Math::CVector4* pvecMinHeightOut, Math::CVector4* pvecMaxHeightOut );
            void _InitChildNodesSIMD( const SNodeLevel& ParentLevel );
            uint32_t _AcquireChildNodes();
            void _ResetChildNodes(); // reset all child nodes, except root ones
            void _ResetChildNodeLevels()
            {
                m_childNodeLevelIndex = 0;
            } // reset counter for used child node levels
            void _FreeChildNodes( UNodeHandle hParent );
            CHILD_NODE_INDEX
            _CalcNodeIndex( const Math::CVector4& vecParentCenter, const Math::CVector4& vecPoint ) const;
            void _SetDrawDataForNode( SNode* );
            void _SetLODData( const SLODInfo& Info, SLODData* pOut ) const;
            void _NotifyLOD( const UNodeHandle& hParent, const UNodeHandle& hNode, const ExtentF32& TopLeftCorner );
            void _AddLOD( const SLODInfo& Info );
            void _SetLODMap( const SLODData& Data );
            void _SetStitches();
            // Main roots are the roots containing full LOD nodes
            void _InitMainRoots( const SViewData& View );
            void _InitRootNodesSIMD( const SViewData& View );
            void _FrustumCull( const SViewData& View );
            void _FrustumCullRoots( const SViewData& View );
            void _BoundingSphereFrustumCull( const SViewData& View );
            void _BoundingSphereFrustumCullNode( const UNodeHandle& hNode, const Math::CFrustum& Frustum );
            void _FrustumCullChildNodes( const SViewData& View );
            void _FrustumCullChildNodes( const SViewData& View, const SNodeLevel& Level, Math::CVector4* pOut );
            void _UpdateRootNode( const uint32_t& rootNodeIndex, const SUpdateRootNodeData& Data );
            void _CalcNodeAABB( uint32_t index, const ImagePtr pImg );
            ExtentU32 _FindMinMax( const ImagePtr pImg, const Rect2D& Rect );
            ExtentF32 CalcChildNodeMinMaxHeight( const Math::CAABB& worldAABB, const ImagePtr pImg,
                                                 CTerrainQuadTree::SNode* pNodeInOut );
            /*float           _CalcScreenSpaceError( const Math::CVector4&
               vecPoint, const float& worldSpaceError, const SViewData& View )
               const;*/
            uint32_t _AcquireChildNodeLevel()
            {
                VKE_ASSERT( m_childNodeLevelIndex < m_vChildNodeLevels.GetCount(), "" );
                VKE_ASSERT( m_childNodeLevelIndex + 1 < UNodeHandle::MAX_NODE_INDEX, "" );
                return m_childNodeLevelIndex++;
            }
            void _SortLODData( const SViewData&, LODDataArray* );

          protected:
            STerrainDesc m_Desc;
            CTerrain* m_pTerrain;
            Math::CVector3 m_avecWorldCorners[ 4 ];
            RootNodeCount m_RootNodeCount; // each 'root' node contains original
                                           // heightmap texture
            SphereArray m_vBoundingSpheres;
            AABBArray m_vAABBs;
            HandleNode m_vChildNodeHandles; // parents and children nodes
            BoolArray m_vNodeVisibility;    // array of bools for each node to
                                            // indicate if a node is visible
            NodeArray m_vNodes;
            RootNodeHeightArray m_vRootNodeHeights; // min/max AABB height for bottom level nodes (lod0 nodes)
            NodeArray m_vVisibleRootNodes;
            HandleArray m_vVisibleRootNodeHandles;
            SInitChildNodesInfo m_FirstLevelNodeBaseInfo;
            Uint32Array m_vFreeNodeIndices;
            NodeLevelArray m_vChildNodeLevels;
            uint32_t m_childNodeLevelIndex = 0;
            NodeLevelArray m_vRootNodeLevels;
            LODMap m_vLODMap; // 1d represent of 2d array of all terrain tiles
                              // at highest lod
            LODDataArrays m_vvLODData;
            LODDataArray m_vLODData;
            LODDataArray m_vSortedLODData;
            TextureIndexArray m_vTextureIndices;
            uint32_t m_terrainHalfSize;
            //Math::CVector4 m_vec4LODThreshold;
            uint16_t m_tileSize;
            uint16_t m_tileInRowCount; // number terrain tiles in one row
            uint16_t m_totalRootCount;
            uint16_t m_tileInRootRowCount; // number of root tiles with lod0
            uint8_t m_maxLODCount;
            bool m_needUpdateLOD = true;
        };
        class VKE_API CTerrain
        {
            friend class CScene;
            friend class CTerrainQuadTree;
            friend class ITerrainRenderer;
            friend class CTerrainVertexFetchRenderer;
            struct TextureTypes
            {
                enum TYPE
                {
                    DIFFUSE,
                    DIFFUSE_NORMAL,
                    _MAX_COUNT
                };
            };

          public:
            static const uint8_t MAX_TEXTURE_COUNT = 2; // max texture count per root node
            using TextureArray = Utils::TCDynamicArray<RenderSystem::TextureHandle, MAX_TEXTURE_COUNT>;
            using TextureViewArray = Utils::TCDynamicArray<RenderSystem::TextureViewHandle, MAX_TEXTURE_COUNT>;
            using TextureArrayArray = Utils::TCDynamicArray<TextureArray, 1>;
            using TextureViewArrayArray = Utils::TCDynamicArray<TextureViewArray, 1>;

          public:
            CTerrain( CScene* pScene )
                : m_pScene( pScene )
            {
            }
            CScene* GetScene() const
            {
                return m_pScene;
            }
            const STerrainDesc& GetDesc() const
            {
                return m_Desc;
            }
            bool CheckDesc( const STerrainDesc& Desc ) const;
            void Update( RenderSystem::CommandBufferPtr );
            void Render( RenderSystem::CommandBufferPtr );
            handle_t CreateRoot( const STerrainRootNodeDesc& );
            void DestroyRoot( const handle_t& );
            static ExtentU16 CalcTextureCount( const STerrainDesc& Desc );

            void SetLODTreshold( float value );

          protected:
            Result _Create( const STerrainDesc& Desc, RenderSystem::CommandBufferPtr );
            void _Destroy();
            void _DestroyRenderer( ITerrainRenderer** );
            RenderSystem::PipelinePtr _GetPipelineForLOD( uint8_t );
            Result _LoadTextures( RenderSystem::CDeviceContext* pCtx );
            Result _LoadTextures( RenderSystem::CDeviceContext* pCtx, const STerrainUpdateBindingData& );
            Result _SplitTexture( RenderSystem::CDeviceContext* pCtx );
            Result _CreateDummyResources( RenderSystem::CommandBufferPtr );
            void _GetBindingDataForRootNode( const uint32_t& rootNodeIdx, STerrainUpdateBindingData* pOut );

          protected:
            STerrainDesc m_Desc;
            uint32_t m_maxTileCount;
            uint32_t m_maxVisibleTiles;
            uint16_t m_tileVertexCount; // num of vertices (in one row) of
                                        // highest lod tile
            uint32_t m_halfSize;        // terrain half size
            Math::CVector3 m_vecExtents;
            Math::CVector3 m_avecCorners[ 4 ];
            CTerrainQuadTree m_QuadTree;
            CTerrainQuadTree::STerrainInfo m_TerrainInfo;
            TextureArray m_vDummyTextures;
            TextureViewArray m_vDummyTexViews;
            TextureArray m_vHeightmapTextures;
            TextureArray m_vHeightmapNormalTextures;
            TextureViewArray m_vHeightmapTexViews;
            TextureViewArray m_vHeightmapNormalTexViews;
            TextureArrayArray m_avvTextures[ TextureTypes::_MAX_COUNT ];
            TextureViewArrayArray m_avvTextureViews[ TextureTypes::_MAX_COUNT ];
            // RenderSystem::TextureHandle
            // m_ahHeightmapTextures[MAX_HEIGHTMAP_TEXTURE_COUNT];
            // RenderSystem::TextureViewHandle m_ahHeightmapTextureViews[
            // MAX_HEIGHTMAP_TEXTURE_COUNT ]; RenderSystem::TextureViewHandle
            // m_hHeigtmapTexView = INVALID_HANDLE;
            RenderSystem::SamplerHandle m_hHeightmapSampler = INVALID_HANDLE;
            CScene* m_pScene;
            ITerrainRenderer* m_pRenderer = nullptr;
            uint32_t m_loadedTextureCount = 0;
        };
    } // namespace Scene
} // namespace VKE