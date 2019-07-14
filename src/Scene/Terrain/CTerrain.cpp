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


            m_tileSize = (Desc.tileRowVertexCount) * Desc.vertexDistance;
            m_maxTileCount = (uint32_t)(Desc.size / m_tileSize);
            // Number of tiles must be power of two according to LODs
            // Each lod is 2x bigger
            m_maxTileCount = Math::CalcPrevPow2( m_maxTileCount );
            m_maxTileCount = Math::CalcNextPow2( m_maxTileCount );
            m_Desc.size = m_maxTileCount * m_tileSize;
            
            // LOD boundary (0-7)
            // Min LOD ==
            uint8_t maxLOD;
            for( maxLOD = 1; maxLOD < 7; ++maxLOD )
            {
                if( m_maxTileCount >> maxLOD == 1 )
                {
                    break;
                }
            }
            //maxLOD += 1; // add lod 0

            m_Desc.lodCount = std::min( maxLOD, m_Desc.lodCount );

            m_maxTileCount *= m_maxTileCount;
            m_maxVisibleTiles = Math::Min( m_maxTileCount, CalcMaxVisibleTiles( m_Desc ) );
            m_vecExtents = Math::CVector3( m_Desc.size * 0.5f );
            m_vecExtents.y = (m_Desc.Height.min + m_Desc.Height.max) * 0.5f;
            
            m_avecCorners[0] = Math::CVector3( m_Desc.vecCenter.x - m_vecExtents.x, m_Desc.vecCenter.y, m_Desc.vecCenter.z + m_vecExtents.z );
            m_avecCorners[1] = Math::CVector3( m_Desc.vecCenter.x + m_vecExtents.x, m_Desc.vecCenter.y, m_Desc.vecCenter.z + m_vecExtents.z );
            m_avecCorners[2] = Math::CVector3( m_Desc.vecCenter.x - m_vecExtents.x, m_Desc.vecCenter.y, m_Desc.vecCenter.z - m_vecExtents.z );
            m_avecCorners[3] = Math::CVector3( m_Desc.vecCenter.x + m_vecExtents.x, m_Desc.vecCenter.y, m_Desc.vecCenter.z - m_vecExtents.z );

            if( VKE_SUCCEEDED( m_QuadTree._Create( m_Desc ) ) )
            {
                if( VKE_SUCCEEDED( m_pRenderer->_Create( m_Desc, pCtx ) ) )
                {
                    ret = VKE_OK;
                }
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

    namespace Scene
    {
        Result CTerrainQuadTree::_Create( const STerrainDesc& Desc )
        {
            Result res = VKE_FAIL;

            m_Desc = Desc;
            
            // Calc num of nodes
            uint32_t nodeCount = (uint32_t)std::pow( 2, Desc.lodCount );
            nodeCount *= nodeCount;

            if( m_vNodes.Reserve( nodeCount ) )
            {
                // Create root
                UNodeHandle Handle;
                Handle.index = 0;
                Handle.childIdx = 0;
                Handle.level = 0;
                m_Root.hParent = Handle;
                m_Root.Handle = Handle;
                const float halfSize = m_Desc.size * 0.5f;
                m_Root.AABB = Math::CAABB( m_Desc.vecCenter, Math::CVector3( halfSize ) );

                res = _CreateNodes( &m_Root, 0 );
            };

            return res;
        }

        Result CTerrainQuadTree::_CreateNodes( SNode* pParent, uint8_t level )
        {
            static const Math::CVector4 aVectors[4] =
            {
                { -1.0f, 1.0f, 0.0f, 0.0f },{ 1.0f, 1.0f, 0.0f, 0.0f },
                { -1.0f, -1.0f, 0.0f, 0.0f },{ 1.0f, -1.0f, 0.0f, 0.0f },
            };

            Result res = VKE_OK;
            const auto currLevel = level;
            if( currLevel < m_Desc.lodCount )
            {
                const auto& ParentAABB = pParent->AABB;
                const auto vecParentCenter = Math::CVector4( ParentAABB.Center );
                const auto vecParentExtents = Math::CVector4( ParentAABB.Extents );
                const auto vecChildExtents = ParentAABB.Extents * 0.5f;
                const auto vecChildExtents4 = vecParentExtents * 0.5f;
                Math::CVector4 vecChildCenter;

                // Create child nodes
                for( uint8_t i = 0; i < 4; ++i )
                {
                    UNodeHandle Handle;
                    Handle.index = m_vNodes.PushBack( {} );
                    Handle.level = currLevel;
                    Handle.childIdx = i;
                    auto& Node = m_vNodes[ Handle.index ];
                    Node.Handle = Handle;
                    Node.hParent = pParent->Handle;
                    
                    Math::CVector4::Mad( aVectors[ i ], vecChildExtents4, vecParentCenter, &vecChildCenter );
                    Node.AABB = Math::CAABB( Math::CVector3{ vecChildCenter }, vecChildExtents );
                    pParent->ahChildren[ Handle.childIdx ] = Handle;
                }

                for( uint8_t i = 0; i < 4; ++i )
                {
                    const auto& hNode = pParent->ahChildren[ i ];
                    auto& Node = m_vNodes[ hNode.index ];
                    res = _CreateNodes( &Node, currLevel + 1 );
                }
            }
            return res;
        }

        CTerrainQuadTree::CHILD_NODE_INDEX CTerrainQuadTree::_CalcNodeIndex(
            const Math::CVector4& vecParentCenter, const Math::CVector4& vecPoint )
        {
            CHILD_NODE_INDEX ret;

            Math::CVector4 vecTmp;
            Math::CVector4::LessOrEquals( vecPoint, vecParentCenter, &vecTmp );
            bool aResults[ 4 ];
            vecTmp.ConvertCompareToBools( aResults );

            static const CHILD_NODE_INDEX aIndices[2][2] =
            {
                { ChildNodeIndices::RIGHT_TOP, ChildNodeIndices::RIGHT_BOTTOM },
                { ChildNodeIndices::LEFT_TOP, ChildNodeIndices::LEFT_BOTTOM }
            };
            // Need only x and z
            ret = aIndices[ aResults[ 0 ] ][ aResults[ 2 ] ];
            return ret;
        }

    } // Scene
} // VKE