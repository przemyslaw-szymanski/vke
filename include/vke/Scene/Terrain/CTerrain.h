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
                    uint32_t    handle;
                };

                static const uint8_t MAX_LOD_COUNT  = 13; // 4 pow 13 == 67108864, fits to 26 bit index
                static const uint8_t LAST_LOD       = MAX_LOD_COUNT - 1u;

                struct SNode
                {
                    UNodeHandle     ahChildren[4];
                    UNodeHandle     hParent;
                    UNodeHandle     Handle;
                    Math::CAABB     AABB;
                    float           boundingSphereRadius;

                    RenderSystem::TextureHandle   hTexture;
                };

                using NodeArray = Utils::TCDynamicArray< SNode, 1 >;

                struct SLODData
                {
                    uint32_t        textureIdx;
                    ExtentU32       TextureOffset;
                    Math::CVector3  vecPosition; // left down corner
                    uint8_t         lod;
                };

                using LODDataArray = Utils::TCDynamicArray< SLODData, 1 >;
                using LODDataArrays = Utils::TCDynamicArray< LODDataArray, 8 >;

                using TextureIndexArray = Utils::TCDynamicArray< uint32_t >;

                struct SViewData
                {
                    float           fovRadians;
                    float           halfFOV;
                    float           screenWidth;
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

            protected:

                Result          _Create(const STerrainDesc& Desc);
                void            _Destroy();
                void            _Update();
                void            _CalcLODs(const SViewData& View);
                void            _CalcLODs( const SNode& Node, const uint32_t& textureIdx, const SViewData& View );
                void            _CalcError( const Math::CVector4& vecPoint, const uint8_t nodeLevel,
                    const SViewData& View, float* pErrOut, float* pDistanceOut ) const;
                UNodeHandle     _FindNode( const SNode& Node, const Math::CVector4& vecPosition ) const;

                Result              _CreateNodes( SNode* pParent, const SCreateNodeData& Data );
                CHILD_NODE_INDEX    _CalcNodeIndex( const Math::CVector4& vecParentCenter,
                    const Math::CVector4& vecPoint ) const;

                void            _NotifyLOD(const UNodeHandle& hParent, const UNodeHandle& hNode,
                    const ExtentF32& TopLeftCorner);

                /*float           _CalcScreenSpaceError( const Math::CVector4& vecPoint, const float& worldSpaceError,
                    const SViewData& View ) const;*/

            protected:

                STerrainDesc        m_Desc;
                CTerrain*           m_pTerrain;
                ExtentU16           m_RootNodeCount; // each 'root' node contains original heightmap texture
                NodeArray           m_vNodes;
                LODDataArrays       m_vvLODData;
                LODDataArray        m_vLODData;
                TextureIndexArray   m_vTextureIndices;
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

                void    Update(RenderSystem::CGraphicsContext* pCtx);
                void    Render(RenderSystem::CGraphicsContext* pCtx);

            protected:

                Result      _Create(const STerrainDesc& Desc, RenderSystem::CDeviceContext*);
                void        _Destroy();
                void        _DestroyRenderer(ITerrainRenderer**);

            protected:

                STerrainDesc        m_Desc;
                uint32_t            m_maxTileCount;
                uint32_t            m_maxVisibleTiles;
                float               m_tileSize;
                Math::CVector3      m_vecExtents;
                Math::CVector3      m_avecCorners[4];
                CTerrainQuadTree    m_QuadTree;

                CScene*             m_pScene;
                ITerrainRenderer*   m_pRenderer = nullptr;
        };
    } // Scene
} // VKE