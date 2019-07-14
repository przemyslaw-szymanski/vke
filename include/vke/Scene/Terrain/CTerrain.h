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
                        uint32_t    level : 3; // up to 7 lods
                        uint32_t    childIdx : 2; // index in parent's node
                        uint32_t    index : 32 - 3 - 2; // index in array
                    };
                    uint32_t    handle;
                };

                struct SNode
                {
                    UNodeHandle     ahChildren[ 4 ];
                    UNodeHandle     hParent;
                    UNodeHandle     Handle;
                    Math::CAABB     AABB;

                    RenderSystem::TextureHandle   hTexture;
                };

                using NodeArray = Utils::TCDynamicArray< SNode, 1 >;

            public:

            protected:

                Result          _Create( const STerrainDesc& Desc );
                void            _Destroy();

                Result              _CreateNodes( SNode* pParent, uint8_t level );
                CHILD_NODE_INDEX    _CalcNodeIndex( const Math::CVector4& vecParentCenter,
                                                    const Math::CVector4& vecPoint );

            protected:

                STerrainDesc    m_Desc;
                CTerrain*       m_pTerrain;
                SNode           m_Root;
                NodeArray       m_vNodes;
        };

        class VKE_API CTerrain
        {
            friend class CScene;
            friend class ITerrainRenderer;
            friend class CTerrainVertexFetchRenderer;

            public:

                CTerrain( CScene* pScene ) :
                    m_pScene( pScene )
                {}

                CScene* GetScene() const { return m_pScene; }
                const STerrainDesc& GetDesc() const { return m_Desc; }
                
                void    Update( RenderSystem::CGraphicsContext* pCtx );
                void    Render( RenderSystem::CGraphicsContext* pCtx );

            protected:

                Result      _Create( const STerrainDesc& Desc, RenderSystem::CDeviceContext* );
                void        _Destroy();
                void        _DestroyRenderer( ITerrainRenderer** );

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