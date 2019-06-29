#include "Scene/Terrain/CTerrain.h"
#include "Scene/Terrain/CTerrainVertexFetchRenderer.h"
#include "Scene/Terrain/CTerrainMeshShaderRenderer.h"

namespace VKE
{
    namespace Scene
    {
        void CTerrain::_Destroy()
        {
            _DestroyRenderer( &m_pRenderer );
        }

        Result CTerrain::_Create( const STerrainDesc& Desc, RenderSystem::CDeviceContext* pCtx )
        {
            Result ret = VKE_FAIL;
            m_Desc = Desc;

            STerrainVertexFetchRendererDesc DefaultDesc;
            if( m_Desc.Renderer.pName == nullptr )
            {
                m_Desc.Renderer.pName = TERRAIN_VERTEX_FETCH_RENDERER_NAME;
                m_Desc.Renderer.pDesc = &DefaultDesc;
            }
            if( strcmp( m_Desc.Renderer.pName, TERRAIN_VERTEX_FETCH_RENDERER_NAME ) == 0 )
            {
                m_pRenderer = VKE_NEW CTerrainVertexFetchRenderer( this );
            }
            else if( strcmp( m_Desc.Renderer.pName, TERRAIN_MESH_SHADING_RENDERER_NAME ) == 0 )
            {

            }
            if( m_pRenderer == nullptr )
            {
                goto ERR;
            }
            if( VKE_SUCCEEDED( m_pRenderer->_Create( Desc, pCtx ) ) )
            {
                ret = VKE_OK;
            }
            return ret;

        ERR:
            _DestroyRenderer( &m_pRenderer );
            return ret;
        }

        void CTerrain::_DestroyRenderer( ITerrainRenderer** ppInOut )
        {
            ITerrainRenderer* pRenderer = *ppInOut;
            pRenderer->_Destroy();
            VKE_DELETE( pRenderer );
        }

        void CTerrain::Render( RenderSystem::CGraphicsContext* pCtx )
        {
            m_pRenderer->Render( pCtx );
        }

    } // Scene
} // VKE