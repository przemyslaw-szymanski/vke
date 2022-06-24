#include "Scene/Terrain/CTerrain.h"
#include "CVkEngine.h"
#include "Core/Managers/CImageManager.h"
#include "Core/Math/Math.h"
#include "Core/Utils/CProfiler.h"
#include "Scene/CScene.h"
#include "Scene/Terrain/CTerrainMeshShaderRenderer.h"
#include "Scene/Terrain/CTerrainVertexFetchRenderer.h"
#define DEBUG_LOD_STITCH_MAP 0
#define INIT_CHILD_NODES_FOR_EACH_ROOT 0
#define DISABLE_FRUSTUM_CULLING 1
#define VKE_PROFILE_TERRAIN 0
#define VKE_PROFILER_TERRAIN_UPDATE 0
#define SAVE_TERRAIN_HEIGHTMAP_NORMAL 1
namespace VKE
{
    namespace Scene
    {
        //float g_lodError = 10;

        void vke_force_inline CalcNodePosition( const Math::CVector4& vec4Center, const float& nodeExtents,
                                                Math::CVector3* pOut )
        {
            pOut->x = vec4Center.x - nodeExtents;
            pOut->y = vec4Center.y;
            pOut->z = vec4Center.z + nodeExtents;
        }
        void vke_force_inline CalcNodePosition( const Math::CVector3& vec3Center, const float& nodeExtents,
                                                Math::CVector3* pOut )
        {
            pOut->x = vec3Center.x - nodeExtents;
            pOut->y = vec3Center.y;
            pOut->z = vec3Center.z + nodeExtents;
        }

        void CalcNearestSpherePoint( const Math::CVector4& vecSphereCenter, const float sphereRadius,
                                     const Math::CVector4& vecPoint, Math::CVector4* pOut )
        {
            Math::CVector4 vecDir = vecPoint - vecSphereCenter;
            vecDir.Normalize();
            Math::CVector4::Mad( vecDir, Math::CVector4( sphereRadius ), vecSphereCenter, pOut );
        }
        struct SCalcTextureOffsetInfo
        {
            Math::CVector3 vecRootMinCorner;
            Math::CVector3 vecRootMaxCorner;
            Math::CVector3 vecRootSize;
            Math::CVector3 vecTilePosition;
            uint32_t tileVertexCount;
            float tileVertexDistance;
            uint32_t rootTextureSize;
            uint8_t nodeLevel;
        };
        ExtentF32 CalcTextureOffset( const SCalcTextureOffsetInfo& Info )
        {
            ExtentF32 Ret = { 0, 0 };
            // const uint32_t lodTexSize = Info.rootTextureSize >> Info.nodeLevel;
            return Ret;
        }
        vke_force_inline uint32_t MapPositionTo1DArrayIndex( const Math::CVector3& vecPos, const uint32_t& tileSize,
                                                             const uint32_t& terrainHalfSize,
                                                             const uint32_t& elementCountInRow )
        {
            // (pos + terrain half size) / element size
            const uint32_t x = ( ( int32_t )vecPos.x + terrainHalfSize ) / tileSize;
            const uint32_t y = ( ( int32_t )-vecPos.z + terrainHalfSize ) / tileSize;
            uint32_t ret = Math::Map2DArrayIndexTo1DArrayIndex( x, y, elementCountInRow );
            return ret;
        }
        // Calc tile size for renderer
        // renderLOD is oposite to quadtree lod. Lowest lod in quadtree is 0
        // (root) while in rendering lod 0 is the highest one (the same as for
        // mipmaps).
        vke_force_inline uint16_t CalcRenderTileSize( const uint8_t renderLOD, const uint16_t baseTileSize )
        {
            return Math::CalcPow2( renderLOD ) * baseTileSize;
        }
        void LoadPosition3( const Math::CVector4 aVecs[ 3 ], uint32_t idx, Math::CVector3* pOut )
        {
            pOut->x = aVecs[ CTerrainQuadTree::SNodeLevel::CENTER_X ].floats[ idx ];
            pOut->y = aVecs[ CTerrainQuadTree::SNodeLevel::CENTER_Y ].floats[ idx ];
            pOut->z = aVecs[ CTerrainQuadTree::SNodeLevel::CENTER_Z ].floats[ idx ];
        }
        void LoadExtents3( const Math::CVector4 aVecs[ 2 ], uint32_t idx, Math::CVector3* pOut )
        {
            pOut->x = aVecs[ CTerrainQuadTree::SNodeLevel::EXTENTS_X ].floats[ idx ];
            pOut->y = aVecs[ CTerrainQuadTree::SNodeLevel::EXTENTS_Y ].floats[ idx ];
        }
        void LoadExtents3( const Math::CVector4 aVecs[ 2 ], uint32_t idx, float* pOut )
        {
            *pOut = aVecs[ CTerrainQuadTree::SNodeLevel::EXTENTS_X ].floats[ idx ];
        }
        void LoadPosition4( const Math::CVector4 aVecs[ 3 ], uint32_t idx, Math::CVector4* pOut )
        {
            pOut->x = aVecs[ CTerrainQuadTree::SNodeLevel::CENTER_X ].floats[ idx ];
            pOut->y = aVecs[ CTerrainQuadTree::SNodeLevel::CENTER_Y ].floats[ idx ];
            pOut->z = aVecs[ CTerrainQuadTree::SNodeLevel::CENTER_Z ].floats[ idx ];
            pOut->w = 1.0f;
        }
        void LoadExtents4( const Math::CVector4 aVecs[ 2 ], uint32_t idx, Math::CVector4* pOut )
        {
            pOut->x = aVecs[ CTerrainQuadTree::SNodeLevel::EXTENTS_X ].floats[ idx ];
            pOut->y = aVecs[ CTerrainQuadTree::SNodeLevel::EXTENTS_Y ].floats[ idx ];
        }
        void LoadAABB( const Math::CVector4 aCenters[ 3 ], const Math::CVector4 aExtents[ 2 ], uint32_t idx,
                       Math::CAABB* pOut )
        {
            pOut->Center.x = aCenters[ CTerrainQuadTree::SNodeLevel::CENTER_X ].floats[ idx ];
            pOut->Center.z = aCenters[ CTerrainQuadTree::SNodeLevel::CENTER_Z ].floats[ idx ];
            pOut->Center.y = aCenters[ CTerrainQuadTree::SNodeLevel::CENTER_Y ].floats[ idx ];
            pOut->Extents.x = aExtents[ CTerrainQuadTree::SNodeLevel::EXTENTS_X ].floats[ idx ];
            pOut->Extents.y = aExtents[ CTerrainQuadTree::SNodeLevel::EXTENTS_Y ].floats[ idx ];
            pOut->Extents.z = pOut->Extents.x;
        }
        void vke_force_inline CalcLenghts3( const Math::CVector4* aVectors, Math::CVector4* pOut )
        {
            Math::CVector4 aMuls[ 3 ], aAdds[ 2 ];
            // x*x, y*y, z*z
            {
                Math::CVector4::Mul( aVectors[ 0 ], aVectors[ 0 ], &aMuls[ 0 ] );
                Math::CVector4::Mul( aVectors[ 1 ], aVectors[ 1 ], &aMuls[ 1 ] );
                Math::CVector4::Mul( aVectors[ 2 ], aVectors[ 2 ], &aMuls[ 2 ] );
            }
            // x*x + y*y + z*z
            {
                Math::CVector4::Add( aMuls[ 0 ], aMuls[ 1 ], &aAdds[ 0 ] );
                Math::CVector4::Add( aMuls[ 2 ], aAdds[ 0 ], &aAdds[ 1 ] );
            }
            // sqrt( x*x + y*y + z*z )
            {
                Math::CVector4::Sqrt( aAdds[ 1 ], pOut );
            }
            // normalize
            // x / sqrt, y / sqrt, z / sqrt
            /*{
            Math::CVector4::Div(aVectors[0], vecSqrt, &aOut[0]);
            Math::CVector4::Div(aVectors[1], vecSqrt, &aOut[1]);
            Math::CVector4::Div(aVectors[2], vecSqrt, &aOut[2]);
            }*/
        }
        void vke_force_inline CalcDistance3( const Math::CVector4* aV1, const Math::CVector4* aV2,
                                             Math::CVector4* pOut )
        {
            Math::CVector4 aSubs[ 3 ];
            Math::CVector4::Sub( aV1[ 0 ], aV2[ 0 ], &aSubs[ 0 ] );
            Math::CVector4::Sub( aV1[ 1 ], aV2[ 1 ], &aSubs[ 1 ] );
            Math::CVector4::Sub( aV1[ 2 ], aV2[ 2 ], &aSubs[ 2 ] );
            // Lengths
            CalcLenghts3( aSubs, pOut );
        }
        void LoadVec3( const Math::CVector4& V, Math::CVector4 aOut[ 3 ] )
        {
            aOut[ 0 ] = { V.x };
            aOut[ 1 ] = { V.z };
            aOut[ 2 ] = { V.y };
        }
#define VKE_LOAD_VEC4_XZY( _vec4 )                                                                                     \
    Math::CVector4{ ( _vec4 ).x }, Math::CVector4{ ( _vec4 ).z }, Math::CVector4                                       \
    {                                                                                                                  \
        ( _vec4 ).y                                                                                                    \
    }
        void CalcScreenSpaceErrors( const Math::CVector4* aPoints, const float& worldSpaceError,
                                    const CTerrainQuadTree::SViewData& View, Math::CVector4* pvecErrorOut,
                                    Math::CVector4* pvecDistanceOut )
        {
            // VKE_PROFILE_SIMPLE();
            const Math::CVector4 aViewPoints[ 3 ] = { Math::CVector4{ View.vecPosition.x },
                                                      Math::CVector4{ View.vecPosition.z },
                                                      Math::CVector4{ View.vecPosition.y } };
            CalcDistance3( aViewPoints, aPoints, pvecDistanceOut );
            const Math::CVector4 vecWorldSpaceError( worldSpaceError );
            Math::CVector4 vecD, vecP;
            /// TODO: Cache this!
            const float k = View.screenWidth / ( 2.0f * std::tanf( View.halfFOV ) );
            const Math::CVector4 vecK( k );
            // d = error / distance
            Math::CVector4::Div( vecWorldSpaceError, *pvecDistanceOut, &vecD );
            // p = d * k
            Math::CVector4::Mul( vecD, vecK, pvecErrorOut );
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
            const float distance = Math::CVector4::Distance( Math::CVector4( View.vecPosition ), vecPoint );
            *pDistanceOut = distance;
            // const float w = distance * 2.0f * std::tanf( View.halfFOV );
            // const float p = ( worldSpaceError * View.screenWidth ) / w;
            /*const auto a = ( View.screenWidth * 0.5f ) / std::tanf(
            View.fovRadians * 0.5f ); const auto p = a * (e / distance);
            return p;*/
            /*const auto a = View.screenWidth / View.fovRadians;
            const auto p = a * ( e / distance );
            return p;*/
            // Urlich formula
            /*const float d = worldSpaceError / distance;
            const float k = View.screenWidth / (View.frustumWidth);
            const float p = d * k;*/
            // 3D engine desing for virtual globes formula
            const float w = distance * View.frustumWidth;
            const float p = ( worldSpaceError * View.screenWidth ) / w;
            *pErrOut = p;
            // Intel formula
            /*const float w = View.screenWidth * 0.5f * Math::Cot(
            View.fovRadians * 0.5f ); const float h = View.screenHeight * 0.5f *
            Math::Cot( View.fovRadians * 0.5f ); const float err = Math::Max( w,
            h ) * ( 0.1f / distance ); *pErrOut = err;*/
        }
        void CalcErrors( const Math::CVector4* aPoints, const float vertexDistance, const uint8_t nodeLevel,
                         const uint8_t lodCount, const CTerrainQuadTree::SViewData& View, Math::CVector4* pvecErrorOut,
                         Math::CVector4* pvecDistanceOut )
        {
            // VKE_PROFILE_SIMPLE();
            const float worldSpaceError = CalcWorldSpaceError( vertexDistance, nodeLevel, lodCount );
            CalcScreenSpaceErrors( aPoints, worldSpaceError, View, pvecErrorOut, pvecDistanceOut );
        }
        void CalcNearestSpherePoints( const Math::CVector4* aSphereCenters, const float sphereRadius,
                                      const Math::CVector4& vec4ViewPos, Math::CVector4* pOut )
        {
            // VKE_PROFILE_SIMPLE();
            Math::CVector4 aDirs[ 3 ], aTmp[ 3 ], aTmp2[ 3 ];
            const Math::CVector4 aViewPos[ 3 ] = { Math::CVector4{ vec4ViewPos.x }, Math::CVector4{ vec4ViewPos.z },
                                                   Math::CVector4{ vec4ViewPos.y } };
            const Math::CVector4 vecRadius( sphereRadius );
            // 68, 0, 4 / 0.99, 0, 0.05
            // 4, 0, 4 / 0.7, 0, 0.7
            //
            //
            Math::CVector4::Sub( aViewPos[ 0 ], aSphereCenters[ 0 ], &aDirs[ 0 ] );
            Math::CVector4::Sub( aViewPos[ 1 ], aSphereCenters[ 1 ], &aDirs[ 1 ] );
            Math::CVector4::Sub( aViewPos[ 2 ], aSphereCenters[ 2 ], &aDirs[ 2 ] );
            // normalize dirs
            {
                Math::CVector4::Mul( aDirs[ 0 ], aDirs[ 0 ], &aTmp[ 0 ] );
                Math::CVector4::Mul( aDirs[ 1 ], aDirs[ 1 ], &aTmp[ 1 ] );
                Math::CVector4::Mul( aDirs[ 2 ], aDirs[ 2 ], &aTmp[ 2 ] );
                Math::CVector4::Add( aTmp[ 0 ], aTmp[ 1 ], &aTmp2[ 0 ] );
                Math::CVector4::Add( aTmp[ 2 ], aTmp2[ 0 ], &aTmp2[ 1 ] );
                Math::CVector4::Sqrt( aTmp2[ 1 ], &aTmp2[ 2 ] );
                Math::CVector4::Div( aDirs[ 0 ], aTmp2[ 2 ], &aDirs[ 0 ] );
                Math::CVector4::Div( aDirs[ 1 ], aTmp2[ 2 ], &aDirs[ 1 ] );
                Math::CVector4::Div( aDirs[ 2 ], aTmp2[ 2 ], &aDirs[ 2 ] );
            }
            Math::CVector4::Mad( aDirs[ 0 ], vecRadius, aSphereCenters[ 0 ], &pOut[ 0 ] );
            Math::CVector4::Mad( aDirs[ 1 ], vecRadius, aSphereCenters[ 1 ], &pOut[ 1 ] );
            Math::CVector4::Mad( aDirs[ 2 ], vecRadius, aSphereCenters[ 2 ], &pOut[ 2 ] );
        }
        uint32_t MapTileIndexToRootIndex( const uint32_t& tileIdx, const ExtentU16& RootNodeCount,
                                          const ExtentU16& TileSize )
        {
            uint32_t ret = 0;
            // const uint16_t tileCountInRoot = TileSize.max / TileSize.min;
            return ret;
        }
        void IntersectsSIMD( const Math::CVector4 aSphereCenters[ 3 ], const Math::CVector4& v4Radius,
                             const Math::CFrustum& Frustum, Math::CVector4* pOut )
        {
            static const int X = 0;
            static const int Y = 2;
            static const int Z = 1;
            const auto& centerX = aSphereCenters[ X ];
            const auto& centerZ = aSphereCenters[ Z ];
            const auto& centerY = aSphereCenters[ Y ];
            const auto& aPlanes = Frustum.aPlanes;
            DirectX::XMVECTOR plx0x1x2x3, ply0y1y2y3, plz0z1z2z3, plw0w1w2w3;
            DirectX::XMVECTOR plx4x5x4x5, ply4y5y4y5, plz4z5z4z5, plw4w5w4w5;
            plx0x1x2x3 = { aPlanes[ 0 ].x, aPlanes[ 1 ].x, aPlanes[ 2 ].x, aPlanes[ 3 ].x };
            plx4x5x4x5 = { aPlanes[ 4 ].x, aPlanes[ 5 ].x, aPlanes[ 4 ].x, aPlanes[ 5 ].x };
            ply0y1y2y3 = { aPlanes[ 0 ].y, aPlanes[ 1 ].y, aPlanes[ 2 ].y, aPlanes[ 3 ].y };
            ply4y5y4y5 = { aPlanes[ 4 ].y, aPlanes[ 5 ].y, aPlanes[ 4 ].y, aPlanes[ 5 ].y };
            plz0z1z2z3 = { aPlanes[ 0 ].z, aPlanes[ 1 ].z, aPlanes[ 2 ].z, aPlanes[ 3 ].z };
            plz4z5z4z5 = { aPlanes[ 4 ].z, aPlanes[ 5 ].z, aPlanes[ 4 ].z, aPlanes[ 5 ].z };
            plw0w1w2w3 = { aPlanes[ 0 ].w, aPlanes[ 1 ].w, aPlanes[ 2 ].w, aPlanes[ 3 ].w };
            plw4w5w4w5 = { aPlanes[ 4 ].w, aPlanes[ 5 ].w, aPlanes[ 4 ].w, aPlanes[ 5 ].w };
            auto mulX = DirectX::XMVectorMultiply( plx0x1x2x3, centerX._Native );
            auto mulY = DirectX::XMVectorMultiply( ply0y1y2y3, centerY._Native );
            auto mulZ = DirectX::XMVectorMultiply( plz0z1z2z3, centerZ._Native );
            auto addXY = DirectX::XMVectorAdd( mulX, mulY );
            auto addXYZ = DirectX::XMVectorAdd( addXY, mulZ );
            auto addXYZW = DirectX::XMVectorAdd( addXYZ, plw0w1w2w3 );
            // Check against each plane of the frustum.
            DirectX::XMVECTOR Outside = DirectX::XMVectorFalseInt();
            DirectX::XMVECTOR InsideAll = DirectX::XMVectorTrueInt();
            DirectX::XMVECTOR CenterInsideAll = DirectX::XMVectorTrueInt();
            auto gt = DirectX::XMVectorGreater( addXYZW, v4Radius._Native );
            auto le = DirectX::XMVectorLessOrEqual( addXYZW, DirectX::XMVectorNegate( v4Radius._Native ) );
            auto ci = DirectX::XMVectorLessOrEqual( v4Radius._Native, Math::CVector4::ZERO._Native );
            Outside = DirectX::XMVectorOrInt( Outside, gt );
            InsideAll = DirectX::XMVectorAndInt( InsideAll, le );
            CenterInsideAll = DirectX::XMVectorAndInt( CenterInsideAll, ci );
            pOut->_Native = Outside;
        }
        void Intersects( const Math::CVector4 aSphereCenters[ 3 ], const Math::CVector4& v4Radius,
                         const Math::CFrustum& Frustum, Math::CVector4* pOut )
        {
            const float radius = v4Radius.x;
            for( uint32_t i = 0; i < 4; ++i )
            {
                Math::CVector3 v3Center;
                LoadPosition3( aSphereCenters, i, &v3Center );
                Math::CBoundingSphere Sphere( v3Center, radius );
                pOut->ints[ i ] = Frustum.Intersects( Sphere );
            }
        }

        ExtentU32 vke_force_inline CalcXY( const uint32_t idx, const uint32_t width )
        {
            ExtentU32 Ret;
            Ret.y = idx / width;
            Ret.x = idx % width;
            return Ret;
        }
        uint32_t vke_force_inline CalcTileCountForLod( uint8_t startLOD, uint8_t LODCount )
        {
            uint32_t ret = 0;
            for( uint8_t i = startLOD; i < LODCount; ++i )
            {
                ret += ( uint32_t )std::pow( 4, i );
            }
            return ret;
        }
        uint16_t CalcRootCount( const STerrainDesc& Desc, uint16_t maxHeightmapSize )
        {
            //// Calc number of roots based on max heightmap texture size
            //// 1 heightmap texel == 1 vertex position
            //// Minimum root count is 2x2
            // uint32_t rootCount = Desc.size / maxHeightmapSize;
            // rootCount = ( uint32_t )Math::Max( 2u, rootCount );
            //// Must be power of 2
            // rootCount = ( uint32_t )Math::CalcNextPow2( rootCount );
            // return ( uint16_t )rootCount;
            ExtentU16 Count = CTerrain::CalcTextureCount( Desc );
            return Count.width;
        }
       

        ExtentU8 CTerrainQuadTree::CalcLODCount( const STerrainDesc& Desc, uint16_t maxHeightmapSize,
                                                 uint8_t maxLODCount )
        {
            ExtentU8 Ret;
            uint32_t rootCount = CalcRootCount( Desc, maxHeightmapSize );
            uint32_t rootSize = Desc.size / rootCount;
            // Calculate absolute lod
            // tileSize is a max lod
            // start from root level
            uint8_t maxLOD = 0;
            for( uint32_t size = rootSize; size >= Desc.TileSize.min; size >>= 1, maxLOD++ )
            {
            }
            maxLOD = Math::Max( maxLOD, 1 ); // avoid 0 max lod
            // Now remove from maxLOD number of LOD for roots
            // for one only root it is lod = 0
            // 2x2 roots lod = 1
            // 4x4 roots lod = 2
            // 8x8 roots lod = 3
            // Check on which position there is 1 bit set
            uint8_t rootLevel = 0;
            // const auto v = rootCount >> rootLevel;
            for( ; rootCount >> rootLevel != 1; ++rootLevel )
            {
            }
            Ret.max = maxLOD;
            Ret.min = rootLevel;
            return Ret;
        }
        uint32_t CalcTileCountForRoot( uint8_t LODCount )
        {
            uint32_t ret = 0;
            for( uint8_t i = 0; i < LODCount; ++i )
            {
                ret += ( uint32_t )std::pow( 4, i );
            }
            return ret;
        }
        void CTerrainQuadTree::_CalcTerrainInfo( const SCalcTerrainInfo& Info, STerrainInfo* pOut )
        {
            const auto& Desc = *Info.pDesc;
            const auto LODCount = CalcLODCount( *Info.pDesc, ( uint16_t )Info.maxRootSize, Info.maxLODCount );
            pOut->maxLODCount = Math::Min( Info.pDesc->lodCount, LODCount.max );

            // Calculate total root count
            // Terrain is a quad
            {
                pOut->RootCount.x = pOut->RootCount.y = CalcRootCount( Desc, ( uint16_t )Info.maxRootSize );
            }
            const uint32_t totalRootCount = pOut->RootCount.width * pOut->RootCount.height;
            // Calc root size
            const uint32_t rootSize = ( uint32_t )Math::CalcPow2( pOut->maxLODCount - 1 ) * Desc.TileSize.min;
            // Calc max visible roots
            {
                uint32_t viewDistance = Math::Max( rootSize, ( uint32_t )Desc.maxViewDistance );
                if( viewDistance == 0 ) // if not defined
                {
                    // Whole terrain is visible
                    viewDistance = Desc.size;
                }
                // Calc how many roots are visible at max distance
                // Visible are all roots in view distance range
                const uint32_t visibleRootCount =
                    ( uint32_t )( viewDistance / rootSize ) * 2; // get all roots along whole quad
                pOut->maxVisibleRootCount = Math::Min( totalRootCount,
                                                       visibleRootCount * visibleRootCount ); // make it a quad
            }
            // Calc total tile count for one root
            {
                const uint32_t nodeCount = CalcTileCountForRoot( pOut->maxLODCount );
                pOut->tileCountForRoot = nodeCount;
            }
            // Calc total level count for child nodes for root
            {
                // Each node level == 4 child nodes
                // l 0 (root) == 1
                pOut->childLevelCountForRoot = CalcTileCountForRoot( pOut->maxLODCount - 1 );
            }
            // Calc max node count for all roots
            // Note for memoy and time efficiency only 2x2 roots contains nodes
            // for all LODs
            {
                const uint32_t nodeCountFor2x2 = pOut->tileCountForRoot * 4;
                pOut->maxNodeCount = totalRootCount + nodeCountFor2x2 - 4; // totalRootCount contains all roots
            }
        }
        void CTerrainQuadTree::_UpdateWorldSize()
        {
            // Calc world corners
            {
                const ExtentU32 WorldSize = { ( uint32_t )( m_RootNodeCount.width * m_Desc.TileSize.max ),
                                              ( uint32_t )( m_RootNodeCount.height * m_Desc.TileSize.max ) };
                CalcNodePosition( m_Desc.vecCenter, m_pTerrain->m_vecExtents.x,
                                  &m_avecWorldCorners[ ChildNodeIndices::LEFT_TOP ] );
                m_avecWorldCorners[ ChildNodeIndices::LEFT_BOTTOM ] = m_avecWorldCorners[ ChildNodeIndices::LEFT_TOP ] -
                                                                      Math::CVector3( 0, 0, ( float )WorldSize.height );
                m_avecWorldCorners[ ChildNodeIndices::RIGHT_TOP ] =
                    m_avecWorldCorners[ ChildNodeIndices::LEFT_TOP ] + Math::CVector3( ( float )WorldSize.width, 0, 0 );
                m_avecWorldCorners[ ChildNodeIndices::RIGHT_BOTTOM ] =
                    m_avecWorldCorners[ ChildNodeIndices::RIGHT_TOP ] -
                    Math::CVector3( 0, 0, ( float )WorldSize.height );
            }
        }
        Result CTerrainQuadTree::_Create( const STerrainDesc& Desc )
        {
            Result res = VKE_FAIL;
            m_Desc = Desc;
            const STerrainInfo& Info = m_pTerrain->m_TerrainInfo;
            // Copy these to avoid cache missess
            m_terrainHalfSize = m_pTerrain->m_halfSize;
            m_tileSize = Desc.TileSize.min;
            m_tileInRowCount = ( uint16_t )( m_terrainHalfSize / m_tileSize ) * 2;
            m_totalRootCount = ( uint16_t )( Info.RootCount.width * Info.RootCount.height );
            m_maxLODCount = Info.maxLODCount;
            m_tileInRootRowCount = Desc.TileSize.max / Desc.TileSize.min;
            
            // Set all tile lods to 0 to avoid vertex shifting
            m_vLODMap.Resize( m_tileInRowCount * m_tileInRowCount, 0 );
            // This quadtree is made of rootNodeCount quadTrees.
            // Each 'sub' quadtree root contains original heightmap texture
            m_RootNodeCount = Info.RootCount;
            const auto vecMinWorldSize = m_Desc.vecCenter - m_pTerrain->m_vecExtents;
            const auto vecMaxWorldSize = m_Desc.vecCenter + m_pTerrain->m_vecExtents;
            m_vRootNodeHeights.Resize( m_totalRootCount );
            _UpdateWorldSize();
            Math::CVector3& vecWorldTopLeftCorner = m_avecWorldCorners[ ChildNodeIndices::LEFT_TOP ];
            if( !m_vTextureIndices.Resize( m_RootNodeCount.x * m_RootNodeCount.y ) )
            {
                res = VKE_FAIL;
            }
            // ExtentU8 LODCount = CalcLODCount(Desc,
            // m_pTerrain->m_maxHeightmapSize, MAX_LOD_COUNT); uint32_t
            // nodeCount = CalcTileCountForLod( LODCount.min, LODCount.max );
            const uint32_t nodeCount = Info.maxNodeCount;
            const bool nodeDataReady = m_vNodes.Resize( nodeCount ) && m_vLODData.Reserve( nodeCount ) &&
                                       m_vNodeVisibility.Resize( nodeCount, true ) &&
                                       m_vBoundingSpheres.Resize( nodeCount ) && m_vAABBs.Resize( nodeCount ) &&
                                       m_vChildNodeHandles.Resize( nodeCount ) &&
                                       m_vVisibleRootNodes.Reserve( m_totalRootCount ) &&
                                       m_vVisibleRootNodeHandles.Reserve( m_totalRootCount ) &&
                                       m_vChildNodeLevels.Resize( Info.childLevelCountForRoot * MAIN_ROOT_COUNT );
            if( m_vvLODData.Resize( 8 ) )
            {
                res = VKE_OK;
            }
            for( uint32_t i = 0; i < 1; ++i )
            {
                m_vvLODData[ i ].Reserve( nodeCount );
            }
            if( nodeDataReady )
            {
                const auto vecRootNodeExtents =
                    m_pTerrain->m_vecExtents / Math::CVector3( m_RootNodeCount.x, 1.0f, m_RootNodeCount.y );
                const auto vecRootNodeSize = vecRootNodeExtents * 2.0f;
                const auto RootAABB = Math::CAABB( m_Desc.vecCenter, vecRootNodeExtents );
                // Node is a square so bounding sphere radius is a diagonal
                const float boundingSphereRadius = ( std::sqrtf( 2.0f ) * vecRootNodeExtents.x );
                // const float boundingSphereRadius2 = vecRootNodeExtents.x;
                Math::CVector3 vecRootNodeCenter;
                uint32_t currRootIdx = 0;
                for( uint16_t z = 0; z < m_RootNodeCount.y; ++z )
                {
                    for( uint16_t x = 0; x < m_RootNodeCount.x; ++x )
                    {
                        // Create root
                        UNodeHandle Handle;
                        // Handle.index = m_vNodes.PushBack( {} );
                        Handle.index = currRootIdx++;
                        Handle.childIdx = 0;
                        Handle.level = 0; // always iterate from 0 even if root
                                          // starts lower (1, 2, etc)
                        auto& Node = m_vNodes[ Handle.index ];
                        Node.hParent.handle = UNDEFINED_U32;
                        Node.Handle = Handle;
                        Node.boundingSphereRadius = boundingSphereRadius;
                        Node.vec3Position.x = vecWorldTopLeftCorner.x + vecRootNodeSize.x * x;
                        Node.vec3Position.y = 0;
                        Node.vec3Position.z = vecWorldTopLeftCorner.z - vecRootNodeSize.z * z;
                        vecRootNodeCenter.x = Node.vec3Position.x + vecRootNodeExtents.x;
                        vecRootNodeCenter.y = 0;
                        vecRootNodeCenter.z = Node.vec3Position.z - vecRootNodeExtents.z;
                        Node.AABB = Math::CAABB( vecRootNodeCenter, vecRootNodeExtents );
                        m_vAABBs[ Handle.index ] = Node.AABB;
                        m_vBoundingSpheres[ Handle.index ] =
                            Math::CBoundingSphere( Node.AABB.Center, Node.boundingSphereRadius );
                    }
                }
                const auto& TmpRoot = m_vNodes[ 0 ];
                m_FirstLevelNodeBaseInfo.boundingSphereRadius = TmpRoot.boundingSphereRadius * 0.5f;
                m_FirstLevelNodeBaseInfo.maxLODCount = m_maxLODCount;
                m_FirstLevelNodeBaseInfo.vecExtents = TmpRoot.AABB.Extents * 0.5f;
                m_FirstLevelNodeBaseInfo.vec4Extents = m_FirstLevelNodeBaseInfo.vecExtents;
                _ResetChildNodes();
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
                    _SetDrawDataForNode( &Root );
                    // res = _CreateChildNodes( Root.Handle, NodeData,
                    // m_Desc.lodCount );
#if INIT_CHILD_NODES_FOR_EACH_ROOT
                    SInitChildNodesInfo NodeInfo;
                    NodeInfo.boundingSphereRadius = NodeData.boundingSphereRadius;
                    NodeInfo.childNodeStartIndex = _AcquireChildNodes();
                    NodeInfo.hParent = Root.Handle;
                    NodeInfo.maxLODCount = m_maxLODCount;
                    NodeInfo.vec4Extents = NodeData.vec4Extents;
                    NodeInfo.vecExtents = NodeData.vecExtents;
                    NodeInfo.vec4ParentCenter = NodeData.vec4ParentCenter;
                    _InitChildNodes( NodeInfo );
#endif
                }
            }
            return res;
        }
        
        void CTerrainQuadTree::_InitRootNodesSIMD( const SViewData& View )
        {
            // Sort nodes by distance
            const auto& vecViewPosition = View.vecPosition;
            {
                // VKE_PROFILE_SIMPLE2("sort"); //140 us
                std::sort( &m_vVisibleRootNodes[ 0 ], &m_vVisibleRootNodes[ 0 ] + m_vVisibleRootNodes.GetCount(),
                           [ & ]( const SNode& Left, const SNode& Right ) {
                               const float leftDistance = Math::CVector3::Distance( Left.AABB.Center, vecViewPosition );
                               const float rightDistance =
                                   Math::CVector3::Distance( Right.AABB.Center, vecViewPosition );
                               return leftDistance < rightDistance;
                           } );
            }

            {
                // VKE_PROFILE_SIMPLE2("reset child nodes"); // 1 us
                _ResetChildNodeLevels();
            }
            {
                // VKE_PROFILE_SIMPLE2("init child nodes"); // 400 us
                SInitChildNodeLevel LevelInit;

                const uint32_t count = Math::Min( m_vVisibleRootNodes.GetCount(), 4 );
                for( uint32_t i = 0; i < count; ++i )
                {
                    auto& Root = m_vVisibleRootNodes[ i ];
                    LevelInit.parentLevel = 0;
                    LevelInit.parentBoundingSphereRadius = Root.boundingSphereRadius;
                    LevelInit.maxLODCount = m_maxLODCount;
                    LevelInit.vecParentSizes.x = Root.AABB.Center.x;
                    LevelInit.vecParentSizes.y = Root.AABB.Center.z;
                    LevelInit.vecParentSizes.z = Root.AABB.Extents.x; // width == depth
                    LevelInit.parentLevel = Root.Handle.level;
                    LevelInit.childLevelIndex = _AcquireChildNodeLevel();
                    LevelInit.bindingIndex = Root.DrawData.bindingIndex; // populate root binding for
                                                                         // all child nodes
                    LevelInit.rootNodeIndex = Root.Handle.index;
                    //LevelInit.CoordsInRootNode = 0;
                    Root.childLevelIndex = LevelInit.childLevelIndex;
                    _InitChildNodesSIMD( LevelInit );
                }
            }
        }
        // Initializes all 4 nodes at once
        // laneIndex is the SIMD index of child node
        void CTerrainQuadTree::_InitChildNodesSIMD( const SInitChildNodeLevel& Info )
        {
            // Get child node level
            SNodeLevel& ChildLevel = m_vChildNodeLevels[ Info.childLevelIndex ];
            // VKE_PROFILE_SIMPLE2("create child nodes for parent SIMD");
            // Top left, top right, bottom left, bottom right
            static const Math::CVector4 vec4DirectionXs( -1.0f, 1.0f, -1.0f, 1.0f );
            static const Math::CVector4 vec4DirectionZs( 1.0f, 1.0f, -1.0f, -1.0f );
            static const Math::CVector4 vec4Half( 0.5f );
            Math::CVector4& vec4CenterXs = ChildLevel.aAABBCenters[ SNodeLevel::CENTER_X ];
            Math::CVector4& vec4CenterZs = ChildLevel.aAABBCenters[ SNodeLevel::CENTER_Z ];
            Math::CVector4& vec4ChildExtentsWD = ChildLevel.aAABBExtents[ SNodeLevel::EXTENTS_X ];

            // SIMD
            {
                Math::CVector4 vec4ParentCenterX, vec4ParentCenterZ, vec4ParentExtentsWD;
                // const Math::CVector4 vec4ParentCenterX( Info.vecParentSizes.x );
                // const Math::CVector4 vec4ParentCenterZ( Info.vecParentSizes.y );
                // const Math::CVector4 vec4ParentExtentsWD( Info.vecParentSizes.z ); // node is a square so width ==
                // depth
                Math::CVector4::Load( Info.vecParentSizes.floats, &vec4ParentCenterX, &vec4ParentCenterZ,
                                      &vec4ParentExtentsWD );
                // Calc child node extents (parent / 2)
                Math::CVector4::Mul( vec4ParentExtentsWD, vec4Half, &vec4ChildExtentsWD );
                // Move node center x/z position towards direction vector in
                // distance of child node size
                Math::CVector4::Mad( vec4DirectionXs, vec4ChildExtentsWD, vec4ParentCenterX, &vec4CenterXs );
                Math::CVector4::Mad( vec4DirectionZs, vec4ChildExtentsWD, vec4ParentCenterZ, &vec4CenterZs );
            }
            ChildLevel.boundingSphereRadius = Info.parentBoundingSphereRadius * 0.5f;
            const uint8_t currLevel = Info.parentLevel + 1;
            ChildLevel.level = currLevel;
            ChildLevel.vecVisibility = Math::CVector4::ONE;
            ChildLevel.DrawData.bindingIndex = Info.bindingIndex;
            ChildLevel.rootNodeIndex = Info.rootNodeIndex;
            //ChildLevel.CoordsInRootNode = Info.CoordsInRootNode;

            const uint8_t childLevel = currLevel + 1;
            if( childLevel < Info.maxLODCount )
            {
                SInitChildNodeLevel ChildLevelInfo;
                ChildLevelInfo.maxLODCount = Info.maxLODCount;
                ChildLevelInfo.parentBoundingSphereRadius = ChildLevel.boundingSphereRadius;
                ChildLevelInfo.parentLevel = currLevel;
                ChildLevelInfo.vecParentSizes.z =
                    ChildLevel.aAABBExtents[ SNodeLevel::EXTENTS_X ].x; // the same for all nodes
                ChildLevelInfo.bindingIndex = Info.bindingIndex;

                for( uint32_t i = 0; i < 4; ++i )
                {
                    ChildLevelInfo.vecParentSizes.x = ChildLevel.aAABBCenters[ SNodeLevel::CENTER_X ].floats[ i ];
                    ChildLevelInfo.vecParentSizes.y = ChildLevel.aAABBCenters[ SNodeLevel::CENTER_Z ].floats[ i ];
                    ChildLevelInfo.childLevelIndex = _AcquireChildNodeLevel();
                    //ChildLevel.aChildLevelIndices[ i ] = ChildLevelInfo.childLevelIndex;
                    UNodeHandle Handle;
                    Handle.childIdx = i;
                    Handle.level = childLevel;
                    Handle.index = ChildLevelInfo.childLevelIndex;
                    ChildLevel.aChildLevelNodeHandles[ i ] = Handle;
                    ChildLevelInfo.rootNodeIndex = ChildLevel.rootNodeIndex;
                   
                    VKE_ASSERT( ChildLevel.rootNodeIndex != UNDEFINED_U32, "" );
                    // VKE_PROFILE_SIMPLE2("create child nodes for parent
                    // SIMD");
                    _InitChildNodesSIMD( ChildLevelInfo );
                }
            }
            else
            {
                //ChildLevel.aChildLevelIndices[ 0 ] = UNDEFINED_U32; // mark there are no more children
                ChildLevel.aChildLevelNodeHandles[ 0 ].handle = UNDEFINED_U32;
                // Update AABB bottom top
                /* Math::CVector4 vec4Min, vec4Max; // min, max height for 4 nodes
                _GetAABBHeightSIMD( ChildLevel, &vec4Min, &vec4Max );
                Math::CVector4 vec4PositionYs, vec4ExtentsY, vec4Tmp1;

                // positionY = (vecMin + vecMax) * 0.5f;
                Math::CVector4::Add( vec4Min, vec4Max, &vec4Tmp1 );
                Math::CVector4::Mul( vec4Tmp1, vec4Half, &vec4PositionYs );
                Math::CVector4::Sub( vec4Max, vec4Min, &vec4Tmp1 );
                Math::CVector4::Abs( vec4Tmp1, &vec4ExtentsY );
                ChildLevel.aAABBCenters[ SNodeLevel::CENTER_Y ] = vec4PositionYs;
                ChildLevel.aAABBExtents[ SNodeLevel::EXTENTS_Y ] = vec4ExtentsY;*/
            }
        }

        struct SRootInfoSIMD
        {
            Math::CVector4 vec4RootCornerXs;
            Math::CVector4 vec4RootCornerZs;
            Math::CVector4 vec4RootSize;
            Math::CVector4 vec4TileSize;
            Math::CVector4 vec4TileCount;
        };

        void MapRootNodeSpaceToGridSpace( const CTerrainQuadTree::SNodeLevel& NodeLevel,
                                          const SRootInfoSIMD& RootInfo, Math::CVector4* pOut )
        {
            const auto& vec4CenterXs = NodeLevel.aAABBCenters[ CTerrainQuadTree::SNodeLevel::CENTER_X ];
            const auto& vec4CenterZs = NodeLevel.aAABBCenters[ CTerrainQuadTree::SNodeLevel::CENTER_Z ];
            const auto& vec4ExtentsXs = NodeLevel.aAABBExtents[ CTerrainQuadTree::SNodeLevel::EXTENTS_X ];
            const auto& vec4ExtentsZs = NodeLevel.aAABBExtents[ CTerrainQuadTree::SNodeLevel::EXTENTS_Z ];
            Math::CVector4 vec4Xs, vec4Zs;
            vec4Xs = vec4CenterXs - vec4ExtentsXs;
            vec4Zs = vec4CenterZs + vec4ExtentsZs;
            Math::CVector4 vec4Tmp1 = Math::CVector4::Abs( ( RootInfo.vec4RootCornerXs - vec4Xs ) / RootInfo.vec4TileSize );
            Math::CVector4 vec4Tmp2 = Math::CVector4::Abs( ( RootInfo.vec4RootCornerZs - vec4Zs ) / RootInfo.vec4TileSize );
            Math::CVector4::Mad( vec4Tmp2, RootInfo.vec4TileCount, vec4Tmp1, pOut );
        }

        void CTerrainQuadTree::_GetAABBHeightSIMD( const SNodeLevel& NodeLevel, Math::CVector4* pvecMinOut,
            Math::CVector4* pvecMaxOut )
        {
            const auto& vRootHeights = m_vRootNodeHeights[ NodeLevel.rootNodeIndex ];
            if( vRootHeights.GetCount() > 0 )
            {
                SRootInfoSIMD RootInfo;
                Math::CVector4 vec4GridCoords{ 0 };
                const auto& RootNode = m_vNodes[ NodeLevel.rootNodeIndex ];
            
                RootInfo.vec4RootCornerXs = Math::CVector4( RootNode.vec3Position.x );
                RootInfo.vec4RootCornerZs = Math::CVector4( RootNode.vec3Position.z );
                RootInfo.vec4RootSize = Math::CVector4( m_Desc.TileSize.max );
                RootInfo.vec4TileCount = Math::CVector4( (float)m_Desc.TileSize.max / m_Desc.TileSize.min );
                RootInfo.vec4TileSize = Math::CVector4( m_Desc.TileSize.min );
                MapRootNodeSpaceToGridSpace( NodeLevel, RootInfo, &vec4GridCoords );
                // Load min/max heights from the array
            
                Math::CVector4& vec4Mins = *pvecMinOut, &vec4Maxs = *pvecMaxOut;
                const auto& Height0 = vRootHeights[ (uint32_t)vec4GridCoords.floats[0] ];
                const auto& Height1 = vRootHeights[ ( uint32_t )vec4GridCoords.floats[ 1 ] ];
                const auto& Height2 = vRootHeights[ ( uint32_t )vec4GridCoords.floats[ 2 ] ];
                const auto& Height3 = vRootHeights[ ( uint32_t )vec4GridCoords.floats[ 3 ] ];
                vec4Mins.floats[ 0 ] = Height0.min;
                vec4Maxs.floats[ 0 ] = Height0.max;
                vec4Mins.floats[ 1 ] = Height1.min;
                vec4Maxs.floats[ 1 ] = Height1.max;
                vec4Mins.floats[ 2 ] = Height2.min;
                vec4Maxs.floats[ 2 ] = Height2.max;
                vec4Mins.floats[ 3 ] = Height3.min;
                vec4Maxs.floats[ 3 ] = Height3.max;
               
            }
        }

        void CTerrainQuadTree::_InitChildNodesSIMD( const SNodeLevel& ParentLevel )
        {
        }
        
        void CTerrainQuadTree::_FreeChildNodes( UNodeHandle hParent )
        {
            SNode& Parent = m_vNodes[ hParent.index ];
            VKE_ASSERT( Parent.ahChildren[ 0 ].handle != UNDEFINED_U32, "" );
            // for (uint32_t i = 0; i < 4; ++i)
            {
                m_vFreeNodeIndices.PushBack( Parent.ahChildren[ 0 ].index );
            }
            for( uint32_t i = 0; i < 4; ++i )
            {
                _FreeChildNodes( Parent.ahChildren[ i ] );
                Parent.ahChildren[ i ].handle = UNDEFINED_U32;
            }
        }
        uint32_t CTerrainQuadTree::_AcquireChildNodes()
        {
            uint32_t ret;
            // Always acquire 4 nodes
            ret = m_vFreeNodeIndices.PopBackFast();
            // Child nodes indices are: ret + 0, ret + 1, ret + 2, ret + 3
            return ret;
        }
        void CTerrainQuadTree::_ResetChildNodes()
        {
            VKE_ASSERT( m_vNodes.IsEmpty() == false, "" );
            // Get all node indices starting from one after last root
            const uint32_t totalNodeCount = m_vNodes.GetCount();
            const uint32_t childNodeCount = totalNodeCount - m_totalRootCount;
            // There always must be a multiple of 4 child nodes
            VKE_ASSERT( childNodeCount % 4 == 0, "" );
            for( uint32_t i = m_totalRootCount; i < totalNodeCount; i += 4 )
            {
                m_vFreeNodeIndices.PushBack( i );
            }
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
            /*const auto& p = pInOut->DrawData.vecPosition;
            VKE_LOG(p.x << ", " << p.z);*/
        }
        
        CTerrainQuadTree::CHILD_NODE_INDEX CTerrainQuadTree::_CalcNodeIndex( const Math::CVector4& vecParentCenter,
                                                                             const Math::CVector4& vecPoint ) const
        {
            CHILD_NODE_INDEX ret;
            Math::CVector4 vecTmp;
            Math::CVector4::LessOrEquals( vecPoint, vecParentCenter, &vecTmp );
            bool aResults[ 4 ];
            vecTmp.ConvertCompareToBools( aResults );
            static const CHILD_NODE_INDEX aIndices[ 2 ][ 2 ] = {
                { ChildNodeIndices::RIGHT_TOP, ChildNodeIndices::RIGHT_BOTTOM },
                { ChildNodeIndices::LEFT_TOP, ChildNodeIndices::LEFT_BOTTOM }
            };
            // Need only x and z
            ret = aIndices[ aResults[ 0 ] ][ aResults[ 2 ] ];
            return ret;
        }
        void CTerrainQuadTree::_Update()
        {
#if VKE_PROFILER_TERRAIN_UPDATE
            VKE_PROFILE_SIMPLE();
#endif
            auto pCamera = m_pTerrain->GetScene()->GetCamera();
            SViewData View;
            View.fovRadians = pCamera->GetFOVAngleRadians();
            View.screenWidth = pCamera->GetViewport().width;
            View.screenHeight = pCamera->GetViewport().height;
            View.frustumWidth = pCamera->CalcFrustumWidth( 1.0f ); // frustum width at 1.0f distance
            View.Frustum = pCamera->GetFrustum();
            View.Frustum.SetFar( m_pTerrain->m_Desc.maxViewDistance );
            View.vecPosition = pCamera->GetPosition();
            View.halfFOV = View.fovRadians * 0.5f;
            static Math::CVector3 vecLastPos = Math::CVector3::ZERO;
            if( vecLastPos != View.vecPosition )
            {
                vecLastPos = View.vecPosition;
            }
            {
                m_vLODData.Clear();
                for( uint32_t i = 0; i < m_vvLODData.GetCount(); ++i )
                {
                    m_vvLODData[ i ].Clear();
                }
            }
            // Cull only roots
            {
#if VKE_PROFILE_TERRAIN
                VKE_PROFILE_SIMPLE2( "Frustum cull roots" );
#endif
                _FrustumCullRoots( View );
            }
            // Determine which root contains the camera
            {
#if !INIT_CHILD_NODES_FOR_EACH_ROOT
#if VKE_PROFILE_TERRAIN
                VKE_PROFILE_SIMPLE2( "Init main roots" );
#endif
                _InitRootNodesSIMD( View );
#endif
            }
            {
#if VKE_PROFILE_TERRAIN
                VKE_PROFILE_SIMPLE2( "FrustumCull" );
#endif
                _FrustumCull( View );
            }
            {

#if VKE_PROFILE_TERRAIN
                VKE_PROFILE_SIMPLE2( "Calc lods" );
#endif
                float lodTreshold = m_Desc.lodTreshold;
                for( uint32_t i = 0; i < m_vVisibleRootNodes.GetCount(); ++i )
                {
                    _CalcLODsSIMD( m_vVisibleRootNodes[ i ], View, lodTreshold );
                }
            }
            {
#if VKE_PROFILE_TERRAIN
                VKE_PROFILE_SIMPLE2( "Merge LODS" );
#endif
                for( uint32_t i = 0; i < m_vvLODData.GetCount(); ++i )
                {
                    m_vLODData.Append( m_vvLODData[ i ] );
                }
            }
            {
#if VKE_PROFILE_TERRAIN
                VKE_PROFILE_SIMPLE2( "Set Stitches" );
#endif
                _SetStitches();
                if( m_pTerrain->m_Desc.distanceSort )
                {
                    _SortLODData( View, &m_vLODData );
                }
            }
        }
        void CTerrainQuadTree::_SortLODData( const SViewData& View, LODDataArray* pOut )
        {
            const auto& vIn = *pOut;
            SLODData* pBegin = &vIn[ 0 ];
            SLODData* pEnd = pBegin + pOut->GetCount();
            std::sort( std::execution::par_unseq, pBegin, pEnd, [ & ]( const SLODData& left, const SLODData& right ) {
                float leftDistance = Math::CVector3::Distance( left.DrawData.vecPosition, View.vecPosition );
                float rightDistance = Math::CVector3::Distance( right.DrawData.vecPosition, View.vecPosition );
                return leftDistance < rightDistance;
            } );
        }

        void CTerrainQuadTree::_CalcLODsSIMD( const SNode& Root, const SViewData& View, float lodTreshold )
        {
            Math::CVector4 vec4ChildCenter, vec4ChildExtents;
            const Math::CVector4 vec4ViewPosition = Math::CVector4( View.vecPosition );
            const bool hasChildNodes = Root.childLevelIndex != UNDEFINED_U32;
            const float boundingSphereRadius = Root.boundingSphereRadius;
            Math::CVector4 vec4NearestPoint;
            const auto& vec4Center = Math::CVector4{ Root.AABB.Center };
            float err, distance;
            CalcNearestSpherePoint( vec4Center, boundingSphereRadius, vec4ViewPosition, &vec4NearestPoint );
            _CalcError( vec4NearestPoint, Root.Handle.level, View, &err, &distance );
            if( hasChildNodes && err > lodTreshold )
            {
                const SNodeLevel& ChildNodes = m_vChildNodeLevels[ Root.childLevelIndex ];
                _CalcLODsSIMD( ChildNodes, Root, View, lodTreshold );
            }
            else
            {
                SLODInfo LODInfo;
                LODInfo.vec4Center = vec4Center;
                LODInfo.nodeExtents = Root.AABB.Extents.x;
                LODInfo.nodeLevel = Root.Handle.level;
                LODInfo.bindingIndex = Root.DrawData.bindingIndex;
                LODInfo.vec3RootPosition = Root.vec3Position;
#if VKE_SCENE_DEBUG
                LODInfo.rootIndex = Root.Handle.index;
#endif
                _AddLOD( LODInfo );
            }
        }
        void CTerrainQuadTree::_CalcLODsSIMD( const SNodeLevel& ChildNodes, const SNode& Root,
            const SViewData& View, float lodTreshold )
        {
            //const bool hasChildNodes = ChildNodes.aChildLevelIndices[ 0 ] != UNDEFINED_U32;
            
            // const float boundingSphereRadius =
            // ChildNodes.boundingSphereRadius;
            const float boundingSphereRadius = ChildNodes.aAABBExtents[ 0 ].x;
            const Math::CVector4 vec4ViewPosition = Math::CVector4( View.vecPosition );
            uint8_t aChildNodeStates[ 4 ]; // 0 - do nothing, 1 - add lod, 2 -
                                           // calc lod for next level
            Math::CVector4 vec4ChildCenter, vec4ChildExtents, vecErrors, vecDistances;
            Math::CVector4 avec4Centers[ 4 ];
            Math::CVector4 vec4NearestPoint[ 4 ];
            Math::CVector4 aNearestPoints[ 3 ], vecCalcLODs;

            Math::CVector4 vecVisibility;
            {
                /// TODO: Use SIMD
                _FrustumCullChildNodes( View, ChildNodes, &vecVisibility );
            }
            {
                // VKE_PROFILE_SIMPLE2("NEW"); // 2us
                CalcNearestSpherePoints( ChildNodes.aAABBCenters, boundingSphereRadius, vec4ViewPosition,
                                         aNearestPoints );
                CalcErrors( aNearestPoints, m_Desc.vertexDistance, ChildNodes.level, m_Desc.lodCount, View, &vecErrors,
                            &vecDistances );
            }
            const bool hasChildNodes = ChildNodes.aChildLevelNodeHandles[ 0 ].handle != UNDEFINED_U32;
            for( uint32_t i = 0; i < 4; ++i )
            {
                const bool visible = vecVisibility.ints[ i ] == 1;
                aChildNodeStates[ i ] = ( 1 + ( hasChildNodes && vecErrors.floats[ i ] > lodTreshold ) ) * visible;
                if( aChildNodeStates[ i ] == 1 )
                {
                    SLODInfo Info;
                    LoadPosition4( ChildNodes.aAABBCenters, i, &Info.vec4Center );
                    LoadExtents3( ChildNodes.aAABBExtents, i, &Info.nodeExtents );
                    Info.nodeLevel = ChildNodes.level;
                    Info.bindingIndex = ChildNodes.DrawData.bindingIndex;
                    Info.vec3RootPosition = m_vNodes[ ChildNodes.rootNodeIndex ].vec3Position;
#if VKE_SCENE_DEBUG
                    Info.rootIndex = ChildNodes.rootNodeIndex;
#endif
                    {
                        _AddLOD( Info );
                    }
                }
            }
            for( uint32_t i = 0; i < 4; ++i )
            {
                // const bool visible = vecVisibility.ints[ i ] == 1;
                if( aChildNodeStates[ i ] == 2 )
                {
                    //const SNodeLevel& Level = m_vChildNodeLevels[ ChildNodes.aChildLevelIndices[ i ] ];
                    const SNodeLevel& Level = m_vChildNodeLevels[ ChildNodes.aChildLevelNodeHandles[ i ].index ];
                    _CalcLODsSIMD( Level, Root, View, lodTreshold );
                }
            }
        }
        
        void CTerrainQuadTree::_SetLODMap( const SLODData& Data )
        {
            // VKE_PROFILE_SIMPLE();
            // Calc how many highest lod tiles contains this chunk
            // LOD map is a map of highest lod tiles
            uint32_t currX, currY;
            Math::Map1DarrayIndexTo2DArrayIndex( Data.idx, m_tileInRowCount, m_tileInRowCount, &currX, &currY );
            const uint8_t lod = Data.lod;
            // Calc from how many minimal tiles this tile is built
            const uint32_t tileRowCount = Math::CalcPow2( lod );
            const uint32_t tileColCount = tileRowCount;
            // Set LOD only on edges due to performance reasons
            // Vertical
#if 1
            const uint32_t rightColIdx = currX + tileColCount - 1;
            const uint32_t bottomRowIdx = currY + tileRowCount - 1;
            // const uint32_t topRowIdx = currY;
            // const uint32_t leftColIdx = currX;
            for( uint32_t y = 0; y < tileRowCount; ++y )
            {
                const uint32_t leftIdx = Math::Map2DArrayIndexTo1DArrayIndex( currX, y + currY, m_tileInRowCount );
                const uint32_t rightIdx =
                    Math::Map2DArrayIndexTo1DArrayIndex( rightColIdx, y + currY, m_tileInRowCount );
                m_vLODMap[ leftIdx ] = lod;
                m_vLODMap[ rightIdx ] = lod;
            }
            for( uint32_t x = 0; x < tileColCount; ++x )
            {
                const uint32_t topIdx = Math::Map2DArrayIndexTo1DArrayIndex( currX + x, currY, m_tileInRowCount );
                const uint32_t bottomIdx =
                    Math::Map2DArrayIndexTo1DArrayIndex( currX + x, bottomRowIdx, m_tileInRowCount );
                m_vLODMap[ topIdx ] = lod;
                m_vLODMap[ bottomIdx ] = lod;
            }
#else
            for( uint32_t y = 0; y < tileColCount; ++y )
            {
                for( uint32_t x = 0; x < tileRowCount; ++x )
                {
                    uint32_t idx = Math::Map2DArrayIndexTo1DArrayIndex( x + currX, y + currY, m_tileInRowCount );
                    m_vLODMap[ idx ] = Data.lod;
                }
            }
#endif
        }
        void CTerrainQuadTree::_SetStitches()
        {
            for( uint32_t i = 0; i < m_vLODData.GetCount(); ++i )
            {
                auto& Data = m_vLODData[ i ];
                uint32_t tileRowCount = Math::CalcPow2( Data.lod ); // number of tiles for this node
                auto pos = Data.DrawData.vecPosition;
                // x, y are the left top corner in the map
                uint32_t x, y;
                Math::Map1DarrayIndexTo2DArrayIndex( Data.idx, m_tileInRowCount, m_tileInRowCount, &x, &y );
                // Calc idxs of neighbours
                uint8_t leftLod = Data.lod;
                uint8_t rightLod = Data.lod;
                uint8_t topLod = Data.lod;
                uint8_t bottomLod = Data.lod;
                if( x > 0 )
                {
                    uint32_t leftIdx = Math::Map2DArrayIndexTo1DArrayIndex( x - 1, y, m_tileInRowCount );
                    leftLod = m_vLODMap[ leftIdx ];
                }
                if( y > 0 )
                {
                    uint32_t topIdx = Math::Map2DArrayIndexTo1DArrayIndex( x, y - 1, m_tileInRowCount );
                    topLod = m_vLODMap[ topIdx ];
                }
                if( x + tileRowCount < m_tileInRowCount )
                {
                    uint32_t rightIdx = Math::Map2DArrayIndexTo1DArrayIndex( x + tileRowCount, y, m_tileInRowCount );
                    rightLod = m_vLODMap[ rightIdx ];
                }
                if( y + tileRowCount < m_tileInRowCount )
                {
                    uint32_t bottomIdx = Math::Map2DArrayIndexTo1DArrayIndex( x, y + tileRowCount, m_tileInRowCount );
                    bottomLod = m_vLODMap[ bottomIdx ];
                }
                if( Data.lod < leftLod )
                {
                    Data.DrawData.leftVertexDiff = ( uint8_t )Math::CalcPow2( leftLod - ( uint8_t )Data.lod );
                }
                if( Data.lod < rightLod )
                {
                    Data.DrawData.rightVertexDiff = ( uint8_t )Math::CalcPow2( rightLod - ( uint8_t )Data.lod );
                }
                if( Data.lod < topLod )
                {
                    Data.DrawData.topVertexDiff = ( uint8_t )Math::CalcPow2( topLod - ( uint8_t )Data.lod );
                }
                if( Data.lod < bottomLod )
                {
                    Data.DrawData.bottomVertexDiff = ( uint8_t )Math::CalcPow2( bottomLod - ( uint8_t )Data.lod );
                    ;
                }
            }
#if DEBUG_LOD_STITCH_MAP
            for( uint32_t y = 0; y < m_tileInRowCount; ++y )
            {
                for( uint32_t x = 0; x < m_tileInRowCount; ++x )
                {
                    uint32_t idx = Math::Map2DArrayIndexTo1DArrayIndex( x, y, m_tileInRowCount );
                    // VKE_LOG( (uint32_t)m_vLODMap[idx] << " " );
                    VKE_LOGGER << ( uint32_t )m_vLODMap[ idx ] << " ";
                }
                VKE_LOGGER << "\n";
            }
            VKE_LOGGER.Flush();
#endif
            // VKE_PROFILE_SIMPLE();
            // m_vLODMap.Reset(m_maxLODCount-1);
        }
        void CTerrainQuadTree::_AddLOD( const SLODInfo& Info )
        {
            SLODData Data;
            _SetLODData( Info, &Data );
            _SetLODMap( Data );
            m_vvLODData[ 0 ].PushBack( Data );
        }
        void CTerrainQuadTree::_SetLODData( const SLODInfo& Info, SLODData* pOut ) const
        {
            const uint8_t highestLod = ( uint8_t )( m_Desc.lodCount - 1 );
            pOut->lod = highestLod - Info.nodeLevel;
            auto& DrawData = pOut->DrawData;
            ImageSize Offset = m_pTerrain->m_Desc.HeightmapOffset;
            {
                /*auto& vecPos = DrawData.vecPosition;
                vecPos.x = Info.vec4Center.x - Info.nodeExtents;
                vecPos.y = Info.vec4Center.y;
                vecPos.z = Info.vec4Center.z + Info.nodeExtents;*/
                CalcNodePosition( Info.vec4Center, Info.nodeExtents, &DrawData.vecPosition );
            }
            pOut->idx =
                MapPositionTo1DArrayIndex( DrawData.vecPosition, m_tileSize, m_terrainHalfSize, m_tileInRowCount );
            DrawData.tileSize = Info.nodeExtents * 2;
            DrawData.pPipeline = m_pTerrain->_GetPipelineForLOD( pOut->lod );
            DrawData.bindingIndex = Info.bindingIndex;
#if VKE_SCENE_DEBUG
            DrawData.rootIdx = Info.rootIndex;
#endif
            Math::CVector3 vec3Offset = Math::CVector3::ZERO;
            const auto rootSize = m_Desc.TileSize.max;
            if( DrawData.tileSize < rootSize )
            {
                vec3Offset = { DrawData.vecPosition.x - Info.vec3RootPosition.x, 0,
                               Info.vec3RootPosition.z - DrawData.vecPosition.z };
            }
            VKE_ASSERT( vec3Offset.x >= 0 && vec3Offset.z >= 0 && vec3Offset.x <= m_Desc.TileSize.max &&
                            vec3Offset.z <= m_Desc.TileSize.max,
                        "" );
            DrawData.TextureOffset = { ( uint16_t )vec3Offset.x, ( uint16_t )vec3Offset.z };
            DrawData.TextureOffset += Offset;
        }
        void CTerrainQuadTree::_NotifyLOD( const UNodeHandle& hParent, const UNodeHandle& hNode,
                                           const ExtentF32& TopLeftCorner )
        {
            auto& Parent = m_vNodes[ hParent.index ];
            if( Parent.hParent.index != UNDEFINED_U32 )
            {
                _NotifyLOD( Parent.hParent, hNode, TopLeftCorner );
            }
            else
            {
                // auto& Node = m_vNodes[hNode.index];
                SLODData Data;
                Data.lod = LAST_LOD - ( uint8_t )hNode.level;
                Data.DrawData.TextureOffset = TopLeftCorner;
                m_vLODData.PushBack( Data );
            }
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
        CTerrainQuadTree::UNodeHandle CTerrainQuadTree::_FindNode( const SNode& Node,
                                                                   const Math::CVector4& vecPosition ) const
        {
            UNodeHandle Ret = Node.Handle;
            if( Ret.level < MAX_LOD_COUNT )
            {
                const Math::CVector4 vecCenter( Node.AABB.Center );
                const auto idx = _CalcNodeIndex( vecCenter, vecPosition );
                const auto& hNode = Node.ahChildren[ idx ];
                const auto& ChildNode = m_vNodes[ hNode.index ];
                Ret = _FindNode( ChildNode, vecPosition );
            }
            return Ret;
        }
        void CTerrainQuadTree::_FrustumCullRoots( const SViewData& View )
        {
            static const bool disable = DISABLE_FRUSTUM_CULLING;
            if constexpr( !disable )
            {
                const auto& Frustum = View.Frustum;
                for( uint32_t i = 0; i < m_totalRootCount; ++i )
                {
                    const auto& Sphere = m_vBoundingSpheres[ i ];
                    const bool isVisible = Frustum.Intersects( Sphere );
                    m_vNodeVisibility[ i ] = isVisible;
                }
                for( uint32_t i = 0; i < m_totalRootCount; ++i )
                {
                    if( m_vNodeVisibility[ i ] )
                    {
                        const auto& AABB = m_vAABBs[ i ];
                        const bool isVisible = Frustum.Intersects( AABB );
                        m_vNodeVisibility[ i ] = isVisible;
                    }
                }
            }
            // Copy all visible roots to a separate buffer
            m_vVisibleRootNodes.Clear();
            m_vVisibleRootNodeHandles.Clear();
            for( uint32_t i = 0; i < m_totalRootCount; ++i )
            {
                if( m_vNodeVisibility[ i ] )
                {
                    m_vVisibleRootNodes.PushBack( m_vNodes[ i ] );
                    m_vVisibleRootNodeHandles.PushBack( m_vNodes[ i ].Handle );
                }
            }
        }
        
        void CTerrainQuadTree::_FrustumCullChildNodes( const SViewData& View )
        {
            if constexpr( !DISABLE_FRUSTUM_CULLING )
            {
                for( uint32_t i = 0; i < m_vVisibleRootNodes.GetCount(); ++i )
                {
                    const auto& Root = m_vVisibleRootNodes[ i ];
                    SNodeLevel& ChildNodes = m_vChildNodeLevels[ Root.childLevelIndex ];
                    _FrustumCullChildNodes( View, ChildNodes, &ChildNodes.vecVisibility );
                }
            }
        }
        void CTerrainQuadTree::_FrustumCullChildNodes( const SViewData& View, const SNodeLevel& Level,
                                                       Math::CVector4* pOut )
        {
            if constexpr( !DISABLE_FRUSTUM_CULLING )
            {
                Intersects( Level.aAABBCenters, Math::CVector4( Level.boundingSphereRadius ), View.Frustum, pOut );
            }
            else
            {
                pOut->ints[ 0 ] = pOut->ints[ 1 ] = pOut->ints[ 2 ] = pOut->ints[ 3 ] = 1; // set visible always to true
            }
        }
        void CTerrainQuadTree::_BoundingSphereFrustumCull( const SViewData& View )
        {
            // By default sets all nodes to visible
            static constexpr bool Visible = DISABLE_FRUSTUM_CULLING;
            m_vNodeVisibility.Reset( Visible );
            const uint32_t rootCount = m_RootNodeCount.x * m_RootNodeCount.y;
            const Math::CFrustum& Frustum = View.Frustum;
            for( uint32_t i = 0; i < rootCount; ++i )
            {
                auto& Node = m_vNodes[ i ];
                _BoundingSphereFrustumCullNode( Node.Handle, Frustum );
            }
        }
        void CTerrainQuadTree::_FrustumCull( const SViewData& View )
        {
            static const bool disable = DISABLE_FRUSTUM_CULLING;
            if( disable )
                return;
            // First pass: Frustum cull only roots
            const auto& Frustum = View.Frustum;
            for( uint32_t i = 0; i < m_vVisibleRootNodeHandles.GetCount(); ++i )
            {
                _BoundingSphereFrustumCullNode( m_vVisibleRootNodeHandles[ i ], Frustum );
            }
        }
        void CTerrainQuadTree::_BoundingSphereFrustumCullNode( const UNodeHandle& hNode, const Math::CFrustum& Frustum )
        {
            const Math::CBoundingSphere& Sphere = m_vBoundingSpheres[ hNode.index ];
            const bool isVisible = Frustum.Intersects( Sphere );
            m_vNodeVisibility[ hNode.index ] = isVisible;
            if( isVisible )
            {
                const auto& ChildNodes = m_vChildNodeHandles[ hNode.index ];
                if( ChildNodes.aHandles[ 0 ].handle != UNDEFINED_U32 )
                {
                    for( uint32_t i = 0; i < 4; ++i )
                    {
                        _BoundingSphereFrustumCullNode( ChildNodes.aHandles[ i ], Frustum );
                    }
                }
            }
        }
        void CTerrainQuadTree::_UpdateRootNode( const uint32_t& rootNodeIndex, const SUpdateRootNodeData& Data )
        {
            // auto& Node = m_vNodes[rootNodeIndex];
        }
        Rect2D MapAABBToImageSpace( const Math::CAABB& AABB, const Math::CAABB& RootAABB, const ImageSize& ImgSize )
        {
            Math::CVector3 vecNodeTopLeftCorner = { AABB.Center.x - AABB.Extents.x, 1, AABB.Center.z + AABB.Extents.z };
            Math::CVector3 vecRootTopLeftCorner = { RootAABB.Center.x - RootAABB.Extents.x, 1,
                                                    RootAABB.Center.z + RootAABB.Extents.z };
            Math::CVector3 vecWSDistanceToRootTopLeftCorner = vecRootTopLeftCorner - vecNodeTopLeftCorner;
            /*Math::CVector3 vecPixelSize = { ( RootAABB.Extents.x * 2 ) / ImgSize.width, 1,
                                            ( RootAABB.Extents.z * 2 ) / ImgSize.height };*/
            Math::CVector3 vecPixelSize = { ImgSize.width / ( RootAABB.Extents.x * 2 ), 1,
                                            ImgSize.height / ( RootAABB.Extents.z * 2 ) };
            Rect2D Rect;
            Rect.Position.x = ( int32_t )( vecWSDistanceToRootTopLeftCorner.x * vecPixelSize.x );
            Rect.Position.y = ( int32_t )( vecWSDistanceToRootTopLeftCorner.y * vecPixelSize.y );
            Rect.Size.x = ( uint32_t )( ( AABB.Extents.x * 2 ) * vecPixelSize.x );
            Rect.Size.y = ( uint32_t )( ( AABB.Extents.y * 2 ) * vecPixelSize.y );
            return Rect;
        }
        uint32_t GetMaxValueForBPP( uint32_t bpp )
        {
            switch( bpp )
            {
                case 4: return 0xF;
                case 8: return 0xFF;
                case 16: return 0xFFFF;
                case 24: return 0xFFFFFF;
                case 32: return 0xFFFFFFFF;
            };
            return 0;
        }
        ExtentF32 CTerrainQuadTree::CalcChildNodeMinMaxHeight( const Math::CAABB& RootAABB, const ImagePtr pImg,
                                                               CTerrainQuadTree::SNode* pNodeInOut )
        {
            auto& Node = *pNodeInOut;
            ExtentF32 MinMaxHeight( 99999999999999.0f, -99999999999999.0f );
            // Parent nodes to calculate bounding boxes based on bottom level nodes
            if( Node.ahChildren[ 0 ].handle != UNDEFINED_U32 )
            {
                for( uint32_t c = 0; c < 4; ++c )
                {
                    auto& ChildNode = m_vNodes[ Node.ahChildren[ c ].index ];
                    auto MinMax = CalcChildNodeMinMaxHeight( RootAABB, pImg, &ChildNode );
                    MinMaxHeight.min = Math::Min( MinMaxHeight.min, MinMax.min );
                    MinMaxHeight.max = Math::Max( MinMaxHeight.max, MinMax.max );
                }
                float distance = MinMaxHeight.max - MinMaxHeight.min;
                Node.AABB.Extents.y = distance * 0.5f;
                Node.AABB.Center.y = ( MinMaxHeight.min + MinMaxHeight.max ) * 0.5f;
            }
            else if( Node.AABB.Extents.y <= 0 ) // bottom level nodes to calculate bounding boxes
            {
                const uint8_t* pData = pImg->GetData();
                // uint32_t bpp = pImg->GetBitsPerPixel();
                const uint16_t dataStrideSize = pImg->GetBitsPerPixel() / 8; // convert bits to bytes
                const auto& Desc = pImg->GetDesc();
                uint32_t maxValue = GetMaxValueForBPP( pImg->GetBitsPerPixel() );
                uint32_t mask = ( maxValue );
                Rect2D Rect = MapAABBToImageSpace( Node.AABB, RootAABB, Desc.Size );
                const auto& Height = m_pTerrain->m_Desc.Height;
                for( uint32_t y = Rect.Position.y; y < Rect.Size.height; ++y )
                {
                    uint32_t data;
                    float weight;
                    for( uint32_t x = Rect.Position.x; x < Rect.Size.width; ++x )
                    {
                        auto index = Math::Map2DArrayIndexTo1DArrayIndex( x, y, Rect.Size.width );
                        const uint8_t* pCurr = pData + ( index * dataStrideSize );
                        data = *( uint32_t* )pCurr;
                        data &= mask;
                        weight = ( float )data / maxValue;
                        float height = ( float )std::lerp( Height.min, Height.max, weight );
                        MinMaxHeight.min = Math::Min( MinMaxHeight.min, height );
                        MinMaxHeight.max = Math::Max( MinMaxHeight.max, height );
                    }
                }
            }
            return MinMaxHeight;
        }
        void CTerrainQuadTree::_CalcNodeAABB( uint32_t index, const ImagePtr pImg )
        {
            auto& Node = m_vNodes[ index ];
            // Only root nodes
            VKE_ASSERT( Node.hParent.handle == UNDEFINED_U32, "" );
            auto& vNodeHeights = m_vRootNodeHeights[ index ];
            if( vNodeHeights.IsEmpty() )
            {
                // Calc number of smallest tiles for the root node
                vNodeHeights.Resize( m_tileInRootRowCount * m_tileInRootRowCount ); // 2d array as 1d array
                const auto& ImgDesc = pImg->GetDesc();
                const uint8_t* pData = pImg->GetData();
                const uint16_t dataStrideSize = pImg->GetBitsPerPixel() / 8; // convert bits to bytes
                ExtentF32 PixelSize = { ( float )ImgDesc.Size.width / m_Desc.TileSize.max,
                                        ( float )ImgDesc.Size.height / m_Desc.TileSize.max };
                ExtentU32 PixelCount = { ( uint32_t )( m_tileSize * PixelSize.width ),
                                         ( uint32_t )( m_tileSize * PixelSize.height ) };
                uint32_t maxValue = GetMaxValueForBPP( pImg->GetBitsPerPixel() );
                uint32_t mask = ( maxValue );
                for( uint32_t ny = 0; ny < m_tileInRootRowCount; ++ny )
                {
                    for( uint32_t nx = 0; nx < m_tileInRootRowCount; ++nx )
                    {
                        auto nodeIndex = Math::Map2DArrayIndexTo1DArrayIndex( nx, ny, m_tileInRootRowCount );
                        uint32_t data;
                        float weight;
                        ExtentF32 MinMaxHeight = { m_Desc.Height.max, m_Desc.Height.min };
                        for( uint32_t py = 0; py < PixelCount.height; ++py )
                        {
                            for( uint32_t px = 0; px < PixelCount.width; ++px )
                            {
                                ExtentU32 Offset = { px + ( nx * PixelCount.width ), py + ( ny * PixelCount.height ) };
                                auto pixelIdx =
                                    Math::Map2DArrayIndexTo1DArrayIndex( Offset.x, Offset.y, ImgDesc.Size.width );
                                const uint8_t* pCurr = pData + ( pixelIdx * dataStrideSize );
                                data = *( uint32_t* )pCurr;
                                data &= mask;
                                weight = ( float )data / maxValue;
                                float height = ( float )std::lerp( m_Desc.Height.min, m_Desc.Height.max, weight );
                                MinMaxHeight.min = Math::Min( MinMaxHeight.min, height );
                                MinMaxHeight.max = Math::Max( MinMaxHeight.max, height );
                            }
                        }
                        vNodeHeights[ nodeIndex ] = MinMaxHeight;
                    }
                }
            }
        }
        ExtentU32 CTerrainQuadTree::_FindMinMax( const ImagePtr pImg, const Rect2D& Rect )
        {
            ExtentU32 MinMax = { UINT_MAX, 0 };
            for( uint32_t y = Rect.Position.y; y < Rect.Size.height; ++y )
            {
                for( uint32_t x = Rect.Position.x; x < Rect.Size.width; ++x )
                {
                }
            }
            return MinMax;
        }

        void CTerrainQuadTree::_InitMainRoots( const SViewData& View )
        {
            // Sort nodes by distance
            const auto& vecViewPosition = View.vecPosition;
            {
                VKE_PROFILE_SIMPLE2( "sort" ); // 140 us
                std::sort( &m_vVisibleRootNodes[ 0 ], &m_vVisibleRootNodes[ 0 ] + m_vVisibleRootNodes.GetCount(),
                           [ & ]( const SNode& Left, const SNode& Right ) {
                               const float leftDistance = Math::CVector3::Distance( Left.AABB.Center, vecViewPosition );
                               const float rightDistance =
                                   Math::CVector3::Distance( Right.AABB.Center, vecViewPosition );
                               return leftDistance < rightDistance;
                           } );
            }
            SInitChildNodesInfo ChildNodeInfo = m_FirstLevelNodeBaseInfo;
            {
                VKE_PROFILE_SIMPLE2( "reset child nodes" ); // 1 us
                _ResetChildNodes();
            }
            {
                VKE_PROFILE_SIMPLE2( "init child nodes" ); // 400 us
                const uint32_t count = Math::Min( m_vVisibleRootNodes.GetCount(), 4 );
                for( uint32_t i = 0; i < count; ++i )
                {
                    SNode& Root = m_vVisibleRootNodes[ i ];
                    ChildNodeInfo.hParent = Root.Handle;
                    ChildNodeInfo.vec4ParentCenter = Root.AABB.Center;
                    ChildNodeInfo.childNodeStartIndex = _AcquireChildNodes();
                    Root.childLevelIndex = ChildNodeInfo.childNodeStartIndex;
                    _InitChildNodes( ChildNodeInfo );
                }
            }
        }
        void CTerrainQuadTree::_InitChildNodes( const SInitChildNodesInfo& Info )
        {
            static const Math::CVector4 aVectors[ 4 ] = { { -1.0f, 0.0f, 1.0f, 0.0f },
                                                          { 1.0f, 0.0f, 1.0f, 0.0f },
                                                          { -1.0f, 0.0f, -1.0f, 0.0f },
                                                          { 1.0f, 0.0f, -1.0f, 0.0f } };
            // Info for children of children
            SInitChildNodesInfo ChildOfChildInfo;
            const auto currLevel = Info.hParent.level;
            const uint8_t childNodeLevel = ( uint8_t )currLevel + 1;
            // if (childNodeLevel < Info.maxLODCount)
            VKE_ASSERT( childNodeLevel < Info.maxLODCount, "" );
            {
                Math::CVector4 vecChildCenter;
                UNodeHandle ahChildNodes[ 4 ];
                const uint32_t parentIndex = Info.hParent.index;
                SNode& Parent = m_vNodes[ Info.hParent.index ];
                ChildOfChildInfo.vec4Extents = Info.vec4Extents * 0.5f;
                ChildOfChildInfo.vecExtents = Math::CVector3( ChildOfChildInfo.vec4Extents );
                ChildOfChildInfo.boundingSphereRadius = Info.boundingSphereRadius * 0.5f;
                ChildOfChildInfo.maxLODCount = Info.maxLODCount;
                {
                    VKE_PROFILE_SIMPLE2( "create child nodes for parent" ); // 6 us
                                                                            // Create child nodes
                                                                            // for parent
                    for( uint8_t i = 0; i < 4; ++i )
                    {
                        UNodeHandle Handle;
                        Handle.index = Info.childNodeStartIndex + i;
                        Handle.level = childNodeLevel;
                        Handle.childIdx = i;
                        auto& Node = m_vNodes[ Handle.index ];
                        Node.Handle = Handle;
                        Node.hParent = Info.hParent;
                        Node.ahChildren[ 0 ].handle = UNDEFINED_U32;
                        Node.ahChildren[ 1 ].handle = UNDEFINED_U32;
                        Node.ahChildren[ 2 ].handle = UNDEFINED_U32;
                        Node.ahChildren[ 3 ].handle = UNDEFINED_U32;
                        Math::CVector4::Mad( aVectors[ i ], Info.vec4Extents, Info.vec4ParentCenter, &vecChildCenter );
                        Node.AABB = Math::CAABB( Math::CVector3{ vecChildCenter }, Info.vecExtents );
                        Node.boundingSphereRadius = Info.boundingSphereRadius;
                        // Set this node as a child for a parent
                        // m_vNodes[hParent.index].ahChildren[i] = Handle;
                        Parent.ahChildren[ i ] = Handle;
                        ahChildNodes[ i ] = Handle;
                        {
                            VKE_PROFILE_SIMPLE2( "store child data" ); // 1.5 us
                            m_vAABBs[ Handle.index ] = Node.AABB;
                            m_vBoundingSpheres[ Handle.index ] =
                                Math::CBoundingSphere( Node.AABB.Center, Node.boundingSphereRadius );
                            m_vChildNodeHandles[ parentIndex ][ i ] = Handle;
                        }
                        _SetDrawDataForNode( &Node );
                    }
                }
                if( childNodeLevel + 1 < Info.maxLODCount )
                {
                    VKE_PROFILE_SIMPLE2( "create children of children" );
                    for( uint8_t i = 0; i < 4; ++i )
                    {
                        const auto& hNode = ahChildNodes[ i ];
                        auto& Node = m_vNodes[ hNode.index ];
                        ChildOfChildInfo.vec4ParentCenter = Node.AABB.Center;
                        ChildOfChildInfo.hParent = Node.Handle;
                        ChildOfChildInfo.childNodeStartIndex = _AcquireChildNodes();
                        _InitChildNodes( ChildOfChildInfo );
                    }
                }
            }
        }
        Result CTerrainQuadTree::_CreateChildNodes( UNodeHandle hParent, const SCreateNodeData& NodeData,
                                                    const uint8_t lodCount )
        {
            static const Math::CVector4 aVectors[ 4 ] = { { -1.0f, 0.0f, 1.0f, 0.0f },
                                                          { 1.0f, 0.0f, 1.0f, 0.0f },
                                                          { -1.0f, 0.0f, -1.0f, 0.0f },
                                                          { 1.0f, 0.0f, -1.0f, 0.0f } };
            // auto& Parent = m_vNodes[ hParent.index ];
            Result res = VKE_OK;
            const auto currLevel = NodeData.level;
            if( currLevel < lodCount )
            {
                SCreateNodeData ChildNodeData;
                ChildNodeData.vec4Extents = NodeData.vec4Extents * 0.5f;
                ChildNodeData.vecExtents = Math::CVector3( ChildNodeData.vec4Extents );
                ChildNodeData.boundingSphereRadius = NodeData.boundingSphereRadius * 0.5f;
                ChildNodeData.level = currLevel + 1;
                Math::CVector4 vecChildCenter;
                UNodeHandle ahChildNodes[ 4 ];
                const uint32_t childNodeStartIdx = _AcquireChildNodes();
                // Create child nodes for parent
                for( uint8_t i = 0; i < 4; ++i )
                {
                    UNodeHandle Handle;
                    // Handle.index = m_vNodes.PushBack({});
                    Handle.index = childNodeStartIdx + i;
                    Handle.level = currLevel;
                    Handle.childIdx = i;
                    auto& Node = m_vNodes[ Handle.index ];
                    Node.Handle = Handle;
                    Node.hParent = NodeData.hParent;
                    Node.ahChildren[ 0 ].handle = UNDEFINED_U32;
                    Node.ahChildren[ 1 ].handle = UNDEFINED_U32;
                    Node.ahChildren[ 2 ].handle = UNDEFINED_U32;
                    Node.ahChildren[ 3 ].handle = UNDEFINED_U32;
                    Math::CVector4::Mad( aVectors[ i ], NodeData.vec4Extents, NodeData.vec4ParentCenter,
                                         &vecChildCenter );
                    Node.AABB = Math::CAABB( Math::CVector3{ vecChildCenter }, NodeData.vecExtents );
                    Node.boundingSphereRadius = NodeData.boundingSphereRadius;
                    // Set this node as a child for a parent
                    m_vNodes[ hParent.index ].ahChildren[ i ] = Handle;
                    ahChildNodes[ i ] = Handle;
                    m_vAABBs[ Handle.index ] = Node.AABB;
                    m_vBoundingSpheres[ Handle.index ] =
                        Math::CBoundingSphere( Node.AABB.Center, Node.boundingSphereRadius );
                    m_vChildNodeHandles[ hParent.index ][ i ] = Handle;
                    _SetDrawDataForNode( &Node );
                }
                for( uint8_t i = 0; i < 4; ++i )
                {
                    const auto& hNode = ahChildNodes[ i ];
                    auto& Node = m_vNodes[ hNode.index ];
                    ChildNodeData.vec4ParentCenter = Node.AABB.Center;
                    ChildNodeData.hParent = Node.Handle;
                    res = _CreateChildNodes( hNode, ChildNodeData, lodCount );
                }
            }
            return res;
        }
        void CTerrainQuadTree::_CalcErrorLODs( const SNode& CurrNode, const uint32_t& textureIdx,
                                               const SViewData& View )
        {
            const auto hCurrNode = CurrNode.Handle;
            const auto& AABB = CurrNode.AABB;
            const Math::CVector3 vecTmpPos = AABB.Center - AABB.Extents;
            // const bool b = AABB.Center.x == 96 && AABB.Center.z == 96;
            //  Instead of a regular bounding sphere radius use size of
            //  AABB.Extents size This approach fixes wrong calculations for node
            //  containing the view point Note that a node is a quad
            Math::CVector4 vecPoint;
            // const float boundingSphereRadius2 = CurrNode.boundingSphereRadius;
            const float boundingSphereRadius = AABB.Extents.x;
            CalcNearestSpherePoint( Math::CVector4( AABB.Center ), boundingSphereRadius,
                                    Math::CVector4( View.vecPosition ), &vecPoint );
            float err, distance;
            _CalcError( vecPoint, hCurrNode.level, View, &err, &distance );
            // float childErr, childDistance;
            static cstr_t indents[] = {
                "",        " ",        "  ",        "   ",        "    ",        "     ",        "      ",
                "       ", "        ", "         ", "          ", "           ", "            ", "             ",
            };
            //  Smallest tiles has no children
            const bool hasChildNodes = CurrNode.ahChildren[ 0 ].handle != UNDEFINED_U32;
            // note, level = 0 == root node. Root nodes has no DrawData
            // specified. Root nodes must not be added to draw path
            if( ( err > g_lodError && hasChildNodes ) /*|| hCurrNode.level == 0*/ )
            {
                {
                    for( uint32_t i = 0; i < 4; ++i )
                    {
                        const auto hNode = CurrNode.ahChildren[ i ];
                        const auto& ChildNode = m_vNodes[ hNode.index ];
                        const auto& AABB2 = ChildNode.AABB;
                        const Math::CVector3 vecTmpPos2 = AABB2.Center - AABB2.Extents;
                        if( m_vNodeVisibility[ hNode.index ] )
                        {
                            _CalcErrorLODs( ChildNode, textureIdx, View );
                        }
                    }
                }
            }
            else // if( hCurrNode.level > 0 )
            {
                SLODInfo Info;
                Info.nodeLevel = ( uint8_t )hCurrNode.level;
                Info.vec4Center = AABB.Center;
                Info.nodeExtents = AABB.Extents.x;
                _AddLOD( Info );
            }
        }
        void CTerrainQuadTree::_CalcDistanceLODs( const SNode& CurrNode, const uint32_t& textureIdx,
                                                  const SViewData& View )
        {
            const auto hCurrNode = CurrNode.Handle;
            const auto& AABB = CurrNode.AABB;
            Math::CVector4 vecPoint;
            CalcNearestSpherePoint( Math::CVector4( AABB.Center ), CurrNode.boundingSphereRadius,
                                    Math::CVector4( View.vecPosition ), &vecPoint );
            //_CalcError( vecPoint, hCurrNode.level, View, &err, &distance );
            // float distance = _CalcDistanceToCenter( AABB.Center, View );
            float distance = Math::CVector4::Distance( vecPoint, Math::CVector4( View.vecPosition ) );
            // float childErr, childDistance;
            static cstr_t indents[] = {
                "",        " ",        "  ",        "   ",        "    ",        "     ",        "      ",
                "       ", "        ", "         ", "          ", "           ", "            ", "             ",
            };
            /*VKE_DBG_LOG( "" << indents[ hCurrNode.level ] << "l: " <<
            hCurrNode.level << " idx: " << hCurrNode.index << " d: " << distance
            << " e: " << err << " c: " << AABB.Center.x << ", " << AABB.Center.z
            << " p: " << vecPoint.x << ", " << vecPoint.z << " vp: " <<
            CurrNode.DrawData.vecPosition.x << ", " <<
            CurrNode.DrawData.vecPosition.z <<
            " s:" << CurrNode.AABB.Extents.x << ", " << CurrNode.AABB.Extents.z
            << "\n" );*/
            const uint8_t highestLod = ( uint8_t )( m_Desc.lodCount - 1 );
            // Smallest tiles has no children
            const bool hasChildNodes = CurrNode.ahChildren[ 0 ].handle != UNDEFINED_U32;
            const bool isRoot = CurrNode.hParent.handle == UNDEFINED_U32;
            // If node is in acceptable distance check its child nodes
            // const uint32_t maxDistance = (uint32_t)m_Desc.maxViewDistance;
            // Calc distance in tile size number
            // 1st distance range == 1 * tile size
            // 2nd distance range = 2 * tile size
            // 3rd distance range = 4 * tile size
            // Nth distance range = n * tile size
            // Calc lod based on distance. Use tile size as counter
            const uint32_t lod = Math::Min( ( ( uint32_t )distance / m_tileSize - 1 ), highestLod );
            // Check if current nod level matches distance lod
            // note, level = 0 == root node. Root nodes has no DrawData
            // specified. Root nodes must not be added to draw path
            // if( ( err > 60 && !hasChildNodes ) || hCurrNode.level == 0 )
            const uint8_t tileLod = highestLod - ( uint8_t )hCurrNode.level;
            if( ( lod < tileLod && hasChildNodes ) || isRoot )
            {
                // Parent has always 0 or 4 children
                // if( CurrNode.ahChildren[ 0 ].handle != UNDEFINED_U32 )
                {
                    // LOG child
                    /*for( uint32_t i = 0; i < 4; ++i )
                    {
                    const auto hNode = CurrNode.ahChildren[ i ];
                    {
                    const auto& Node = m_vNodes[ hNode.index ];
                    CalcNearestSpherePoint( Math::CVector4( Node.AABB.Center ),
                    Node.boundingSphereRadius, Math::CVector4( View.vecPosition
                    ), &vecPoint ); _CalcError( vecPoint, hNode.level, View,
                    &childErr, &childDistance );

                    VKE_DBG_LOG( "" << indents[ hNode.level ] << "l: " <<
                    hNode.level << " idx: " << hNode.index << " d: " <<
                    childDistance << " e: " << childErr << " c: " <<
                    Node.AABB.Center.x << ", " << Node.AABB.Center.z << " p: "
                    << vecPoint.x << ", " << vecPoint.z << "\n" );
                    }
                    }*/
                    for( uint32_t i = 0; i < 4; ++i )
                    {
                        const auto hNode = CurrNode.ahChildren[ i ];
                        const auto& ChildNode = m_vNodes[ hNode.index ];
                        // if( ChildNode.ahChildren[ 0 ].handle != UNDEFINED_U32
                        // )
                        {
                            _CalcDistanceLODs( ChildNode, textureIdx, View );
                        }
                    }
                }
            }
            else // if( hCurrNode.level > 0 )
            {
                /*SLODData Data;
                Data.lod = highestLod - ( uint8_t )hCurrNode.level;
                VKE_ASSERT( CurrNode.DrawData.pPipeline.IsValid(), "" );
                Data.DrawData = CurrNode.DrawData;
                Data.idx = MapPositionTo1DArrayIndex( Data.DrawData.vecPosition,
                m_tileSize, m_terrainHalfSize, m_tileInRowCount );

                SLODInfo Info;
                Info.nodeLevel = (uint8_t)hCurrNode.level;

                _AddLOD( Info );*/
                VKE_ASSERT( false, "not implemented" );
            }
        }
    } // namespace Scene
} // namespace VKE