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
            _DestroyRenderer(&m_pRenderer);
        }

        uint32_t CalcMaxVisibleTiles(const STerrainDesc& Desc)
        {
            uint32_t ret = 0;
            // maxViewDistance is a radius so 2x maxViewDistance == a diameter
            // Using diameter horizontally and vertically we get a square
            // All possible visible tiles should cover this square
            float dimm = Desc.maxViewDistance * 2;
            float tileDimm = Desc.tileRowVertexCount * Desc.vertexDistance;
            float tileCount = dimm / tileDimm;
            ret = (uint32_t)ceilf(tileCount * tileCount);
            return ret;
        }

        Result CTerrain::_Create(const STerrainDesc& Desc, RenderSystem::CDeviceContext* pCtx)
        {
            Result ret = VKE_FAIL;
            m_Desc = Desc;

            STerrainVertexFetchRendererDesc DefaultDesc;
            if (m_Desc.Renderer.pName == nullptr)
            {
                m_Desc.Renderer.pName = TERRAIN_VERTEX_FETCH_RENDERER_NAME;
                m_Desc.Renderer.pDesc = &DefaultDesc;
            }
            if (strcmp(m_Desc.Renderer.pName, TERRAIN_VERTEX_FETCH_RENDERER_NAME) == 0)
            {
                m_pRenderer = VKE_NEW CTerrainVertexFetchRenderer(this);
            }
            else if (strcmp(m_Desc.Renderer.pName, TERRAIN_MESH_SHADING_RENDERER_NAME) == 0)
            {

            }
            if (m_pRenderer == nullptr)
            {
                goto ERR;
            }


            m_tileSize = (Desc.tileRowVertexCount) * Desc.vertexDistance;
            m_maxTileCount = (uint32_t)(Desc.size / m_tileSize);
            // Number of tiles must be power of two according to LODs
            // Each lod is 2x bigger
            m_maxTileCount = Math::CalcNextPow2(m_maxTileCount);
            m_Desc.size = m_maxTileCount * m_tileSize;

            // LOD boundary (0-7)
            // Min LOD ==
            uint8_t maxLOD;
            for (maxLOD = 0; maxLOD < 15; ++maxLOD)
            {
                if (m_maxTileCount >> maxLOD == 1)
                {
                    break;
                }
            }
            //maxLOD += 1; // add lod 0

            m_Desc.lodCount = std::min<uint8_t>( maxLOD, CTerrainQuadTree::MAX_LOD_COUNT );

            m_maxTileCount *= m_maxTileCount;
            m_maxVisibleTiles = Math::Min(m_maxTileCount, CalcMaxVisibleTiles(m_Desc));
            m_vecExtents = Math::CVector3(m_Desc.size * 0.5f);
            m_vecExtents.y = (m_Desc.Height.min + m_Desc.Height.max) * 0.5f;

            m_avecCorners[0] = Math::CVector3(m_Desc.vecCenter.x - m_vecExtents.x, m_Desc.vecCenter.y, m_Desc.vecCenter.z + m_vecExtents.z);
            m_avecCorners[1] = Math::CVector3(m_Desc.vecCenter.x + m_vecExtents.x, m_Desc.vecCenter.y, m_Desc.vecCenter.z + m_vecExtents.z);
            m_avecCorners[2] = Math::CVector3(m_Desc.vecCenter.x - m_vecExtents.x, m_Desc.vecCenter.y, m_Desc.vecCenter.z - m_vecExtents.z);
            m_avecCorners[3] = Math::CVector3(m_Desc.vecCenter.x + m_vecExtents.x, m_Desc.vecCenter.y, m_Desc.vecCenter.z - m_vecExtents.z);

            m_QuadTree.m_pTerrain = this;
            if (VKE_SUCCEEDED(m_QuadTree._Create(m_Desc)))
            {
                if (VKE_SUCCEEDED(m_pRenderer->_Create(m_Desc, pCtx)))
                {
                    ret = VKE_OK;
                }
            }
            return ret;

        ERR:
            _DestroyRenderer(&m_pRenderer);
            return ret;
        }

        void CTerrain::_DestroyRenderer(ITerrainRenderer** ppInOut)
        {
            ITerrainRenderer* pRenderer = *ppInOut;
            pRenderer->_Destroy();
            VKE_DELETE(pRenderer);
        }

        void CTerrain::Update(RenderSystem::CGraphicsContext* pCtx)
        {
            m_QuadTree._Update();
            m_pRenderer->Update(pCtx, m_pScene->GetRenderCamera());
        }

        void CTerrain::Render(RenderSystem::CGraphicsContext* pCtx)
        {
            m_pRenderer->Render(pCtx, m_pScene->GetRenderCamera());
        }

    } // Scene

    namespace Scene
    {
        Result CTerrainQuadTree::_Create(const STerrainDesc& Desc)
        {
            Result res = VKE_FAIL;

            m_Desc = Desc;

            // Calc num of nodes
            uint32_t nodeCount = (uint32_t)std::pow(4, Desc.lodCount);

            if( m_vNodes.Reserve( nodeCount ) && m_vLODData.Reserve( nodeCount ) )
            {
                // Create root
                UNodeHandle Handle;
                Handle.index = 0;
                Handle.childIdx = 0;
                Handle.level = 0;
                m_Root.hParent = Handle;
                m_Root.Handle = Handle;

                m_Root.AABB = Math::CAABB(m_Desc.vecCenter, m_pTerrain->m_vecExtents);
                m_Root.boundingSphereRadius = ( std::sqrtf( 2.0f ) * 0.5f ) * m_Desc.size;

                SCreateNodeData NodeData;
                NodeData.boundingSphereRadius = m_Root.boundingSphereRadius * 0.5f;
                NodeData.level = 0;
                NodeData.vecExtents = m_Root.AABB.Extents * 0.5f;
                NodeData.vec4Extents = NodeData.vecExtents;
                NodeData.vec4ParentCenter = m_Root.AABB.Center;
                NodeData.hParent.handle = 0;

                res = _CreateNodes( &m_Root, NodeData );
            };

            return res;
        }

        void CTerrainQuadTree::_Destroy()
        {
            m_vNodes.Destroy();
        }

        Result CTerrainQuadTree::_CreateNodes(SNode* pParent, const SCreateNodeData& NodeData)
        {
            static const Math::CVector4 aVectors[4] =
            {
                {-1.0f, 0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 1.0f, 0.0f},
                {-1.0f, 0.0f, -1.0f, 0.0f}, {1.0f, 0.0f, -1.0f, 0.0f}
            };

            Result res = VKE_OK;
            const auto currLevel = NodeData.level;
            if (currLevel < m_Desc.lodCount)
            {
                SCreateNodeData ChildNodeData;

                ChildNodeData.vec4Extents = NodeData.vec4Extents * 0.5f;
                ChildNodeData.vecExtents = Math::CVector3(ChildNodeData.vec4Extents);
                ChildNodeData.boundingSphereRadius = NodeData.boundingSphereRadius * 0.5f;
                ChildNodeData.level = currLevel + 1;

                Math::CVector4 vecChildCenter;

                // Create child nodes
                for (uint8_t i = 0; i < 4; ++i)
                {
                    UNodeHandle Handle;
                    Handle.index = m_vNodes.PushBack({});
                    Handle.level = currLevel;
                    Handle.childIdx = i;
                    auto& Node = m_vNodes[Handle.index];
                    Node.Handle = Handle;
                    Node.hParent = NodeData.hParent;

                    Node.ahChildren[ 0 ].handle = UNDEFINED_U32;
                    Node.ahChildren[ 1 ].handle = UNDEFINED_U32;
                    Node.ahChildren[ 2 ].handle = UNDEFINED_U32;
                    Node.ahChildren[ 3 ].handle = UNDEFINED_U32;

                    Math::CVector4::Mad( aVectors[i], NodeData.vec4Extents, NodeData.vec4ParentCenter, &vecChildCenter );
                    Node.AABB = Math::CAABB( Math::CVector3{ vecChildCenter }, NodeData.vecExtents );
                    Node.boundingSphereRadius = NodeData.boundingSphereRadius;
                    pParent->ahChildren[Handle.childIdx] = Handle;
                }

                for (uint8_t i = 0; i < 4; ++i)
                {
                    const auto& hNode = pParent->ahChildren[i];
                    auto& Node = m_vNodes[hNode.index];
                    ChildNodeData.vec4ParentCenter = Node.AABB.Center;
                    ChildNodeData.hParent = Node.Handle;

                    res = _CreateNodes( &Node, ChildNodeData );
                }
            }
            return res;
        }

        CTerrainQuadTree::CHILD_NODE_INDEX CTerrainQuadTree::_CalcNodeIndex( const Math::CVector4& vecParentCenter,
            const Math::CVector4& vecPoint) const
        {
            CHILD_NODE_INDEX ret;

            Math::CVector4 vecTmp;
            Math::CVector4::LessOrEquals(vecPoint, vecParentCenter, &vecTmp);
            bool aResults[4];
            vecTmp.ConvertCompareToBools(aResults);

            static const CHILD_NODE_INDEX aIndices[2][2] =
            {
                {ChildNodeIndices::RIGHT_TOP, ChildNodeIndices::RIGHT_BOTTOM},
                {ChildNodeIndices::LEFT_TOP, ChildNodeIndices::LEFT_BOTTOM}
            };
            // Need only x and z
            ret = aIndices[aResults[0]][aResults[2]];
            return ret;
        }

        void CTerrainQuadTree::_Update()
        {
            auto pCamera = m_pTerrain->GetScene()->GetCamera();
            SViewData View;
            View.fovRadians = pCamera->GetFOVAngleRadians();
            View.screenWidth = pCamera->GetViewport().width;
            View.Frustum = pCamera->GetFrustum();
            View.vecPosition = pCamera->GetPosition();
            View.halfFOV = View.fovRadians * 0.5f;
            //View.halfFOV = (Math::PI / 3.0f) * 0.5f;

            m_vLODData.Clear();

            for (uint32_t i = 0; i < 4; ++i)
            {
                auto hNode = m_Root.ahChildren[i];
                _CalcLODs(m_vNodes[hNode.index], View );
            }
        }

        void CalcNearestSpherePoint( const Math::CVector4& vecSphereCenter, const float sphereRadius,
            const Math::CVector4& vecPoint, Math::CVector4* pOut )
        {
            Math::CVector4 vecDir = vecPoint - vecSphereCenter;
            vecDir.Normalize();
            Math::CVector4::Mad( vecDir, Math::CVector4( sphereRadius ), vecSphereCenter, pOut );
        }

        void CTerrainQuadTree::_CalcLODs( const SNode& CurrNode, const SViewData& View )
        {
            const auto hCurrNode = CurrNode.Handle;
            const auto& AABB = CurrNode.AABB;
            Math::CVector4 vecPoint;
            CalcNearestSpherePoint( Math::CVector4( AABB.Center ), CurrNode.boundingSphereRadius,
                Math::CVector4( View.vecPosition ), &vecPoint );
            float err, distance;
            _CalcError( vecPoint, hCurrNode.level, View, &err, &distance );

            float childErr, childDistance;

            static cstr_t indents[] =
            {
                "",
                " ",
                "  ",
                "   ",
                "    ",
                "     ",
                "      ",
                "       ",
                "        ",
                "         ",
                "          ",
                "           ",
                "            ",
                "             ",
            };

            VKE_DBG_LOG( "" << indents[ hCurrNode.level ] << "l: " << hCurrNode.level << " idx: " << hCurrNode.index <<
                         " d: " << distance << " e: " << err <<
                         " c: " << AABB.Center.x << ", " << AABB.Center.z <<
                         " p: " << vecPoint.x << ", " << vecPoint.z << "\n" );

            if( err > 5.0f )
            {
                // Parent has always 0 or 4 children
                if( CurrNode.ahChildren[ 0 ].handle != UNDEFINED_U32 )
                {
                    for( uint32_t i = 0; i < 4; ++i )
                    {
                        const auto hNode = CurrNode.ahChildren[ i ];
                        {
                            const auto& Node = m_vNodes[ hNode.index ];
                            CalcNearestSpherePoint( Math::CVector4( Node.AABB.Center ), Node.boundingSphereRadius,
                                                    Math::CVector4( View.vecPosition ), &vecPoint );
                            _CalcError( vecPoint, hNode.level, View, &childErr, &childDistance );

                            VKE_DBG_LOG( "" << indents[ hNode.level ] << "l: " << hNode.level << " idx: " << hNode.index <<
                                         " d: " << childDistance << " e: " << childErr <<
                                         " c: " << Node.AABB.Center.x << ", " << Node.AABB.Center.z <<
                                         " p: " << vecPoint.x << ", " << vecPoint.z << "\n" );
                        }
                    }
                    for( uint32_t i = 0; i < 4; ++i )
                    {
                        const auto hNode = CurrNode.ahChildren[ i ];
                        const auto& ChildNode = m_vNodes[ hNode.index ];
                        if( ChildNode.ahChildren[ 0 ].handle != UNDEFINED_U32 )
                        {
                            _CalcLODs( ChildNode, View );
                        }
                    }
                }
            }
            else
            {

            }
        }

        float CalcWorldSpaceError( const float vertexDistance, const uint8_t nodeLevel, const uint8_t levelCount )
        {
            const uint32_t levelDistance = ( 1u << ( levelCount - nodeLevel - 1 ) );
            const float ret = vertexDistance * levelDistance;
            return ret;
        }

        void CalcScreenSpaceError( const Math::CVector4& vecPoint, const float& worldSpaceError,
            const CTerrainQuadTree::SViewData& View, float* pErrOut, float* pDistanceOut )
        {
            const float distance = Math::CVector4::Distance(Math::CVector4(View.vecPosition), vecPoint);

            //const float w = distance * 2.0f * std::tanf( View.halfFOV );
            //const float p = ( worldSpaceError * View.screenWidth ) / w;

            /*const auto a = ( View.screenWidth * 0.5f ) / std::tanf( View.fovRadians * 0.5f );
            const auto p = a * (e / distance);
            return p;*/

            /*const auto a = View.screenWidth / View.fovRadians;
            const auto p = a * ( e / distance );
            return p;*/

            // Urlich formula
            const float d = worldSpaceError / distance;
            const float k = View.screenWidth / (2.0f * std::tanf(View.halfFOV));
            const float p = d * k;

            *pErrOut = p;
            *pDistanceOut = distance;
        }

        void CTerrainQuadTree::_CalcError( const Math::CVector4& vecPoint, const uint8_t nodeLevel,
            const SViewData& View, float* pErrOut, float* pDistanceOut ) const
        {
            const float worldSpaceError = CalcWorldSpaceError( m_Desc.vertexDistance, nodeLevel, m_Desc.lodCount );
            CalcScreenSpaceError( vecPoint, worldSpaceError, View, pErrOut, pDistanceOut );
        }

        CTerrainQuadTree::UNodeHandle CTerrainQuadTree::_FindNode( const SNode& Node, const Math::CVector4& vecPosition ) const
        {
            UNodeHandle Ret = Node.Handle;
            if( Ret.level < MAX_LOD_COUNT )
            {
                const Math::CVector4 vecCenter( Node.AABB.Center );
                const auto idx = _CalcNodeIndex( vecCenter, vecPosition );
                const auto& hNode = Node.ahChildren[idx];
                const auto& ChildNode = m_vNodes[hNode.index];
                Ret = _FindNode(ChildNode, vecPosition);
            }

            return Ret;
        }

        

    } // Scene
} // VKE