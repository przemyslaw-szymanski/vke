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

        class VKE_API CTerrain
        {
            friend class CScene;
            public:

                CTerrain( CScene* pScene ) :
                    m_pScene( pScene )
                {}

                CScene* GetScene() const { return m_pScene; }
                
                void    Render( RenderSystem::CGraphicsContext* pCtx );

            protected:

                Result      _Create( const STerrainDesc& Desc, RenderSystem::CDeviceContext* );
                void        _Destroy();
                void        _DestroyRenderer( ITerrainRenderer** );

            protected:

                STerrainDesc        m_Desc;
                CScene*             m_pScene;
                ITerrainRenderer*   m_pRenderer = nullptr;
        };
    } // Scene
} // VKE