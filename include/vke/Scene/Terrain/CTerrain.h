#pragma once

#include "Scene/Common.h"
#include "Core/Math/Math.h"

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

        class VKE_API CTerrainQuadTree
        {
            friend class CTerrain;
            friend class ITerrainRenderer;

            public:

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
                    struct
                    {
                        uint32_t    level       : 4; // up to 15 lods
                        uint32_t    childIdx    : 2; // index in parent's node
                        uint32_t    index       : 32 - 4 - 2; // index in array, 67108863 nodes max
                    };
                    uint32_t    handle = UNDEFINED_U32;
                };

                struct SChildNodeHandles
                {
                    UNodeHandle aHandles[4];
                    SChildNodeHandles& operator=(const SChildNodeHandles& Other)
                    {
                        for (uint32_t i = 0; i < 4; ++i)
                        {
                            aHandles[i] = Other.aHandles[i];
                        }
                        return *this;
                    }

                    UNodeHandle& operator[](const uint32_t idx) { return aHandles[idx]; }
                };

                struct SStitch
                {
                    uint8_t lod : 4;
                    uint8_t corner : 4;
                };

                static const uint8_t MAX_LOD_COUNT  = 13; // 4 pow 13 == 67108864, fits to 26 bit index
                static const uint8_t LAST_LOD       = MAX_LOD_COUNT - 1u;
                static const uint32_t MAX_ROOT_SIZE = 1024 * 2;

                struct SDrawData
                {
                    RenderSystem::PipelinePtr   pPipeline;
                    Math::CVector3              vecPosition; // left top corner
                    uint8_t                     topVertexDiff = 0; // num of vertices different to lower lod
                    uint8_t                     bottomVertexDiff = 0;
                    uint8_t                     leftVertexDiff = 0;
                    uint8_t                     rightVertexDiff = 0;
                    float                       tileSize = 0;
                    ExtentU16                   TextureOffset; // texel position offset, unnormalized [0, tex size]
                    uint8_t                     textureIdx; // heightmap texture id
                };

                struct SNode
                {
                    UNodeHandle     ahChildren[4];
                    UNodeHandle     hParent;
                    UNodeHandle     Handle;
                    Math::CAABB     AABB;
                    float           boundingSphereRadius;

                    SDrawData       DrawData;
                };

                using NodeArray = Utils::TCDynamicArray< SNode, 1 >;
                using Uint32Array = Utils::TCDynamicArray< uint32_t, 1 >;
                using AABBArray = Utils::TCDynamicArray< Math::CAABB, 1 >;
                using SphereArray = Utils::TCDynamicArray< Math::CBoundingSphere, 1 >;
                using HandleNode = Utils::TCDynamicArray< SChildNodeHandles, 1 >;
                using BoolArray = Utils::TCDynamicArray< bool, 1 >;
                using HandleArray = Utils::TCDynamicArray< UNodeHandle, 1 >;

                struct SLODData
                {
                    SDrawData       DrawData;
                    uint32_t        lod : 4;
                    uint32_t        idx : 28; // chunk index in the array
                };

                struct STerrainInfo
                {
                    //uint8_t     rootLOD; // LOD for root (not always 0 or 1)
                    uint8_t     maxLODCount;
                    ExtentU16   RootCount; // number of top level nodes
                    uint32_t    tileCountForRoot; // tile count for a single root with all LODs
                    uint32_t    maxVisibleRootCount; // max visible top level (0) tiles
                    uint32_t    maxNodeCount; // max node count shared by all roots
                };

                struct SCalcTerrainInfo
                {
                    const STerrainDesc* pDesc;
                    uint32_t            maxRootSize;
                    uint8_t             maxLODCount;
                };

                using LODDataArray = Utils::TCDynamicArray< SLODData, 1 >;
                using LODDataArrays = Utils::TCDynamicArray< LODDataArray, 8 >;

                using TextureIndexArray = Utils::TCDynamicArray< uint8_t >;

                using LODMap = Utils::TCDynamicArray< uint8_t, 1 >;

                struct SViewData
                {
                    float           fovRadians;
                    float           halfFOV;
                    float           screenWidth;
                    float           screenHeight;
                    Math::CFrustum  Frustum;
                    Math::CVector3  vecPosition;
                };

                struct SCreateNodeData
                {
                    Math::CVector4  vec4ParentCenter;
                    Math::CVector4  vec4Extents;
                    Math::CVector3  vecExtents;
                    UNodeHandle     hParent;
                    float           boundingSphereRadius;
                    uint8_t         level;
                };

                struct SInitChildNodesInfo
                {
                    UNodeHandle     hParent;
                    uint8_t         maxLODCount;
                    uint32_t        childNodeStartIndex; // start index in node buffer, startIndex + 3 for next 4 children
                    Math::CVector4  vec4ParentCenter;
                    Math::CVector4  vec4Extents;
                    Math::CVector3  vecExtents;
                    float           boundingSphereRadius;
                };

            public:

                const LODDataArray& GetLODData() const { return m_vLODData; }
                // Calculates min and max lod count
                static ExtentU8    CalcLODCount(const STerrainDesc& Desc, uint16_t maxHeightmapSize,
                    uint8_t maxLODCount);

            protected:

                Result          _Create(const STerrainDesc& Desc);
                void            _Destroy();
                void            _Update();
                void            _CalcLODs(const SViewData& View);
                void            _CalcErrorLODs( const SNode& Node, const uint32_t& textureIdx, const SViewData& View );
                void            _CalcDistanceLODs( const SNode& Node, const uint32_t& textureIdx, const SViewData& View );
                void            _CalcError( const Math::CVector4& vecPoint, const uint8_t nodeLevel,
                    const SViewData& View, float* pErrOut, float* pDistanceOut ) const;
                UNodeHandle     _FindNode( const SNode& Node, const Math::CVector4& vecPosition ) const;
                float           _CalcDistanceToCenter( const Math::CVector3& vecPoint, const SViewData& View );

                static void     _CalcTerrainInfo(const SCalcTerrainInfo& Info, STerrainInfo* pOut);

                Result              _CreateChildNodes( UNodeHandle hParent, const SCreateNodeData& Data, const uint8_t lodCount );
                Result              _CreateChildNodes(const SCreateNodeData& Data);
                void                _InitChildNodes(const SInitChildNodesInfo& Info);
                uint32_t            _AcquireChildNodes();
                void                _ResetChildNodes(); // reset all child nodes, except root ones
                void                _FreeChildNodes(UNodeHandle hParent);

                CHILD_NODE_INDEX    _CalcNodeIndex( const Math::CVector4& vecParentCenter,
                    const Math::CVector4& vecPoint ) const;
                void            _SetDrawDataForNode( SNode* );

                void            _NotifyLOD(const UNodeHandle& hParent, const UNodeHandle& hNode,
                    const ExtentF32& TopLeftCorner);

                void            _AddLOD( const SLODData& Data );
                void            _SetLODMap( const SLODData& Data );
                void            _SetStitches();

                void            _BoundingSphereFrustumCull(const SViewData& View);
                void            _BoundingSphereFrustumCullNode(const UNodeHandle& hNode, const Math::CFrustum& Frustum);

                /*float           _CalcScreenSpaceError( const Math::CVector4& vecPoint, const float& worldSpaceError,
                    const SViewData& View ) const;*/

            protected:

                STerrainDesc        m_Desc;
                CTerrain*           m_pTerrain;
                ExtentU16           m_RootNodeCount; // each 'root' node contains original heightmap texture
                SphereArray         m_vBoundingSpheres;
                AABBArray           m_vAABBs;
                HandleNode          m_vChildNodeHandles; // parents and children nodes
                BoolArray           m_vNodeVisibility; // array of bools for each node to indicate if a node is visible
                NodeArray           m_vNodes;
                Uint32Array         m_vFreeNodeIndices;
                LODMap              m_vLODMap; // 1d represent of 2d array of all terrain tiles at highest lod
                LODDataArrays       m_vvLODData;
                LODDataArray        m_vLODData;
                TextureIndexArray   m_vTextureIndices;
                uint32_t            m_terrainHalfSize;
                uint16_t            m_tileSize;
                uint16_t            m_tileInRowCount; // number terrain tiles in one row
                uint16_t            m_totalRootCount;
                uint8_t             m_maxLODCount;
        };

        class VKE_API CTerrain
        {
            friend class CScene;
            friend class CTerrainQuadTree;
            friend class ITerrainRenderer;
            friend class CTerrainVertexFetchRenderer;

            public:

                static const uint8_t MAX_HEIGHTMAP_TEXTURE_COUNT = 10;
                static const uint8_t MAX_TEXTURE_COUNT = 255;

            public:

                CTerrain(CScene* pScene) :
                    m_pScene(pScene)
                {}

                CScene* GetScene() const
                {
                    return m_pScene;
                }

                const STerrainDesc& GetDesc() const
                {
                    return m_Desc;
                }

                bool    CheckDesc(const STerrainDesc& Desc) const;

                void    Update(RenderSystem::CGraphicsContext* pCtx);
                void    Render(RenderSystem::CGraphicsContext* pCtx);

            protected:

                Result      _Create(const STerrainDesc& Desc, RenderSystem::CDeviceContext*);
                void        _Destroy();
                void        _DestroyRenderer(ITerrainRenderer**);
                RenderSystem::PipelinePtr   _GetPipelineForLOD( uint8_t );

                Result      _LoadTextures(RenderSystem::CDeviceContext* pCtx);

            protected:

                STerrainDesc        m_Desc;
                uint32_t            m_maxTileCount;
                uint32_t            m_maxVisibleTiles;
                uint16_t            m_tileVertexCount; // num of vertices (in one row) of highest lod tile
                uint32_t            m_halfSize; // terrain half size
                Math::CVector3      m_vecExtents;
                Math::CVector3      m_avecCorners[4];
                CTerrainQuadTree    m_QuadTree;
                CTerrainQuadTree::STerrainInfo  m_TerrainInfo;
                RenderSystem::TextureHandle     m_hHeightmapTexture = INVALID_HANDLE;
                RenderSystem::TextureViewHandle m_ahHeightmapTextureViews[ MAX_HEIGHTMAP_TEXTURE_COUNT ];
                RenderSystem::TextureViewHandle m_hHeigtmapTexView = INVALID_HANDLE;
                RenderSystem::SamplerHandle     m_hHeightmapSampler = INVALID_HANDLE;

                CScene*             m_pScene;
                ITerrainRenderer*   m_pRenderer = nullptr;
        };
    } // Scene
} // VKE