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

                struct SStitch
                {
                    uint8_t lod : 4;
                    uint8_t corner : 4;
                };

                static const uint8_t MAX_LOD_COUNT  = 13; // 4 pow 13 == 67108864, fits to 26 bit index
                static const uint8_t LAST_LOD       = MAX_LOD_COUNT - 1u;

                struct SDrawData
                {
                    RenderSystem::PipelinePtr   pPipeline;
                    Math::CVector3              vecPosition; // left top corner
                    uint8_t                     topVertexDiff = 0; // num of vertices different to lower lod
                    uint8_t                     bottomVertexDiff = 0;
                    uint8_t                     leftVertexDiff = 0;
                    uint8_t                     rightVertexDiff = 0;
                    float                       tileSize = 0;
                    ExtentU32                   TextureOffset; // coords offset
                    uint32_t                    textureIdx; // heightmap texture id
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

                struct SLODData
                {
                    SDrawData       DrawData;
                    uint32_t        lod : 4;
                    uint32_t        idx : 28; // chunk index in the array
                };

                using LODDataArray = Utils::TCDynamicArray< SLODData, 1 >;
                using LODDataArrays = Utils::TCDynamicArray< LODDataArray, 8 >;

                using TextureIndexArray = Utils::TCDynamicArray< uint32_t >;

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

            public:

                const LODDataArray& GetLODData() const { return m_vLODData; }

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

                Result              _CreateNodes( UNodeHandle hParent, const SCreateNodeData& Data );
                Result              _CreateNodes(const SCreateNodeData& Data);

                CHILD_NODE_INDEX    _CalcNodeIndex( const Math::CVector4& vecParentCenter,
                    const Math::CVector4& vecPoint ) const;
                void            _SetDrawDataForNode( SNode* );

                void            _NotifyLOD(const UNodeHandle& hParent, const UNodeHandle& hNode,
                    const ExtentF32& TopLeftCorner);

                void            _AddLOD( const SLODData& Data );
                void            _SetLODMap( const SLODData& Data );
                void            _SetStitches();

                /*float           _CalcScreenSpaceError( const Math::CVector4& vecPoint, const float& worldSpaceError,
                    const SViewData& View ) const;*/

            protected:

                STerrainDesc        m_Desc;
                CTerrain*           m_pTerrain;
                ExtentU16           m_RootNodeCount; // each 'root' node contains original heightmap texture
                NodeArray           m_vNodes;
                LODMap              m_vLODMap; // 1d represent of 2d array of all terrain tiles at highest lod
                LODDataArrays       m_vvLODData;
                LODDataArray        m_vLODData;
                TextureIndexArray   m_vTextureIndices;
                uint16_t            m_terrainHalfSize;
                uint16_t            m_tileSize;
                uint16_t            m_tileInRowCount; // number terrain tiles in one row
        };

        class VKE_API CTerrain
        {
            friend class CScene;
            friend class CTerrainQuadTree;
            friend class ITerrainRenderer;
            friend class CTerrainVertexFetchRenderer;

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
                uint16_t            m_maxTileCount;
                uint16_t            m_maxVisibleTiles;
                uint16_t            m_tileVertexCount; // num of vertices (in one row) of highest lod tile
                uint16_t            m_halfSize; // terrain half size
                Math::CVector3      m_vecExtents;
                Math::CVector3      m_avecCorners[4];
                CTerrainQuadTree    m_QuadTree;
                RenderSystem::TextureHandle     m_hHeightmapTexture = INVALID_HANDLE;
                RenderSystem::TextureViewHandle m_hHeigtmapTexView = INVALID_HANDLE;
                RenderSystem::SamplerHandle     m_hHeightmapSampler = INVALID_HANDLE;

                CScene*             m_pScene;
                ITerrainRenderer*   m_pRenderer = nullptr;
        };
    } // Scene
} // VKE