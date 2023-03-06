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
#define DISABLE_FRUSTUM_CULLING 0
#define VKE_PROFILE_TERRAIN 0
#define VKE_PROFILER_TERRAIN_UPDATE 0
#define SAVE_TERRAIN_HEIGHTMAP_NORMAL 1
#define DISABLE_HEIGHT_CALC 1
#define ASYNC_LOADING 1
#define VKE_TERRAIN_LOAD_TEXTURES 1

namespace VKE
{
    namespace Scene
    {
        ExtentU16 CTerrain::CalcTextureCount( const STerrainDesc& Desc )
        {
            /*const auto size = Math::CalcNextPow2( Desc.size );
            const auto maxSize = Math::Min( Desc.TileSize.max, size );
            pOut->width = size / maxSize;
            pOut->height = size / maxSize;*/
            // Calc number of roots based on max heightmap texture size
            // 1 heightmap texel == 1 vertex position
            // Minimum root count is 2x2
            const auto size = Math::CalcNextPow2( Desc.size );
            const auto rootSize = Math::Min( Desc.TileSize.max, size );
            uint32_t rootCount = Desc.size / rootSize;
            rootCount = ( uint32_t )Math::Max( 2u, rootCount );
            // Must be power of 2
            rootCount = ( uint32_t )Math::CalcNextPow2( rootCount );
            return { ( uint16_t )rootCount, ( uint16_t )rootCount };
        }
        void CTerrain::_Destroy()
        {
            if( m_pRenderer != nullptr )
            {
                _DestroyRenderer( &m_pRenderer );
            }
            m_QuadTree._Destroy();
        }
        uint32_t CalcMaxVisibleTiles( const STerrainDesc& Desc )
        {
            uint32_t ret = 0;
            // maxViewDistance is a radius so 2x maxViewDistance == a diameter
            // Using diameter horizontally and vertically we get a square
            // All possible visible tiles should cover this square
            float dimm = Desc.maxViewDistance * 2;
            // float tileDimm = Desc.tileRowVertexCount * Desc.vertexDistance;
            float tileDimm = Desc.TileSize.min;
            float tileCount = dimm / tileDimm;
            ret = ( uint32_t )ceilf( tileCount * tileCount );
            return ret;
        }
        
        bool CTerrain::CheckDesc( const STerrainDesc& Desc ) const
        {
            bool ret = true;
            // Tile size must be pow of 2
            if( !Math::IsPow2( Desc.TileSize.min ) )
            {
                ret = false;
            }
            // vertexDistance
            if( Desc.vertexDistance < 1.0f )
            {
                // Below 1.0 acceptable values are 0.125, 0.25, 0.5
                // because next LODs must go to 1, 2, 4, 8 etc values to be
                // power of 2
                if( Desc.vertexDistance != 0.125f || Desc.vertexDistance != 0.25f || Desc.vertexDistance != 0.5f )
                {
                    ret = false;
                }
            }
            else if( !Math::IsPow2( ( uint32_t )Desc.vertexDistance ) )
            {
                // if above 1.0 vertexDistance must be power of 2
                ret = false;
            }
            return ret;
        }
        Result CTerrain::_Create( const STerrainDesc& Desc, RenderSystem::CommandBufferPtr pCommandBuffer )
        {
            Result ret = VKE_FAIL;
            m_Desc = Desc;
            auto pCtx = pCommandBuffer->GetContext()->GetDeviceContext();
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
            // m_tileSize = (uint32_t)((float)(Desc.tileRowVertexCount) *
            // Desc.vertexDistance);
            m_tileVertexCount = ( uint16_t )( ( float )Desc.TileSize.min / Desc.vertexDistance );
            // Round up terrain size to pow 2
            m_Desc.size = Math::CalcNextPow2( Desc.size );
            m_maxTileCount = ( uint16_t )( m_Desc.size / Desc.TileSize.min );
            if (m_Desc.lodCount == 0)
            {
                m_Desc.lodCount = CTerrainQuadTree::MAX_LOD_COUNT;
            }
            // Number of tiles must be power of two according to LODs
            // Each lod is 2x bigger
            m_halfSize = m_Desc.size / 2;
            CTerrainQuadTree::SCalcTerrainInfo CalcInfo;
            CalcInfo.pDesc = &m_Desc;
            CalcInfo.maxLODCount = CTerrainQuadTree::MAX_LOD_COUNT;
            CalcInfo.maxRootSize = Math::Min( m_Desc.size, Desc.TileSize.max );
            CTerrainQuadTree::_CalcTerrainInfo( CalcInfo, &m_TerrainInfo );
            /// ExtentU8 LODCount = CTerrainQuadTree::CalcLODCount( m_Desc,
            /// m_maxHeightmapSize, CTerrainQuadTree::MAX_LOD_COUNT );
            m_Desc.lodCount = m_TerrainInfo.maxLODCount; // Math::Min(Desc.lodCount, m_TerrainInfo.maxLODCount);
            m_maxTileCount *= m_maxTileCount;
            m_maxVisibleTiles = Math::Min( m_maxTileCount, ( uint16_t )CalcMaxVisibleTiles( m_Desc ) );
            m_vecExtents = Math::CVector3( m_Desc.size * 0.5f );
            //m_vecExtents.y = ( m_Desc.Height.min + m_Desc.Height.max ) * 0.5f;
            m_vecExtents.y = 0; // at create time it is not known the real min and max height
            m_avecCorners[ 0 ] = Math::CVector3( m_Desc.vecCenter.x - m_vecExtents.x, m_Desc.vecCenter.y,
                                                 m_Desc.vecCenter.z + m_vecExtents.z );
            m_avecCorners[ 1 ] = Math::CVector3( m_Desc.vecCenter.x + m_vecExtents.x, m_Desc.vecCenter.y,
                                                 m_Desc.vecCenter.z + m_vecExtents.z );
            m_avecCorners[ 2 ] = Math::CVector3( m_Desc.vecCenter.x - m_vecExtents.x, m_Desc.vecCenter.y,
                                                 m_Desc.vecCenter.z - m_vecExtents.z );
            m_avecCorners[ 3 ] = Math::CVector3( m_Desc.vecCenter.x + m_vecExtents.x, m_Desc.vecCenter.y,
                                                 m_Desc.vecCenter.z - m_vecExtents.z );
            m_QuadTree.m_pTerrain = this;
            {
                RenderSystem::SSamplerDesc SamplerDesc;
                SamplerDesc.Filter.min = RenderSystem::SamplerFilters::LINEAR;
                SamplerDesc.Filter.mag = RenderSystem::SamplerFilters::LINEAR;
                SamplerDesc.mipmapMode = RenderSystem::MipmapModes::LINEAR;
                SamplerDesc.AddressMode.U = RenderSystem::AddressModes::CLAMP_TO_BORDER;
                SamplerDesc.AddressMode.V = SamplerDesc.AddressMode.U;
                m_hHeightmapSampler = pCtx->CreateSampler( SamplerDesc );
            }
            ret = _CreateDummyResources( pCommandBuffer );
            if( VKE_FAILED( ret ) )
            {
            }
            ret = m_pRenderer->_Create( m_Desc, pCommandBuffer );
            if( VKE_FAILED( ret ) )
            {
            }
            ret = m_QuadTree._Create( m_Desc );
            if( VKE_FAILED( ret ) )
            {
                goto ERR;
            }

            m_pScene->GetDeviceContext()->LogMemoryDebug();
            return ret;
ERR:
            _DestroyRenderer( &m_pRenderer );
            return ret;
        }

        Result CTerrain::LoadTile( const SLoadTerrainTileInfo& Info, RenderSystem::CommandBufferPtr pCmdBuffer )
        {
            Result ret = VKE_OK;
            uint32_t tileIdx = Math::Map2DArrayIndexTo1DArrayIndex( Info.Position.x, Info.Position.y, m_QuadTree.m_RootNodeCount.width );
            auto& Node = m_QuadTree.m_vNodes[ tileIdx ];
            auto pDevice = pCmdBuffer->GetContext()->GetDeviceContext();
            STerrainUpdateBindingData BindingData;
            _GetBindingDataForRootNode( Node.Handle.index, &BindingData );
#if VKE_TERRAIN_LOAD_TEXTURES
            if( !Info.Heightmap.IsEmpty() )
            {
                ResourceName Name;
                Name.Format( "Heightmap_%d_%d", Info.Position.x, Info.Position.y );
                ret = _LoadTileTexture( pDevice, BindingData, Info.Heightmap.GetData(), Name.GetData(),
                                        &m_vHeightmapTextures, &m_vHeightmapTexViews,
                    &m_avpPendingTextures[TextureTypes::HEIGHTMAP] );
            }
            if( !Info.HeightmapNormal.IsEmpty() )
            {
                ResourceName Name;
                Name.Format( "HeightmapN_%d_%d", Info.Position.x, Info.Position.y );
                ret = _LoadTileTexture( pDevice, BindingData, Info.HeightmapNormal.GetData(), 
                    Name.GetData(),
                                        &m_vHeightmapNormalTextures, &m_vHeightmapNormalTexViews,
                                        &m_avpPendingTextures[TextureTypes::HEIGHTMAP_NORMAL] );
            }
            for( uint32_t i = 0; i < Info.vSplatmaps.GetCount(); ++i )
            {
                const auto& Splatmap = Info.vSplatmaps[ i ];
                if( !Splatmap.IsEmpty() )
                {
                    ResourceName Name;
                    Name.Format( "Splat%d_%d_%d", i, Info.Position.x, Info.Position.y );
                    ret = _LoadTileTexture( pDevice, BindingData,Splatmap.GetData(), Name.GetData(),
                                            &m_vSplatmapTextures, &m_vSplatmapTexViews,
                                            &m_avpPendingTextures[ TextureTypes::SPLAT ] );
                }
            }
            for (uint32_t i = 0; i < Info.vDiffuseTextures.GetCount(); ++i)
            {
                _LoadTileTexture( pDevice, BindingData, Info.vDiffuseTextures[ i ].GetData(),
                                  Info.vDiffuseTextures[ i ].GetData(), &m_avTextures[ TextureTypes::DIFFUSE ],
                                  &m_avTextureViews[ TextureTypes::DIFFUSE ],
                                  &m_avpPendingTextures[ TextureTypes::DIFFUSE ] );
            }
#endif
            m_pRenderer->UpdateBindings( BindingData );
            
            return ret;
        }

        void CTerrain::_DestroyRenderer( ITerrainRenderer** ppInOut )
        {
            ITerrainRenderer* pRenderer = *ppInOut;
            pRenderer->_Destroy();
            VKE_DELETE( pRenderer );
            *ppInOut = pRenderer;
        }
        Result CTerrain::_CreateDummyResources( RenderSystem::CommandBufferPtr pCommandBuffer )
        {
            Result ret = VKE_FAIL;
            {
                auto pCtx = pCommandBuffer->GetContext();
                auto pDevice = pCtx->GetDeviceContext();
                RenderSystem::STextureDesc Desc;
                Desc.Size = { 1, 1 };
                Desc.format = RenderSystem::Formats::R8G8B8A8_UNORM;
                Desc.arrayElementCount = 1;
                Desc.memoryUsage = RenderSystem::MemoryUsages::GPU_ACCESS | RenderSystem::MemoryUsages::TEXTURE;
                Desc.mipmapCount = 1;
                Desc.sliceCount = 1;
                Desc.type = RenderSystem::TextureTypes::TEXTURE_2D;
                Desc.usage = RenderSystem::TextureUsages::SAMPLED;
                Desc.Name = "VKETerrainDummy";
                //VKE_RENDER_SYSTEM_SET_DEBUG_NAME( Desc, "VKETerrainDummy" );
                Desc.SetDebugName( Desc.Name.GetData() );
                RenderSystem::SCreateTextureDesc CreateDesc;
                CreateDesc.Texture = Desc;
                CreateDesc.Create.flags = Core::CreateResourceFlags::DEFAULT;
                auto hTex = pDevice->CreateTexture( CreateDesc );
                if( hTex != INVALID_HANDLE )
                {
                    pCtx->SetTextureState( pCommandBuffer, RenderSystem::TextureStates::SHADER_READ, &hTex );
                    m_vDummyTextures.Resize( MAX_TEXTURE_COUNT, hTex );
                    auto hTexView = pDevice->GetTextureView( hTex )->GetHandle();
                    if( hTexView != INVALID_HANDLE )
                    {
                        m_vDummyTexViews.Resize( MAX_TEXTURE_COUNT, hTexView );
                        VKE_LOG( "Create terrain dummy texture: " << hTex.handle << ": " << hTexView.handle );
                        ret = VKE_OK;
                        uint32_t heightmapCount = 0;
                        /*for( uint32_t y = 0; y < m_Desc.Heightmap.vvFileNames.GetCount(); ++y )
                        {
                            for( uint32_t x = 0; x < m_Desc.Heightmap.vvFileNames[ y ].GetCount(); ++x )
                            {
                                heightmapCount++;
                            }
                        }*/
                        //heightmapCount = std::max( 1u, m_Desc.vTileTextures.GetCount() );
                        heightmapCount = m_Desc.size / m_Desc.TileSize.max;
                        heightmapCount *= heightmapCount;
                        m_vHeightmapTextures.Resize( heightmapCount, m_vDummyTextures[ 0 ] );
                        m_vHeightmapTexViews.Resize( heightmapCount, m_vDummyTexViews[ 0 ] );
                        m_vHeightmapNormalTextures.Resize( heightmapCount, m_vDummyTextures[ 0 ] );
                        m_vHeightmapNormalTexViews.Resize( heightmapCount, m_vDummyTexViews[ 0 ] );
                        m_vHeightmapNormalTextures.Resize( heightmapCount );
                        m_vSplatmapTextures.Resize( heightmapCount, m_vDummyTextures[0] );
                        m_vSplatmapTexViews.Resize( heightmapCount, m_vDummyTexViews[ 0 ] );
                        for (uint32_t i = 0; i < TextureTypes::_MAX_COUNT; ++i)
                        {
                            m_avTextures[ i ].Resize( heightmapCount, m_vDummyTextures[ 0 ] );
                            m_avTextureViews[ i ].Resize( heightmapCount, m_vDummyTexViews[ 0 ] );
                            m_avpPendingTextures[ i ].Resize( heightmapCount, RenderSystem::TextureRefPtr() );
                        }
                    }
                    else
                    {
                        VKE_LOG_ERR( "Unable to create terrain dummy texture view." );
                    }
                }
                else
                {
                    VKE_LOG_ERR( "Unable to create terrain dummy texture." );
                }
            }
            return ret;
        }
        Core::ImageSize CalcRequiredHeightmapSize( const ExtentU16& RootCount, uint16_t rootSize )
        {
            Core::ImageSize Ret = { RootCount.width * rootSize + 1u, RootCount.height * rootSize + 1u };
            for( uint16_t y = 0; y < RootCount.height; ++y )
            {
                for( uint16_t x = 0; x < RootCount.width; ++x )
                {
                }
            }
            return Ret;
        }
        Result CreateTextures( RenderSystem::CDeviceContext* pCtx, const ExtentU16& RootCount, uint16_t rootSize,
                               Core::CImageManager* pImgMgr, const Core::ImageHandle& hImg, const STerrainDesc& Desc,
                               CTerrain::TextureHandleArray* pInOut )
        {
            Result ret = VKE_FAIL;
            Core::SImageRegion Region;
            Core::SSliceImageInfo SliceInfo;
            SliceInfo.hSrcImage = hImg;
            const Core::ImageSize RequiredSize = CalcRequiredHeightmapSize( RootCount, rootSize );
            if( VKE_SUCCEEDED( pImgMgr->Resize( RequiredSize, &SliceInfo.hSrcImage ) ) )
            {
                for( uint16_t y = 0; y < RootCount.y; ++y )
                {
                    Region.Size.height = rootSize + 1; // ((y + 1) < m_TerrainInfo.RootCount.y);
                    Region.Offset.y = y * rootSize;
                    for( uint16_t x = 0; x < RootCount.x; ++x )
                    {
                        // For last slice do not add +1 to not read out of
                        // bounds from src image
                        Region.Size.width = rootSize + 1; // ((x + 1) < m_TerrainInfo.RootCount.x);
                        Region.Offset.x = x * rootSize;
                        SliceInfo.vRegions.PushBack( Region );
                    }
                }
            }
            Utils::TCDynamicArray<Core::ImageHandle> vImages( SliceInfo.vRegions.GetCount() );
            if( VKE_SUCCEEDED( pImgMgr->Slice( SliceInfo, &vImages[ 0 ] ) ) )
            {
                if( pInOut->Reserve( vImages.GetCount() ) )
                {
                    for( uint32_t i = 0; i < vImages.GetCount(); ++i )
                    {
                        char name[ 128 ];
                        uint32_t x, y;
                        Math::Map1DarrayIndexTo2DArrayIndex( i, RootCount.x, RootCount.y, &x, &y );
                        vke_sprintf( name, 128, "vke_slice_heightmap%d_%d-%d.png", rootSize, x, y );
#if( SAVE_TERRAIN_HEIGHTMAP_SLICES )
                        Core::SSaveImageInfo SaveInfo;
                        SaveInfo.hImage = vImages[ i ];
                        SaveInfo.format = Core::ImageFileFormats::PNG;
                        SaveInfo.pFileName = name;
                        pImgMgr->Save( SaveInfo );
#endif
                        RenderSystem::SCreateTextureDesc TexDesc;
                        TexDesc.hImage = vImages[ i ];
                        TexDesc.Texture.Name = name;
                        VKE_RENDER_SYSTEM_SET_DEBUG_NAME( TexDesc.Texture, name );
                        RenderSystem::TextureHandle hTex = pCtx->CreateTexture( TexDesc );
                        if( hTex != INVALID_HANDLE )
                        {
                            pInOut->PushBack( hTex );
                        }
                        pImgMgr->DestroyImage( &vImages[ i ] );
                    }
                }
            }
            if( pInOut->GetCount() == vImages.GetCount() )
            {
                ret = VKE_OK;
            }
            else
            {
                VKE_LOG_ERR( "One ore more texture was not created from split image." );
            }
            return ret;
        }
        Result CTerrain::_SplitTexture( RenderSystem::CDeviceContext* pCtx )
        {
            Result ret = VKE_ENOTFOUND;
            return ret;
        }
        Result CTerrain::_LoadTileTexture( RenderSystem::CDeviceContext* pCtx,
            const STerrainUpdateBindingData& Data,
                                           cstr_t pFileName, cstr_t pResourceName,
                                            CTerrain::TextureHandleArray* pvTextures,
                                            CTerrain::TextureViewArray* pvTexViews,
            CTerrain::TexturePtrArray* pvPendingTextures)
        {
            Result ret = VKE_OK;
           // char name[ 128 ];
            uint32_t x, y;
            uint32_t w = m_TerrainInfo.RootCount.width;
            uint32_t h = m_TerrainInfo.RootCount.height;
            Math::Map1DarrayIndexTo2DArrayIndex( Data.index, w, h, &x, &y );
            {
                //vke_sprintf( name, sizeof( name ), pNameFormat, x, y );
                Core::SLoadFileInfo Info;
                //Info.FileInfo.pFileName = m_Desc.Heightmap.vvFileNames[ x ][ y ];
                Info.FileInfo.FileName = pFileName;
                Info.CreateInfo.flags = Core::CreateResourceFlags::ASYNC | Core::CreateResourceFlags::DEFERRED |
                                        Core::CreateResourceFlags::DO_NOT_DESTROY_STAGING_RESOURCES;
#if !ASYNC_LOADING
                Info.CreateInfo.flags = Core::CreateResourceFlags::DEFAULT;
#endif
                Info.CreateInfo.TaskFlags = Threads::TaskFlags::HEAVY_WORK | Threads::TaskFlags::LOW_PRIORITY;

                RenderSystem::TextureHandle hTex;
                ret = pCtx->LoadTexture( Info, &hTex );
                if( VKE_SUCCEEDED( ret ) )
                {
                    auto pTex = pCtx->GetTexture( hTex );
                    ( *pvTextures )[ Data.index ] = ( hTex );
                    ( *pvPendingTextures )[ Data.index ] = pTex;
                }
            }

            return ret;
        }
        Result CTerrain::_LoadTextures( RenderSystem::CDeviceContext* pCtx )
        {
            Result ret = VKE_FAIL;
            // uint32_t heightmapCount = 0;
            ret = _SplitTexture( pCtx );
            if( ret == VKE_ENOTFOUND )
            {
                ret = VKE_FAIL;
                uint32_t currIndex = 0;
                char name[ 1024 ];
                //for( uint32_t y = 0; y < m_Desc.Heightmap.vvFileNames.GetCount(); ++y )
                for( uint32_t i = 0; i < m_Desc.vTileTextures.GetCount(); ++i )
                {
                    //for( uint32_t x = 0; x < m_Desc.Heightmap.vvFileNames[ y ].GetCount(); ++x )
                    {
                        const auto& Textures = m_Desc.vTileTextures[ i ];
                        uint32_t x, y;
                        Math::Map1DarrayIndexTo2DArrayIndex( i, m_TerrainInfo.RootCount.width,
                                                             m_TerrainInfo.RootCount.height, &x, &y );
                        {
                            vke_sprintf( name, sizeof( name ), "Heightmap_%d_%d", x, y );
                            Core::SLoadFileInfo Info;
                            //Info.FileInfo.pFileName = m_Desc.Heightmap.vvFileNames[ x ][ y ];
                            Info.FileInfo.FileName = Textures.Heightmap.GetData();
                            Info.CreateInfo.flags = Core::CreateResourceFlags::ASYNC;

                            RenderSystem::TextureHandle hTex;
                            ret = pCtx->LoadTexture( Info, &hTex );
                            if( VKE_SUCCEEDED( ret ) )
                            {
                                m_vHeightmapTextures[ currIndex ] = ( hTex );
                            }
                        }
                        {
                            vke_sprintf( name, sizeof( name ), "Heightmap_normal_%d_%d", x, y );
                            Core::SLoadFileInfo Info;
                            //Info.FileInfo.pFileName = m_Desc.Heightmap.vvNormalNames[ x ][ y ];
                            Info.FileInfo.FileName = Textures.HeightmapNormal.GetData();
                            Info.CreateInfo.flags = Core::CreateResourceFlags::ASYNC;
                            //Info.FileInfo.pName = name;
                            RenderSystem::TextureHandle hTex = INVALID_HANDLE;
                            //ret = pCtx->LoadTexture( Info, &hTex );
                            if( VKE_SUCCEEDED( ret ) )
                            {
                                m_vHeightmapNormalTextures[ currIndex ] = ( hTex );
                            }
                        }
                        currIndex++;
                    }
                }
            }
            ret = VKE_OK;
           
            return ret;
        }
        void CTerrain::_GetBindingDataForRootNode( const uint32_t& rootNodeIdx, STerrainUpdateBindingData* pOut )
        {
            pOut->index = rootNodeIdx;
            pOut->hHeightmap = m_vDummyTexViews[ 0 ];
            pOut->hHeightmapNormal = m_vDummyTexViews[ 0 ];
            pOut->hBilinearSampler = m_hHeightmapSampler;
            // pOut->phDiffuses = m_vDummyTexViews.GetData();
            // pOut->phDiffuseNormals = m_vDummyTexViews.GetData();
            // pOut->diffuseTextureCount = ( uint16_t )m_vDummyTexViews.GetCount();
            // pOut->hDiffuseSampler = m_hHeightmapSampler;
            if( !m_vHeightmapTexViews.IsEmpty() )
            {
                pOut->hHeightmap = m_vHeightmapTexViews[ rootNodeIdx ];
                pOut->hHeightmapNormal = m_vHeightmapNormalTexViews[ rootNodeIdx ];
            }

        }
        bool g_updateQT = true;
        bool g_updateRenderer = true;
        void CTerrain::Update( RenderSystem::CommandBufferPtr pCommandBuffer )
        {
#if VKE_DEBUG
            if(g_updateQT)
#endif            
            m_QuadTree._Update();
#if VKE_DEBUG
            if(g_updateRenderer)
#endif
            m_pRenderer->Update( pCommandBuffer, m_pScene );
        }
        void CTerrain::Render( RenderSystem::CommandBufferPtr pCommandbuffer )
        {
            m_pRenderer->Render( pCommandbuffer, m_pScene );
        }
        RenderSystem::PipelinePtr CTerrain::_GetPipelineForLOD( uint8_t lod )
        {
            return m_pRenderer->_GetPipelineForLOD( lod );
        }

        void CTerrain::SetLODTreshold(float value)
        {
            m_Desc.lodTreshold = value;
            m_QuadTree.m_Desc.lodTreshold = value;
            m_QuadTree.m_needUpdateLOD = true;
        }

    } // namespace Scene
   
} // namespace VKE