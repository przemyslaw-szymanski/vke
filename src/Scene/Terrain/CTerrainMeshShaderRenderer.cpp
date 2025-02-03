#include "CTerrainMeshShaderRenderer.h"

#include "Scene/Terrain/CTerrain.h"

#include "RenderSystem/CCommandBuffer.h"
#include "RenderSystem/CContextBase.h"
#include "RenderSystem/CDeviceContext.h"
#include "CVkEngine.h"
#include "Core/Managers/CResourceManager.h"
#include "RenderSystem/CRenderSystem.h"


namespace VKE::Scene
{
    CTerrainMeshShadingRenderer::CTerrainMeshShadingRenderer(CTerrain* pTerrain) :
        m_pTerrain{ pTerrain }
    {

    }

    CTerrainMeshShadingRenderer::~CTerrainMeshShadingRenderer()
    {
        _Destroy();
    }

    void CTerrainMeshShadingRenderer::_Destroy()
    {

    }

    Result CTerrainMeshShadingRenderer::_Create(const STerrainDesc& Desc,
        RenderSystem::CommandBufferPtr pCmdBuffer )
    {
        Result ret = _CreateVertexBuffer( pCmdBuffer );
        if (VKE_SUCCEEDED(ret))
        {
            ret = _CreateIndexBuffer( pCmdBuffer, 0 );
            if (VKE_SUCCEEDED(ret))
            {
                ret = _CreateMeshletBuffer( pCmdBuffer );
                if (VKE_SUCCEEDED(ret))
                {
                    ret = _CreateVertexIndexBuffer( pCmdBuffer );
                    if (VKE_SUCCEEDED(ret))
                    {
                        ret = _CreateInstancingBuffer( pCmdBuffer );
                        if (VKE_SUCCEEDED(ret))
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

        auto pDevice = pCmdBuffer->GetContext()->GetDeviceContext();
        RenderSystem::SUpdateBindingsHelper UpdateInfo;
        UpdateInfo.AddBinding( 0u, 0u, m_pVertexBuffer->GetSize(),
            m_pVertexBuffer->GetHandle(),
            RenderSystem::BindingTypes::BUFFER );
        UpdateInfo.AddBinding( 1u, 0u, m_pTriangleBuffer->GetSize(),
            m_pTriangleBuffer->GetHandle(),
            RenderSystem::BindingTypes::BUFFER );
        UpdateInfo.AddBinding( 2u, 0u, m_pMeshletBuffer->GetSize(),
            m_pMeshletBuffer->GetHandle(),
            RenderSystem::BindingTypes::BUFFER );
        UpdateInfo.AddBinding( 3u, 0u, m_pTileBuffer->GetSize(),
            m_pTileBuffer->GetHandle(),
            RenderSystem::BindingTypes::BUFFER );
        pDevice->UpdateDescriptorSet( UpdateInfo, &m_hTileDescSet );

        return ret;
    }

    Result CTerrainMeshShadingRenderer::_CreateResourceDescriptor(
        RenderSystem::CommandBufferPtr pCmdBuff )
    {
        Result ret = VKE_OK;
        auto pDevice = pCmdBuff->GetContext()->GetDeviceContext();
        RenderSystem::SCreateBindingDesc BindingDesc;
        {
            BindingDesc.SetDebugName( "MeshShaderTerrain" );
            BindingDesc.AddBuffer( 0, RenderSystem::PipelineStages::MESH, 1u );
            BindingDesc.AddBuffer( 1, RenderSystem::PipelineStages::MESH, 1u );
            BindingDesc.AddBuffer( 2, RenderSystem::PipelineStages::MESH, 1u );
            BindingDesc.AddBuffer( 3, RenderSystem::PipelineStages::ALL, 1u );
            BindingDesc.SetDebugName( "MeshShaderTerrain" );
            BindingDesc.LayoutDesc.SetDebugName( "MeshShaderTerrain" );
        }
        m_hTileDescSet = pDevice->CreateResourceBindings( BindingDesc );
        if (m_hTileDescSet == INVALID_HANDLE)
        {
            ret = VKE_FAIL;
        }
        return ret;
    }

    Result CTerrainMeshShadingRenderer::_CreateVertexBuffer( RenderSystem::CommandBufferPtr pCmdBuffer )
    {
        Result ret = VKE_FAIL;
        using PositionType = Math::CVector4;
        struct SVertex
        {
            PositionType Position;
            //ExtentF32    Texcoords;
        };
        auto pCtx = pCmdBuffer->GetContext()->GetDeviceContext();
        Utils::TCDynamicArray<SVertex, 1> vVertices;
        auto tileVertexCount = m_pTerrain->m_Desc.TileSize.min / m_pTerrain->m_Desc.vertexDistance;
        const auto lodCount = 1;
        const uint32_t vertexCountPerRow = (uint32_t)tileVertexCount + 1;
        const uint32_t lodVertexCount = vertexCountPerRow * vertexCountPerRow;
        //const uint32_t tileVertexSize = lodVertexCount * sizeof( SVertex );
        uint32_t totalVertexCount = lodVertexCount * lodCount;
        vVertices.Resize( totalVertexCount );
        //Math::CVector3 vecCurr = Math::CVector3::ZERO;
        PositionType CurrPos;
        CurrPos.x = 0;
        CurrPos.y = 0;
        float step = m_pTerrain->m_Desc.vertexDistance;
        uint32_t idx = 0;
        const float tileSize = tileVertexCount * step;
        const float halfTileSize = tileSize * 0.5f;
        bool startWith00 = true;
        ExtentF32 X = { -halfTileSize, halfTileSize };
        ExtentF32 Z = { -halfTileSize, halfTileSize };
        ExtentF32 Texcoords;
        // top left to right bottom
        if( startWith00 )
        {
            X = { 0, tileSize };
            Z = { 0, tileSize };
            for( uint8_t lod = 0; lod < lodCount; ++lod, step *= 2 )
            {
                // step = ( float )Math::CalcPow2( lod );
                //m_vDrawLODs[ lod ].vertexBufferOffset = lod * tileVertexSize;
                for( uint32_t z = 0; z < vertexCountPerRow; ++z )
                {
                    //vecCurr.z = Z.min - z * step;
                    CurrPos.y = Z.min - z * step;
                    Texcoords.y = ( float )( z ) / tileVertexCount;
                    for( uint32_t x = 0; x < vertexCountPerRow; ++x )
                    {
                        //vecCurr.x = X.min + x * step;
                        CurrPos.x = X.min + x * step;
                        Texcoords.x = ( float )( x ) / tileVertexCount;
                        //vVertices[ idx++ ] = { vecCurr, Texcoords };
                        auto& Vert = vVertices[ idx++ ];
                        Vert.Position = CurrPos;
                    }
                }
            }
        }
        else
        {
            for( uint8_t lod = 0; lod < lodCount; ++lod, step *= 2 )
            {
                // step = ( float )Math::CalcPow2( lod );
                //m_vDrawLODs[ lod ].vertexBufferOffset = lod * tileVertexSize;
                for( uint32_t z = 0; z < vertexCountPerRow; ++z )
                {
                    //vecCurr.z = Z.max - z * step;
                    CurrPos.y = Z.max - z * step;
                    Texcoords.y = ( float )( z ) / tileVertexCount;
                    for( uint32_t x = 0; x < vertexCountPerRow; ++x )
                    {
                        CurrPos.x = X.min + x * step;
                        Texcoords.x = ( float )( x ) / tileVertexCount;
                        auto& Vert = vVertices[ idx++ ];
                        Vert.Position = CurrPos;
                    }
                }
            }
        }
        if(true)
        {
#define VERT_C 4
            vVertices.Clear();
            uint32_t c = VERT_C;
            vVertices.Resize( c * c );
            float dist = 1.0f / c;
            PositionType BasePos;
            BasePos.x = -0.5f;
            BasePos.y = 0.5f;
            for (uint32_t y = 0; y < c; ++y)
            {
                
                for (uint32_t x = 0; x < c; ++x)
                {
                    auto& Pos = vVertices[ Math::Map2DArrayIndexTo1DArrayIndex( x, y, c ) ];
                    Pos.Position.x = BasePos.x + x * dist;
                    Pos.Position.y = BasePos.y - y * dist;
                }
            }
        }

        RenderSystem::SCreateBufferDesc BuffDesc;
        BuffDesc.Create.flags = Core::CreateResourceFlags::DEFAULT;
        BuffDesc.Buffer.memoryUsage = RenderSystem::MemoryUsages::STATIC | RenderSystem::MemoryUsages::BUFFER;
        BuffDesc.Buffer.usage = RenderSystem::BufferUsages::BUFFER;
        BuffDesc.Buffer.size = vVertices.GetCount() * sizeof( SVertex );
        BuffDesc.Buffer.SetDebugName( "VertexBuffer" );
        auto hBuffer = ( pCtx->CreateBuffer( BuffDesc ) );
        if( hBuffer != INVALID_HANDLE )
        {
            m_pVertexBuffer = pCtx->GetBuffer( hBuffer );
            RenderSystem::SUpdateMemoryInfo UpdateInfo;
            UpdateInfo.dataSize = BuffDesc.Buffer.size;
            UpdateInfo.dstDataOffset = 0;
            UpdateInfo.pData = vVertices.GetData();
#if VKE_RENDER_SYSTEM_DEBUG
            RenderSystem::SDebugInfo DebugInfo =
            {
                .pText = "Upload MeshShader VertexBuffer",
                .Color = RenderSystem::SColor::GREEN
            };
            UpdateInfo.pDebugInfo = &DebugInfo;
#endif
            ret = pCmdBuffer->GetContext()->UpdateBuffer(
                pCmdBuffer, UpdateInfo, &hBuffer );
        }
        return ret;
    }

    Result CTerrainMeshShadingRenderer::_CreateTriangleBuffer(
        RenderSystem::CommandBufferPtr pCmdBuffer )
    {
        Result ret = VKE_FAIL;
        struct STriangle
        {
            uint32_t v0, v1, v2;
        };
        
        Utils::TCDynamicArray<STriangle, 126> vTriangles;
        uint32_t vertexCount = VERT_C;
        uint32_t triCount = (vertexCount-1)*(vertexCount-1)*2;
        vTriangles.Reserve( triCount );

#define CALC_XY( _x, _y, _w ) ( uint16_t )( ( _x ) + ( _w ) * ( _y ) )
#define CALC_IDX_00( _x, _y, _w ) CALC_XY( _x, _y, _w )
#define CALC_IDX_01( _x, _y, _w ) CALC_XY( _x, _y + 1, _w )
#define CALC_IDX_10( _x, _y, _w ) CALC_XY( _x + 1, _y, _w )
#define CALC_IDX_11( _x, _y, _w ) CALC_XY( _x + 1, _y + 1, _w )

        for( uint16_t y = 0; y < vertexCount-1; ++y )
        {
            for( uint16_t x = 0; x < vertexCount-1; ++x )
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
                STriangle Tri0;
                Tri0.v0 = v00;
                Tri0.v1 = v01;
                Tri0.v2 = v10;
                vTriangles.PushBack( Tri0 );
                /*
                *   *   (0,0)----(1,0)
                / |     |   /    |
                *---*   (0,1)----(1,1)
                */
                STriangle Tri1;
                Tri1.v0 = v10;
                Tri1.v1 = v01;
                Tri1.v2 = v11;
                vTriangles.PushBack( Tri1 );
#endif
            }
        }

        auto pCtx = pCmdBuffer->GetContext()->GetDeviceContext();
        RenderSystem::SCreateBufferDesc BuffDesc;
        BuffDesc.Create.flags = Core::CreateResourceFlags::DEFAULT;
        BuffDesc.Buffer.memoryUsage = RenderSystem::MemoryUsages::STATIC | RenderSystem::MemoryUsages::BUFFER;
        BuffDesc.Buffer.usage = RenderSystem::BufferUsages::BUFFER;
        BuffDesc.Buffer.size = vTriangles.GetCount() * sizeof( STriangle );
        BuffDesc.Buffer.SetDebugName( "TriangleBuffer" );
        auto hBuffer = ( pCtx->CreateBuffer( BuffDesc ) );
        if( hBuffer != INVALID_HANDLE )
        {
            m_pTriangleBuffer = pCtx->GetBuffer( hBuffer );
            RenderSystem::SUpdateMemoryInfo UpdateInfo;
            UpdateInfo.dataSize = BuffDesc.Buffer.size;
            UpdateInfo.dstDataOffset = 0;
            UpdateInfo.pData = vTriangles.GetData();
#if VKE_RENDER_SYSTEM_DEBUG
            RenderSystem::SDebugInfo DebugInfo =
            {
                .pText = "Upload MeshShader TriangleBuffer",
                .Color = RenderSystem::SColor::GREEN
            };
            UpdateInfo.pDebugInfo = &DebugInfo;
#endif
            ret = pCmdBuffer->GetContext()->UpdateBuffer( pCmdBuffer, UpdateInfo, &hBuffer );
        }
        return ret;
    }
    
    Result CTerrainMeshShadingRenderer::_CreateIndexBuffer(
        RenderSystem::CommandBufferPtr pCmdBuffer, uint8_t lod )
    {
        Result ret = VKE_FAIL;
        using IndexType = uint32_t;
        using IndexBuffer = Utils::TCDynamicArray<IndexType, 1>;
        IndexBuffer vIndices;
        const auto& Desc = m_pTerrain->m_Desc;
        // const uint32_t vertexCount = Desc.tileRowVertexCount + 1;
        uint32_t vertexCount = ( uint32_t )( ( float )Desc.TileSize.min / Desc.vertexDistance ) + 1;
        uint32_t indexCount = 0;
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
        const uint32_t currVertexCount = vertexCount >> lod;
        const uint32_t currIndexCount = ( ( currVertexCount - 1 ) * 2 ) * ( currVertexCount - 1 );
        indexCount += currIndexCount * 3;
        vIndices.Resize( indexCount );
        uint32_t currIdx = 0;
#undef CALC_XY
#undef CALC_IDX_00
#undef CALC_IDX_01
#undef CALC_IDX_10
#undef CALC_IDX_11
#define CALC_XY( _x, _y, _w ) ( uint16_t )( ( _x ) + ( _w ) * ( _y ) )
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
        BuffDesc.Create.flags = Core::CreateResourceFlags::DEFAULT;
        BuffDesc.Buffer.memoryUsage = RenderSystem::MemoryUsages::STATIC | RenderSystem::MemoryUsages::BUFFER;
        BuffDesc.Buffer.usage = RenderSystem::BufferUsages::BUFFER;
        BuffDesc.Buffer.size = vIndices.GetCount() * sizeof( IndexType );
        BuffDesc.Buffer.SetDebugName( "IndexBuffer" );
        auto pContext = pCmdBuffer->GetContext();
        auto pDevice = pContext->GetDeviceContext();
        auto hBuffer = pDevice->CreateBuffer( BuffDesc );
        if( hBuffer != INVALID_HANDLE )
        {
            m_pIndexBuffer = pDevice->GetBuffer( hBuffer );
            RenderSystem::SUpdateMemoryInfo UpdateInfo;
            UpdateInfo.dataSize = BuffDesc.Buffer.size;
            UpdateInfo.dstDataOffset = 0;
            UpdateInfo.pData = vIndices.GetData();
            ret = pContext->UpdateBuffer( pCmdBuffer, UpdateInfo, &hBuffer );
        }
        return ret;
    }

    Result CTerrainMeshShadingRenderer::_CreateMeshletBuffer(RenderSystem::CommandBufferPtr pCmdBuffer)
    {
        Result ret = VKE_FAIL;

        struct SMeshletInfo
        {
            ExtentF32 Position;
            float pad1;
            float pad2;
            Math::CVector3 vec3Color;
            float pad3;
        };

        const auto& TerrainDesc = m_pTerrain->m_Desc;
        //const uint32_t maxMeshletVertexNum = 64;
        const uint32_t maxMeshletVertexNumInRow = 8; // 8*8 = 64
        const uint32_t tileVertexNumInRow = (uint32_t)( (float)TerrainDesc.TileSize.min / TerrainDesc.vertexDistance );
        const uint32_t meshletNumInRow = tileVertexNumInRow / maxMeshletVertexNumInRow;
        const uint32_t meshletCount = Math::CalcNumPow2( meshletNumInRow );
        const float meshletDistance = TerrainDesc.vertexDistance * tileVertexNumInRow;
        ( void )meshletDistance;
        const uint32_t bufferSize = sizeof(SMeshletInfo) * meshletCount;

        Utils::TCDynamicArray<SMeshletInfo, 1> vMeshlets;
        if( ( vMeshlets.Resize( meshletCount ) ) )
        {
            for(uint32_t i = 0; i < vMeshlets.GetCount(); ++i)
            {
                auto& Meshlet = vMeshlets[ i ];
                Meshlet.Position = { 0, 0 } ;//Math::Map1DarrayIndexTo2DArrayIndex( i, meshletNumInRow, meshletNumInRow ) * meshletDistance;
                Meshlet.vec3Color.x = ( float )( rand() % 155 + 100 );
                Meshlet.vec3Color.y = ( float )( rand() % 155 + 100 );
                Meshlet.vec3Color.z = ( float )( rand() % 155 + 100 );
                Meshlet.vec3Color.x = ( float )( 1 );
                Meshlet.vec3Color.y = ( float )( 0 );
                Meshlet.vec3Color.z = ( float )( 0 );
            }
            RenderSystem::SCreateBufferDesc Desc;
            Desc.Buffer.memoryUsage
                = RenderSystem::MemoryUsages::STATIC_BUFFER | RenderSystem::MemoryUsages::GPU_ACCESS;
            Desc.Buffer.usage = RenderSystem::BufferUsages::BUFFER | RenderSystem::BufferUsages::TRANSFER_DST;
            Desc.Buffer.size = bufferSize;
            Desc.Buffer.SetDebugName( "MeshletBuffer" );
            auto pDevice = pCmdBuffer->GetContext()->GetDeviceContext();
            auto hBuffer = pDevice->CreateBuffer( Desc );
            if( hBuffer != INVALID_HANDLE )
            {
                m_pMeshletBuffer = pDevice->GetBuffer( hBuffer );
                RenderSystem::SUpdateMemoryInfo UpdateInfo;
                UpdateInfo.dataSize = bufferSize;
                UpdateInfo.pData = vMeshlets.GetData();
                ret = pCmdBuffer->GetContext()->UpdateBuffer(
                    pCmdBuffer, UpdateInfo, &hBuffer );
            }
        }
        else
        {
            VKE_LOG_ERR( "Failed to allocate memory for terrain meshlets." );
        }
        return ret;
    }

    Result CTerrainMeshShadingRenderer::_CreateVertexIndexBuffer(RenderSystem::CommandBufferPtr pCmdBuffer)
    {
        Result ret = VKE_OK;
        return ret;
    }

    Result CTerrainMeshShadingRenderer::_CreateInstancingBuffer(RenderSystem::CommandBufferPtr pCmdBuff)
    {
        Result ret = VKE_FAIL;
        
        auto pDevice = pCmdBuff->GetContext()->GetDeviceContext();
        
        const bool needBufferResize = m_vBindingData.IsEmpty() ||
            m_vBindingData.GetCount() % 1024 != 0;

        if( needBufferResize )
        {
            if( m_pTileBuffer.IsValid() )
            {
                pDevice->DestroyBuffer( &m_pTileBuffer );
            }
            const uint32_t dataSize = Math::Round( Math::Max( 1u, m_vBindingData.GetCount() ), 1024u );
            const uint32_t bufferSize = sizeof( SInstancingData ) * dataSize;
            RenderSystem::SCreateBufferDesc Desc;
            Desc.Buffer.SetDebugName( "TerrainInstancing" );
            Desc.Buffer.memoryUsage
                = RenderSystem::MemoryUsages::STATIC_BUFFER | RenderSystem::MemoryUsages::GPU_ACCESS;
            Desc.Buffer.usage = RenderSystem::BufferUsages::BUFFER | RenderSystem::BufferUsages::TRANSFER_DST;
            Desc.Buffer.size = bufferSize;
            auto hBuffer = pDevice->CreateBuffer( Desc );
            if( hBuffer != INVALID_HANDLE )
            {
                m_pTileBuffer = pDevice->GetBuffer( hBuffer );
                ret = VKE_OK;
            }
        }
        return ret;
    }

    void CTerrainMeshShadingRenderer::Update(
        RenderSystem::CommandBufferPtr pCmdBuff, CScene*)
    {
        
    }

    void CTerrainMeshShadingRenderer::UpdateBindings(const STerrainUpdateBindingData& Data)
    {
        
    }

    Result CTerrainMeshShadingRenderer::UpdateBindings(
        RenderSystem::CommandBufferPtr pCmdBuff,
        const STerrainUpdateBindingData& Data )
    {
        m_vBindingData.PushBack( std::move( Data ) );
        Result ret = _CreateInstancingBuffer( pCmdBuff );

        return ret;
    }

    void CTerrainMeshShadingRenderer::_UploadInstancingBuffer(
        RenderSystem::CommandBufferPtr pCmdBuffer )
    {
        //RenderSystem::SUpdateMemoryInfo Info;
        //Info.dataSize = m_vBindingData.GetData();
        //Info.pData = m_vB
        //pCmdBuffer->GetContext()->UpdateBuffer( pCmdBuffer, Info, &m_hInstancingBuffer );
    }

    Result CTerrainMeshShadingRenderer::_CreatePipeline(RenderSystem::CommandBufferPtr pCmdBuff)
    {
        const auto& CurrState = pCmdBuff->GetCurrentState();
        if( CurrState.RenderPass.hash != m_renderPassHash )
        {
            auto pDevice = pCmdBuff->GetContext()->GetDeviceContext();
            auto pResMgr = pDevice->GetRenderSystem()->GetEngine()->GetResourceManager();
            if( m_pMeshShader.IsNull() )
            {
                RenderSystem::SCreateShaderDesc Desc =
                {
                    .Create=
                    {
                        .stages = Core::ResourceStages::FULL_LOAD,
                        .flags = Core::CreateResourceFlags::DEFERRED
                    },
                    .Shader =
                    {
                        .FileInfo =
                        {
                            .FileName = "data/shaders/terrain.ms.hlsl"
                        },
                        .type = RenderSystem::ShaderTypes::MESH,
                        //.profile = RenderSystem::ShaderProfiles::PROFILE_6_4,
                        .EntryPoint = "TerrainMS",
                        .Name = "TerrainMeshShader",
                    }
                };
                //m_pMeshShader = pDevice->CreateShader( Desc );
                m_pMeshShader = pResMgr->LoadShader( Desc );
            }
            if (m_pPixelShader.IsNull())
            {
                RenderSystem::SCreateShaderDesc Desc;
                Desc.Create.stages = Core::ResourceStages::FULL_LOAD;
                Desc.Create.flags = Core::CreateResourceFlags::DEFERRED;
                Desc.Shader.FileInfo.FileName = "data/shaders/terrain.ps.hlsl";
                Desc.Shader.type = RenderSystem::ShaderTypes::PIXEL;
                Desc.Shader.Name = "TerrainPixelShader";
                Desc.Shader.EntryPoint = "TerrainPS";
                m_pPixelShader = pResMgr->LoadShader( Desc );
            }
            if( ( m_pMeshShader.IsValid() ) &&
                ( m_pPixelShader.IsValid() ) )
            {
                auto hDescLayout = pDevice->GetDescriptorSetLayout( m_hTileDescSet );
                RenderSystem::SPipelineLayoutDesc LayoutDesc;
                LayoutDesc.SetDebugName( "MeshShaderTerrain" );
                LayoutDesc.vDescriptorSetLayouts.PushBack( hDescLayout );
                
                RenderSystem::SPipelineCreateDesc Desc;
                Desc.Create.OnCreate = [&](void* pData)
                {
                    RenderSystem::PipelinePtr pPipeline = *reinterpret_cast<RenderSystem::PipelinePtr*>( pData );
                };
                Desc.Create.flags = Core::CreateResourceFlags::DEFERRED;
                Desc.Pipeline.vColorRenderTargetFormats = CurrState.RenderPass.PipelineInfo.vColorRenderTargetFormats;
                Desc.Pipeline.depthRenderTargetFormat = CurrState.RenderPass.PipelineInfo.depthRenderTargetFormat;
                Desc.Pipeline.stencilRenderTargetFormat = CurrState.RenderPass.PipelineInfo.stencilRenderTargetFormat;
                Desc.Pipeline.Rasterization.Polygon.cullMode = RenderSystem::CullModes::NONE;
                Desc.Pipeline.Rasterization.Polygon.frontFace = RenderSystem::FrontFaces::COUNTER_CLOCKWISE;
                Desc.Pipeline.Rasterization.Polygon.mode = RenderSystem::PolygonModes::FILL;
                Desc.Pipeline.InputLayout.enable = false;
                Desc.Pipeline.hLayout = pDevice->CreatePipelineLayout( LayoutDesc )->GetHandle();
                Desc.Pipeline.Shaders.apShaders[ RenderSystem::ShaderTypes::MESH ] = m_pMeshShader;
                Desc.Pipeline.Shaders.apShaders[ RenderSystem::ShaderTypes::PIXEL ] = m_pPixelShader;
                Desc.Pipeline.SetDebugName( "MeshShaderTerrain" );
                m_pColorPipeline = pResMgr->CreatePipeline( Desc );
                m_pCurrPipeline = m_pColorPipeline;
                if (m_pColorPipeline.IsValid())
                {
                    m_renderPassHash = CurrState.RenderPass.hash;
                }
            }
        }
        return VKE_OK;
    }

    void CTerrainMeshShadingRenderer::Render(
        RenderSystem::CommandBufferPtr pCmdBuff, CScene*)
    {
        _CreatePipeline( pCmdBuff );
        if( m_pCurrPipeline.IsValid() && m_pCurrPipeline->IsResourceReady() )
        {
            pCmdBuff->Bind( m_pCurrPipeline );

            pCmdBuff->Bind( 0, m_hTileDescSet );
            pCmdBuff->DrawMesh( 1, 1, 1 );
        }
    }
}