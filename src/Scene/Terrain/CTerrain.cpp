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
            m_QuadTree._Destroy();
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


            m_tileSize = (uint32_t)((float)(Desc.tileRowVertexCount) * Desc.vertexDistance);
            m_maxTileCount = (uint32_t)(Desc.size / m_tileSize);
            // Number of tiles must be power of two according to LODs
            // Each lod is 2x bigger
            m_maxTileCount = Math::CalcNextPow2(m_maxTileCount);
            m_Desc.size = m_maxTileCount * m_tileSize;
            m_halfSize = m_Desc.size / 2;

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
            
            if( VKE_SUCCEEDED( m_pRenderer->_Create( m_Desc, pCtx ) ) )
            {
                if( VKE_SUCCEEDED( m_QuadTree._Create( m_Desc ) ) )
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

        RenderSystem::PipelinePtr CTerrain::_GetPipelineForLOD( uint8_t lod )
        {
            return m_pRenderer->_GetPipelineForLOD( lod );
        }

    } // Scene

    namespace Scene
    {
        ExtentU32 vke_force_inline CalcXY( const uint32_t idx, const uint32_t width )
        {
            ExtentU32 Ret;
            Ret.y = idx / width;
            Ret.x = idx % width;
            return Ret;
        }

        uint32_t vke_force_inline CalcTileCountForLod( uint8_t lod )
        {
            return Math::CalcPow2( lod );
        }

        Result CTerrainQuadTree::_Create(const STerrainDesc& Desc)
        {
            Result res = VKE_FAIL;

            m_Desc = Desc;
            // Copy these to avoid cache missess
            m_terrainHalfSize = m_pTerrain->m_halfSize;
            m_tileSize = m_pTerrain->m_tileSize;
            m_tileInRowCount = ( m_terrainHalfSize / m_tileSize ) * 2;

            m_vLODMap.Resize( m_tileInRowCount * m_tileInRowCount, Desc.lodCount-1 );

            // This quadtree is made of rootNodeCount quadTrees.
            // Each 'sub' quadtree root contains original heightmap texture
            m_RootNodeCount = { 2, 2 }; // tmp
            const auto vecMinSize = m_Desc.vecCenter - m_pTerrain->m_vecExtents;
            const auto vecMaxSize = m_Desc.vecCenter + m_pTerrain->m_vecExtents;
            m_vTextureIndices.Resize(m_RootNodeCount.x * m_RootNodeCount.y);

            // Calc num of nodes
            uint32_t nodeCount = (uint32_t)std::pow(4, Desc.lodCount);

            if( m_vNodes.Reserve( nodeCount ) && m_vLODData.Reserve( nodeCount ) )
            {
                const auto vecRootNodeExtents = m_pTerrain->m_vecExtents / Math::CVector3( m_RootNodeCount.x, 1.0f, m_RootNodeCount.y );
                auto AABB = Math::CAABB(m_Desc.vecCenter, m_pTerrain->m_vecExtents);
                const float boundingSphereRadius = ( (std::sqrtf( 2.0f ) * 0.5f ) * vecRootNodeExtents.x );
                Math::CVector3 vecRootNodeCenter;

                for( uint16_t z = 0; z < m_RootNodeCount.y; ++z )
                {
                    for( uint16_t x = 0; x < m_RootNodeCount.x; ++x )
                    {
                        // Create root
                        UNodeHandle Handle;
                        Handle.index = m_vNodes.PushBack( {} );
                        Handle.childIdx = 0;
                        Handle.level = 0;
                        auto& Node = m_vNodes[ Handle.index ];
                        Node.hParent.handle = UNDEFINED_U32;
                        Node.Handle = Handle;
                        Node.boundingSphereRadius = boundingSphereRadius;
                        const float minX = vecMinSize.x + m_pTerrain->m_vecExtents.x * x;
                        const float minZ = vecMinSize.z + m_pTerrain->m_vecExtents.z * z;
                        vecRootNodeCenter.x = minX + vecRootNodeExtents.x;
                        vecRootNodeCenter.y = 0;
                        vecRootNodeCenter.z = minZ + vecRootNodeExtents.z;

                        Node.AABB = Math::CAABB( vecRootNodeCenter, vecRootNodeExtents );
                    }
                }
                const uint32_t rootCount = m_RootNodeCount.x * m_RootNodeCount.y;
                for( uint32_t i = 0; i < rootCount; ++i )
                {
                    auto& Root = m_vNodes[ i ];
                    SCreateNodeData NodeData;
                    NodeData.boundingSphereRadius = Root.boundingSphereRadius * 0.5f;
                    NodeData.level = 1;
                    NodeData.vecExtents = Root.AABB.Extents * 0.5f;
                    NodeData.vec4Extents = NodeData.vecExtents;
                    NodeData.vec4ParentCenter = Root.AABB.Center;
                    NodeData.hParent = Root.Handle;
                    res = _CreateNodes( Root.Handle, NodeData );
                }

                m_vvLODData.Resize(8);
            }

            return res;
        }

        void CTerrainQuadTree::_Destroy()
        {
            m_vNodes.Destroy();
        }

        void CTerrainQuadTree::_SetDrawDataForNode( CTerrainQuadTree::SNode* pInOut )
        {
            auto pPipeline = this->m_pTerrain->_GetPipelineForLOD( pInOut->Handle.level );
            VKE_ASSERT( pPipeline.IsValid(), "Pipeline must not be null at this stage" );
            pInOut->DrawData.pPipeline = pPipeline;
            pInOut->DrawData.vecPosition.x = pInOut->AABB.Center.x - pInOut->AABB.Extents.x;
            pInOut->DrawData.vecPosition.y = pInOut->AABB.Center.y;
            pInOut->DrawData.vecPosition.z = pInOut->AABB.Center.z + pInOut->AABB.Extents.z;
        }

        Result CTerrainQuadTree::_CreateNodes(UNodeHandle hParent, const SCreateNodeData& NodeData)
        {
            static const Math::CVector4 aVectors[4] =
            {
                {-1.0f, 0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 1.0f, 0.0f},
                {-1.0f, 0.0f, -1.0f, 0.0f}, {1.0f, 0.0f, -1.0f, 0.0f}
            };

            //auto& Parent = m_vNodes[ hParent.index ];
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
                UNodeHandle ahChildNodes[4];

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
                    //Parent.ahChildren[Handle.childIdx] = Handle;
                    m_vNodes[hParent.index].ahChildren[i] = Handle;
                    ahChildNodes[i] = Handle;

                    _SetDrawDataForNode( &Node );
                }

                for (uint8_t i = 0; i < 4; ++i)
                {
                    const auto& hNode = ahChildNodes[i];
                    auto& Node = m_vNodes[hNode.index];
                    ChildNodeData.vec4ParentCenter = Node.AABB.Center;
                    ChildNodeData.hParent = Node.Handle;

                    res = _CreateNodes( hNode, ChildNodeData );
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
            
            static Math::CVector3 vecLastPos = Math::CVector3::ZERO;
            if( vecLastPos != View.vecPosition )
            {
                vecLastPos = View.vecPosition;
            }

            m_vLODData.Clear();
            for (uint32_t i = 0; i < m_vvLODData.GetCount(); ++i)
            {
                m_vvLODData[i].Clear();
            }

            const uint32_t nodeCount = m_RootNodeCount.x * m_RootNodeCount.y;
            for (uint32_t i = 0; i < nodeCount; ++i)
            {
                auto& Node = m_vNodes[i];
                _CalcDistanceLODs( Node, m_vTextureIndices[i], View );
            }
            for (uint32_t i = 0; i < m_vvLODData.GetCount(); ++i)
            {
                m_vLODData.Append(m_vvLODData[i]);
            }

            /*for( uint32_t y = 0; y < m_tileInRowCount; ++y )
            {
                for( uint32_t x = 0; x < m_tileInRowCount; ++x )
                {
                    uint32_t idx = Math::Map2DArrayIndexTo1DArrayIndex( x, y, m_tileInRowCount );
                    //VKE_LOG( (uint32_t)m_vLODMap[idx] << " " );
                    VKE_LOGGER << ( uint32_t )m_vLODMap[ idx ] << " ";
                }
                VKE_LOGGER << "\n";
            }
            VKE_LOGGER.Flush();*/
        }

        void CalcNearestSpherePoint( const Math::CVector4& vecSphereCenter, const float sphereRadius,
            const Math::CVector4& vecPoint, Math::CVector4* pOut )
        {
            Math::CVector4 vecDir = vecPoint - vecSphereCenter;
            vecDir.Normalize();
            Math::CVector4::Mad( vecDir, Math::CVector4( sphereRadius ), vecSphereCenter, pOut );
        }

        void CTerrainQuadTree::_CalcLODs(const SViewData& View)
        {

        }

        struct SCalcTextureOffsetInfo
        {
            Math::CVector3  vecRootMinCorner;
            Math::CVector3  vecRootMaxCorner;
            Math::CVector3  vecRootSize;
            Math::CVector3  vecTilePosition;
            uint32_t        tileVertexCount;
            float           tileVertexDistance;
            uint32_t        rootTextureSize;
            uint8_t         nodeLevel;
        };

        ExtentF32 CalcTextureOffset( const SCalcTextureOffsetInfo& Info )
        {
            ExtentF32 Ret = {0, 0};

            const uint32_t lodTexSize = Info.rootTextureSize >> Info.nodeLevel;

            return Ret;
        }

        vke_force_inline uint32_t MapPositionTo1DArrayIndex( const Math::CVector3& vecPos,
                                                             const uint32_t& tileSize,
                                                             const uint32_t& terrainHalfSize,
                                                             const uint32_t& elementCountInRow )
        {
            // (pos + terrain half size) / element size 
            const uint32_t x = ( (int32_t)vecPos.x + terrainHalfSize ) / tileSize;
            const uint32_t y = ( (int32_t)vecPos.y + terrainHalfSize ) / tileSize;
            return Math::Map2DArrayIndexTo1DArrayIndex( x, y, elementCountInRow );
        }

        void CTerrainQuadTree::_CalcErrorLODs( const SNode& CurrNode, const uint32_t& textureIdx,
                                               const SViewData& View )
        {
            const auto hCurrNode = CurrNode.Handle;
            const auto& AABB = CurrNode.AABB;
            Math::CVector4 vecPoint;
            CalcNearestSpherePoint( Math::CVector4( AABB.Center ), CurrNode.boundingSphereRadius,
                Math::CVector4( View.vecPosition ), &vecPoint );
            float err, distance;
            _CalcError( vecPoint, hCurrNode.level, View, &err, &distance );

            //float childErr, childDistance;

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

            /*VKE_DBG_LOG( "" << indents[ hCurrNode.level ] << "l: " << hCurrNode.level << " idx: " << hCurrNode.index <<
                         " d: " << distance << " e: " << err <<
                         " c: " << AABB.Center.x << ", " << AABB.Center.z <<
                         " p: " << vecPoint.x << ", " << vecPoint.z << 
                         " vp: " << CurrNode.DrawData.vecPosition.x << ", " << CurrNode.DrawData.vecPosition.z <<
                         " s:" << CurrNode.AABB.Extents.x << ", " << CurrNode.AABB.Extents.z << "\n" );*/
            const uint8_t highestLod = (uint8_t)(m_Desc.lodCount - 1);
            // Smallest tiles has no children
            const bool hasChildNodes = CurrNode.ahChildren[ 0 ].handle != UNDEFINED_U32;
            // note, level = 0 == root node. Root nodes has no DrawData specified.
            // Root nodes must not be added to draw path
            if( ( err > 60 && hasChildNodes ) || hCurrNode.level == 0 )
            {
                // Parent has always 0 or 4 children
                //if( CurrNode.ahChildren[ 0 ].handle != UNDEFINED_U32 )
                {
                    // LOG child
                    /*for( uint32_t i = 0; i < 4; ++i )
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
                    }*/
                    for( uint32_t i = 0; i < 4; ++i )
                    {
                        const auto hNode = CurrNode.ahChildren[ i ];
                        const auto& ChildNode = m_vNodes[ hNode.index ];
                        //if( ChildNode.ahChildren[ 0 ].handle != UNDEFINED_U32 )
                        {
                            _CalcErrorLODs( ChildNode, textureIdx, View );
                        }
                    }
                }
            }
            else //if( hCurrNode.level > 0 )
            {
                SLODData Data;
                Data.lod = highestLod - (uint8_t)hCurrNode.level;
                //Data.DrawData.textureIdx = textureIdx;
                //Data.DrawData.vecPosition = AABB.Center;
                Data.DrawData = CurrNode.DrawData;
                Data.idx = MapPositionTo1DArrayIndex( Data.DrawData.vecPosition, m_tileSize,
                                                      m_terrainHalfSize, m_tileInRowCount );
                /*VKE_DBG_LOG("" << indents[hCurrNode.level] << "l: " << hCurrNode.level << " idx: " << hCurrNode.index <<
                    " d: " << distance << " e: " << err <<
                    " c: " << AABB.Center.x << ", " << AABB.Center.z <<
                    " p: " << vecPoint.x << ", " << vecPoint.z <<
                    " vp: " << CurrNode.DrawData.vecPosition.x << ", " << CurrNode.DrawData.vecPosition.z <<
                    " s:" << CurrNode.AABB.Extents.x << ", " << CurrNode.AABB.Extents.z << "\n");*/
                _AddLOD( Data );
            }
        }

        void CTerrainQuadTree::_CalcDistanceLODs( const SNode& CurrNode, const uint32_t& textureIdx,
                                               const SViewData& View )
        {
            const auto hCurrNode = CurrNode.Handle;
            const auto& AABB = CurrNode.AABB;
            /*Math::CVector4 vecPoint;
            CalcNearestSpherePoint( Math::CVector4( AABB.Center ), CurrNode.boundingSphereRadius,
                                    Math::CVector4( View.vecPosition ), &vecPoint );*/
            
            //_CalcError( vecPoint, hCurrNode.level, View, &err, &distance );
            float distance = _CalcDistanceToCenter( AABB.Center, View );

            //float childErr, childDistance;

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

            /*VKE_DBG_LOG( "" << indents[ hCurrNode.level ] << "l: " << hCurrNode.level << " idx: " << hCurrNode.index <<
            " d: " << distance << " e: " << err <<
            " c: " << AABB.Center.x << ", " << AABB.Center.z <<
            " p: " << vecPoint.x << ", " << vecPoint.z <<
            " vp: " << CurrNode.DrawData.vecPosition.x << ", " << CurrNode.DrawData.vecPosition.z <<
            " s:" << CurrNode.AABB.Extents.x << ", " << CurrNode.AABB.Extents.z << "\n" );*/
            const uint8_t highestLod = ( uint8_t )( m_Desc.lodCount - 1 );
            // Smallest tiles has no children
            const bool hasChildNodes = CurrNode.ahChildren[ 0 ].handle != UNDEFINED_U32;
            const bool isRoot = CurrNode.hParent.handle == UNDEFINED_U32;
            // If node is in acceptable distance check its child nodes
            //const uint32_t maxDistance = (uint32_t)m_Desc.maxViewDistance;
            //Calc distance in tile size number
            // 1st distance range == 1 * tile size
            // 2nd distance range = 2 * tile size
            // 3rd distance range = 4 * tile size
            // Nth distance range = n * tile size
            // Calc lod based on distance. Use tile size as counter
            const uint32_t lod = Math::Min( ( (uint32_t)distance / m_tileSize - 1 ), highestLod );
            // Check if current nod level matches distance lod
   
            // note, level = 0 == root node. Root nodes has no DrawData specified.
            // Root nodes must not be added to draw path
            //if( ( err > 60 && !hasChildNodes ) || hCurrNode.level == 0 )
            const uint8_t tileLod = highestLod - ( uint8_t )hCurrNode.level;
            if( (lod >= tileLod && hasChildNodes) || isRoot )
            {
                // Parent has always 0 or 4 children
                //if( CurrNode.ahChildren[ 0 ].handle != UNDEFINED_U32 )
                {
                    // LOG child
                    /*for( uint32_t i = 0; i < 4; ++i )
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
                    }*/
                    for( uint32_t i = 0; i < 4; ++i )
                    {
                        const auto hNode = CurrNode.ahChildren[ i ];
                        const auto& ChildNode = m_vNodes[ hNode.index ];
                        //if( ChildNode.ahChildren[ 0 ].handle != UNDEFINED_U32 )
                        {
                            _CalcDistanceLODs( ChildNode, textureIdx, View );
                        }
                    }
                }
            }
            else //if( hCurrNode.level > 0 )
            {
                SLODData Data;
                Data.lod = highestLod - ( uint8_t )hCurrNode.level;
                //Data.DrawData.textureIdx = textureIdx;
                //Data.DrawData.vecPosition = AABB.Center;
                Data.DrawData = CurrNode.DrawData;
                Data.idx = MapPositionTo1DArrayIndex( Data.DrawData.vecPosition, m_tileSize,
                                                      m_terrainHalfSize, m_tileInRowCount );
                /*VKE_DBG_LOG("" << indents[hCurrNode.level] << "l: " << hCurrNode.level << " idx: " << hCurrNode.index <<
                " d: " << distance << " e: " << err <<
                " c: " << AABB.Center.x << ", " << AABB.Center.z <<
                " p: " << vecPoint.x << ", " << vecPoint.z <<
                " vp: " << CurrNode.DrawData.vecPosition.x << ", " << CurrNode.DrawData.vecPosition.z <<
                " s:" << CurrNode.AABB.Extents.x << ", " << CurrNode.AABB.Extents.z << "\n");*/
                _AddLOD( Data );
            }
        }

        void CTerrainQuadTree::_SetLODMap( const SLODData& Data )
        {
            // Calc how many highest lod tiles contains this chunk
            // LOD map is a map of highest lod tiles
            uint32_t currX, currY;
            Math::Map1DarrayIndexTo2DArrayIndex( Data.idx, m_tileInRowCount, &currX, &currY );
            uint32_t tileRowCount = Math::CalcPow2( Data.lod );
            uint32_t tileColCount = tileRowCount;
            
            for( uint32_t y = 0; y < tileColCount; ++y )
            {
                for( uint32_t x = 0; x < tileRowCount; ++x )
                {
                    uint32_t idx = Math::Map2DArrayIndexTo1DArrayIndex( x + currX, y + currY, m_tileInRowCount );
                    m_vLODMap[ idx ] = Data.lod;
                }
            }
        }

        void CTerrainQuadTree::_AddLOD( const SLODData& Data )
        {
            if( m_vvLODData[ 0 ].IsEmpty() )
            {
                m_vvLODData[ 0 ].Reserve( 1000 );
            }
            _SetLODMap( Data );
            m_vvLODData[ 0 ].PushBack( Data );
        }

        void CTerrainQuadTree::_NotifyLOD(const UNodeHandle& hParent, const UNodeHandle& hNode,
            const ExtentF32& TopLeftCorner)
        {
            auto& Parent = m_vNodes[hParent.index];

            if (Parent.hParent.index != UNDEFINED_U32)
            {
                _NotifyLOD(Parent.hParent, hNode, TopLeftCorner);
            }
            else
            {
                //auto& Node = m_vNodes[hNode.index];
                SLODData Data;
                Data.lod = LAST_LOD - (uint8_t)hNode.level;
                Data.DrawData.textureIdx = m_vTextureIndices[hParent.index];
                Data.DrawData.TextureOffset = TopLeftCorner;
                m_vLODData.PushBack(Data);
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

        float CTerrainQuadTree::_CalcDistanceToCenter( const Math::CVector3& vecPoint, const SViewData& View )
        {
            return Math::CVector3::Distance( vecPoint, ( View.vecPosition ) );
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