#pragma once

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

                CScene*             m_pScene;
                ITerrainRenderer*   m_pRenderer = nullptr;
        };
    } // Scene
} // VKE