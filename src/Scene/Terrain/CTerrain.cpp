#include "Scene/Terrain/CTerrain.h"
#include "Scene/Terrain/CTerrainVertexFetchRenderer.h"
#include "Scene/Terrain/CTerrainMeshShaderRenderer.h"

#include "Core/Math/Math.h"

#include "Scene/CScene.h"
#include "CVkEngine.h"
#include "Core/Managers/CImageManager.h"

#include "Core/Utils/CProfiler.h"

#define DEBUG_LOD_STITCH_MAP 0
#define INIT_CHILD_NODES_FOR_EACH_ROOT 0
#define DISABLE_FRUSTUM_CULLING 1
#define VKE_PROFILE_TERRAIN 0
#define VKE_PROFILER_TERRAIN_UPDATE 0

namespace VKE
{
    namespace Scene
    {
        void CTerrain::_Destroy()
        {
            if( m_pRenderer != nullptr )
            {
                _DestroyRenderer( &m_pRenderer );
            }
            m_QuadTree._Destroy();
        }

        uint32_t CalcMaxVisibleTiles(const STerrainDesc& Desc)
        {
            uint32_t ret = 0;
            // maxViewDistance is a radius so 2x maxViewDistance == a diameter
            // Using diameter horizontally and vertically we get a square
            // All possible visible tiles should cover this square
            float dimm = Desc.maxViewDistance * 2;
            //float tileDimm = Desc.tileRowVertexCount * Desc.vertexDistance;
            float tileDimm = Desc.tileSize;
            float tileCount = dimm / tileDimm;
            ret = (uint32_t)ceilf(tileCount * tileCount);
            return ret;
        }

        bool CTerrain::CheckDesc(const STerrainDesc& Desc) const
        {
            bool ret = true;
            // Tile size must be pow of 2
            if( !Math::IsPow2( Desc.tileSize ) )
            {
                ret = false;
            }
            // vertexDistance
            if(  Desc.vertexDistance < 1.0f )
            {
                // Below 1.0 acceptable values are 0.125, 0.25, 0.5
                // because next LODs must go to 1, 2, 4, 8 etc values to be power of 2
                if( Desc.vertexDistance != 0.125f || Desc.vertexDistance != 0.25f || Desc.vertexDistance != 0.5f )
                {
                    ret = false;
                }
            }
            else if( !Math::IsPow2( (uint32_t)Desc.vertexDistance ) )
            {
                // if above 1.0 vertexDistance must be power of 2
                ret = false;
            }
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

           // m_tileSize = (uint32_t)((float)(Desc.tileRowVertexCount) * Desc.vertexDistance);
            m_tileVertexCount = (uint16_t)((float)Desc.tileSize / Desc.vertexDistance);
            // Round up terrain size to pow 2
            m_Desc.size = Math::CalcNextPow2( Desc.size );
            m_maxTileCount = (uint16_t)(m_Desc.size / Desc.tileSize);
            // Number of tiles must be power of two according to LODs
            // Each lod is 2x bigger
            //m_maxTileCount = Math::CalcNextPow2(m_maxTileCount);
            //m_Desc.size = m_maxTileCount * m_tileSize;
            m_halfSize = m_Desc.size / 2;

            CTerrainQuadTree::SCalcTerrainInfo CalcInfo;
            CalcInfo.pDesc = &m_Desc;
            CalcInfo.maxLODCount = CTerrainQuadTree::MAX_LOD_COUNT;
            CalcInfo.maxRootSize = CTerrainQuadTree::MAX_ROOT_SIZE;
            CTerrainQuadTree::_CalcTerrainInfo(CalcInfo, &m_TerrainInfo);

            ///ExtentU8 LODCount = CTerrainQuadTree::CalcLODCount( m_Desc, m_maxHeightmapSize, CTerrainQuadTree::MAX_LOD_COUNT );
            m_Desc.lodCount = m_TerrainInfo.maxLODCount;

            m_maxTileCount *= m_maxTileCount;
            m_maxVisibleTiles = Math::Min(m_maxTileCount, (uint16_t)CalcMaxVisibleTiles(m_Desc));
            m_vecExtents = Math::CVector3(m_Desc.size * 0.5f);
            m_vecExtents.y = (m_Desc.Height.min + m_Desc.Height.max) * 0.5f;

            m_avecCorners[0] = Math::CVector3(m_Desc.vecCenter.x - m_vecExtents.x, m_Desc.vecCenter.y, m_Desc.vecCenter.z + m_vecExtents.z);
            m_avecCorners[1] = Math::CVector3(m_Desc.vecCenter.x + m_vecExtents.x, m_Desc.vecCenter.y, m_Desc.vecCenter.z + m_vecExtents.z);
            m_avecCorners[2] = Math::CVector3(m_Desc.vecCenter.x - m_vecExtents.x, m_Desc.vecCenter.y, m_Desc.vecCenter.z - m_vecExtents.z);
            m_avecCorners[3] = Math::CVector3(m_Desc.vecCenter.x + m_vecExtents.x, m_Desc.vecCenter.y, m_Desc.vecCenter.z - m_vecExtents.z);

            m_QuadTree.m_pTerrain = this;

            if (VKE_SUCCEEDED(_LoadTextures(pCtx)))
            {
                if (VKE_SUCCEEDED(m_pRenderer->_Create(m_Desc, pCtx)))
                {
                    if (VKE_SUCCEEDED(m_QuadTree._Create(m_Desc)))
                    {
                        ret = VKE_OK;
                    }
                    else
                    {
                        goto ERR;
                    }
                }
                else
                {
                    goto ERR;
                }
            }
            else
            {
                goto ERR;
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
            *ppInOut = pRenderer;
        }

        Result CTerrain::_LoadTextures(RenderSystem::CDeviceContext* pCtx)
        {
            Result ret = VKE_FAIL;

            Core::SLoadFileInfo Info;
            Info.FileInfo.pFileName = m_Desc.Heightmap.pFileName;
            Info.CreateInfo.async = false;
            m_hHeightmapTexture = pCtx->LoadTexture( Info );

            if(m_hHeightmapTexture != INVALID_HANDLE )
            {
                m_hHeigtmapTexView = pCtx->GetTexture(m_hHeightmapTexture)->GetView()->GetHandle();
                RenderSystem::SSamplerDesc SamplerDesc;
                SamplerDesc.Filter.min = RenderSystem::SamplerFilters::LINEAR;
                SamplerDesc.Filter.mag = RenderSystem::SamplerFilters::LINEAR;
                SamplerDesc.mipmapMode = RenderSystem::MipmapModes::LINEAR;
                SamplerDesc.AddressMode.U = RenderSystem::AddressModes::CLAMP_TO_BORDER;
                SamplerDesc.AddressMode.V = SamplerDesc.AddressMode.U;
                m_hHeightmapSampler = pCtx->CreateSampler(SamplerDesc);
                pCtx->GetGraphicsContext(0)->SetTextureState( RenderSystem::TextureStates::SHADER_READ, &m_hHeightmapTexture);

                for (uint32_t i = 0; i < MAX_HEIGHTMAP_TEXTURE_COUNT; ++i)
                {
                    m_ahHeightmapTextureViews[i] = m_hHeigtmapTexView;
                }

                ret = VKE_OK;
            }
            return ret;
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

        uint32_t vke_force_inline CalcTileCountForLod( uint8_t startLOD, uint8_t LODCount )
        {
            uint32_t ret = 0;
            for( uint8_t i = startLOD; i < LODCount; ++i )
            {
                ret += (uint32_t)std::pow(4, i);
            }
            return ret;
        }

        uint16_t CalcRootCount(const STerrainDesc& Desc, uint16_t maxHeightmapSize)
        {
            // Calc number of roots based on max heightmap texture size
            // 1 heightmap texel == 1 vertex position
            // Minimum root count is 2x2
            uint32_t rootCount = Desc.size / maxHeightmapSize;
            rootCount = (uint32_t)Math::Max(2u, rootCount);
            // Must be power of 2
            rootCount = (uint32_t)Math::CalcNextPow2(rootCount);

            return (uint16_t)rootCount;
        }

        float CalcWorldSpaceError(const float vertexDistance, const uint8_t nodeLevel, const uint8_t levelCount);

        ExtentU8 CTerrainQuadTree::CalcLODCount(const STerrainDesc& Desc, uint16_t maxHeightmapSize,
            uint8_t maxLODCount)
        {
            ExtentU8 Ret;
            uint32_t rootCount = CalcRootCount(Desc, maxHeightmapSize);
            uint32_t rootSize = Desc.size / rootCount;
            // Calculate absolute lod
            // tileSize is a max lod
            // start from root level
            uint8_t maxLOD = 0;
            for (uint32_t size = rootSize; size >= Desc.tileSize; size >>= 1, maxLOD++)
            {
            }
            // Now remove from maxLOD number of LOD for roots
            // for one only root it is lod = 0
            // 2x2 roots lod = 1
            // 4x4 roots lod = 2
            // 8x8 roots lod = 3
            // Check on which position there is 1 bit set
            uint8_t rootLevel = 0;
            const auto v = rootCount >> rootLevel;
            for( ; rootCount >> rootLevel != 1; ++rootLevel)
            {

            }
            Ret.max = maxLOD;
            Ret.min = rootLevel;
            return Ret;
        }

        uint32_t CalcTileCountForRoot(uint8_t LODCount)
        {
            uint32_t ret = 0;
            for (uint8_t i = 0; i < LODCount; ++i)
            {
                ret += (uint32_t)std::pow(4, i);
            }
            return ret;
        }

        void CTerrainQuadTree::_CalcTerrainInfo(const SCalcTerrainInfo& Info, STerrainInfo* pOut)
        {
            const auto& Desc = *Info.pDesc;
            const auto LODCount = CalcLODCount( *Info.pDesc, (uint16_t)Info.maxRootSize, Info.maxLODCount );
            pOut->maxLODCount = LODCount.max;
            //pOut->rootLOD = LODCount.min;

            // Calculate total root count
            // Terrain is a quad
            {
                pOut->RootCount.x = pOut->RootCount.y = CalcRootCount(Desc, (uint16_t)Info.maxRootSize);
            }
            const uint32_t totalRootCount = pOut->RootCount.width * pOut->RootCount.height;
            // Calc root size
            const uint32_t rootSize = (uint32_t)Math::CalcPow2(pOut->maxLODCount - 1) * Desc.tileSize;

            // Calc max visible roots
            {
                uint32_t viewDistance = Math::Max( rootSize, (uint32_t)Desc.maxViewDistance );
                if (viewDistance == 0) // if not defined
                {
                    // Whole terrain is visible
                    viewDistance = Desc.size;
                }

                // Calc how many roots are visible at max distance
                // Visible are all roots in view distance range
                const uint32_t visibleRootCount = (uint32_t)( viewDistance / rootSize ) * 2; // get all roots along whole quad
                pOut->maxVisibleRootCount = Math::Min( totalRootCount, visibleRootCount * visibleRootCount ); // make it a quad
            }
            // Calc total tile count for one root
            {
                const uint32_t nodeCount = CalcTileCountForRoot(pOut->maxLODCount);
                pOut->tileCountForRoot = nodeCount;
            }
            // Calc total level count for child nodes for root
            {
                // Each node level == 4 nodes
                // l 0 (root) == 1
                pOut->childLevelCountForRoot = CalcTileCountForRoot(pOut->maxLODCount - 1);
            }
            // Calc max node count for all roots
            // Note for memoy and time efficiency only 2x2 roots contains nodes for all LODs
            {
                const uint32_t nodeCountFor2x2 = pOut->tileCountForRoot * 4;
                pOut->maxNodeCount = totalRootCount + nodeCountFor2x2 - 4; // totalRootCount contains all roots
            }
        }

        Result CTerrainQuadTree::_Create(const STerrainDesc& Desc)
        {
            Result res = VKE_FAIL;

            m_Desc = Desc;
            const STerrainInfo& Info = m_pTerrain->m_TerrainInfo;

            // Copy these to avoid cache missess
            m_terrainHalfSize = m_pTerrain->m_halfSize;
            m_tileSize = Desc.tileSize;
            m_tileInRowCount = (uint16_t)( m_terrainHalfSize / m_tileSize ) * 2;

            m_totalRootCount = (uint16_t)(Info.RootCount.width * Info.RootCount.height);
            m_maxLODCount = Info.maxLODCount;

            // Set all tile lods to 0 to avoid vertex shifting
            m_vLODMap.Resize( m_tileInRowCount * m_tileInRowCount, 0 );

            // This quadtree is made of rootNodeCount quadTrees.
            // Each 'sub' quadtree root contains original heightmap texture
            //const auto rootRowCount = CalcRootCount(Desc, m_pTerrain->m_maxHeightmapSize);
            //m_RootNodeCount = {rootRowCount, rootRowCount};
            m_RootNodeCount = Info.RootCount;

            const auto vecMinSize = m_Desc.vecCenter - m_pTerrain->m_vecExtents;
            const auto vecMaxSize = m_Desc.vecCenter + m_pTerrain->m_vecExtents;

            if(!m_vTextureIndices.Resize(m_RootNodeCount.x * m_RootNodeCount.y))
            {
                res = VKE_FAIL;
            }

            //ExtentU8 LODCount = CalcLODCount(Desc, m_pTerrain->m_maxHeightmapSize, MAX_LOD_COUNT);
            //uint32_t nodeCount = CalcTileCountForLod( LODCount.min, LODCount.max );
            const uint32_t nodeCount = Info.maxNodeCount;

            const bool nodeDataReady = m_vNodes.Resize(nodeCount) && m_vLODData.Reserve(nodeCount) &&
                m_vNodeVisibility.Resize( nodeCount, true ) && m_vBoundingSpheres.Resize( nodeCount ) &&
                m_vAABBs.Resize( nodeCount ) && m_vChildNodeHandles.Resize( nodeCount ) &&
                m_vVisibleRootNodes.Reserve(m_totalRootCount) && m_vVisibleRootNodeHandles.Reserve(m_totalRootCount) &&
                m_vChildNodeLevels.Resize(Info.childLevelCountForRoot * MAIN_ROOT_COUNT);

            if (m_vvLODData.Resize(8))
            {
                res = VKE_OK;
            }
            for (uint32_t i = 0; i < 1; ++i)
            {
                m_vvLODData[i].Reserve(nodeCount);
            }

            if( nodeDataReady )
            {
                const auto vecRootNodeExtents = m_pTerrain->m_vecExtents / Math::CVector3( m_RootNodeCount.x, 1.0f, m_RootNodeCount.y );
                const auto vecRootNodeSize = vecRootNodeExtents * 2.0f;
                const auto RootAABB = Math::CAABB(m_Desc.vecCenter, vecRootNodeExtents);
                // Node is a square so bounding sphere radius is a diagonal
                const float boundingSphereRadius = (std::sqrtf( 2.0f ) * vecRootNodeExtents.x );
                const float boundingSphereRadius2 = vecRootNodeExtents.x;
                Math::CVector3 vecRootNodeCenter;

                uint32_t currRootIdx = 0;
                for( uint16_t z = 0; z < m_RootNodeCount.y; ++z )
                {
                    for( uint16_t x = 0; x < m_RootNodeCount.x; ++x )
                    {
                        // Create root
                        UNodeHandle Handle;
                        //Handle.index = m_vNodes.PushBack( {} );
                        Handle.index = currRootIdx++;
                        Handle.childIdx = 0;
                        Handle.level = 0; // always iterate from 0 even if root starts lower (1, 2, etc)
                        auto& Node = m_vNodes[ Handle.index ];
                        Node.hParent.handle = UNDEFINED_U32;
                        Node.Handle = Handle;
                        Node.boundingSphereRadius = boundingSphereRadius;
                        const float minX = vecMinSize.x + vecRootNodeSize.x * x;
                        const float minZ = vecMinSize.z + vecRootNodeSize.z * z;
                        vecRootNodeCenter.x = minX + vecRootNodeExtents.x;
                        vecRootNodeCenter.y = 0;
                        vecRootNodeCenter.z = minZ + vecRootNodeExtents.z;

                        Node.AABB = Math::CAABB( vecRootNodeCenter, vecRootNodeExtents );
                        m_vAABBs[Handle.index] = Node.AABB;
                        m_vBoundingSpheres[Handle.index] = Math::CBoundingSphere( Node.AABB.Center, Node.boundingSphereRadius );
                    }
                }

                const auto& TmpRoot = m_vNodes[0];
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
                    //res = _CreateChildNodes( Root.Handle, NodeData, m_Desc.lodCount );
#if INIT_CHILD_NODES_FOR_EACH_ROOT
                    SInitChildNodesInfo NodeInfo;
                    NodeInfo.boundingSphereRadius = NodeData.boundingSphereRadius;
                    NodeInfo.childNodeStartIndex = _AcquireChildNodes();
                    NodeInfo.hParent = Root.Handle;
                    NodeInfo.maxLODCount = m_maxLODCount;
                    NodeInfo.vec4Extents = NodeData.vec4Extents;
                    NodeInfo.vecExtents = NodeData.vecExtents;
                    NodeInfo.vec4ParentCenter = NodeData.vec4ParentCenter;
                    _InitChildNodes(NodeInfo);
#endif
                }
            }

            return res;
        }

        void CTerrainQuadTree::_InitMainRoots(const SViewData& View)
        {
            // Sort nodes by distance
            const auto& vecViewPosition = View.vecPosition;

            {
                VKE_PROFILE_SIMPLE2("sort"); //140 us
                std::sort(&m_vVisibleRootNodes[0], &m_vVisibleRootNodes[0] + m_vVisibleRootNodes.GetCount(),
                    [&](const SNode& Left, const SNode& Right)
                {
                    const float leftDistance = Math::CVector3::Distance(Left.AABB.Center, vecViewPosition);
                    const float rightDistance = Math::CVector3::Distance(Right.AABB.Center, vecViewPosition);
                    return leftDistance < rightDistance;
                });
            }
            SInitChildNodesInfo ChildNodeInfo = m_FirstLevelNodeBaseInfo;
            {
                VKE_PROFILE_SIMPLE2("reset child nodes"); // 1 us
                _ResetChildNodes();
            }
            {
                VKE_PROFILE_SIMPLE2("init child nodes"); // 400 us
                for (uint32_t i = 0; i < 4; ++i)
                {
                    SNode& Root = m_vVisibleRootNodes[i];
                    ChildNodeInfo.hParent = Root.Handle;
                    ChildNodeInfo.vec4ParentCenter = Root.AABB.Center;
                    ChildNodeInfo.childNodeStartIndex = _AcquireChildNodes();
                    Root.childLevelIndex = ChildNodeInfo.childNodeStartIndex;
                    _InitChildNodes(ChildNodeInfo);
                }
            }
        }

        void CTerrainQuadTree::_InitMainRootsSIMD(const SViewData& View)
        {
            // Sort nodes by distance
            const auto& vecViewPosition = View.vecPosition;

            {
                //VKE_PROFILE_SIMPLE2("sort"); //140 us
                std::sort(&m_vVisibleRootNodes[0], &m_vVisibleRootNodes[0] + m_vVisibleRootNodes.GetCount(),
                    [&](const SNode& Left, const SNode& Right)
                {
                    const float leftDistance = Math::CVector3::Distance(Left.AABB.Center, vecViewPosition);
                    const float rightDistance = Math::CVector3::Distance(Right.AABB.Center, vecViewPosition);
                    return leftDistance < rightDistance;
                });
            }
            //SInitChildNodesInfo ChildNodeInfo = m_FirstLevelNodeBaseInfo;
            {
                //VKE_PROFILE_SIMPLE2("reset child nodes"); // 1 us
                _ResetChildNodeLevels();
            }
            {
                // VKE_PROFILE_SIMPLE2("init child nodes"); // 400 us
                SInitChildNodeLevel LevelInit;
                //LevelInit.maxLODCount = m_FirstLevelNodeBaseInfo.maxLODCount;
                //LevelInit.parentBoundingSphereRadius = m_FirstLevelNodeBaseInfo.boundingSphereRadius;
                //LevelInit.parentLevel = 0;

                for (uint32_t i = 0; i < 4; ++i)
                {
                    auto& Root = m_vVisibleRootNodes[i];
                    LevelInit.parentLevel = 0;
                    LevelInit.parentBoundingSphereRadius = Root.boundingSphereRadius;
                    LevelInit.maxLODCount = m_maxLODCount;
                    LevelInit.vecParentSizes.x = Root.AABB.Center.x;
                    LevelInit.vecParentSizes.y = Root.AABB.Center.z;
                    LevelInit.vecParentSizes.z = Root.AABB.Extents.x; // width == depth
                    LevelInit.parentLevel = Root.Handle.level;
                    LevelInit.childLevelIndex = _AcquireChildNodeLevel();
                    Root.childLevelIndex = LevelInit.childLevelIndex;
                    _InitChildNodesSIMD(LevelInit);
                }
            }
            /*SNodeLevel Level;
            Level.childLevelIndex = _AcquireChildNodeLevel();
            for (uint32_t i = 0; i < 4; ++i)
            {
            const SNode& Root = m_vVisibleRootNodes[i];
            Level.vAABBCenters[SNodeLevel::X].floats[i] = Root.AABB.Center.x;
            Level.vAABBCenters[SNodeLevel::Z].floats[i] = Root.AABB.Center.x;
            Level.vec4Extents = Root.AABB.Extents;
            Level.boundingSphereRadius = Root.boundingSphereRadius;
            Level.level = Root.Handle.level;
            }
            _InitChildNodesSIMD(Level);*/
        }

        // Initializes all 4 nodes at once
        void CTerrainQuadTree::_InitChildNodesSIMD(const SInitChildNodeLevel& Info)
        {
            // Get child node level
            SNodeLevel& ChildLevel = m_vChildNodeLevels[Info.childLevelIndex];

            //VKE_PROFILE_SIMPLE2("create child nodes for parent SIMD");
            // Top left, top right, bottom left, bottom right
            static const Math::CVector4 vec4DirectionXs(-1.0f, 1.0f, -1.0f, 1.0f);
            static const Math::CVector4 vec4DirectionZs(1.0f, 1.0f, -1.0f, -1.0f);
            static const Math::CVector4 vec4Half(0.5f);

            Math::CVector4& vec4CenterXs = ChildLevel.aAABBCenters[SNodeLevel::X];
            Math::CVector4& vec4CenterZs = ChildLevel.aAABBCenters[SNodeLevel::Z];
            Math::CVector4& vec4ChildExtentsWD = ChildLevel.aAABBExtents[SNodeLevel::X];
            // SIMD
            {
                const Math::CVector4 vec4ParentCenterX(Info.vecParentSizes.x);
                const Math::CVector4 vec4ParentCenterZ(Info.vecParentSizes.y);
                const Math::CVector4 vec4ParentExtentsWD(Info.vecParentSizes.z); // node is a square so width == depth
                                                                                 // Calc child node extents (parent / 2)
                Math::CVector4::Mul(vec4ParentExtentsWD, vec4Half, &vec4ChildExtentsWD);
                // Move node center x/z position towards direction vector in distance of child node size
                Math::CVector4::Mad(vec4DirectionXs, vec4ChildExtentsWD, vec4ParentCenterX, &vec4CenterXs);
                Math::CVector4::Mad(vec4DirectionZs, vec4ChildExtentsWD, vec4ParentCenterZ, &vec4CenterZs);
            }

            ChildLevel.boundingSphereRadius = Info.parentBoundingSphereRadius * 0.5f;
            const uint8_t currLevel = Info.parentLevel + 1;
            ChildLevel.level = currLevel;

            /*const Math::CVector3 tmp[4] =
            {
            { vec4CenterXs.x, 0, vec4CenterZs.x },
            { vec4CenterXs.y, 0, vec4CenterZs.y },
            { vec4CenterXs.z, 0, vec4CenterZs.z },
            { vec4CenterXs.w, 0, vec4CenterZs.w }
            };*/

            const uint8_t childLevel = currLevel + 1;
            if (childLevel < Info.maxLODCount)
            {
                SInitChildNodeLevel ChildLevelInfo;
                ChildLevelInfo.maxLODCount = Info.maxLODCount;
                ChildLevelInfo.parentBoundingSphereRadius = ChildLevel.boundingSphereRadius;
                ChildLevelInfo.parentLevel = currLevel;
                ChildLevelInfo.vecParentSizes.z = ChildLevel.aAABBExtents[SNodeLevel::X].x; // the same for all nodes

                for (uint32_t i = 0; i < 4; ++i)
                {
                    ChildLevelInfo.vecParentSizes.x = ChildLevel.aAABBCenters[SNodeLevel::X].floats[i];
                    ChildLevelInfo.vecParentSizes.y = ChildLevel.aAABBCenters[SNodeLevel::Z].floats[i];
                    ChildLevelInfo.childLevelIndex = _AcquireChildNodeLevel();
                    ChildLevel.aChildLevelIndices[i] = ChildLevelInfo.childLevelIndex;
                    //VKE_PROFILE_SIMPLE2("create child nodes for parent SIMD");
                    _InitChildNodesSIMD(ChildLevelInfo);
                }
            }
            else
            {
                ChildLevel.aChildLevelIndices[0] = UNDEFINED_U32; // mark there are no more children
            }
        }

        void CTerrainQuadTree::_InitChildNodesSIMD(const SNodeLevel& ParentLevel)
        {
        }

        void CTerrainQuadTree::_InitChildNodes(const SInitChildNodesInfo& Info)
        {
            static const Math::CVector4 aVectors[4] =
            {
                {-1.0f, 0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 1.0f, 0.0f},
            {-1.0f, 0.0f, -1.0f, 0.0f}, {1.0f, 0.0f, -1.0f, 0.0f}
            };

            // Info for children of children
            SInitChildNodesInfo ChildOfChildInfo;

            const auto currLevel = Info.hParent.level;
            const uint8_t childNodeLevel = (uint8_t)currLevel + 1;
            //if (childNodeLevel < Info.maxLODCount)
            VKE_ASSERT(childNodeLevel < Info.maxLODCount, "");
            {
                Math::CVector4 vecChildCenter;
                UNodeHandle ahChildNodes[4];
                const uint32_t parentIndex = Info.hParent.index;
                SNode& Parent = m_vNodes[Info.hParent.index];

                ChildOfChildInfo.vec4Extents = Info.vec4Extents * 0.5f;
                ChildOfChildInfo.vecExtents = Math::CVector3(ChildOfChildInfo.vec4Extents);
                ChildOfChildInfo.boundingSphereRadius = Info.boundingSphereRadius * 0.5f;
                ChildOfChildInfo.maxLODCount = Info.maxLODCount;

                {
                    VKE_PROFILE_SIMPLE2("create child nodes for parent"); //6 us
                                                                          // Create child nodes for parent
                    for (uint8_t i = 0; i < 4; ++i)
                    {
                        UNodeHandle Handle;
                        Handle.index = Info.childNodeStartIndex + i;
                        Handle.level = childNodeLevel;
                        Handle.childIdx = i;
                        auto& Node = m_vNodes[Handle.index];
                        Node.Handle = Handle;
                        Node.hParent = Info.hParent;

                        Node.ahChildren[0].handle = UNDEFINED_U32;
                        Node.ahChildren[1].handle = UNDEFINED_U32;
                        Node.ahChildren[2].handle = UNDEFINED_U32;
                        Node.ahChildren[3].handle = UNDEFINED_U32;

                        Math::CVector4::Mad(aVectors[i], Info.vec4Extents, Info.vec4ParentCenter, &vecChildCenter);
                        Node.AABB = Math::CAABB(Math::CVector3{vecChildCenter}, Info.vecExtents);
                        Node.boundingSphereRadius = Info.boundingSphereRadius;
                        // Set this node as a child for a parent
                        //m_vNodes[hParent.index].ahChildren[i] = Handle;
                        Parent.ahChildren[i] = Handle;
                        ahChildNodes[i] = Handle;

                        {
                            VKE_PROFILE_SIMPLE2("store child data"); // 1.5 us
                            m_vAABBs[Handle.index] = Node.AABB;
                            m_vBoundingSpheres[Handle.index] = Math::CBoundingSphere(Node.AABB.Center, Node.boundingSphereRadius);
                            m_vChildNodeHandles[parentIndex][i] = Handle;
                        }

                        _SetDrawDataForNode(&Node);
                    }
                }
                if (childNodeLevel + 1 < Info.maxLODCount)
                {
                    VKE_PROFILE_SIMPLE2("create children of children");
                    for (uint8_t i = 0; i < 4; ++i)
                    {
                        const auto& hNode = ahChildNodes[i];
                        auto& Node = m_vNodes[hNode.index];
                        ChildOfChildInfo.vec4ParentCenter = Node.AABB.Center;
                        ChildOfChildInfo.hParent = Node.Handle;
                        ChildOfChildInfo.childNodeStartIndex = _AcquireChildNodes();
                        _InitChildNodes(ChildOfChildInfo);
                    }
                }
            }
        }

        void CTerrainQuadTree::_FreeChildNodes(UNodeHandle hParent)
        {
            SNode& Parent = m_vNodes[hParent.index];
            VKE_ASSERT(Parent.ahChildren[0].handle != UNDEFINED_U32, "");
            //for (uint32_t i = 0; i < 4; ++i)
            {
                m_vFreeNodeIndices.PushBack( Parent.ahChildren[0].index );
            }

            for (uint32_t i = 0; i < 4; ++i)
            {
                _FreeChildNodes( Parent.ahChildren[i] );
                Parent.ahChildren[i].handle = UNDEFINED_U32;
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
            VKE_ASSERT(m_vNodes.IsEmpty() == false, "");
            // Get all node indices starting from one after last root
            const uint32_t totalNodeCount = m_vNodes.GetCount();
            const uint32_t childNodeCount = totalNodeCount - m_totalRootCount;
            // There always must be a multiple of 4 child nodes
            VKE_ASSERT(childNodeCount % 4 == 0, "");
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

        Result CTerrainQuadTree::_CreateChildNodes(UNodeHandle hParent, const SCreateNodeData& NodeData,
            const uint8_t lodCount)
        {
            static const Math::CVector4 aVectors[4] =
            {
                {-1.0f, 0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 1.0f, 0.0f},
                {-1.0f, 0.0f, -1.0f, 0.0f}, {1.0f, 0.0f, -1.0f, 0.0f}
            };

            //auto& Parent = m_vNodes[ hParent.index ];
            Result res = VKE_OK;
            const auto currLevel = NodeData.level;
            if (currLevel < lodCount)
            {
                SCreateNodeData ChildNodeData;

                ChildNodeData.vec4Extents = NodeData.vec4Extents * 0.5f;
                ChildNodeData.vecExtents = Math::CVector3(ChildNodeData.vec4Extents);
                ChildNodeData.boundingSphereRadius = NodeData.boundingSphereRadius * 0.5f;
                ChildNodeData.level = currLevel + 1;

                Math::CVector4 vecChildCenter;
                UNodeHandle ahChildNodes[4];
                const uint32_t childNodeStartIdx = _AcquireChildNodes();
                // Create child nodes for parent
                for (uint8_t i = 0; i < 4; ++i)
                {
                    UNodeHandle Handle;
                    //Handle.index = m_vNodes.PushBack({});
                    Handle.index = childNodeStartIdx + i;
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
                    // Set this node as a child for a parent
                    m_vNodes[hParent.index].ahChildren[i] = Handle;
                    ahChildNodes[i] = Handle;

                    m_vAABBs[Handle.index] = Node.AABB;
                    m_vBoundingSpheres[Handle.index] = Math::CBoundingSphere(Node.AABB.Center, Node.boundingSphereRadius);
                    m_vChildNodeHandles[hParent.index][i] = Handle;


                    _SetDrawDataForNode( &Node );
                }

                for (uint8_t i = 0; i < 4; ++i)
                {
                    const auto& hNode = ahChildNodes[i];
                    auto& Node = m_vNodes[hNode.index];
                    ChildNodeData.vec4ParentCenter = Node.AABB.Center;
                    ChildNodeData.hParent = Node.Handle;

                    res = _CreateChildNodes( hNode, ChildNodeData, lodCount );
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
#if VKE_PROFILER_TERRAIN_UPDATE
            VKE_PROFILE_SIMPLE();
#endif
            auto pCamera = m_pTerrain->GetScene()->GetCamera();
            SViewData View;
            View.fovRadians = pCamera->GetFOVAngleRadians();
            View.screenWidth = pCamera->GetViewport().width;
            View.screenHeight = pCamera->GetViewport().height;
            View.frustumWidth = pCamera->CalcFrustumWidth(1.0f); // frustum width at 1.0f distance
            View.Frustum = pCamera->GetFrustum();
            View.vecPosition = pCamera->GetPosition();
            View.halfFOV = View.fovRadians * 0.5f;

            static Math::CVector3 vecLastPos = Math::CVector3::ZERO;
            if( vecLastPos != View.vecPosition )
            {
                vecLastPos = View.vecPosition;
            }

            {
                m_vLODData.Clear();
                for (uint32_t i = 0; i < m_vvLODData.GetCount(); ++i)
                {
                    m_vvLODData[i].Clear();
                }
            }
            // Cull only roots
            {
#if VKE_PROFILE_TERRAIN
                VKE_PROFILE_SIMPLE2("Frustum cull roots");
#endif
                _FrustumCullRoots(View);
            }
            // Determine which root contains the camera
            {
#if !INIT_CHILD_NODES_FOR_EACH_ROOT
#   if VKE_PROFILE_TERRAIN
                VKE_PROFILE_SIMPLE2("Init main roots");
#   endif
                _InitMainRootsSIMD(View);
#endif
            }
            {
#if VKE_PROFILE_TERRAIN
                VKE_PROFILE_SIMPLE2("FrustumCull");
#endif
                _FrustumCull(View);
            }
            {
                //VKE_PROFILE_SIMPLE2("CalcLODs");
                //const uint32_t nodeCount = m_RootNodeCount.x * m_RootNodeCount.y;
                //for (uint32_t i = 0; i < nodeCount; ++i)
                //{
                //    auto& Node = m_vNodes[i];
                //    if (m_vNodeVisibility[Node.Handle.index])
                //    {
                //        //_CalcErrorLODs(Node, m_vTextureIndices[i], View);
                //        _CalcLODsSIMD(Node, View);
                //    }
                //}
#if VKE_PROFILE_TERRAIN
                VKE_PROFILE_SIMPLE2("Calc lods");
#endif
                for (uint32_t i = 0; i < m_vVisibleRootNodes.GetCount(); ++i)
                {
                    _CalcLODsSIMD(m_vVisibleRootNodes[i], View);
                }
            }
            {
#if VKE_PROFILE_TERRAIN
                VKE_PROFILE_SIMPLE2("Merge LODS");
#endif
                for (uint32_t i = 0; i < m_vvLODData.GetCount(); ++i)
                {
                    m_vLODData.Append(m_vvLODData[i]);
                }
            }
            {
#if VKE_PROFILE_TERRAIN
                VKE_PROFILE_SIMPLE2("Set Stitches");
#endif
                _SetStitches();
            }
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
            const uint32_t y = ( (int32_t)-vecPos.z + terrainHalfSize ) / tileSize;
            uint32_t ret =  Math::Map2DArrayIndexTo1DArrayIndex( x, y, elementCountInRow );

            return ret;
        }

        // Calc tile size for renderer
        // renderLOD is oposite to quadtree lod. Lowest lod in quadtree is 0 (root) while in rendering
        // lod 0 is the highest one (the same as for mipmaps).
        vke_force_inline uint16_t CalcRenderTileSize(const uint8_t renderLOD, const uint16_t baseTileSize)
        {
            return Math::CalcPow2(renderLOD) * baseTileSize;
        }

        void CTerrainQuadTree::_CalcErrorLODs( const SNode& CurrNode, const uint32_t& textureIdx,
                                               const SViewData& View )
        {
            const auto hCurrNode = CurrNode.Handle;
            const auto& AABB = CurrNode.AABB;
            const Math::CVector3 vecTmpPos = AABB.Center - AABB.Extents;
            const bool b = AABB.Center.x == 96 && AABB.Center.z == 96;

            // Instead of a regular bounding sphere radius use size of AABB.Extents size
            // This approach fixes wrong calculations for node containing the view point
            // Note that a node is a quad
            Math::CVector4 vecPoint;
            const float boundingSphereRadius2 = CurrNode.boundingSphereRadius;
            const float boundingSphereRadius = AABB.Extents.x;
            CalcNearestSpherePoint( Math::CVector4( AABB.Center ), boundingSphereRadius,
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
            if( ( err > 60 && hasChildNodes ) /*|| hCurrNode.level == 0*/ )
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
                        const auto& AABB2 = ChildNode.AABB;
                        const Math::CVector3 vecTmpPos2 = AABB2.Center - AABB2.Extents;
                        if( m_vNodeVisibility[ hNode.index ] )
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
                Data.DrawData.tileSize = AABB.Extents.x * 2;
                Data.idx = MapPositionTo1DArrayIndex( Data.DrawData.vecPosition, m_tileSize,
                                                      m_terrainHalfSize, m_tileInRowCount );
                /*VKE_DBG_LOG("" << indents[hCurrNode.level] << "l: " << hCurrNode.level << " idx: " << hCurrNode.index <<
                    " d: " << distance << " e: " << err <<
                    " c: " << AABB.Center.x << ", " << AABB.Center.z <<
                    " p: " << vecPoint.x << ", " << vecPoint.z <<
                    " vp: " << CurrNode.DrawData.vecPosition.x << ", " << CurrNode.DrawData.vecPosition.z <<
                    " s:" << CurrNode.AABB.Extents.x << ", " << CurrNode.AABB.Extents.z << "\n");*/

                //if (hCurrNode.level > 0)
                {
                    _SetLODMap(Data);
                }
                _AddLOD( Data );
            }
        }

        void LoadPosition3(const Math::CVector4 aVecs[3], uint32_t idx, Math::CVector3* pOut)
        {
            pOut->x = aVecs[CTerrainQuadTree::SNodeLevel::X].floats[idx];
            pOut->y = aVecs[CTerrainQuadTree::SNodeLevel::Y].floats[idx];
            pOut->z = aVecs[CTerrainQuadTree::SNodeLevel::Z].floats[idx];
        }

        void LoadExtents3(const Math::CVector4 aVecs[2], uint32_t idx, Math::CVector3* pOut)
        {
            pOut->x = aVecs[CTerrainQuadTree::SNodeLevel::X].floats[idx];
            pOut->y = aVecs[CTerrainQuadTree::SNodeLevel::Y].floats[idx];
        }

        void LoadPosition4(const Math::CVector4 aVecs[3], uint32_t idx, Math::CVector4* pOut)
        {
            pOut->x = aVecs[CTerrainQuadTree::SNodeLevel::X].floats[idx];
            pOut->y = aVecs[CTerrainQuadTree::SNodeLevel::Y].floats[idx];
            pOut->z = aVecs[CTerrainQuadTree::SNodeLevel::Z].floats[idx];
        }

        void LoadExtents4(const Math::CVector4 aVecs[2], uint32_t idx, Math::CVector4* pOut)
        {
            pOut->x = aVecs[CTerrainQuadTree::SNodeLevel::X].floats[idx];
            pOut->y = aVecs[CTerrainQuadTree::SNodeLevel::Y].floats[idx];
        }

        void vke_force_inline CalcLenghts3(const Math::CVector4* aVectors, Math::CVector4* pOut)
        {
            Math::CVector4 aMuls[3], aAdds[2];
            // x*x, y*y, z*z
            {
                Math::CVector4::Mul(aVectors[0], aVectors[0], &aMuls[0]);
                Math::CVector4::Mul(aVectors[1], aVectors[1], &aMuls[1]);
                Math::CVector4::Mul(aVectors[2], aVectors[2], &aMuls[2]);
            }
            // x*x + y*y + z*z
            {
                Math::CVector4::Add(aMuls[0], aMuls[1], &aAdds[0]);
                Math::CVector4::Add(aMuls[2], aAdds[0], &aAdds[1]);
            }
            // sqrt( x*x + y*y + z*z )
            {
                Math::CVector4::Sqrt(aAdds[1], pOut);
            }
            // normalize
            // x / sqrt, y / sqrt, z / sqrt
            /*{
            Math::CVector4::Div(aVectors[0], vecSqrt, &aOut[0]);
            Math::CVector4::Div(aVectors[1], vecSqrt, &aOut[1]);
            Math::CVector4::Div(aVectors[2], vecSqrt, &aOut[2]);
            }*/
        }

        void vke_force_inline CalcDistance3(const Math::CVector4* aV1, const Math::CVector4* aV2,
            Math::CVector4* pOut)
        {
            Math::CVector4 aSubs[3];
            Math::CVector4::Sub(aV1[0], aV2[0], &aSubs[0]);
            Math::CVector4::Sub(aV1[1], aV2[1], &aSubs[1]);
            Math::CVector4::Sub(aV1[2], aV2[2], &aSubs[2]);
            // Lengths
            CalcLenghts3(aSubs, pOut);
        }

        void LoadVec3(const Math::CVector4& V, Math::CVector4 aOut[3])
        {
            aOut[0] = {V.x};
            aOut[1] = {V.z};
            aOut[2] = {V.y};
        }

#define VKE_LOAD_VEC4_XZY(_vec4) Math::CVector4{(_vec4).x}, Math::CVector4{(_vec4).z}, Math::CVector4{(_vec4).y}

        void CalcScreenSpaceErrors(const Math::CVector4* aPoints, const float& worldSpaceError,
            const CTerrainQuadTree::SViewData& View, Math::CVector4* pvecErrorOut, Math::CVector4* pvecDistanceOut)
        {
            //VKE_PROFILE_SIMPLE();
            const Math::CVector4 aViewPoints[3] =
            {
                Math::CVector4{View.vecPosition.x},
                Math::CVector4{View.vecPosition.z},
                Math::CVector4{View.vecPosition.y}
            };

            CalcDistance3(aViewPoints, aPoints, pvecDistanceOut);
            const Math::CVector4 vecWorldSpaceError(worldSpaceError);
            Math::CVector4 vecD, vecP;
            /// TODO: Cache this!
            const float k = View.screenWidth / (2.0f * std::tanf(View.halfFOV));
            const Math::CVector4 vecK(k);
            // d = error / distance
            Math::CVector4::Div(vecWorldSpaceError, *pvecDistanceOut, &vecD);
            // p = d * k
            Math::CVector4::Mul(vecD, vecK, pvecErrorOut);
        }

        void CalcErrors(const Math::CVector4* aPoints, const float vertexDistance, const uint8_t nodeLevel,
            const uint8_t lodCount,
            const CTerrainQuadTree::SViewData& View, Math::CVector4* pvecErrorOut, Math::CVector4* pvecDistanceOut)
        {
            //VKE_PROFILE_SIMPLE();
            const float worldSpaceError = CalcWorldSpaceError(vertexDistance, nodeLevel, lodCount);
            CalcScreenSpaceErrors(aPoints, worldSpaceError, View, pvecErrorOut, pvecDistanceOut);
        }

        void CalcNearestSpherePoints(const Math::CVector4* aSphereCenters, const float sphereRadius,
            const Math::CVector4& vec4ViewPos, Math::CVector4* pOut)
        {
            //VKE_PROFILE_SIMPLE();
            Math::CVector4 aDirs[3], aTmp[3], aTmp2[3];
            const Math::CVector4 aViewPos[3] =
            {
                Math::CVector4{vec4ViewPos.x},
                Math::CVector4{vec4ViewPos.z},
                Math::CVector4{vec4ViewPos.y}
            };
            const Math::CVector4 vecRadius(sphereRadius);

            // 68, 0, 4 / 0.99, 0, 0.05
            // 4, 0, 4 / 0.7, 0, 0.7
            //
            //
            Math::CVector4::Sub(aViewPos[0], aSphereCenters[0], &aDirs[0]);
            Math::CVector4::Sub(aViewPos[1], aSphereCenters[1], &aDirs[1]);
            Math::CVector4::Sub(aViewPos[2], aSphereCenters[2], &aDirs[2]);

            // normalize dirs
            {
                Math::CVector4::Mul(aDirs[0], aDirs[0], &aTmp[0]);
                Math::CVector4::Mul(aDirs[1], aDirs[1], &aTmp[1]);
                Math::CVector4::Mul(aDirs[2], aDirs[2], &aTmp[2]);

                Math::CVector4::Add(aTmp[0], aTmp[1], &aTmp2[0]);
                Math::CVector4::Add(aTmp[2], aTmp2[0], &aTmp2[1]);

                Math::CVector4::Sqrt(aTmp2[1], &aTmp2[2]);

                Math::CVector4::Div(aDirs[0], aTmp2[2], &aDirs[0]);
                Math::CVector4::Div(aDirs[1], aTmp2[2], &aDirs[1]);
                Math::CVector4::Div(aDirs[2], aTmp2[2], &aDirs[2]);
            }

            Math::CVector4::Mad(aDirs[0], vecRadius, aSphereCenters[0], &pOut[0]);
            Math::CVector4::Mad(aDirs[1], vecRadius, aSphereCenters[1], &pOut[1]);
            Math::CVector4::Mad(aDirs[2], vecRadius, aSphereCenters[2], &pOut[2]);
        }

        void CTerrainQuadTree::_CalcLODsSIMD(const SNode& Root, const SViewData& View)
        {
            Math::CVector4 vec4ChildCenter, vec4ChildExtents;
            const Math::CVector4 vec4ViewPosition = Math::CVector4(View.vecPosition);
            const bool hasChildNodes = Root.childLevelIndex != UNDEFINED_U32;
            const float boundingSphereRadius = Root.boundingSphereRadius;
            Math::CVector4 vec4NearestPoint;
            const Math::CVector4 vec4Center = Math::CVector4{Root.AABB.Center};
            float err, distance;
            CalcNearestSpherePoint(vec4Center, boundingSphereRadius, vec4ViewPosition, &vec4NearestPoint);
            _CalcError(vec4NearestPoint, Root.Handle.level, View, &err, &distance);
            if (hasChildNodes && err > 60)
            {
                const SNodeLevel& ChildNodes = m_vChildNodeLevels[Root.childLevelIndex];
                _CalcLODsSIMD(ChildNodes, Root, View);
            }
            else
            {
                const uint8_t highestLod = (uint8_t)(m_Desc.lodCount - 1);

                SLODData Data;
                Data.lod = highestLod - Root.Handle.level;
                const Math::CVector3& vecSize = Root.AABB.Extents;
                auto& vecPos = Data.DrawData.vecPosition;
                vecPos.x = vec4Center.x - vecSize.x;
                vecPos.y = vec4Center.y;
                vecPos.z = vec4Center.z + vecSize.x;
                Data.DrawData.tileSize = vecSize.x * 2;
                Data.DrawData.pPipeline = Root.DrawData.pPipeline;
                Data.idx = MapPositionTo1DArrayIndex(vecPos, m_tileSize, m_terrainHalfSize, m_tileInRowCount);
                _SetLODMap(Data);
                _AddLOD(Data);
            }
        }

        void CTerrainQuadTree::_CalcLODsSIMD(const SNodeLevel& ChildNodes,
            const SNode& Root, const SViewData& View)
        {
            const bool hasChildNodes = ChildNodes.aChildLevelIndices[0] != UNDEFINED_U32;
            //const float boundingSphereRadius = ChildNodes.boundingSphereRadius;
            const float boundingSphereRadius = ChildNodes.aAABBExtents[0].x;
            const Math::CVector4 vec4ViewPosition = Math::CVector4( View.vecPosition );
            bool aCalcLODs[4];

            Math::CVector4 vec4ChildCenter, vec4ChildExtents, vecErrors, vecDistances;
            Math::CVector4 avec4Centers[4];
            Math::CVector4 vec4NearestPoint[4];

            Math::CVector4 aNearestPoints[3], vecCalcLODs;
            static const Math::CVector4 vecErrorTreshold(60.0f);

            //{
            //    //VKE_PROFILE_SIMPLE2("OLD"); // 1-3us
            //    for (uint32_t i = 0; i < 4; ++i)
            //    {
            //        avec4Centers[i] = {ChildNodes.aAABBCenters[SNodeLevel::X].floats[i], 0.0f, ChildNodes.aAABBCenters[SNodeLevel::Z].floats[i], 0.0f};

            //        CalcNearestSpherePoint(avec4Centers[i], boundingSphereRadius, vec4ViewPosition, &vec4NearestPoint[i]);
            //        _CalcError(vec4NearestPoint[i], ChildNodes.level, View, &vecErrors.floats[i], &vecDistances.floats[i]);
            //    }
            //}
            {
                //VKE_PROFILE_SIMPLE2("NEW"); // 2us
                CalcNearestSpherePoints(ChildNodes.aAABBCenters, boundingSphereRadius, vec4ViewPosition,
                    aNearestPoints);
                CalcErrors(aNearestPoints, m_Desc.vertexDistance, ChildNodes.level, m_Desc.lodCount, View, &vecErrors,
                    &vecDistances);
            }

            for (uint32_t i = 0; i < 4; ++i)
            {
                aCalcLODs[i] = hasChildNodes && vecErrors.floats[i] > 60;
                if (!aCalcLODs[i])
                {
                    const uint8_t highestLod = (uint8_t)(m_Desc.lodCount - 1);

                    SLODData Data;
                    Data.lod = highestLod - ChildNodes.level;
                    Math::CVector3 vecSize;
                    LoadExtents3(ChildNodes.aAABBExtents, i, &vecSize);
                    auto& vecPos = Data.DrawData.vecPosition;
                    // Calc top left corner
                    vecPos.x = ChildNodes.aAABBCenters[SNodeLevel::X].floats[i] - vecSize.x;
                    vecPos.z = ChildNodes.aAABBCenters[SNodeLevel::Z].floats[i] + vecSize.x;
                    vecPos.y = ChildNodes.aAABBCenters[SNodeLevel::Y].floats[i];
                    Data.DrawData.tileSize = vecSize.x * 2;
                    Data.DrawData.pPipeline = Root.DrawData.pPipeline;
                    Data.idx = MapPositionTo1DArrayIndex(vecPos, m_tileSize, m_terrainHalfSize, m_tileInRowCount);
                    _SetLODMap(Data);
                    _AddLOD(Data);
                }
            }

            for (uint32_t i = 0; i < 4; ++i)
            {
                if (aCalcLODs[i])
                {
                    const SNodeLevel& Level = m_vChildNodeLevels[ChildNodes.aChildLevelIndices[i]];
                    _CalcLODsSIMD(Level, Root, View);
                }
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
            //float distance = _CalcDistanceToCenter( AABB.Center, View );
            float distance = Math::CVector4::Distance( vecPoint, Math::CVector4( View.vecPosition ) );

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
            if( (lod < tileLod && hasChildNodes) || isRoot )
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
                VKE_ASSERT( CurrNode.DrawData.pPipeline.IsValid(), "" );
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
            //VKE_PROFILE_SIMPLE();
            // Calc how many highest lod tiles contains this chunk
            // LOD map is a map of highest lod tiles
            uint32_t currX, currY;
            Math::Map1DarrayIndexTo2DArrayIndex( Data.idx, m_tileInRowCount, &currX, &currY );
            const uint8_t lod = Data.lod;
            // Calc from how many minimal tiles this tile is built
            const uint32_t tileRowCount = Math::CalcPow2( lod );
            const uint32_t tileColCount = tileRowCount;
            // Set LOD only on edges due to performance reasons
            // Vertical
#if 1
            const uint32_t rightColIdx = currX + tileColCount - 1;
            const uint32_t bottomRowIdx = currY + tileRowCount - 1;
            const uint32_t topRowIdx = currY;
            const uint32_t leftColIdx = currX;

            for (uint32_t y = 0; y < tileRowCount; ++y)
            {
                const uint32_t leftIdx = Math::Map2DArrayIndexTo1DArrayIndex( currX, y + currY, m_tileInRowCount );
                const uint32_t rightIdx = Math::Map2DArrayIndexTo1DArrayIndex( rightColIdx, y + currY, m_tileInRowCount );
                m_vLODMap[leftIdx] = lod;
                m_vLODMap[rightIdx] = lod;
            }

            for (uint32_t x = 0; x < tileColCount; ++x)
            {
                const uint32_t topIdx = Math::Map2DArrayIndexTo1DArrayIndex( currX + x, currY, m_tileInRowCount );
                const uint32_t bottomIdx = Math::Map2DArrayIndexTo1DArrayIndex( currX + x, bottomRowIdx, m_tileInRowCount );
                m_vLODMap[topIdx] = lod;
                m_vLODMap[bottomIdx] = lod;
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
            for (uint32_t i = 0; i < m_vLODData.GetCount(); ++i)
            {
                auto& Data = m_vLODData[i];
                uint32_t tileRowCount = Math::CalcPow2(Data.lod); // number of tiles for this node
                auto pos = Data.DrawData.vecPosition;
                // x, y are the left top corner in the map
                uint32_t x, y;
                Math::Map1DarrayIndexTo2DArrayIndex(Data.idx, m_tileInRowCount, &x, &y);
                // Calc idxs of neighbours
                uint8_t leftLod = Data.lod;
                uint8_t rightLod = Data.lod;
                uint8_t topLod = Data.lod;
                uint8_t bottomLod = Data.lod;

                if (x > 0)
                {
                    uint32_t leftIdx = Math::Map2DArrayIndexTo1DArrayIndex(x - 1, y, m_tileInRowCount);
                    leftLod = m_vLODMap[leftIdx];
                }
                if (y > 0)
                {
                    uint32_t topIdx = Math::Map2DArrayIndexTo1DArrayIndex(x, y - 1, m_tileInRowCount);
                    topLod = m_vLODMap[topIdx];
                }
                if (x + tileRowCount < m_tileInRowCount)
                {
                    uint32_t rightIdx = Math::Map2DArrayIndexTo1DArrayIndex(x + tileRowCount, y, m_tileInRowCount);
                    rightLod = m_vLODMap[rightIdx];
                }
                if (y + tileRowCount < m_tileInRowCount)
                {
                    uint32_t bottomIdx = Math::Map2DArrayIndexTo1DArrayIndex(x, y + tileRowCount, m_tileInRowCount);
                    bottomLod = m_vLODMap[bottomIdx];
                }

                if (Data.lod < leftLod)
                {
                    Data.DrawData.leftVertexDiff = (uint8_t)Math::CalcPow2(leftLod - (uint8_t)Data.lod);
                }
                if (Data.lod < rightLod)
                {
                    Data.DrawData.rightVertexDiff = (uint8_t)Math::CalcPow2(rightLod - (uint8_t)Data.lod);
                }
                if (Data.lod < topLod)
                {
                    Data.DrawData.topVertexDiff = (uint8_t)Math::CalcPow2(topLod - (uint8_t)Data.lod);
                }
                if (Data.lod < bottomLod)
                {
                    Data.DrawData.bottomVertexDiff = (uint8_t)Math::CalcPow2(bottomLod - (uint8_t)Data.lod);;
                }
            }

#if DEBUG_LOD_STITCH_MAP
            for (uint32_t y = 0; y < m_tileInRowCount; ++y)
            {
                for (uint32_t x = 0; x < m_tileInRowCount; ++x)
                {
                    uint32_t idx = Math::Map2DArrayIndexTo1DArrayIndex(x, y, m_tileInRowCount);
                    //VKE_LOG( (uint32_t)m_vLODMap[idx] << " " );
                    VKE_LOGGER << (uint32_t)m_vLODMap[idx] << " ";
                }
                VKE_LOGGER << "\n";
            }
            VKE_LOGGER.Flush();
#endif
            //VKE_PROFILE_SIMPLE();
            //m_vLODMap.Reset(m_maxLODCount-1);
        }

        void CTerrainQuadTree::_AddLOD( const SLODData& Data )
        {
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
            *pDistanceOut = distance;

            //const float w = distance * 2.0f * std::tanf( View.halfFOV );
            //const float p = ( worldSpaceError * View.screenWidth ) / w;

            /*const auto a = ( View.screenWidth * 0.5f ) / std::tanf( View.fovRadians * 0.5f );
            const auto p = a * (e / distance);
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
            const float p = (worldSpaceError * View.screenWidth) / w;

            *pErrOut = p;

            // Intel formula
            /*const float w = View.screenWidth * 0.5f * Math::Cot( View.fovRadians * 0.5f );
            const float h = View.screenHeight * 0.5f * Math::Cot( View.fovRadians * 0.5f );
            const float err = Math::Max( w, h ) * ( 0.1f / distance );
            *pErrOut = err;*/
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

        void CTerrainQuadTree::_FrustumCull(const SViewData& View)
        {
            static const bool disable = DISABLE_FRUSTUM_CULLING;
            if (disable) return;
            // First pass: Frustum cull only roots
            const auto& Frustum = View.Frustum;
            for (uint32_t i = 0; i < m_vVisibleRootNodeHandles.GetCount(); ++i)
            {
                _BoundingSphereFrustumCullNode(m_vVisibleRootNodeHandles[i], Frustum);
            }
        }

        void CTerrainQuadTree::_FrustumCullRoots(const SViewData& View)
        {
            static const bool disable = DISABLE_FRUSTUM_CULLING;
            if( !disable )
            {
                const auto& Frustum = View.Frustum;
                for (uint32_t i = 0; i < m_totalRootCount; ++i)
                {
                    const auto& Sphere = m_vBoundingSpheres[i];
                    const bool isVisible = Frustum.Intersects(Sphere);
                    m_vNodeVisibility[i] = isVisible;
                }
            }
            // Copy all visible roots to a separate buffer
            m_vVisibleRootNodes.Clear();
            m_vVisibleRootNodeHandles.Clear();
            for (uint32_t i = 0; i < m_totalRootCount; ++i)
            {
                if (m_vNodeVisibility[i])
                {
                    m_vVisibleRootNodes.PushBack( m_vNodes[i] );
                    m_vVisibleRootNodeHandles.PushBack(m_vNodes[i].Handle);
                }
            }
        }

        void CTerrainQuadTree::_BoundingSphereFrustumCull(const SViewData& View)
        {
            // By default sets all nodes to visible
            m_vNodeVisibility.Reset( false );

            const uint32_t rootCount = m_RootNodeCount.x * m_RootNodeCount.y;
            const Math::CFrustum& Frustum = View.Frustum;

            for (uint32_t i = 0; i < rootCount; ++i)
            {
                auto& Node = m_vNodes[i];
                _BoundingSphereFrustumCullNode(Node.Handle, Frustum);
            }
        }

        void CTerrainQuadTree::_BoundingSphereFrustumCullNode(const UNodeHandle& hNode, const Math::CFrustum& Frustum)
        {
            const Math::CBoundingSphere& Sphere = m_vBoundingSpheres[hNode.index];
            const bool isVisible = Frustum.Intersects(Sphere);
            m_vNodeVisibility[hNode.index] = isVisible;
            if(isVisible)
            {
                const auto& ChildNodes = m_vChildNodeHandles[hNode.index];
                if (ChildNodes.aHandles[0].handle != UNDEFINED_U32)
                {
                    for (uint32_t i = 0; i < 4; ++i)
                    {
                        _BoundingSphereFrustumCullNode(ChildNodes.aHandles[i], Frustum);
                    }
                }
            }
        }

    } // Scene
} // VKE