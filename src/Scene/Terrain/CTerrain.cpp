#include "Scene/Terrain/CTerrain.h"
#include "Scene/Terrain/CTerrainVertexFetchRenderer.h"
#include "Scene/Terrain/CTerrainMeshShaderRenderer.h"

#include "Core/Math/Math.h"

#include "Scene/CScene.h"

namespace VKE
{
    namespace Scene
    {
        void CTerrain::_Destroy()
        {
            _DestroyRenderer( &m_pRenderer );
        }

        uint32_t CalcMaxVisibleTiles( const STerrainDesc& Desc )
        {
            uint32_t ret = 0;
            // maxViewDistance is a radius so 2x maxViewDistance == a diameter
            // Using diameter horizontally and vertically we get a square
            // All possible visible tiles should cover this square
            float dimm = Desc.maxViewDistance * 2;
            float tileDimm = Desc.tileRowVertexCount * Desc.vertexDistance;
            float tileCount = dimm / tileDimm;
            ret = (uint32_t)ceilf( tileCount * tileCount );
            return ret;
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


            m_tileSize = (Desc.tileRowVertexCount-1) * Desc.vertexDistance;
            m_maxTileCount = (uint32_t)(Desc.size / m_tileSize);
            m_Desc.size = m_maxTileCount * m_tileSize;
            
            m_maxTileCount *= m_maxTileCount;
            m_maxVisibleTiles = Math::Min( m_maxTileCount, CalcMaxVisibleTiles( m_Desc ) );
            m_vecExtents = Math::CVector3( m_Desc.size * 0.5f );
            m_vecExtents.y = (m_Desc.Height.min + m_Desc.Height.max) * 0.5f;
            
            m_avecCorners[0] = Math::CVector3( m_Desc.vecCenter.x - m_vecExtents.x, m_Desc.vecCenter.y, m_Desc.vecCenter.z + m_vecExtents.z );
            m_avecCorners[1] = Math::CVector3( m_Desc.vecCenter.x + m_vecExtents.x, m_Desc.vecCenter.y, m_Desc.vecCenter.z + m_vecExtents.z );
            m_avecCorners[2] = Math::CVector3( m_Desc.vecCenter.x - m_vecExtents.x, m_Desc.vecCenter.y, m_Desc.vecCenter.z - m_vecExtents.z );
            m_avecCorners[3] = Math::CVector3( m_Desc.vecCenter.x + m_vecExtents.x, m_Desc.vecCenter.y, m_Desc.vecCenter.z - m_vecExtents.z );

            if( VKE_SUCCEEDED( m_pRenderer->_Create( m_Desc, pCtx ) ) )
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

        void CTerrain::Update( RenderSystem::CGraphicsContext* pCtx )
        {
            m_pRenderer->Update( pCtx, m_pScene->GetRenderCamera() );
        }

        void CTerrain::Render( RenderSystem::CGraphicsContext* pCtx )
        {
            m_pRenderer->Render( pCtx, m_pScene->GetRenderCamera() );
        }

    } // Scene
} // VKE