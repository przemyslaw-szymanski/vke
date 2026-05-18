#include "CTerrainMeshShaderRenderer.h"

#include "Scene/Terrain/CTerrain.h"
#include "Scene/CScene.h"

#include "RenderSystem/CCommandBuffer.h"
#include "RenderSystem/CContextBase.h"
#include "RenderSystem/CDeviceContext.h"
#include "CVkEngine.h"
#include "Core/Managers/CResourceManager.h"
#include "RenderSystem/CRenderSystem.h"

namespace VKE::Scene
{
    CTerrainMeshShadingRenderer::CTerrainMeshShadingRenderer( CTerrain* pTerrain ) : m_pTerrain{ pTerrain }
    {
    }

    CTerrainMeshShadingRenderer::~CTerrainMeshShadingRenderer()
    {
        _Destroy();
    }

    void CTerrainMeshShadingRenderer::_Destroy()
    {
    }

    Result CTerrainMeshShadingRenderer::_Create( const STerrainDesc& Desc, RenderSystem::CommandBufferPtr pCmdBuffer )
    {
        const auto& TerrainDesc        = m_pTerrain->m_Desc;
        m_MeshletDesc.vertexCountInRow = 7;
        // Calculate base of meshlet info
        m_SubTileDesc.meshletSize     = ( m_MeshletDesc.vertexCountInRow * TerrainDesc.vertexDistance );
        m_SubTileDesc.meshletDistance = m_SubTileDesc.meshletSize - TerrainDesc.vertexDistance;
        float meshletCountInRow       = ( (float)TerrainDesc.TileSize.min / m_SubTileDesc.meshletDistance );
        // Adjust sizes to threadgroup size
        meshletCountInRow             = (float)Math::Round( (uint32_t)meshletCountInRow, m_TaskDesc.threadgroupSize );
        m_SubTileDesc.meshletDistance = TerrainDesc.TileSize.min / meshletCountInRow;
        // meshletSize = meshletDistance / ((vertexCount-1)/(vertexCount))
        m_SubTileDesc.meshletSize = m_SubTileDesc.meshletDistance /
                                    ( (float)( m_MeshletDesc.vertexCountInRow - 1 ) / m_MeshletDesc.vertexCountInRow );
        // vertexDistance = meshletSize - meshletDistance
        m_MeshletDesc.vertexDistance = m_SubTileDesc.meshletSize - m_SubTileDesc.meshletDistance;
        // m_TileDesc.meshletSize = m_MeshletDesc.vertexCountInRow * m_MeshletDesc.vertexDistance;
        // m_TileDesc.meshletDistance = m_TileDesc.meshletSize - m_MeshletDesc.vertexDistance;

        m_SubTileDesc.meshletCountInRow = static_cast< uint32_t >( meshletCountInRow );
        // m_MeshletDesc.vertexDistance = static_cast<float>( TerrainDesc.TileSize.min ) / (
        // m_TileDesc.meshletCountInRow );
        m_SubTileDesc.totalMeshletCount = m_SubTileDesc.meshletCountInRow * m_SubTileDesc.meshletCountInRow;

        m_SubTileDesc.dispatchSize = m_TaskDesc.threadgroupSize;
        // uint32_t remainingMeshThreadCount = m_TileDesc.totalMeshletCount % m_TaskDesc.threadgroupSize;
        m_SubTileDesc.dispatchSize = m_SubTileDesc.totalMeshletCount / m_TaskDesc.threadgroupSize;

        m_MeshletDesc.totalTriangleCount =
            ( m_MeshletDesc.vertexCountInRow - 1 ) * ( m_MeshletDesc.vertexCountInRow - 1 ) * 2;
        m_TaskDesc.tileCountInRow    = TerrainDesc.TileSize.max / TerrainDesc.TileSize.min;
        m_TaskDesc.totalSubTileCount = Math::CalcNumPow2( TerrainDesc.TileSize.max / TerrainDesc.TileSize.min );
        m_TaskDesc.dispatchSize      = ( m_SubTileDesc.dispatchSize ) * m_TaskDesc.totalSubTileCount;

        Result ret = _CreateVertexBuffer( pCmdBuffer );
        if( VKE_SUCCEEDED( ret ) )
        {
            ret = _CreateIndexBuffer( pCmdBuffer, 0 );
            if( VKE_SUCCEEDED( ret ) )
            {
                ret = _CreateMeshletBuffer( pCmdBuffer );
                if( VKE_SUCCEEDED( ret ) )
                {
                    ret = _CreateVertexIndexBuffer( pCmdBuffer );
                    if( VKE_SUCCEEDED( ret ) )
                    {
                        ret = _CreateInstancingBuffer( pCmdBuffer );
                        if( VKE_SUCCEEDED( ret ) )
                        {
                            ret = _CreateResourceDescriptor( pCmdBuffer );
                            if( VKE_SUCCEEDED( ret ) )
                            {
                                ret = _CreateTriangleBuffer( pCmdBuffer );
                            }
                        }
                    }
                }
            }
        }

        auto                                pDevice = pCmdBuffer->GetContext()->GetDeviceContext();
        RenderSystem::SUpdateBindingsHelper UpdateInfo;
        UpdateInfo.AddBinding(
            0u, 0u, m_pVertexBuffer->GetSize(), m_pVertexBuffer->GetHandle(), RenderSystem::BindingTypes::BUFFER );
        UpdateInfo.AddBinding(
            1u, 0u, m_pTriangleBuffer->GetSize(), m_pTriangleBuffer->GetHandle(), RenderSystem::BindingTypes::BUFFER );
        UpdateInfo.AddBinding(
            2u, 0u, m_pMeshletBuffer->GetSize(), m_pMeshletBuffer->GetHandle(), RenderSystem::BindingTypes::BUFFER );
        UpdateInfo.AddBinding(
            3u, 0u, m_pTileBuffer->GetSize(), m_pTileBuffer->GetHandle(), RenderSystem::BindingTypes::BUFFER );
        UpdateInfo.AddBinding(
            4u, 0u, m_pDebugBuffer->GetSize(), m_pDebugBuffer->GetHandle(), RenderSystem::BindingTypes::BUFFER );
        pDevice->UpdateDescriptorSet( UpdateInfo, &m_hTileDescSet );

        return ret;
    }

    Result CTerrainMeshShadingRenderer::_CreateResourceDescriptor( RenderSystem::CommandBufferPtr pCmdBuff )
    {
        Result                           ret     = VKE_OK;
        auto                             pDevice = pCmdBuff->GetContext()->GetDeviceContext();
        RenderSystem::SCreateBindingDesc BindingDesc;
        {
            BindingDesc.SetDebugName( "MeshShaderTerrain" );
            BindingDesc.AddBuffer( 0, RenderSystem::PipelineStages::MESH, 1u );
            BindingDesc.AddBuffer( 1, RenderSystem::PipelineStages::MESH, 1u );
            BindingDesc.AddBuffer( 2, RenderSystem::PipelineStages::MESH, 1u );
            BindingDesc.AddBuffer( 3, RenderSystem::PipelineStages::ALL, 1u );
            BindingDesc.AddBuffer( 4, RenderSystem::PipelineStages::ALL, 1u );

            BindingDesc.SetDebugName( "MeshShaderTerrain" );
            BindingDesc.LayoutDesc.SetDebugName( "MeshShaderTerrain" );
        }
        m_hTileDescSet = pDevice->CreateResourceBindings( BindingDesc );
        if( m_hTileDescSet == INVALID_HANDLE )
        {
            ret = VKE_FAIL;
        }
        return ret;
    }

    Result CTerrainMeshShadingRenderer::_CreateMeshletBuffer( RenderSystem::CommandBufferPtr pCmdBuffer )
    {
        Result ret = VKE_FAIL;

        struct SMeshletInfo
        {
            ExtentF32      Position;
            float          pad1;
            float          pad2;
            Math::CVector3 vec3Color;
            float          pad3;
        };

        // const auto& TerrainDesc = m_pTerrain->m_Desc;

        const uint32_t meshletNumInRow = m_SubTileDesc.meshletCountInRow;
        const uint32_t meshletCount    = Math::CalcNumPow2( meshletNumInRow );
        const float    meshletDistance =
            m_SubTileDesc.meshletDistance; // m_TileDesc.meshletSize - TerrainDesc.vertexDistance*1;
        (void)meshletDistance;
        const uint32_t bufferSize = sizeof( SMeshletInfo ) * meshletCount;

        Utils::TCDynamicArray< SMeshletInfo, 1 > vMeshlets;
        if( ( vMeshlets.Resize( meshletCount ) ) )
        {
            // Render front to back
            const ExtentF32 BasePosition( 0.0f, -(float)m_pTerrain->m_Desc.TileSize.min );

            for( uint32_t i = 0; i < vMeshlets.GetCount(); ++i )
            {
                auto&           Meshlet = vMeshlets[ i ];
                const ExtentF32 ArrayPosition =
                    ExtentF32( Math::Map1DarrayIndexTo2DArrayIndex( i, meshletNumInRow, meshletNumInRow ) );
                Meshlet.Position    = BasePosition + ArrayPosition * ExtentF32( meshletDistance );
                Meshlet.vec3Color.x = (float)( ( rand() % 155 + 100 ) / 255.0f );
                Meshlet.vec3Color.y = (float)( ( rand() % 155 + 100 ) / 255.0f );
                Meshlet.vec3Color.z = (float)( ( rand() % 155 + 100 ) / 255.0f );
            }
            RenderSystem::SCreateBufferDesc Desc;
            Desc.Buffer.memoryUsage =
                RenderSystem::MemoryUsages::STATIC_BUFFER | RenderSystem::MemoryUsages::GPU_ACCESS;
            Desc.Buffer.usage = RenderSystem::BUFFER_USAGE( RenderSystem::BufferUsages::BUFFER | RenderSystem::BufferUsages::TRANSFER_DST );
            Desc.Buffer.size  = bufferSize;
            Desc.Buffer.SetDebugName( "MeshletBuffer" );
            auto pDevice = pCmdBuffer->GetContext()->GetDeviceContext();
            auto hBuffer = pDevice->CreateBuffer( Desc );
            if( hBuffer != INVALID_HANDLE )
            {
                m_pMeshletBuffer = pDevice->GetBuffer( hBuffer );
                RenderSystem::SUpdateMemoryInfo UpdateInfo;
                UpdateInfo.dataSize = bufferSize;
                UpdateInfo.pData    = vMeshlets.GetData();
                ret                 = pCmdBuffer->GetContext()->UpdateBuffer( pCmdBuffer, UpdateInfo, &hBuffer );
            }

        }
        else
        {
            VKE_LOG_ERR( "Failed to allocate memory for terrain meshlets." );
        }
        return ret;
    }

    Result CTerrainMeshShadingRenderer::_CreateVertexIndexBuffer( RenderSystem::CommandBufferPtr pCmdBuffer )
    {
        Result ret = VKE_OK;
        return ret;
    }

    Result CTerrainMeshShadingRenderer::_CreateInstancingBuffer( RenderSystem::CommandBufferPtr pCmdBuff )
    {
        Result ret = VKE_FAIL;

        auto pDevice = pCmdBuff->GetContext()->GetDeviceContext();

        m_vTileData.Clear();
        for( uint32_t i = 0; i < m_vBindingData.GetCount(); ++i )
        {
            for( uint32_t s = 0; s < m_vBindingData[ i ].vSubTiles.GetCount(); ++s )
            {
                const auto&         Tile = m_vBindingData[ i ].vSubTiles[ s ];
                STileGPUBindingData Data;
                Data.Position                      = Tile.Position;
                Data.HeightmapNormalIndices.index2 = 2; // lod factor
                m_vTileData.PushBack( Data );
            }
        }

        const uint32_t currSize = sizeof( STileGPUBindingData ) * m_vTileData.GetCount();
        const bool     needBufferResize =
            m_vTileData.IsEmpty() || ( m_pTileBuffer.IsValid() && m_pTileBuffer->GetSize() < currSize );
        if( needBufferResize )
        {
            if( m_pTileBuffer.IsValid() )
            {
                // pDevice->DestroyBuffer( &m_pTileBuffer );
            }
            const uint32_t                  dataSize   = Math::Round( Math::Max( 1u, m_vTileData.GetCount() ), 1024u );
            const uint32_t                  bufferSize = sizeof( STileGPUBindingData ) * dataSize;
            RenderSystem::SCreateBufferDesc Desc;
            Desc.Buffer.SetDebugName( "MeshShaderTerrainTileData" );
            Desc.Buffer.memoryUsage =
                RenderSystem::MemoryUsages::STATIC_BUFFER | RenderSystem::MemoryUsages::GPU_ACCESS;
            Desc.Buffer.usage = RenderSystem::BUFFER_USAGE( RenderSystem::BufferUsages::BUFFER | RenderSystem::BufferUsages::TRANSFER_DST );
            Desc.Buffer.size  = bufferSize;
            auto hBuffer      = pDevice->CreateBuffer( Desc );
            if( hBuffer != INVALID_HANDLE )
            {
                m_pTileBuffer = pDevice->GetBuffer( hBuffer );
                ret           = VKE_OK;
            }
        }

        if( !m_vTileData.IsEmpty() )
        {
            RenderSystem::SUpdateMemoryInfo Update;
            Update.dataSize      = sizeof( STileGPUBindingData ) * m_vTileData.GetCount();
            Update.dstDataOffset = 0;
            Update.pData         = m_vTileData.GetData();
            pCmdBuff->GetContext()->UpdateBuffer( pCmdBuff, Update, &m_pTileBuffer );
        }
        return ret;
    }

    void CTerrainMeshShadingRenderer::Update( RenderSystem::CommandBufferPtr pCmdBuff, CScene* )
    {
    }

    void CTerrainMeshShadingRenderer::UpdateBindings( STerrainUpdateBindingData& Data )
    {
    }

    Result CTerrainMeshShadingRenderer::UpdateBindings( RenderSystem::CommandBufferPtr pCmdBuff,
                                                        STerrainUpdateBindingData&     Data )
    {
        m_hSceneDescSet = m_pTerrain->GetScene()->GetBindings();

        m_vBindingData.PushBack( std::move( Data ) );

        Result ret = _CreateInstancingBuffer( pCmdBuff );

        return ret;
    }

    void CTerrainMeshShadingRenderer::_UploadInstancingBuffer( RenderSystem::CommandBufferPtr pCmdBuffer )
    {
        // RenderSystem::SUpdateMemoryInfo Info;
        // Info.dataSize = m_vBindingData.GetData();
        // Info.pData = m_vB
        // pCmdBuffer->GetContext()->UpdateBuffer( pCmdBuffer, Info, &m_hInstancingBuffer );
    }

    Result CTerrainMeshShadingRenderer::_CreatePipeline( RenderSystem::CommandBufferPtr pCmdBuff )
    {
        const auto& CurrState = pCmdBuff->GetCurrentState();

        if( CurrState.RenderPass.hash != m_renderPassHash )
        {

            const auto&                                  TerrainDesc = m_pTerrain->m_Desc;
            const RenderSystem::SShaderDesc::DefineArray vDefines    = {
                { ( L"TASK_THREADGROUP_SIZE" ), ShaderCompilerString( m_TaskDesc.threadgroupSize ).GetData() },
                { ( L"MESH_THREADGROUP_SIZE" ), ShaderCompilerString( m_MeshletDesc.threadgroupSize ).GetData() },
                { ( L"VERTEX_COUNT_PER_ROW" ), ShaderCompilerString( m_MeshletDesc.vertexCountInRow ).GetData() },
                { ( L"VERTEX_DISTANCE" ), ShaderCompilerString( m_MeshletDesc.vertexDistance ).GetData() },
                { ( L"MESHLET_COUNT_PER_ROW" ), ShaderCompilerString( m_SubTileDesc.meshletCountInRow ).GetData() },
                { ( L"TOTAL_PRIMITIVE_COUNT" ), ShaderCompilerString( m_MeshletDesc.totalTriangleCount ).GetData() },
                { ( L"TOTAL_VERTEX_COUNT" ),
                     ShaderCompilerString( m_MeshletDesc.vertexCountInRow * m_MeshletDesc.vertexCountInRow ).GetData() },
                { ( L"SUBTILE_MESHLET_COUNT" ), ShaderCompilerString( m_SubTileDesc.totalMeshletCount ).GetData() },
                { ( L"SUBTILE_SIZE" ), ShaderCompilerString( TerrainDesc.TileSize.min ).GetData() },
                { ( L"SUBTILE_WORKGROUP_SIZE" ), ShaderCompilerString( m_SubTileDesc.dispatchSize ).GetData() },
                { ( L"MESHLET_SIZE" ), ShaderCompilerString( m_SubTileDesc.meshletSize ).GetData() },
                { ( L"MESHLET_DISTANCE" ), ShaderCompilerString( m_SubTileDesc.meshletDistance ).GetData() },
                { ( L"CALC_MESHLET_POS" ), ShaderCompilerString( 1 ).GetData() },
                { ( L"SUBTILE_IN_ROW_COUNT" ), ShaderCompilerString( m_TaskDesc.tileCountInRow ).GetData() },
                { ( L"TOP_VERTEX_POS_Z" ), ShaderCompilerString( 0u ).GetData() },
                { ( L"BOTTOM_VERTEX_POS_Z" ),
                     ShaderCompilerString( -(int32_t)( ( m_MeshletDesc.vertexCountInRow - 1 ) ) ).GetData() },
                { ( L"LEFT_VERTEX_POS_X" ), ShaderCompilerString( 0u ).GetData() },
                { ( L"RIGHT_VERTEX_POS_X" ), ShaderCompilerString( m_MeshletDesc.vertexCountInRow - 1 ).GetData() },
            };

            auto pDevice = pCmdBuff->GetContext()->GetDeviceContext();
            auto pResMgr = pDevice->GetRenderSystem()->GetEngine()->GetResourceManager();
            if( m_pMeshShader.IsNull() )
            {
                RenderSystem::SCreateShaderDesc Desc = { .Create = { .stages = Core::ResourceStages::FULL_LOAD,
                                                                     .flags  = Core::CreateResourceFlags::DEFERRED },
                                                         .Shader = {
                                                             .FileInfo = { .FileName = "data/shaders/terrain.ms.hlsl" },
                                                             .type     = RenderSystem::ShaderTypes::MESH,
                                                             //.profile = RenderSystem::ShaderProfiles::PROFILE_6_4,
                                                             .EntryPoint = "TerrainMS",
                                                             .Name       = "TerrainMeshShader",
                                                             .vDefines   = vDefines } };
                // m_pMeshShader = pDevice->CreateShader( Desc );
                m_pMeshShader = pResMgr->LoadShader( Desc );
            }
            if( m_pTaskShader.IsNull() )
            {
                RenderSystem::SCreateShaderDesc Desc;
                Desc.Create.stages            = Core::ResourceStages::FULL_LOAD;
                Desc.Create.flags             = Core::CreateResourceFlags::DEFERRED;
                Desc.Shader.FileInfo.FileName = "data/shaders/terrain.ms.hlsl";
                Desc.Shader.type              = RenderSystem::ShaderTypes::TASK;
                Desc.Shader.Name              = "TerrainTaskShader";
                Desc.Shader.EntryPoint        = "TerrainTS";
                Desc.Shader.vDefines          = vDefines;
                m_pTaskShader                 = pResMgr->LoadShader( Desc );
            }
            if( m_pPixelShader.IsNull() )
            {
                RenderSystem::SCreateShaderDesc Desc;
                Desc.Create.stages            = Core::ResourceStages::FULL_LOAD;
                Desc.Create.flags             = Core::CreateResourceFlags::DEFERRED;
                Desc.Shader.FileInfo.FileName = "data/shaders/terrain.ps.hlsl";
                Desc.Shader.type              = RenderSystem::ShaderTypes::PIXEL;
                Desc.Shader.Name              = "TerrainPixelShader";
                Desc.Shader.EntryPoint        = "TerrainPS";
                Desc.Shader.vDefines          = vDefines;
                m_pPixelShader                = pResMgr->LoadShader( Desc );
            }
            if( ( m_pMeshShader.IsValid() ) && ( m_pPixelShader.IsValid() ) && ( m_pTaskShader.IsValid() ) )
            {
                m_hSceneDescSet          = m_pTerrain->GetScene()->GetBindings();
                auto hSceneBindingLayout = pDevice->GetDescriptorSetLayout( m_hSceneDescSet );
                auto hDescLayout         = pDevice->GetDescriptorSetLayout( m_hTileDescSet );
                RenderSystem::SPipelineLayoutDesc LayoutDesc;
                LayoutDesc.SetDebugName( "MeshShaderTerrain" );
                LayoutDesc.vDescriptorSetLayouts = { hDescLayout, hSceneBindingLayout };

                RenderSystem::SPipelineCreateDesc Desc;
                Desc.Create.OnCreate = [ & ]( void* pData ) {
                    RenderSystem::PipelinePtr pPipeline = *reinterpret_cast< RenderSystem::PipelinePtr* >( pData );
                };
                Desc.Create.flags                       = Core::CreateResourceFlags::DEFERRED;
                Desc.Pipeline.vColorRenderTargetFormats = CurrState.RenderPass.PipelineInfo.vColorRenderTargetFormats;
                Desc.Pipeline.depthRenderTargetFormat   = CurrState.RenderPass.PipelineInfo.depthRenderTargetFormat;
                Desc.Pipeline.stencilRenderTargetFormat = CurrState.RenderPass.PipelineInfo.stencilRenderTargetFormat;
                Desc.Pipeline.Rasterization.Polygon.cullMode  = RenderSystem::CullModes::NONE;
                Desc.Pipeline.Rasterization.Polygon.frontFace = RenderSystem::FrontFaces::COUNTER_CLOCKWISE;
                Desc.Pipeline.Rasterization.Polygon.mode      = RenderSystem::PolygonModes::WIREFRAME;
                Desc.Pipeline.InputLayout.enable              = false;
                Desc.Pipeline.hLayout = pDevice->CreatePipelineLayout( LayoutDesc )->GetHandle();
                Desc.Pipeline.Shaders.apShaders[ RenderSystem::ShaderTypes::MESH ]  = m_pMeshShader;
                Desc.Pipeline.Shaders.apShaders[ RenderSystem::ShaderTypes::TASK ]  = m_pTaskShader;
                Desc.Pipeline.Shaders.apShaders[ RenderSystem::ShaderTypes::PIXEL ] = m_pPixelShader;
                Desc.Pipeline.SetDebugName( "MeshShaderTerrain" );
                m_pColorPipeline = pResMgr->CreatePipeline( Desc );
                m_pCurrPipeline  = m_pColorPipeline;
                if( m_pColorPipeline.IsValid() )
                {
                    m_renderPassHash = CurrState.RenderPass.hash;
                }
            }
        }
        return VKE_OK;
    }

    void CTerrainMeshShadingRenderer::Render( RenderSystem::CommandBufferPtr pCmdBuff, CScene* )
    {
        _CreatePipeline( pCmdBuff );
        if( m_pCurrPipeline.IsValid() && m_pCurrPipeline->IsResourceReady() )
        {
            pCmdBuff->Bind( m_pCurrPipeline );

            pCmdBuff->Bind( 0, m_hTileDescSet );
            pCmdBuff->Bind( 1, m_hSceneDescSet );
            pCmdBuff->DrawMesh( 3 * m_SubTileDesc.dispatchSize, 1, 1 );
            // pCmdBuff->DrawMesh( 1 * 1, 1, 1 );
        }
    }

    Result CTerrainMeshShadingRenderer::_CreateVertexBuffer( RenderSystem::CommandBufferPtr pCmdBuffer )
    {
        Result ret         = VKE_FAIL;
        using PositionType = ExtentF32;

        struct SVertex
        {
            PositionType Position;
            // ExtentF32    Texcoords;
        };

        auto                                pCtx = pCmdBuffer->GetContext()->GetDeviceContext();
        Utils::TCDynamicArray< SVertex, 1 > vVertices;
        auto           tileVertexCount   = m_pTerrain->m_Desc.TileSize.min / m_pTerrain->m_Desc.vertexDistance;
        const auto     lodCount          = 1;
        const uint32_t vertexCountPerRow = (uint32_t)tileVertexCount + 1;
        const uint32_t lodVertexCount    = vertexCountPerRow * vertexCountPerRow;
        // const uint32_t tileVertexSize = lodVertexCount * sizeof( SVertex );
        uint32_t totalVertexCount = lodVertexCount * lodCount;
        vVertices.Resize( totalVertexCount );
        // Math::CVector3 vecCurr = Math::CVector3::ZERO;
        PositionType CurrPos;
        CurrPos.x                = 0;
        CurrPos.y                = 0;
        float       step         = 1;
        const float tileSize     = tileVertexCount * step;
        const float halfTileSize = tileSize * 0.5f;
        ExtentF32   X            = { -halfTileSize, halfTileSize };
        ExtentF32   Z            = { -halfTileSize, halfTileSize };
        // top left to right bottom
        if( true )
        {
            vVertices.Clear();
            uint32_t c = m_MeshletDesc.vertexCountInRow;
            vVertices.Resize( c * c );
            float        dist = 1;
            PositionType BasePos;
            BasePos.x = 0;
            BasePos.y = 0;
            for( uint32_t y = 0; y < c; ++y )
            {
                for( uint32_t x = 0; x < c; ++x )
                {
                    auto& Pos      = vVertices[ Math::Map2DArrayIndexTo1DArrayIndex( x, y, c ) ];
                    Pos.Position.x = BasePos.x + x * dist;
                    float y1       = BasePos.y - y * dist;
                    Pos.Position.y = y1;
                }
            }
        }
        RenderSystem::SCreateBufferDesc BuffDesc;
        BuffDesc.Create.flags       = Core::CreateResourceFlags::DEFAULT;
        BuffDesc.Buffer.memoryUsage = RenderSystem::MemoryUsages::STATIC | RenderSystem::MemoryUsages::BUFFER;
        BuffDesc.Buffer.usage       = RenderSystem::BufferUsages::BUFFER;
        BuffDesc.Buffer.size        = vVertices.GetCount() * sizeof( SVertex );
        BuffDesc.Buffer.SetDebugName( "VertexBuffer" );
        auto hBuffer = ( pCtx->CreateBuffer( BuffDesc ) );
        if( hBuffer != INVALID_HANDLE )
        {
            m_pVertexBuffer = pCtx->GetBuffer( hBuffer );
            RenderSystem::SUpdateMemoryInfo UpdateInfo;
            UpdateInfo.dataSize      = BuffDesc.Buffer.size;
            UpdateInfo.dstDataOffset = 0;
            UpdateInfo.pData         = vVertices.GetData();
#if VKE_RENDER_SYSTEM_DEBUG
            RenderSystem::SDebugInfo DebugInfo = { .pText = "Upload MeshShader VertexBuffer",
                                                   .Color = RenderSystem::SColor::GREEN };
            UpdateInfo.pDebugInfo              = &DebugInfo;
#endif
            ret = pCmdBuffer->GetContext()->UpdateBuffer( pCmdBuffer, UpdateInfo, &hBuffer );
        }
        return ret;
    }

    Result CTerrainMeshShadingRenderer::_CreateTriangleBuffer( RenderSystem::CommandBufferPtr pCmdBuffer )
    {
        Result ret = VKE_FAIL;

        struct SPackedTriangle
        {
            union
            {
                struct
                {
                    uint32_t v2 : 10;
                    uint32_t v1 : 10;
                    uint32_t v0 : 10;
                    uint32_t pad : 2;
                };

                uint32_t v;
            };
        };

        struct STriangle
        {
            uint32_t v0, v1, v2, v3;
        };
#if VKE_MS_TERRAIN_DEBUG
        const size_t                            DataSize = sizeof( STriangle );
        Utils::TCDynamicArray< STriangle, 126 > vTriangles;
#else
        const size_t                           DataSize = sizeof( uint32_t );
        Utils::TCDynamicArray< uint32_t, 126 > vTriangles;
#endif
        uint32_t vertexCount = m_MeshletDesc.vertexCountInRow;
        uint32_t triCount    = ( vertexCount - 1 ) * ( vertexCount - 1 ) * 2;
        vTriangles.Reserve( triCount );
#define CALC_XY( _x, _y, _w ) (uint16_t)( ( _x ) + ( _w ) * ( _y ) )
#define CALC_IDX_00( _x, _y, _w ) CALC_XY( _x, _y, _w )
#define CALC_IDX_01( _x, _y, _w ) CALC_XY( _x, _y + 1, _w )
#define CALC_IDX_10( _x, _y, _w ) CALC_XY( _x + 1, _y, _w )
#define CALC_IDX_11( _x, _y, _w ) CALC_XY( _x + 1, _y + 1, _w )
        for( uint16_t y = 0; y < vertexCount - 1; ++y )
        {
            for( uint16_t x = 0; x < vertexCount - 1; ++x )
            {
                const auto v00 = CALC_IDX_00( x, y, vertexCount );
                const auto v10 = CALC_IDX_10( x, y, vertexCount );
                const auto v01 = CALC_IDX_01( x, y, vertexCount );
                const auto v11 = CALC_IDX_11( x, y, vertexCount );
#if VKE_SCENE_TERRAIN_CCW
#if VKE_SCENE_TERRAIN_VB_START_FROM_TOP_LEFT_CORNER
                /*
                *---*   (0,0)----(1,0)
                | /       |   /    |
                *   *   (0,1)----(1,1)
                */
                vIndices[ currIdx++ ] = v00;
                vIndices[ currIdx++ ] = v01;
                vIndices[ currIdx++ ] = v10;
                /*
                *   *   (0,0)----(1,0)
                  / |     |   /    |
                *---*   (0,1)----(1,1)
                */
                vIndices[ currIdx++ ] = v10;
                vIndices[ currIdx++ ] = v01;
                vIndices[ currIdx++ ] = v11;
#else
                /*
                *---*   (0,1)----(1,1)
                | /       |   /    |
                *   *   (0,0)----(1,0)
                */
                vIndices[ currIdx++ ] = v01;
                vIndices[ currIdx++ ] = v10;
                vIndices[ currIdx++ ] = v00;
                /*
                *   *   (0,1)----(1,1)
                  / |     |   /    |
                *---*   (0,0)----(1,0)
                */
                vIndices[ currIdx++ ] = v11;
                vIndices[ currIdx++ ] = v10;
                vIndices[ currIdx++ ] = v01;
#endif
#else
                /*
                *---*   (0,0)----(1,0)
                | /       |   /    |
                *   *   (0,1)----(1,1)
                */
#if VKE_MS_TERRAIN_DEBUG
                STriangle Tri0;
                Tri0.v0 = v00;
                Tri0.v1 = v01;
                Tri0.v2 = v10;
                STriangle Tri1;
                Tri1.v0 = v10;
                Tri1.v1 = v01;
                Tri1.v2 = v11;
                vTriangles.PushBack( Tri0 );
                vTriangles.PushBack( Tri1 );
#else
                SPackedTriangle PTri0;
                PTri0.v0 = v00;
                PTri0.v1 = v01;
                PTri0.v2 = v10;
                vTriangles.PushBack( PTri0.v );
                /*
                *   *   (0,0)----(1,0)
                / |     |   /    |
                *---*   (0,1)----(1,1)
                */
                SPackedTriangle PTri1;
                PTri1.v0 = v10;
                PTri1.v1 = v01;
                PTri1.v2 = v11;
                vTriangles.PushBack( PTri1.v );
#endif // DBG
#endif
            }
        }
        VKE_ASSERT( m_MeshletDesc.totalTriangleCount == vTriangles.GetCount() );
        auto                            pCtx = pCmdBuffer->GetContext()->GetDeviceContext();
        RenderSystem::SCreateBufferDesc BuffDesc;
        BuffDesc.Create.flags       = Core::CreateResourceFlags::DEFAULT;
        BuffDesc.Buffer.memoryUsage = RenderSystem::MemoryUsages::STATIC | RenderSystem::MemoryUsages::BUFFER;
        BuffDesc.Buffer.usage       = RenderSystem::BufferUsages::BUFFER;
        BuffDesc.Buffer.size        = vTriangles.GetCount() * DataSize;
        BuffDesc.Buffer.SetDebugName( "TriangleBuffer" );
        auto hBuffer = ( pCtx->CreateBuffer( BuffDesc ) );
        if( hBuffer != INVALID_HANDLE )
        {
            m_pTriangleBuffer = pCtx->GetBuffer( hBuffer );
            RenderSystem::SUpdateMemoryInfo UpdateInfo;
            UpdateInfo.dataSize      = BuffDesc.Buffer.size;
            UpdateInfo.dstDataOffset = 0;
            UpdateInfo.pData         = vTriangles.GetData();
#if VKE_RENDER_SYSTEM_DEBUG
            RenderSystem::SDebugInfo DebugInfo = { .pText = "Upload MeshShader TriangleBuffer",
                                                   .Color = RenderSystem::SColor::GREEN };
            UpdateInfo.pDebugInfo              = &DebugInfo;
#endif
            ret = pCmdBuffer->GetContext()->UpdateBuffer( pCmdBuffer, UpdateInfo, &hBuffer );
            if( VKE_SUCCEEDED( ret ) )
            {
                BuffDesc.Buffer.size =
                    sizeof( float ) * 4 * m_SubTileDesc.totalMeshletCount * m_TaskDesc.totalSubTileCount;
                BuffDesc.Buffer.memoryUsage = RenderSystem::MemoryUsages::CPU_READ | RenderSystem::MemoryUsages::BUFFER;
                hBuffer                     = pCtx->CreateBuffer( BuffDesc );
                m_pDebugBuffer              = pCtx->GetBuffer( hBuffer );
            }
        }
        return ret;
    }

    Result CTerrainMeshShadingRenderer::_CreateIndexBuffer( RenderSystem::CommandBufferPtr pCmdBuffer, uint8_t lod )
    {
        Result ret        = VKE_FAIL;
        using IndexType   = uint32_t;
        using IndexBuffer = Utils::TCDynamicArray< IndexType, 1 >;
        IndexBuffer vIndices;
        const auto& Desc = m_pTerrain->m_Desc;
        // const uint32_t vertexCount = Desc.tileRowVertexCount + 1;
        uint32_t vertexCount = (uint32_t)( (float)Desc.TileSize.min / Desc.vertexDistance ) + 1;
        uint32_t indexCount  = 0;
        // Calc index count
        // ( ( (vert - 1) * 2 ) * ( (vert-1) ) ) * 3
        // (4-1) * 2 * (4-1) * 3 = 3 * 2 * 3 * 3 = 6 * 9 = 54
        // (2-1) * 2 * (2-1) * 3 = 1 * 2 * 1 * 3 = 6
        /*
        *   *   *   *
        | / | / | / |
        *   *   *   *
        | / | / | / |
        *   *   *   *
        | / | / | / |
        *   *   *   *
        */
        const uint32_t currVertexCount  = vertexCount >> lod;
        const uint32_t currIndexCount   = ( ( currVertexCount - 1 ) * 2 ) * ( currVertexCount - 1 );
        indexCount                     += currIndexCount * 3;
        vIndices.Resize( indexCount );
        uint32_t currIdx = 0;
#undef CALC_XY
#undef CALC_IDX_00
#undef CALC_IDX_01
#undef CALC_IDX_10
#undef CALC_IDX_11
#define CALC_XY( _x, _y, _w ) (uint16_t)( ( _x ) + ( _w ) * ( _y ) )
#if VKE_SCENE_TERRAIN_VB_START_FROM_TOP_LEFT_CORNER
#define CALC_IDX_00( _x, _y, _w ) CALC_XY( _x, _y, _w )
#define CALC_IDX_01( _x, _y, _w ) CALC_XY( _x, _y + 1, _w )
#define CALC_IDX_10( _x, _y, _w ) CALC_XY( _x + 1, _y, _w )
#define CALC_IDX_11( _x, _y, _w ) CALC_XY( _x + 1, _y + 1, _w )
#else
#define CALC_IDX_00( _x, _y, _w ) CALC_XY( _x, _y + 1, _w )
#define CALC_IDX_01( _x, _y, _w ) CALC_XY( _x, _y, _w )
#define CALC_IDX_10( _x, _y, _w ) CALC_XY( _x + 1, _y + 1, _w )
#define CALC_IDX_11( _x, _y, _w ) CALC_XY( _x + 1, _y, _w )
#endif
        for( uint16_t y = 0; y < vertexCount - 1; ++y )
        {
            for( uint16_t x = 0; x < vertexCount - 1; ++x )
            {
                const auto v00 = CALC_IDX_00( x, y, vertexCount );
                const auto v10 = CALC_IDX_10( x, y, vertexCount );
                const auto v01 = CALC_IDX_01( x, y, vertexCount );
                const auto v11 = CALC_IDX_11( x, y, vertexCount );
#if VKE_SCENE_TERRAIN_CCW
#if VKE_SCENE_TERRAIN_VB_START_FROM_TOP_LEFT_CORNER
                /*
                *---*   (0,0)----(1,0)
                | /       |   /    |
                *   *   (0,1)----(1,1)
                */
                vIndices[ currIdx++ ] = v00;
                vIndices[ currIdx++ ] = v01;
                vIndices[ currIdx++ ] = v10;
                /*
                *   *   (0,0)----(1,0)
                  / |     |   /    |
                *---*   (0,1)----(1,1)
                */
                vIndices[ currIdx++ ] = v10;
                vIndices[ currIdx++ ] = v01;
                vIndices[ currIdx++ ] = v11;
#else
                /*
                *---*   (0,1)----(1,1)
                | /       |   /    |
                *   *   (0,0)----(1,0)
                */
                vIndices[ currIdx++ ] = v01;
                vIndices[ currIdx++ ] = v10;
                vIndices[ currIdx++ ] = v00;
                /*
                *   *   (0,1)----(1,1)
                  / |     |   /    |
                *---*   (0,0)----(1,0)
                */
                vIndices[ currIdx++ ] = v11;
                vIndices[ currIdx++ ] = v10;
                vIndices[ currIdx++ ] = v01;
#endif
#else
                /*
                *---*   (0,0)----(1,0)
                | /       |   /    |
                *   *   (0,1)----(1,1)
                */
                vIndices[ currIdx++ ] = v00;
                vIndices[ currIdx++ ] = v10;
                vIndices[ currIdx++ ] = v01;
                /*
                *   *   (0,0)----(1,0)
                / |     |   /    |
                *---*   (0,1)----(1,1)
                */
                vIndices[ currIdx++ ] = v01;
                vIndices[ currIdx++ ] = v10;
                vIndices[ currIdx++ ] = v11;
#endif
            }
        }
        VKE_ASSERT2( vIndices.GetCount() == currIdx, "" );
        // vIndices = {0,1,2};
        RenderSystem::SCreateBufferDesc BuffDesc;
        BuffDesc.Create.flags       = Core::CreateResourceFlags::DEFAULT;
        BuffDesc.Buffer.memoryUsage = RenderSystem::MemoryUsages::STATIC | RenderSystem::MemoryUsages::BUFFER;
        BuffDesc.Buffer.usage       = RenderSystem::BufferUsages::BUFFER;
        BuffDesc.Buffer.size        = vIndices.GetCount() * sizeof( IndexType );
        BuffDesc.Buffer.SetDebugName( "IndexBuffer" );
        auto pContext = pCmdBuffer->GetContext();
        auto pDevice  = pContext->GetDeviceContext();
        auto hBuffer  = pDevice->CreateBuffer( BuffDesc );
        if( hBuffer != INVALID_HANDLE )
        {
            m_pIndexBuffer = pDevice->GetBuffer( hBuffer );
            RenderSystem::SUpdateMemoryInfo UpdateInfo;
            UpdateInfo.dataSize      = BuffDesc.Buffer.size;
            UpdateInfo.dstDataOffset = 0;
            UpdateInfo.pData         = vIndices.GetData();
            ret                      = pContext->UpdateBuffer( pCmdBuffer, UpdateInfo, &hBuffer );
        }
        return ret;
    }
} // namespace VKE::Scene