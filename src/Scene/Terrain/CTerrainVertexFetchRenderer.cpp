#include "Scene/Terrain/CTerrainVertexFetchRenderer.h"
#include "Scene/Terrain/CTerrain.h"
#include "Scene/CScene.h"

#include "RenderSystem/CDeviceContext.h"
#include "RenderSystem/CGraphicsContext.h"

#include "RenderSystem/IFrameGraph.h"

#include "Core/Utils/CProfiler.h"

#include "RenderSystem/CDDI.h"


#define VKE_SCENE_TERRAIN_DEBUG_LOD 1
#define RENDER_WIREFRAME 0
#define VKE_SCENE_TERRAIN_CCW VKE_USE_LEFT_HANDED_COORDINATES
//(0 && VKE_USE_LEFT_HANDED_COORDINATES)
#define VKE_TERRAIN_PROFILE_RENDERING 0
#define VKE_SCENE_TERRAIN_VB_START_FROM_TOP_LEFT_CORNER 0
#define VKE_TERRAIN_INSTANCING_RENDERING 1
#define VKE_TERRAIN_BINDLESS_TEXTURES 1

#include "RenderSystem/CRenderSystem.h"
#include "Core/Managers/CFileManager.h"
#include "RenderSystem/CDeviceContext.h"
#include "CVkEngine.h"


namespace VKE
{
    namespace Scene
    {
        struct SPushConstants
        {
            Math::CVector3  vecPosition;
            uint32_t        textureId;
        };

        static cstr_t g_pGLSLTerrainVS = VKE_TO_STRING
        (
            #version 450 core\n

            #define BASE_TILE_SIZE 32.0
            #define TILE_VERTEX_COUNT 32.0

            layout(set = 0, binding = 0) uniform PerFrameTerrainConstantBuffer
            {
                mat4    mtxViewProj;
                vec2    vec2TerrainSize;
                vec2    vec2TerrainHeight;
                uint    tileRowVertexCount;
            } FrameData;

            layout(set = 1, binding = 0) uniform PerTileConstantBuffer
            {
                vec4    vec4Position;
                vec4    vec4Color;
                uint    vertexShift;
                float   tileSize;
                uint    topVertexShift; // vertex move to highest lod to create stitches
                uint    bottomVertexShift;
                uint    leftVertexShift;
                uint    rightVertexShift;
            } TileData;

            layout(set = 1, binding = 1) uniform sampler VertexFetchSampler;
            layout(set = 1, binding = 2) uniform texture2D HeightmapTextures[10];
            //layout(set = 1, binding = 1) uniform sampler2D HeightmapTexture;

            layout(location = 0) in vec3 iPosition;
            //layout(location = 1) in vec2 iTexcoord;
            layout(location = 0) out vec4 oColor;
            layout(location = 1) out vec2 oTexcoord;

            struct SVertexShift
            {
                uint top;
                uint bottom;
                uint left;
                uint right;
            };

            SVertexShift UnpackVertexShift(uint vertexShift)
            {
                SVertexShift s;
                s.top = (vertexShift >> 24u) & 0xFF;
                s.bottom = (vertexShift >> 16u) & 0xFF;
                s.left = (vertexShift >> 8u) & 0xFF;
                s.right = (vertexShift) & 0xFF;
                return s;
            }

            float SampleToRange(float v, vec2 vec2Range)
            {
                float range = vec2Range.y - vec2Range.x;
                return vec2Range.x + v * range;
            }

            void main()
            {
                mat4 mtxMVP = FrameData.mtxViewProj;
                vec3 iPos = iPosition;
                //vec2 texSize = textureSize(HeightmapTexture, 0);
                vec2 texSize = textureSize(sampler2D(HeightmapTextures[0], VertexFetchSampler), 0);

                // Vertex shift is packed in order: top, bottom, left, right
                SVertexShift Shift = UnpackVertexShift(TileData.vertexShift);
                Shift.top = TileData.topVertexShift;
                Shift.bottom = TileData.bottomVertexShift;
                Shift.left = TileData.leftVertexShift;
                Shift.right = TileData.rightVertexShift;
                // There is only one vertex buffer used with the highest lod.
                // Highest lod is the smallest, most dense drawcall
                // tileRowVertexCount is configured in an app (TerrainDesc)
                float vertexDistance = (TileData.tileSize / TILE_VERTEX_COUNT);
                // For each lod vertex position must be scaled by vertex distance
                /*iPos *= vertexDistance;

                if( iPos.z == 0.0f && Shift.top > 0 )
                {
                iPos.x -= mod(gl_VertexIndex, Shift.top) * vertexDistance;
                }
                else if (iPos.z == -TileData.tileSize && Shift.bottom > 0)
                {
                iPos.x -= mod(gl_VertexIndex, Shift.bottom) * vertexDistance;
                }
                else if (iPos.x == 0.0f && Shift.left > 0)
                {
                iPos.z -= mod(gl_VertexIndex, Shift.left) * vertexDistance;
                }
                else if (iPos.x == TileData.tileSize && Shift.right > 0)
                {
                float modulo = mod(gl_VertexIndex, Shift.right);
                iPos.z -= modulo * (vertexDistance);
                }*/

                if (iPos.z == 0.0f && Shift.top > 0)
                {
                    iPos.x -= mod(iPos.x, Shift.top);
                }
                else if (iPos.z == -BASE_TILE_SIZE && Shift.bottom > 0)
                {
                    iPos.x -= mod(iPos.x, Shift.bottom);
                }
                else if (iPos.x == 0.0f && Shift.left > 0)
                {
                    iPos.z -= mod(iPos.z, Shift.left);
                }
                if (iPos.x == BASE_TILE_SIZE && Shift.right > 0)
                {
                    iPos.z -= mod(iPos.z, Shift.right);
                }
                iPos *= vertexDistance;

                vec3 v3Pos = iPos + TileData.vec4Position.xyz;

                vec2 v2HalfSize = texSize * 0.5f;

                ivec2 v2Texcoords = ivec2(v3Pos.x + v2HalfSize.x, v3Pos.z + v2HalfSize.y);
                //vec4 height = texelFetch(HeightmapTexture, v2Texcoords, 0);
                vec4 height = texelFetch(sampler2D(HeightmapTextures[0], VertexFetchSampler), v2Texcoords, 0);
                //height = texture( HeightmapTexture, v2Texcoords / texSize );

                v3Pos.y = SampleToRange(height.r, FrameData.vec2TerrainHeight);

                gl_Position = mtxMVP * vec4(v3Pos, 1.0);
                //gl_Position = vec4(v2Texcoords.x, 0, v2Texcoords.y, 1.0);
                oColor = TileData.vec4Color;
                oTexcoord = v2Texcoords / texSize;
            }
        );

        static cstr_t g_pGLSLTerrainPS = VKE_TO_STRING
        (
            #version 450 core\n

            //layout(set = 1, binding = 1) uniform sampler2D HeightmapTexture;
            layout(set = 1, binding = 1) uniform sampler VertexFetchSampler;
            layout(set = 1, binding = 2) uniform texture2D HeightmapTextures[10];

            layout(location = 0) in vec4 iColor;
            layout(location = 1) in vec2 iTexcoord;
            layout(location = 0) out vec4 oColor;

            void main()
            {
                //oColor = texture( HeightmapTexture, iTexcoord );
                oColor = texture(sampler2D(HeightmapTextures[0], VertexFetchSampler), iTexcoord);
                //oColor *= iColor;
            }
        );

        static cstr_t g_pHLSLTerrainVS =
#include "Shaders/VertexFetch.vs.hlsl.h"
        static const char* g_pHLSLTerrainPS =
#include "Shaders/VertexFetch.ps.hlsl.h"

#if VKE_USE_HLSL_SYNTAX
        cstr_t g_pTerrainVS = g_pHLSLTerrainVS;
        cstr_t g_pTerrainPS = g_pHLSLTerrainPS;
#else
        cstr_t g_pTerrainVS = g_pGLSLTerrainVS;
        cstr_t g_pTerrainPS = g_pGLSLTerrainPS;
#endif



        RenderSystem::SCreateBindingDesc g_TileBindingDesc;
        RenderSystem::SCreateBindingDesc g_InstanceBindingDesc;

        CTerrainVertexFetchRenderer::~CTerrainVertexFetchRenderer()
        {

        }

        void CTerrainVertexFetchRenderer::_Destroy()
        {

        }

        RenderSystem::PipelinePtr CTerrainVertexFetchRenderer::_GetPipelineForLOD( uint8_t lod )
        {
            const auto& LOD = m_vDrawLODs[ lod ];
            return LOD.pPipeline;
        }

        Result CTerrainVertexFetchRenderer::_CreateVertexBuffer( const STerrainDesc& Desc,
                                                                 RenderSystem::CommandBufferPtr pCommandBuffer )
        {
            auto pCtx = pCommandBuffer->GetContext()->GetDeviceContext();
            Utils::TCDynamicArray<SVertex, 1> vVertices;
            auto tileVertexCount = m_pTerrain->m_tileVertexCount;

            const auto lodCount = Desc.lodCount;
            const uint32_t vertexCountPerRow = tileVertexCount + 1;
            const uint32_t lodVertexCount = vertexCountPerRow * vertexCountPerRow;
            const uint32_t tileVertexSize = lodVertexCount * sizeof( SVertex );
            uint32_t totalVertexCount = lodVertexCount * lodCount;

            vVertices.Resize( totalVertexCount );

            Math::CVector3 vecCurr = Math::CVector3::ZERO;
            float step = Desc.vertexDistance;
            uint32_t idx = 0;

            const float tileSize = tileVertexCount * Desc.vertexDistance;
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
                    //step = ( float )Math::CalcPow2( lod );
                    m_vDrawLODs[ lod ].vertexBufferOffset = lod * tileVertexSize;
                    for( uint32_t z = 0; z < vertexCountPerRow; ++z )
                    {
                        vecCurr.z = Z.min - z * step;
                        Texcoords.y = (float)(z) / tileVertexCount;
                        for( uint32_t x = 0; x < vertexCountPerRow; ++x )
                        {
                            vecCurr.x = X.min + x * step;
                            Texcoords.x = (float)(x) / tileVertexCount;
                            vVertices[ idx++ ] = { vecCurr, Texcoords };
                        }
                    }
                }
            }
            else
            {
                for( uint8_t lod = 0; lod < lodCount; ++lod, step *= 2 )
                {
                    //step = ( float )Math::CalcPow2( lod );
                    m_vDrawLODs[ lod ].vertexBufferOffset = lod * tileVertexSize;
                    for( uint32_t z = 0; z < vertexCountPerRow; ++z )
                    {
                        vecCurr.z = Z.max - z * step;
                        Texcoords.y = (float)(z) / tileVertexCount;

                        for( uint32_t x = 0; x < vertexCountPerRow; ++x )
                        {
                            vecCurr.x = X.min + x * step;
                            Texcoords.x = (float)(x) / tileVertexCount;

                            vVertices[idx++] = { vecCurr, Texcoords };
                        }
                    }
                }
            }

            RenderSystem::SCreateBufferDesc BuffDesc;
            BuffDesc.Create.flags = Core::CreateResourceFlags::DEFAULT;
            BuffDesc.Buffer.memoryUsage = RenderSystem::MemoryUsages::STATIC | RenderSystem::MemoryUsages::BUFFER;
            BuffDesc.Buffer.usage = RenderSystem::BufferUsages::VERTEX_BUFFER;
            BuffDesc.Buffer.size = vVertices.GetCount() * sizeof( SVertex );
            BuffDesc.Buffer.SetDebugName( "VKE_Scene_VertexFetchTerrain_VertexBuffer" );
            m_ahVertexBuffers[DrawTypes::TRIANGLES] = HandleCast<RenderSystem::VertexBufferHandle>( pCtx->CreateBuffer( BuffDesc ) );
            RenderSystem::SUpdateMemoryInfo UpdateInfo;
            UpdateInfo.dataSize = BuffDesc.Buffer.size;
            UpdateInfo.dstDataOffset = 0;
            UpdateInfo.pData = vVertices.GetData();
            Result ret = pCommandBuffer->GetContext()->UpdateBuffer(
                pCommandBuffer, UpdateInfo, ( RenderSystem::BufferHandle* )&m_ahVertexBuffers[DrawTypes::TRIANGLES] );
            m_ahVertexBuffers[ DrawTypes::QUADS ] = m_ahVertexBuffers[DrawTypes::TRIANGLES]; // The same vertex buffer for both types
            return ret;
        }

        uint32_t vke_force_inline CalcIndexCountForLOD( const uint32_t vertexCount, const uint8_t lodIndex )
        {
            const uint32_t currVertexCount = vertexCount >> lodIndex;
            const uint32_t currIndexCount = ((currVertexCount - 1) * 2) * (currVertexCount - 1);
            const uint32_t ret = currIndexCount * 3;
            return ret;
        }

        uint32_t inline CalcTotalIndexCount( const uint32_t vertexCount, const uint8_t lodCount )
        {
            uint32_t ret = 0;
            for( uint8_t i = 0; i < lodCount; ++i )
            {
                ret += CalcIndexCountForLOD( vertexCount, i );
            }
            return ret;
        }

        using IndexType = uint16_t;
        using IndexBuffer = Utils::TCDynamicArray< IndexType, 1 >;
        IndexBuffer CreateTriangleIndices( const STerrainDesc& Desc, uint8_t lod )
        {
            IndexBuffer vIndices;
            //const uint32_t vertexCount = Desc.tileRowVertexCount + 1;
            uint32_t vertexCount = (uint32_t)((float)Desc.TileSize.min / Desc.vertexDistance) + 1;

            uint32_t indexCount = 0;
            // Calc index count
            // ( ( (vert - 1) * 2 ) * ( (vert-1) ) ) * 3
            // (4-1) * 2 * (4-1) * 3 = 3 * 2 * 3 = 6 * 3 = 18 * 3
            // (2-1) * 2 * (2-1) * 3 = 1 * 2 * 1 * 3 = 6
            // 2 * 2 * 2 = 8 * 3 = 24
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
            const uint32_t currIndexCount = ((currVertexCount - 1) * 2) * (currVertexCount - 1);
            indexCount += currIndexCount * 3;
            
            vIndices.Resize( indexCount );
            uint32_t currIdx = 0;

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
#   if VKE_SCENE_TERRAIN_VB_START_FROM_TOP_LEFT_CORNER
                    /*
                    *---*   (0,0)----(1,0)
                    | /       |   /    |
                    *   *   (0,1)----(1,1)
                    */
                    vIndices[currIdx++] = v00;
                    vIndices[currIdx++] = v01;
                    vIndices[currIdx++] = v10;
                    /*
                    *   *   (0,0)----(1,0)
                      / |     |   /    |
                    *---*   (0,1)----(1,1)
                    */
                    vIndices[currIdx++] = v10;
                    vIndices[currIdx++] = v01;
                    vIndices[currIdx++] = v11;
#   else
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
#   endif
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
            //vIndices = {0,1,2};
            return vIndices;
        }

        IndexBuffer CreateQuadIndices( const STerrainDesc& Desc, uint8_t lod )
        {
            IndexBuffer vIndices;
            // const uint32_t vertexCount = Desc.tileRowVertexCount + 1;
            uint32_t vertexCount = ( uint32_t )( ( float )Desc.TileSize.min / Desc.vertexDistance ) + 1;
            uint32_t indexCount = 0;
            // Calc index count
            // ((vert-1) * 4) * (vert-1)
            /*
            *   *   *   *
            | \ | \ | \ |
            *   *   *   *
            | \ | \ | \ |
            *   *   *   *
            | \ | \ | \ |
            *   *   *   *
            */
            const uint32_t currVertexCount = vertexCount >> lod;
            const uint32_t currIndexCount = ( ( currVertexCount - 1 ) * 4 ) * ( currVertexCount - 1 );
            indexCount = currIndexCount;
            
            vIndices.Resize( indexCount );
            uint32_t currIdx = 0;

            for( uint16_t y = 0; y < vertexCount - 1; ++y )
            {
                for( uint16_t x = 0; x < vertexCount - 1; ++x )
                {
                    const auto v00 = CALC_IDX_00( x, y, vertexCount );
                    const auto v10 = CALC_IDX_10( x, y, vertexCount );
                    const auto v01 = CALC_IDX_01( x, y, vertexCount );
                    const auto v11 = CALC_IDX_11( x, y, vertexCount );
                    ( void )v11;
                    ( void )v00;
                    ( void )v10;
                    ( void )v01;
#if VKE_SCENE_TERRAIN_CCW
#   if VKE_SCENE_TERRAIN_VB_START_FROM_TOP_LEFT_CORNER
                    /*
                    (0,0)----(1,0)
                      |        |
                    (0,1)----(1,1)
                    */
                    vIndices[ currIdx++ ] = v00;
                    vIndices[ currIdx++ ] = v01;
                    vIndices[ currIdx++ ] = v10;
                    vIndices[ currIdx++ ] = v11;
#   else
                    /*
                    (0,1)----(1,1)
                      |        |
                    (0,0)----(1,0)
                    */
                    vIndices[ currIdx++ ] = v00;
                    vIndices[ currIdx++ ] = v10;
                    vIndices[ currIdx++ ] = v01;
                    vIndices[ currIdx++ ] = v11;
#   endif
#else // CW
#   if VKE_SCENE_TERRAIN_VB_START_FROM_TOP_LEFT_CORNER
                    /*
                    (0,0)----(1,0)
                      |        |
                    (0,1)----(1,1)
                    */
                    vIndices[ currIdx++ ] = v00;
                    vIndices[ currIdx++ ] = v01;
                    vIndices[ currIdx++ ] = v10;
                    vIndices[ currIdx++ ] = v11;
#   else
                    /*
                    (0,1)----(1,1)
                      |        |
                    (0,0)----(1,0)
                    */
                    vIndices[ currIdx++ ] = v00;
                    vIndices[ currIdx++ ] = v01;
                    vIndices[ currIdx++ ] = v10;
                    vIndices[ currIdx++ ] = v11;
#   endif
#endif
                }
            }

            VKE_ASSERT2( vIndices.GetCount() == currIdx, "" );
            // vIndices = {0,1,2};
            return vIndices;
        }


        Result CTerrainVertexFetchRenderer::_Create( const STerrainDesc& Desc,
                                                     RenderSystem::CommandBufferPtr pCommandBuffer )
        {
            Result ret = VKE_FAIL;

            SDrawcallDesc DrawcallDesc;
            DrawcallDesc.type = RenderSystem::DrawcallTypes::STATIC_OPAQUE;
            auto pCtx = pCommandBuffer->GetContext()->GetDeviceContext();

            const auto vTriIndices = CreateTriangleIndices( Desc, 0 );
            const auto vQuadIndices = CreateQuadIndices( Desc, 0 );

            m_vDrawLODs.Resize( Desc.lodCount );

            _CreateVertexBuffer( Desc, pCommandBuffer );


            if( VKE_SUCCEEDED( _CreateConstantBuffers( pCtx ) ) )
            {
                ret = _CreateBindings( pCommandBuffer );
            }

            if (VKE_SUCCEEDED(ret))
            {
                for (uint8_t i = 0; i < Desc.lodCount; ++i)
                {
                    //auto& LOD = m_pDrawcall->GetLOD( i );
                    auto& LOD = m_vDrawLODs[i];
                    LOD.pPipeline = _CreatePipeline(Desc, i, pCtx);
                }

                RenderSystem::SCreateBufferDesc BuffDesc;
                RenderSystem::SUpdateMemoryInfo UpdateInfo;
                {
                    BuffDesc.Buffer.usage = RenderSystem::BufferUsages::INDEX_BUFFER;
                    BuffDesc.Buffer.memoryUsage = RenderSystem::MemoryUsages::STATIC | RenderSystem::MemoryUsages::BUFFER;
                    BuffDesc.Buffer.size = vTriIndices.GetCount() * sizeof( IndexType );
                    BuffDesc.Buffer.indexType = RenderSystem::IndexTypes::UINT16;
                    BuffDesc.Buffer.SetDebugName( "VKE_Scene_TerrainVertexFetch_IndexBuffer" );
                    BuffDesc.Create.flags = Core::CreateResourceFlags::DEFAULT;
                    //BuffDesc.Create.pfnCallback = [ & ]( const void*, void* ) {};
                    m_ahIndexBuffers[ DrawTypes::TRIANGLES ] =
                        HandleCast<RenderSystem::IndexBufferHandle>( pCtx->CreateBuffer( BuffDesc ) );
                    UpdateInfo.dataSize = BuffDesc.Buffer.size;
                    UpdateInfo.pData = vTriIndices.GetData();
                    pCommandBuffer->GetContext()->UpdateBuffer(
                        pCommandBuffer, UpdateInfo,
                        ( RenderSystem::BufferHandle* )&m_ahIndexBuffers[ DrawTypes::TRIANGLES ] );
                }
                {
                    BuffDesc.Buffer.size = vQuadIndices.GetCount() * sizeof( IndexType );
                    m_ahIndexBuffers[ DrawTypes::QUADS ] =
                        HandleCast<RenderSystem::IndexBufferHandle>( pCtx->CreateBuffer( BuffDesc ) );
                    UpdateInfo.dataSize = BuffDesc.Buffer.size;
                    UpdateInfo.pData = vQuadIndices.GetData();
                    pCommandBuffer->GetContext()->UpdateBuffer(
                        pCommandBuffer, UpdateInfo,
                        ( RenderSystem::BufferHandle* )&m_ahIndexBuffers[ DrawTypes::QUADS ] );
                }
            }
            {
                auto& DrawParams = m_aDrawParams[ DrawTypes::TRIANGLES ];
                DrawParams.Indexed.indexCount = vTriIndices.GetCount();
                DrawParams.Indexed.instanceCount = 1;
                DrawParams.Indexed.startIndex = 0;
                DrawParams.Indexed.startInstance = 0;
                DrawParams.Indexed.vertexOffset = 0;
            }
            {
                auto& DrawParams = m_aDrawParams[ DrawTypes::QUADS ];
                DrawParams.Indexed.indexCount = vQuadIndices.GetCount();
                DrawParams.Indexed.instanceCount = 1;
                DrawParams.Indexed.startIndex = 0;
                DrawParams.Indexed.startInstance = 0;
                DrawParams.Indexed.vertexOffset = 0;
            }
            return ret;

        }

        Result CTerrainVertexFetchRenderer::_CreateBindings(RenderSystem::CommandBufferPtr pCommandBuffer)
        {
            Result ret = VKE_FAIL;
            VKE_ASSERT2( m_pConstantBuffer.IsValid(), "" );
            auto pCtx = pCommandBuffer->GetContext();
            auto pDevice = pCtx->GetDeviceContext();

            auto pDeviceCtx = pCtx->GetDeviceContext();
            uint16_t maxTextures = ( uint16_t )pDeviceCtx->GetDeviceInfo().Limits.Binding.Stage.maxTextureCount / 2 - 2;
            maxTextures = Math::Min( maxTextures, CTerrain::MAX_TEXTURE_COUNT );
            {
                // Constant buffer
                g_TileBindingDesc.AddConstantBuffer( 0, RenderSystem::PipelineStages::ALL );
                // Heightmap texture
#if VKE_TERRAIN_BINDLESS_TEXTURES
                // Color texture sampler
                g_TileBindingDesc.AddSamplers( 1, RenderSystem::PipelineStages::ALL );
                // Heightmap textures
                g_TileBindingDesc.AddTextures( 2, RenderSystem::PipelineStages::ALL,
                                                   ( uint16_t )m_pTerrain->m_vHeightmapTexViews.GetCount() );
                // Heightmap normal textures
                g_TileBindingDesc.AddTextures( 3, RenderSystem::PipelineStages::ALL,
                                                   ( uint16_t )m_pTerrain->m_vHeightmapNormalTexViews.GetCount() );
                g_TileBindingDesc.AddTextures( 4, RenderSystem::PipelineStages::ALL,
                                               ( uint16_t )m_pTerrain->m_vSplatmapTexViews.GetCount() );
                g_TileBindingDesc.AddTextures(
                    6, RenderSystem::PipelineStages::ALL,
                    ( uint16_t )m_pTerrain->m_avTextureViews[ CTerrain::TextureTypes::DIFFUSE ].GetCount() );
                
#else
                g_TileBindingDesc.AddTextures( 1, RenderSystem::PipelineStages::ALL );
                // Heightmap normal texture
                g_TileBindingDesc.AddTextures( 2, RenderSystem::PipelineStages::ALL );
#endif
                
                // Color diffuse textures
                // g_TileBindingDesc.AddTextures(4, RenderSystem::PipelineStages::PIXEL, (uint16_t)maxTetures);
                // Color normal textures
                // g_TileBindingDesc.AddTextures(5, RenderSystem::PipelineStages::PIXEL, (uint16_t)maxTetures);

                g_TileBindingDesc.LayoutDesc.SetDebugName( "VKE_Scene_Terrain_Tiling_Bindings1" );
                g_TileBindingDesc.SetDebugName( g_TileBindingDesc.LayoutDesc.GetDebugName() );
            }
            // Instancing
            {
                g_InstanceBindingDesc.AddConstantBuffer( 0, RenderSystem::PipelineStages::ALL );
                // Instance buffer
                g_InstanceBindingDesc.AddStorageBuffer( 1, RenderSystem::PipelineStages::ALL, 1 );
                // Color texture sampler
                g_InstanceBindingDesc.AddSamplers( 2, RenderSystem::PipelineStages::ALL );
                // Heightmap textures
                g_InstanceBindingDesc.AddTextures( 3, RenderSystem::PipelineStages::ALL, (uint16_t)m_pTerrain->m_vHeightmapTexViews.GetCount() );
                // Heightmap normal textures
                g_InstanceBindingDesc.AddTextures( 4, RenderSystem::PipelineStages::ALL, (uint16_t)m_pTerrain->m_vHeightmapNormalTexViews.GetCount() );
                // Heightmap normal textures
                g_InstanceBindingDesc.AddTextures( 5, RenderSystem::PipelineStages::ALL,
                                                   ( uint16_t )m_pTerrain->m_vSplatmapTexViews.GetCount() );
                g_InstanceBindingDesc.AddTextures( 6, RenderSystem::PipelineStages::ALL,
                                                   ( uint16_t )m_pTerrain->m_avTextureViews[CTerrain::TextureTypes::DIFFUSE].GetCount() );
                

                g_InstanceBindingDesc.LayoutDesc.SetDebugName( "VKE_Scene_Terrain_Instancing_Bindings1" );
                g_InstanceBindingDesc.SetDebugName( g_InstanceBindingDesc.LayoutDesc.GetDebugName() );
            }

            RenderSystem::SCreateBindingDesc BindingDesc;
            {
                BindingDesc.AddConstantBuffer( 0, RenderSystem::PipelineStages::ALL );
                BindingDesc.LayoutDesc.SetDebugName( "VKE_Scene_Terrain_Bindings0" );
                BindingDesc.SetDebugName( "VKE_Scene_Terrain_Bindings0" );
                for( uint32_t f = 0; f < MAX_FRAME_COUNT; ++f )
                {
                    m_ahPerFrameDescSets[ f ] = pDevice->CreateResourceBindings( BindingDesc );
                    if( m_ahPerFrameDescSets[ f ] != INVALID_HANDLE )
                    {
                        RenderSystem::SUpdateBindingsHelper UpdateInfo;
                        {
                            UpdateInfo.Reset();
                            UpdateInfo.AddBinding( 0, m_pConstantBuffer->CalcAbsoluteOffset( 0, 0 ),
                                                   m_pConstantBuffer->GetRegionElementSize( 0 ),
                                                   m_pConstantBuffer->GetHandle(),
                                RenderSystem::BindingTypes::DYNAMIC_CONSTANT_BUFFER );
                            pDevice->UpdateDescriptorSet( UpdateInfo, &m_ahPerFrameDescSets[f] );
                        }
#if VKE_TERRAIN_INSTANCING_RENDERING
                        {
                            _CreateInstancingBindings( f );
                        }
#else
                        {
                            auto idx = _CreateTileBindings( f );
                            m_ahPerTileDescSets[f] = m_avTileBindings[f][ idx ];
                        }
#endif
                        ret = VKE_OK;
                    }
                }
            }
            return ret;
        }

        uint32_t CTerrainVertexFetchRenderer::_CreateTileBindings( 
            uint32_t resourceIndex)
        {
            uint32_t ret = UNDEFINED_U32;
            //auto pCtx = pCommandBuffer->GetContext();
            //auto pDevice = pCtx->GetDeviceContext();
            auto pDevice = m_pTerrain->m_pScene->m_pDeviceCtx;
            auto hBinding = pDevice->CreateResourceBindings(g_TileBindingDesc);

            if (hBinding != INVALID_HANDLE)
            {
                auto& vTileBindings = m_avTileBindings[ resourceIndex ];
                ret = vTileBindings.PushBack(hBinding);
            }
#if VKE_TERRAIN_BINDLESS_TEXTURES
            _UpdateTileBindings( resourceIndex );
#endif
            return ret;
        }

        uint32_t CTerrainVertexFetchRenderer::_CreateInstancingBindings( uint32_t resourceIndex )
        {
            uint32_t ret = 0;
            auto pDevice = m_pTerrain->m_pScene->m_pDeviceCtx;
            //for( uint32_t i = 0; i < CTerrainQuadTree::MAX_LOD_COUNT; ++i )
            {
                auto hBindings = pDevice->CreateResourceBindings( g_InstanceBindingDesc );
                //ret = m_avhPerInstancedDrawDescSets[ resourceIndex ].PushBack( hBindings );
                m_ahPerInstancedDrawDescSets[ resourceIndex ] = hBindings;
            }
            _UpdateInstancingBindings( resourceIndex );
            return ret;
        }

        Result CTerrainVertexFetchRenderer::_UpdateTileBindings( const uint32_t& resourceIndex )
        {
            auto& vTileBindings = m_avTileBindings[ m_backBufferIndex ];
            //auto& hBinding = vTileBindings[ Data.index ];
            for( uint32_t i = 0; i < vTileBindings.GetCount(); ++i )
            {
                auto& hBindings = vTileBindings[ i ];
                RenderSystem::SUpdateBindingsHelper UpdateInfo;
                UpdateInfo.AddBinding( 0, m_pConstantBuffer->CalcAbsoluteOffset( 1, 0 ),
                                       m_pConstantBuffer->GetRegionElementSize( 1 ), m_pConstantBuffer->GetHandle(),
                                       RenderSystem::BindingTypes::DYNAMIC_CONSTANT_BUFFER );
                UpdateInfo.AddBinding( 1, &m_pTerrain->m_hHeightmapSampler, 1 );
                //if( Data.hHeightmap != INVALID_HANDLE )
                UpdateInfo.AddBinding( 2, &m_pTerrain->m_vHeightmapTexViews[ 0 ],
                                       ( uint16_t )m_pTerrain->m_vHeightmapTexViews.GetCount() );
                UpdateInfo.AddBinding( 3, &m_pTerrain->m_vHeightmapNormalTexViews[ 0 ],
                                       ( uint16_t )m_pTerrain->m_vHeightmapNormalTexViews.GetCount() );
                UpdateInfo.AddBinding( 4, &m_pTerrain->m_vSplatmapTexViews[ 0 ],
                                       ( uint16_t )m_pTerrain->m_vSplatmapTexViews.GetCount() );
                //if( Data.hBilinearSampler != INVALID_HANDLE )
                
                // UpdateInfo.AddBinding(3, &Data.hDiffuseSampler, 1);
                // UpdateInfo.AddBinding(4, Data.phDiffuses, Data.diffuseTextureCount);
                // UpdateInfo.AddBinding(5, Data.phDiffuseNormals, Data.diffuseTextureCount);
                // pCommandBuffer->GetContext()->GetDeviceContext()->UpdateDescriptorSet( UpdateInfo, &hBinding );
                m_pTerrain->m_pScene->m_pDeviceCtx->UpdateDescriptorSet( UpdateInfo, &hBindings );
            }
            return VKE_OK;
        }

        Result CTerrainVertexFetchRenderer::_UpdateInstancingBindings( uint32_t backBufferIndex )
        {
            auto pDevice = m_pTerrain->m_pScene->m_pDeviceCtx;
            // for( uint16_t i = 0; i < pInstanceDataBuffer->GetRegionCount(); ++i )
            {
                RenderSystem::SUpdateBindingsHelper UpdateInfo;
                auto& hDescSet = m_ahPerInstancedDrawDescSets[ backBufferIndex ];
                uint32_t lodRangeSize = m_pInstacingDataBuffer->GetSize() / CTerrainQuadTree::MAX_LOD_COUNT;
                uint32_t offset = 0;
                UpdateInfo.Reset();
                UpdateInfo.AddBinding( 0, 0, m_pConstantBuffer->GetSize(), m_pConstantBuffer->GetHandle(),
                                       RenderSystem::BindingTypes::DYNAMIC_CONSTANT_BUFFER );
                UpdateInfo.AddBinding( 1, offset, lodRangeSize, m_pInstacingDataBuffer->GetHandle(),
                                       RenderSystem::BindingTypes::DYNAMIC_STORAGE_BUFFER );
                UpdateInfo.AddBinding( 2, &m_pTerrain->m_hHeightmapSampler, 1 );
                // UpdateInfo.AddBinding( 1, &m_pTerrain->m_vDummyTexViews[ 0 ], 1 );
                UpdateInfo.AddBinding( 3, &m_pTerrain->m_vHeightmapTexViews[ 0 ],
                                       ( uint16_t )m_pTerrain->m_vHeightmapTexViews.GetCount() );
                UpdateInfo.AddBinding( 4, &m_pTerrain->m_vHeightmapNormalTexViews[ 0 ],
                                       ( uint16_t )m_pTerrain->m_vHeightmapNormalTexViews.GetCount() );
                UpdateInfo.AddBinding( 5, &m_pTerrain->m_avTextureViews[ CTerrain::TextureTypes::SPLAT ][ 0 ],
                    ( uint16_t )m_pTerrain->m_avTextureViews[ CTerrain::TextureTypes::SPLAT ].GetCount() );
                UpdateInfo.AddBinding( 6, &m_pTerrain->m_avTextureViews[CTerrain::TextureTypes::DIFFUSE][ 0 ],
                                       ( uint16_t )m_pTerrain->m_avTextureViews[CTerrain::TextureTypes::DIFFUSE].GetCount() );
                VKE_LOG( "Update terrain instancing bindings for resource index: " << backBufferIndex );
                pDevice->UpdateDescriptorSet( UpdateInfo, &hDescSet );
                // Copy it to the next one
                auto nextIndex = ( backBufferIndex + 1 ) % MAX_FRAME_COUNT;
                auto& hNextDescSet = m_ahPerInstancedDrawDescSets[ nextIndex ];
                if( hNextDescSet != INVALID_HANDLE )
                {
                    RenderSystem::SCopyDescriptorSetInfo CopyInfo;
                    CopyInfo.hDst = hNextDescSet;
                    CopyInfo.hSrc = hDescSet;
                    //pDevice->UpdateDescriptorSet( CopyInfo );
                }
                m_lastBindingsUpdateIndex = backBufferIndex;
            }
            return VKE_OK;
        }

        Result CTerrainVertexFetchRenderer::UpdateBindings( RenderSystem::CommandBufferPtr pCommandBuffer,
            const STerrainUpdateBindingData& Data)
        {
            Result ret = VKE_OK;
            {
                ret = VKE_FAIL;
                
                // Create required bindings
                for( uint32_t f = 0; f < MAX_FRAME_COUNT; ++f )
                {
#if VKE_TERRAIN_INSTANCING_RENDERING
#else
                    auto& vTileBindings = m_avTileBindings[ f ];
                    for( uint32_t i = vTileBindings.GetCount(); i <= Data.index; ++i )
                    {
                        if( _CreateTileBindings( f ) == UNDEFINED_U32 )
                        {
                            return ret;
                        }
                    }
#endif
                }
#if VKE_TERRAIN_INSTANCING_RENDERING
#else

#   if VKE_TERRAIN_BINDLESS_TEXTURES

#   else
                auto& vTileBindings = m_avTileBindings[ m_backBufferIndex ];
                auto& hBinding = vTileBindings[ Data.index ];
                RenderSystem::SUpdateBindingsHelper UpdateInfo;
                UpdateInfo.AddBinding( 0, m_pConstantBuffer->CalcAbsoluteOffset( 1, 0 ),
                                       m_pConstantBuffer->GetRegionElementSize( 1 ),
                    m_pConstantBuffer->GetHandle(), RenderSystem::BindingTypes::DYNAMIC_CONSTANT_BUFFER );
                if( Data.hHeightmap != INVALID_HANDLE )
                {
                    UpdateInfo.AddBinding( 1, &Data.hHeightmap, 1 );
                }
                if( Data.hHeightmapNormal != INVALID_HANDLE )
                {
                    UpdateInfo.AddBinding( 2, &Data.hHeightmapNormal, 1 );
                }
                if( Data.hBilinearSampler != INVALID_HANDLE )
                {
                    UpdateInfo.AddBinding( 3, &Data.hBilinearSampler, 1 );
                }
                // UpdateInfo.AddBinding(3, &Data.hDiffuseSampler, 1);
                // UpdateInfo.AddBinding(4, Data.phDiffuses, Data.diffuseTextureCount);
                // UpdateInfo.AddBinding(5, Data.phDiffuseNormals, Data.diffuseTextureCount);
                //pCommandBuffer->GetContext()->GetDeviceContext()->UpdateDescriptorSet( UpdateInfo, &hBinding );
                m_pTerrain->m_pScene->m_pDeviceCtx->UpdateDescriptorSet( UpdateInfo, &hBinding );
#   endif // BINDLESS
#endif
                ret = VKE_OK;
            }
            return ret;
        }

        void CTerrainVertexFetchRenderer::UpdateBindings( const STerrainUpdateBindingData& Data)
        {
            // Create required bindings
            for( uint32_t f = 0; f < MAX_FRAME_COUNT; ++f )
            {
#if VKE_TERRAIN_INSTANCING_RENDERING

#else
                auto& vTileBindings = m_avTileBindings[ f ];
                for( uint32_t i = vTileBindings.GetCount(); i <= Data.index; ++i )
                {
                    if( _CreateTileBindings( f ) == UNDEFINED_U32 )
                    {
           
                    }
                }
#endif
            }
#if VKE_TERRAIN_INSTANCING_RENDERING
            //_UpdateInstancingBindings( m_resourceIndex );
            m_needUpdateBindings = true;
#else
            m_needUpdateBindings = true;
#   if VKE_TERRAIN_BINDLESS_TEXTURES

#   else
            auto& vTileBindings = m_avTileBindings[ 1 ];
            auto& hBinding = vTileBindings[ Data.index ];
            RenderSystem::SUpdateBindingsHelper UpdateInfo;
            UpdateInfo.AddBinding( 0, m_pConstantBuffer->CalcAbsoluteOffset( 1, 0 ),
                                   m_pConstantBuffer->GetRegionElementSize( 1 ),
                m_pConstantBuffer->GetHandle(), RenderSystem::BindingTypes::DYNAMIC_CONSTANT_BUFFER );
            if( Data.hHeightmap != INVALID_HANDLE )
            {
                UpdateInfo.AddBinding( 1, &Data.hHeightmap, 1 );
            }
            if( Data.hHeightmapNormal != INVALID_HANDLE )
            {
                UpdateInfo.AddBinding( 2, &Data.hHeightmapNormal, 1 );
            }
            if( Data.hBilinearSampler != INVALID_HANDLE )
            {
                UpdateInfo.AddBinding( 3, &Data.hBilinearSampler, 1 );
            }

            m_pTerrain->m_pScene->m_pDeviceCtx->UpdateDescriptorSet( UpdateInfo, &hBinding );

            std::swap( m_avTileBindings[ 0 ][ Data.index ], m_avTileBindings[ 1 ][ Data.index ] );
#   endif // BINDLESS
#endif // TILING
        }

        Result CTerrainVertexFetchRenderer::_CreateConstantBuffers( RenderSystem::CDeviceContext* pCtx )
        {
            Result ret = VKE_FAIL;
            uint32_t maxTileCountInRoot = m_pTerrain->m_Desc.TileSize.max / m_pTerrain->m_Desc.TileSize.min;
            maxTileCountInRoot *= maxTileCountInRoot; // rows and cols
            uint32_t maxTileCount =
                maxTileCountInRoot * m_pTerrain->m_TerrainInfo.RootCount.width * m_pTerrain->m_TerrainInfo.RootCount.height;
            ( void )maxTileCount;
            RenderSystem::SCreateBufferDesc Desc;
            Desc.Create.flags = Core::CreateResourceFlags::DEFAULT;
            Desc.Buffer.memoryUsage = RenderSystem::MemoryUsages::GPU_ACCESS | RenderSystem::MemoryUsages::BUFFER;
            Desc.Buffer.usage = RenderSystem::BufferUsages::CONSTANT_BUFFER;
#if VKE_TERRAIN_INSTANCING_RENDERING
            Desc.Buffer.size = sizeof( SConstantBuffer );
#else
            Desc.Buffer.size = 0;
            Desc.Buffer.vRegions =
            {
                //RenderSystem::SBufferRegion( 1u, (uint16_t)sizeof( SPerFrameConstantBuffer ) ),
                RenderSystem::SBufferRegion( m_pTerrain->m_TerrainInfo.maxNodeCount, (uint16_t)sizeof( SPerDrawConstantBufferData ) )
            };
#endif
            Desc.Buffer.SetDebugName( "VKE_Scene_VertexFetchTerrain_ConstantBuffer" );
            Desc.Buffer.stagingBufferRegionCount = MAX_FRAME_COUNT;
            auto hBuffer = pCtx->CreateBuffer( Desc );
            if( hBuffer != INVALID_HANDLE )
            {
                ret = VKE_OK;
            }
            m_pConstantBuffer = pCtx->GetBuffer(hBuffer);
#if VKE_TERRAIN_INSTANCING_RENDERING
            if( m_pConstantBuffer != nullptr )
            {
                for( uint16_t i = 0; i < MAX_FRAME_COUNT; ++i )
                {
                    SConstantBuffer CB;
                    CB.Height = m_pTerrain->m_Desc.Height;
                    CB.TerrainSize = { m_pTerrain->m_Desc.size };
                    CB.tileRowVertexCount = m_pTerrain->m_tileVertexCount;

                    void* pData = m_pConstantBuffer->GetStaging()->MapRegion( i, 0 );
                    RenderSystem::SBufferWriter Builder( pData, m_pConstantBuffer->GetStaging()->GetRegionSize(i) );
                    Builder.Write( CB );
                    m_pConstantBuffer->GetStaging()->Unmap();
                }
            }
#endif
#if VKE_TERRAIN_INSTANCING_RENDERING
            uint32_t maxTileCountForOneLOD = maxTileCountInRoot * 4; // 4 roots at once
            /// TODO: optimize max number of instances
            //for( uint32_t i = 0; i < 1; ++i )
            {
                Desc.Buffer.usage = RenderSystem::BufferUsages::STORAGE_BUFFER;
                Desc.Buffer.size = 0;
                /*Desc.Buffer.vRegions.Resize(
                    CTerrainQuadTree::MAX_LOD_COUNT,
                    RenderSystem::SBufferRegion( maxTileCountForOneLOD, ( uint16_t )sizeof( SPerDrawConstantBufferData ) ) );*/
                /*Desc.Buffer.size = CTerrainQuadTree::MAX_LOD_COUNT *
                                   ( maxTileCountForOneLOD * ( uint32_t )sizeof( SPerDrawConstantBufferData ) );*/
                auto maxCount = CTerrainQuadTree::MAX_LOD_COUNT * maxTileCountForOneLOD;
                maxCount = m_pTerrain->m_TerrainInfo.maxNodeCount;
                Desc.Buffer.vRegions.Resize(1, RenderSystem::SBufferRegion(maxCount, ( uint32_t )sizeof( SPerDrawConstantBufferData )));
                Desc.Buffer.SetDebugName( "VKE_Scene_VertexFetchTerrain_InstancingDataBuffer" );
                Desc.Buffer.stagingBufferRegionCount = MAX_FRAME_COUNT;
                hBuffer = pCtx->CreateBuffer( Desc );
                m_pInstacingDataBuffer = pCtx->GetBuffer( hBuffer );
            }
            /*{
                Desc.Buffer.usage = RenderSystem::BufferUsages::UPLOAD;
                Desc.Buffer.SetDebugName( "VKE_Scene_VertexFetchTerrain_InstancingDataUploadCPUBuffer" );
                Desc.Buffer.memoryUsage = RenderSystem::MemoryUsages::UPLOAD_BUFFER;
                Desc.Buffer.vRegions.Resize( MAX_FRAME_COUNT, Desc.Buffer.vRegions[ 0 ] );
                hBuffer = pCtx->CreateBuffer( Desc );
                m_pInstancingDataCPUBuffer = pCtx->GetBuffer( hBuffer );
            }*/
#endif
            return ret;
        }

        RenderSystem::PipelinePtr CTerrainVertexFetchRenderer::_CreatePipeline(
            const STerrainDesc& Desc, uint8_t lod,
            RenderSystem::CDeviceContext* pCtx )//
        {
            RenderSystem::PipelinePtr pRet;

            const ShaderCompilerString BaseTileSizeStr = ShaderCompilerString(Desc.TileSize.min);
            const ShaderCompilerString TileVertexCountStr =
                ShaderCompilerString(( uint32_t )( Desc.TileSize.min / Desc.vertexDistance ));
            const ShaderCompilerString BaseVertexDistanceStr = ShaderCompilerString(Desc.vertexDistance);
            const uint32_t patchControlPoints = Desc.Tesselation.quadMode ? 4 : 3;
            const ShaderCompilerString ControlPointsStr = ShaderCompilerString( patchControlPoints );
            const ShaderCompilerString TessellationFactorMin = ShaderCompilerString( Desc.Tesselation.Factors.min );
            const ShaderCompilerString TessellationFactorMax = ShaderCompilerString( Desc.Tesselation.Factors.max );
            const ShaderCompilerString TessellationMaxDistance = ShaderCompilerString( Desc.Tesselation.maxDistance );
            const ShaderCompilerString TessellationFactorDistancePowFactorReductionSpeed =
                ShaderCompilerString( Desc.Tesselation.lodReductionFactor );
            const ShaderCompilerString TessellationFactorDistancePowMultiplier =
                ShaderCompilerString( 1 );

            const bool tesselationEnabled = Desc.Tesselation.Factors.max > 0;

#if VKE_SCENE_TERRAIN_DEBUG_SHADER
            Core::SLoadFileInfo FileDesc;
            FileDesc.FileInfo.FileName = "data/shaders/terrain-dev.hlsl";
            auto pFile = m_pTerrain->m_pScene->GetDeviceContext()->GetRenderSystem()->GetEngine()->GetManagers().pFileMgr->LoadFile(
                FileDesc );
            g_pTerrainVS = (cstr_t)pFile->GetData();
            g_pTerrainPS = g_pTerrainVS;
#endif

            RenderSystem::SShaderData VsData, PsData, HsData, DsData;
            VsData.pCode = (uint8_t*)g_pTerrainVS;
            VsData.codeSize = (uint32_t)strlen( g_pTerrainVS );
            VsData.stage = RenderSystem::ShaderCompilationStages::HIGH_LEVEL_TEXT;
            VsData.type = RenderSystem::ShaderTypes::VERTEX;

#if VKE_TERRAIN_INSTANCING_RENDERING
            cstr_t pEntryPoint = "vs_main_instancing";
            if( tesselationEnabled )
            {
                pEntryPoint = "vs_main_instancing_tess";
            }
#else
            cstr_t pEntryPoint = "vs_main";
            if(tesselationEnabled)
            {
                pEntryPoint = "vs_main_tess";
            }
#endif
            RenderSystem::SCreateShaderDesc VsDesc, PsDesc, HsDesc, DsDesc;
            VsDesc.Create.flags = Core::CreateResourceFlags::DEFAULT;
            //VsDesc.Shader.FileInfo.pName = "VertexFetchTerrainVS";
            VsDesc.Shader.pData = &VsData;
            VsDesc.Shader.EntryPoint = pEntryPoint;
            VsDesc.Shader.Name = "VertexFetchTerrainVS";
            VsDesc.Shader.type = RenderSystem::ShaderTypes::VERTEX;
            VsDesc.Shader.vDefines =
            {
                { VKE_SHADER_COMPILER_STR( "INSTANCING_MODE" ), ShaderCompilerString(VKE_TERRAIN_INSTANCING_RENDERING) },
                { VKE_SHADER_COMPILER_STR( "BASE_TILE_SIZE" ), BaseTileSizeStr },
                { VKE_SHADER_COMPILER_STR( "TILE_VERTEX_COUNT" ), TileVertexCountStr },
                { VKE_SHADER_COMPILER_STR( "BASE_VERTEX_DISTANCE" ), BaseVertexDistanceStr},
                { VKE_SHADER_COMPILER_STR( "TESS_FACTOR_MIN" ), TessellationFactorMin },
                { VKE_SHADER_COMPILER_STR( "TESS_FACTOR_MAX" ), TessellationFactorMax },
                { VKE_SHADER_COMPILER_STR( "TESS_FACTOR_MAX_DISTANCE" ), TessellationMaxDistance },
                { VKE_SHADER_COMPILER_STR( "TESS_FACTOR_DISTANCE_POW_REDUCTION_SPEED" ),
                  TessellationFactorDistancePowFactorReductionSpeed },
                { VKE_SHADER_COMPILER_STR( "TESS_FACTOR_DISTANCE_POW_REDUCTION_SPEED_MULTIPLIER" ),
                  ShaderCompilerString(Desc.Tesselation.lodReductionSpeed) },
                { VKE_SHADER_COMPILER_STR( "TESS_FACTOR_FLAT_SURFACE_REDUCTION" ),
                  ShaderCompilerString( Desc.Tesselation.flatSurfaceReduction ) },
            };

            if( tesselationEnabled )
            {
                VsDesc.Shader.vDefines.PushBack(
                    { VKE_SHADER_COMPILER_STR( "CONTROL_POINTS" ), ControlPointsStr } );
            }

            auto pVs = pCtx->CreateShader( VsDesc );

            PsData.pCode = (uint8_t*)g_pTerrainPS;
            PsData.codeSize = (uint32_t)strlen( g_pTerrainPS );
            PsData.stage = RenderSystem::ShaderCompilationStages::HIGH_LEVEL_TEXT;
            PsData.type = RenderSystem::ShaderTypes::PIXEL;

            char aPsEntryPointName[32];
            lod = Math::Min(lod, 3);
            vke_sprintf( aPsEntryPointName, 32, "ps_main%d", lod );

            /*PsDesc.Create.flags = Core::CreateResourceFlags::DEFAULT;
            PsDesc.Shader.FileInfo.pName = "VertexFetchTerrianPS";
            PsDesc.Shader.EntryPoint = aPsEntryPointName;
            PsDesc.Shader.Name = "VertexFetchTerrainPS";
            PsDesc.Shader.pData = &PsData;
            PsDesc.Shader.type = RenderSystem::ShaderTypes::PIXEL;*/
            PsDesc = VsDesc;
            //PsDesc.Shader.FileInfo.pName = "VertexFetchTerrainPS";
            PsDesc.Shader.EntryPoint = aPsEntryPointName;
            PsDesc.Shader.Name = "VertexFetchTerrainPS";
            PsDesc.Shader.type = RenderSystem::ShaderTypes::PIXEL;
            PsDesc.Shader.pData = &PsData;

            auto pPs = pCtx->CreateShader( PsDesc );

            HsData = VsData;
            HsData.type = RenderSystem::ShaderTypes::HULL;

            HsDesc = VsDesc;
            //HsDesc.Shader.FileInfo.pName = "VertexFetchTerrainHS";
            HsDesc.Shader.EntryPoint = "hs_main";
            HsDesc.Shader.Name = "VertexFetchTerrainHS";
            HsDesc.Shader.type = HsData.type;
            HsDesc.Shader.pData = &HsData;

            DsData = VsData;
            DsData.type = RenderSystem::ShaderTypes::DOMAIN;
            DsDesc = VsDesc;
            //DsDesc.Shader.FileInfo.pName = "VertexFetchTerrainDS";
            DsDesc.Shader.EntryPoint = "ds_main";
            DsDesc.Shader.Name = "VertexFetchTerrainDS";
            DsDesc.Shader.type = DsData.type;
            DsDesc.Shader.pData = &DsData;

            ShaderRefPtr pHs, pDs;
            if( tesselationEnabled )
            {
                pHs = pCtx->CreateShader( HsDesc );
                pDs = pCtx->CreateShader( DsDesc );
            }
            RenderSystem::SPipelineLayoutDesc LayoutDesc;
            LayoutDesc.vDescriptorSetLayouts =
            { 
                pCtx->GetDescriptorSetLayout( m_ahPerFrameDescSets[0] ),
#if VKE_TERRAIN_INSTANCING_RENDERING
                pCtx->GetDescriptorSetLayout( m_ahPerInstancedDrawDescSets[ 0 ] )
#else
                pCtx->GetDescriptorSetLayout( m_ahPerTileDescSets[0] )
#endif
            };
            LayoutDesc.vPushConstants =
            {
                {RenderSystem::PipelineStages::ALL, sizeof(SPushConstants), 0}
            };
            auto pLayout = pCtx->CreatePipelineLayout(LayoutDesc);

            RenderSystem::SPipelineCreateDesc PipelineDesc;
            PipelineDesc.Create.flags = Core::CreateResourceFlags::DEFAULT;
            auto& Pipeline = PipelineDesc.Pipeline;

            PipelineDesc.Pipeline.hLayout = pLayout->GetHandle();

            PipelineDesc.Pipeline.InputLayout.topology = RenderSystem::PrimitiveTopologies::TRIANGLE_LIST;
            if( tesselationEnabled )
            {
                PipelineDesc.Pipeline.InputLayout.topology = RenderSystem::PrimitiveTopologies::PATCH_LIST;
            }
            VKE::RenderSystem::SPipelineDesc::SInputLayout::SVertexAttribute VA;
            VA.vertexBufferBindingIndex = 0;
            VA.stride = sizeof(SVertex);
            {
                VA.format = VKE::RenderSystem::Formats::R32G32B32_SFLOAT;
                VA.location = 0;
                VA.offset = 0;
                PipelineDesc.Pipeline.InputLayout.vVertexAttributes.PushBack(VA);
            }
            {
                VA.format = VKE::RenderSystem::Formats::R32G32_SFLOAT;
                VA.location = 1;
                VA.offset = sizeof(float) * 2;
                //PipelineDesc.Pipeline.InputLayout.vVertexAttributes.PushBack(VA);
            }

            {
                Pipeline.Rasterization.Polygon.mode =
                    ( RENDER_WIREFRAME ) ? RenderSystem::PolygonModes::WIREFRAME : RenderSystem::PolygonModes::FILL;
                Pipeline.Rasterization.Polygon.cullMode = RenderSystem::CullModes::NONE;
                Pipeline.Rasterization.Polygon.frontFace = VKE_SCENE_TERRAIN_CCW ?
                    RenderSystem::FrontFaces::COUNTER_CLOCKWISE : RenderSystem::FrontFaces::CLOCKWISE;
            }


            PipelineDesc.Pipeline.Shaders.apShaders[RenderSystem::ShaderTypes::VERTEX] = pVs;
            PipelineDesc.Pipeline.Shaders.apShaders[RenderSystem::ShaderTypes::PIXEL] = pPs;
            if( tesselationEnabled )
            {
                PipelineDesc.Pipeline.Shaders.apShaders[ RenderSystem::ShaderTypes::DOMAIN ] = pDs;
                PipelineDesc.Pipeline.Shaders.apShaders[ RenderSystem::ShaderTypes::HULL ] = pHs;
            }
            {
                auto& Depth = Pipeline.DepthStencil.Depth;
                Depth.enable = true;
                Depth.test = true;
                Depth.write = true;
                Depth.compareFunc = RenderSystem::CompareFunctions::LESS_EQUAL;
            }
            if(tesselationEnabled)
            {
                PipelineDesc.Pipeline.Tesselation.enable = true;
                PipelineDesc.Pipeline.Tesselation.patchControlPoints = (uint8_t)patchControlPoints;
                PipelineDesc.Pipeline.Tesselation.domainOrigin = RenderSystem::TessellationDomainOrigins::LOWER_LEFT;
            }

            //VKE_RENDER_SYSTEM_SET_DEBUG_NAME( PipelineDesc.Pipeline, "TerrainVertexFetchRenderer" );
            PipelineDesc.Pipeline.SetDebugName( "TerrainVertexFetchRenderer" );
            pRet = pCtx->CreatePipeline( PipelineDesc );
            
            if( pRet.IsNull() )
            {
                for( uint32_t i = 0; i < Desc.vDDIRenderPasses.GetCount(); ++i )
                {
                    PipelineDesc.Pipeline.hDDIRenderPass = Desc.vDDIRenderPasses[ i ];
                    //VKE_RENDER_SYSTEM_SET_DEBUG_NAME( PipelineDesc.Pipeline, "TerrainVertexFetchRenderer" );
                    PipelineDesc.Pipeline.SetDebugName( "TerrainVertexFetchRenderer" );
                    pRet = pCtx->CreatePipeline( PipelineDesc );
                }
                for( uint32_t i = 0; i < Desc.vRenderPasses.GetCount(); ++i )
                {
                    auto hPass = Desc.vRenderPasses[ i ];
                    PipelineDesc.Pipeline.hRenderPass = hPass;
                    //VKE_RENDER_SYSTEM_SET_DEBUG_NAME( PipelineDesc.Pipeline, "TerrainVertexFetchRenderer" );
                    PipelineDesc.Pipeline.SetDebugName( "TerrainVertexFetchRenderer" );
                    pRet = pCtx->CreatePipeline( PipelineDesc );
                }
            }
            return pRet;
        }


        void CTerrainVertexFetchRenderer::Update( RenderSystem::CommandBufferPtr pCommandBuffer, CScene* pScene )
        {
            m_prevResourceIndex = m_backBufferIndex;
            m_backBufferIndex = pCommandBuffer->GetBackBufferIndex();
            //VKE_LOG( "Update frame: " << m_resourceIndex << " cmd buffer: " << pCommandBuffer->GetDDIObject() );
#if VKE_SCENE_TERRAIN_DEBUG
            RenderSystem::SDebugInfo Info;
            Info.pText = "CTerrainVertexFetchRenderer::_UpdateDrawcalls";
            Info.Color = RenderSystem::SColor::GREEN;
            //pCtx->GetTransferContext()->GetCommandBuffer()->BeginDebugInfo( &Info );
            pCommandBuffer->BeginDebugInfo( &Info );
#endif
            _UpdateDrawcalls( pScene->GetCamera() );
#if VKE_SCENE_TERRAIN_DEBUG
            //pCtx->GetTransferContext()->GetCommandBuffer()->EndDebugInfo();
            pCommandBuffer->EndDebugInfo();
#endif
            _SortDrawcalls();
            auto pDevice = pScene->GetDeviceContext();
            auto& hCurrFence = m_ahFences[ m_backBufferIndex ];
            bool isFenceReady = hCurrFence == DDI_NULL_HANDLE || pDevice->IsFenceSignaled( hCurrFence );
            if( isFenceReady )
            {
                RenderSystem::SCopyBufferInfo CopyInfo;
                CopyInfo.hDDIDstBuffer = m_pConstantBuffer->GetDDIObject();
                CopyInfo.hDDISrcBuffer = m_pConstantBuffer->GetStaging()->GetDDIObject();
                CopyInfo.Region.dstBufferOffset = 0;
                CopyInfo.Region.srcBufferOffset =
                    m_pConstantBuffer->GetStaging()->CalcAbsoluteOffset( m_backBufferIndex, 0 );
                CopyInfo.Region.size = m_pConstantBuffer->GetStaging()->GetRegionSize( m_backBufferIndex );
                pCommandBuffer->Copy( CopyInfo );
#if VKE_TERRAIN_INSTANCING_RENDERING
                _UpdateInstancingBuffers( pCommandBuffer, pScene->GetViewCamera() );
#else
                _UpdateTilingConstantBuffers( pCommandBuffer, pScene->GetViewCamera() );
#endif
                if( m_needUpdateBindings && m_prevResourceIndex != m_backBufferIndex )
                {
                    m_needUpdateBindings = false;
#if VKE_TERRAIN_INSTANCING_RENDERING
                    _UpdateInstancingBindings( m_backBufferIndex );
#else
                    _UpdateTileBindings( m_backBufferIndex );
#endif
                    //_UpdateInstancingBuffers( pCommandBuffer, pScene->GetViewCamera() );
                }

                hCurrFence = pCommandBuffer->GetFence();
            }
            else
            {
                //VKE_LOG( "NO Update with Fence: " << hCurrFence );
            }

        }

        void CTerrainVertexFetchRenderer::_SortDrawcalls()
        {
            auto& vDrawcalls = m_pTerrain->m_QuadTree.GetLODData();
            std::sort( vDrawcalls.GetData(), vDrawcalls.GetData() + vDrawcalls.GetCount(),
                [](const auto& Left, const auto& Right)
                {
                    return Left.lod < Right.lod;
                } );
        }

        uint32_t Pack4BytesToUint(uint8_t a, uint8_t b, uint8_t c, uint8_t d)
        {
            uint32_t ret = 0;
            ret = ( a << 24 ) | ( b << 16 ) | ( c << 8 ) | d;
            return ret;
        }

        bool g_updateConstantBuffers = true;

        uint32_t PackUint8ToUint32( uint8_t v1, uint8_t v2, uint8_t v3, uint8_t v4 )
        {
            uint32_t ret = ( v1 << 24 ) | ( v2 << 16 ) | ( v3 << 8 ) | (v4);
            return ret;
        }

        uint32_t PackUint16ToUint32( uint16_t v1, uint32_t v2 )
        {
            return ( v1 << 16 ) | ( v2 );
        }

        void CTerrainVertexFetchRenderer::_UpdateInstancingBuffers( RenderSystem::CommandBufferPtr pCommandBuffer,
                                                                    CCamera* pCamera )
        {
            if( !g_updateConstantBuffers )
                return;
            static const Math::CVector4 aColors[] = {
                { 1.0f, 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, 1.0f, 1.0f },
                { 1.0f, 1.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 1.0f, 1.0f }, { 1.0f, 0.5f, 0.5f, 1.0f },
                { 0.5f, 0.5f, 1.0f, 1.0f }, { 0.2f, 0.4f, 0.5f, 1.0f },
            };
            
            //auto pCtx = pCommandBuffer->GetContext();
            //auto pDevice = pCtx->GetDeviceContext();
            
            {

            }
            {
                const auto& vLODData = m_pTerrain->m_QuadTree.GetLODData();
                const float tileSize = m_pTerrain->m_tileVertexCount * m_pTerrain->m_Desc.vertexDistance;

                m_vInstnacingInfos.Clear();
                if( !vLODData.IsEmpty() )
                {
                    //VKE_PROFILE_SIMPLE2( "Update" );

                    //SPerDrawConstantBufferData PerDrawData;
                    RenderSystem::PipelinePtr pCurrPipeline;
                    auto pStagingBuffer = m_pInstacingDataBuffer->GetStaging();
                    auto regionBaseOffset = pStagingBuffer->CalcAbsoluteOffset( m_backBufferIndex, 0 );

                    //UpdateInfo.hLockedStagingBuffer = hLock;

                    uint8_t prevLod = vLODData[0].lod;
                    uint8_t currLod = prevLod;
                    SInstancingInfo Info = {};
                    //uint32_t offset = 0;
                    uint32_t prevLodDrawIdx = 0;
                    uint32_t sizeWritten = 0;
                    SPerDrawConstantBufferData* pStagingBufferData =
                        ( SPerDrawConstantBufferData* )pStagingBuffer->MapRegion( m_backBufferIndex, 0 );

                    for( uint32_t i = 0; i < vLODData.GetCount(); ++i )
                    {
                        const auto& LODData = vLODData[ i ];
                        currLod = LODData.lod;

                        if( currLod != prevLod )
                        {
                            Info.pPipeline = LODData.DrawData.pPipeline;
                            // Note that this is offset in GPU buffer. GPU buffer is not double/tripple buffered
                            // so all offsets starts from 0
                            Info.bufferOffset = pStagingBuffer->CalcAbsoluteOffset( 0, prevLodDrawIdx );
                            m_vInstnacingInfos.PushBack( Info );
                            Info.instanceCount = 0;
                            prevLod = currLod;
                            prevLodDrawIdx = i;
                        }
                        
                        SPerDrawConstantBufferData& PerDrawData = *pStagingBufferData;
                        PerDrawData.vecPosition = LODData.DrawData.vecPosition;
                        PerDrawData.topVertexDiff = LODData.DrawData.topVertexDiff;
                        PerDrawData.bottomVertexDiff = LODData.DrawData.bottomVertexDiff;
                        PerDrawData.leftVertexDiff = LODData.DrawData.leftVertexDiff;
                        PerDrawData.rightVertexDiff = LODData.DrawData.rightVertexDiff;
                        PerDrawData.vertexDiff =
                            Pack4BytesToUint( LODData.DrawData.topVertexDiff, LODData.DrawData.bottomVertexDiff,
                                                LODData.DrawData.leftVertexDiff, LODData.DrawData.rightVertexDiff );
                        PerDrawData.vecLodColor = aColors[ LODData.lod ];
                        PerDrawData.textureIdx = LODData.DrawData.rootIdx;
                        // Tile size depends on lod.
                        // Max lod is defined in TerrainDesc.tileRowVertexCount
                        // For each lod tile size increases two times
                        // lod0 = tileRowVertexCount * vertexDistance * 1
                        // lod1 = lod0 * 2
                        // lod2 = lod0 * 4
                        PerDrawData.tileSize = Math::CalcPow2( currLod ) * tileSize;
                        PerDrawData.TexcoordOffset = LODData.DrawData.TextureOffset;

                        for( uint16_t set = 0; set < MAX_SPLATMAP_SET_COUNT; ++set )
                        {
                            uint32_t s = set * 4;
                            PerDrawData.aSplatMapIndices[ set ]
                                = PackUint8ToUint32( ( uint8_t )PerDrawData.textureIdx, 0xFF, 0xFF, 0xFF );

                            for( uint16_t splatIndex = 0; splatIndex < 4; ++splatIndex, ++s )
                            {
                                
                                for( uint16_t texIndex = 0; texIndex < 2; texIndex += 1 )
                                {
                                    uint32_t packed = PackUint16ToUint32( texIndex*2, texIndex*2 + 1 );
                                    PerDrawData.aaTextureIndices[ s ][ texIndex ] = packed;
                                }
                            }
                        }
                            
                        //UpdateInfo.dataAlignedSize = pBufferData->GetRegionElementSize( 0u );
                        ////auto baseOffset = pBufferData->CalcAbsoluteOffset( currLod, Info.instanceCount );
                        //auto baseOffset = pBufferData->CalcAbsoluteOffset( 0, i );
                        //offset = i * UpdateInfo.dataAlignedSize;
                        //UpdateInfo.stagingBufferOffset = baseOffset;
                        //UpdateInfo.dataSize = sizeof( SPerDrawConstantBufferData );
                        //UpdateInfo.pSrcData = &PerDrawData;
                        //const auto res = pDevice->UpdateStagingBuffer( UpdateInfo );
                        //VKE_ASSERT2( VKE_SUCCEEDED( res ), "" );
                        Info.instanceCount++;
                        pStagingBufferData++;
                        sizeWritten += pStagingBuffer->GetRegionElementSize( 0u );
                    }
                    // Add last LOD
                    if( currLod == prevLod && Info.instanceCount > 0 )
                    {
                        Info.pPipeline = vLODData[currLod].DrawData.pPipeline;
                        Info.bufferOffset = pStagingBuffer->CalcAbsoluteOffset( 0, prevLodDrawIdx );
                        m_vInstnacingInfos.PushBack( Info );
                        Info.instanceCount = 0;
                        prevLod = currLod;
                    }

                    pStagingBuffer->Unmap();

                    RenderSystem::SCopyBufferInfo CopyInfo;
                    CopyInfo.hDDIDstBuffer = m_pInstacingDataBuffer->GetDDIObject();
                    CopyInfo.hDDISrcBuffer = pStagingBuffer->GetDDIObject();
                    CopyInfo.Region.dstBufferOffset = 0;
                    CopyInfo.Region.srcBufferOffset = regionBaseOffset;
                    CopyInfo.Region.size = sizeWritten;
                    pCommandBuffer->Copy( CopyInfo );
                }
                /*RenderSystem::SUnlockBufferInfo UnlockInfo;
                UnlockInfo.dstBufferOffset = 0;
                UnlockInfo.hUpdateInfo = hLock;
                UnlockInfo.pDstBuffer = pBufferData.Get();*/
                // UnlockInfo.totalSize = pBufferData->GetSize();
                //const Result res = pDevice->UnlockStagingBuffer( pCtx, UnlockInfo );
                //VKE_ASSERT2( res == VKE_OK, "" );
                
            }
        }

        void CTerrainVertexFetchRenderer::_UpdateTilingConstantBuffers( RenderSystem::CommandBufferPtr pCommandBuffer,
                                                                  CCamera* pCamera )
        {
            if( !g_updateConstantBuffers )
                return;

            static const Math::CVector4 aColors[] =
            {
                { 1.0f, 0.0f, 0.0f, 1.0f },
                { 0.0f, 1.0f, 0.0f, 1.0f },
                { 0.0f, 0.0f, 1.0f, 1.0f },
                { 1.0f, 1.0f, 0.0f, 1.0f },
                { 1.0f, 0.0f, 1.0f, 1.0f },
                { 1.0f, 0.5f, 0.5f, 1.0f },
                { 0.5f, 0.5f, 1.0f, 1.0f },
                { 0.2f, 0.4f, 0.5f, 1.0f },
            };

            auto pCtx = pCommandBuffer->GetContext();
            auto pDevice = pCtx->GetDeviceContext();
            
            
            auto pBufferData = m_pConstantBuffer;
            const uint32_t size = pBufferData->GetSize();

            auto hLock = pDevice->LockStagingBuffer( size );

            if( hLock != UNDEFINED_U32 )
            {
                
                SPerDrawConstantBufferData PerDrawData;
                RenderSystem::SUpdateStagingBufferInfo UpdateInfo;
                
                {
                    const auto& vLODData = m_pTerrain->m_QuadTree.GetLODData();
                    const float tileSize = m_pTerrain->m_tileVertexCount * m_pTerrain->m_Desc.vertexDistance;
                    //VKE_PROFILE_SIMPLE2( "Update" );
                    
                    for( uint32_t i = 0; i < vLODData.GetCount(); ++i )
                    {
                        const auto& Curr = vLODData[i];
                        PerDrawData.vecPosition = Curr.DrawData.vecPosition;
                        PerDrawData.topVertexDiff = Curr.DrawData.topVertexDiff;
                        PerDrawData.bottomVertexDiff = Curr.DrawData.bottomVertexDiff;
                        PerDrawData.leftVertexDiff = Curr.DrawData.leftVertexDiff;
                        PerDrawData.rightVertexDiff = Curr.DrawData.rightVertexDiff;
                        PerDrawData.vertexDiff = Pack4BytesToUint(Curr.DrawData.topVertexDiff,
                            Curr.DrawData.bottomVertexDiff,
                            Curr.DrawData.leftVertexDiff,
                            Curr.DrawData.rightVertexDiff);
                        PerDrawData.vecLodColor = aColors[ Curr.lod ];
                        PerDrawData.textureIdx = Curr.DrawData.rootIdx;
                        // Tile size depends on lod.
                        // Max lod is defined in TerrainDesc.tileRowVertexCount
                        // For each lod tile size increases two times
                        // lod0 = tileRowVertexCount * vertexDistance * 1
                        // lod1 = lod0 * 2
                        // lod2 = lod0 * 4
                        PerDrawData.tileSize = Math::CalcPow2(Curr.lod) * tileSize;
                        PerDrawData.TexcoordOffset = Curr.DrawData.TextureOffset;

                        UpdateInfo.stagingBufferOffset = m_pConstantBuffer->CalcAbsoluteOffset(1, i);
                        //UpdateInfo.stagingBufferOffset = m_pConstantBuffer->CalcOffsetInRegion(1u, i);
                        UpdateInfo.dataAlignedSize = m_pConstantBuffer->GetRegionElementSize( 1u );
                        UpdateInfo.dataSize = sizeof(SPerDrawConstantBufferData);
                        UpdateInfo.pSrcData = &PerDrawData;

                        const auto res = pDevice->UpdateStagingBuffer( UpdateInfo );
                        VKE_ASSERT2(VKE_SUCCEEDED(res), "");
                    }
                }
                RenderSystem::SUnlockBufferInfo UnlockInfo;
                UnlockInfo.dstBufferOffset = 0;
                UnlockInfo.hUpdateInfo = hLock;
                UnlockInfo.pDstBuffer = pBufferData.Get();
                const Result res = pDevice->UnlockStagingBuffer( pCtx, UnlockInfo );
                VKE_ASSERT2( res == VKE_OK, "" );
            }
        }

        // Calculates tile center position and top-left corner position
        void vke_force_inline CalcTilePositions( float tileSize, const Math::CVector3& vecLeftTopTerrainCorner,
            const ExtentF32& TileIndex, Math::CVector3* pvecCenterOut, Math::CVector3* pvecTopLeftCorner )
        {
            const float x = vecLeftTopTerrainCorner.x + (uint32_t)TileIndex.x * tileSize;
            const float z = vecLeftTopTerrainCorner.z - (uint32_t)TileIndex.y * tileSize;
            pvecTopLeftCorner->x = x;
            pvecTopLeftCorner->y = 0;
            pvecTopLeftCorner->z = z - tileSize; // tile vertices z are in range 0-N
            pvecCenterOut->x = pvecTopLeftCorner->x + tileSize * 0.5f;
            pvecCenterOut->y = 0;
            pvecCenterOut->z = pvecTopLeftCorner->z + tileSize * 0.5f;
        }

        uint32_t vke_force_inline CalcCircuitDrawcallCount( uint32_t level, uint32_t* colCount, uint32_t* rowCount )
        {
            // level 0 == 1
            // level 1 == 3 * 2 + 1 * 2 == 6 + 2 = 8
            // level 2 == (3+2) * 2 + 3 * 2 == 10 + 6 == 16
            // level 3 == (5+2) * 2 + 5 * 2 == 14 + 10 == 24
            // level 4 == 9 * 2 + 7 * 2 == 18 + 14 == 32
            // level N = (N*2+1) * 2 + (N*2+1 - 2) * 2
            VKE_ASSERT2( level > 0, "" );
            *colCount = ((level * 2) + 1);
            *rowCount = (*colCount - 2);
            uint32_t ret = *colCount * 2 + *rowCount * 2;
            return ret;
        }

        // Clamps (-float, +float) to (0, float*2)
        void vke_force_inline Convert3DPositionToTileSpace( const Math::CVector3& vecTerrainExtents,
            const Math::CVector3& vecPos, ExtentF32* pOut )
        {
            pOut->x = vecPos.x + vecTerrainExtents.width;
            pOut->y = vecPos.z + vecTerrainExtents.depth;
        }

        void vke_force_inline CalcTileIndex( float tileSize, const ExtentF32& Position, ExtentF32* pIndexOut )
        {
            pIndexOut->x = (Position.x / tileSize);
            pIndexOut->y = (Position.y / tileSize);
        }

        /*void CTerrainVertexFetchRenderer::_UpdateTileConstantBufferData( const SPerDrawConstantBufferData& Data,
            uint32_t drawIdx )
        {
            auto pData = (SPerDrawConstantBufferData*)&m_vConstantBufferData[m_pConstantBuffer->CalcOffset( 1, (uint16_t)drawIdx )];
            *pData = Data;
        }*/

        void CTerrainVertexFetchRenderer::_UpdateDrawcalls( CCamera* pCamera )
        {

        }
        bool g_render = true;
        bool g_draw = true;
        bool g_bindPipeline = true;
        bool g_bindBuffers = true;
        bool g_bindCBuffers = true;
        bool g_pipeline = true;
        bool g_draws = true;

        void CTerrainVertexFetchRenderer::Render( RenderSystem::CommandBufferPtr pCommandBuffer, CScene* pScene )
        {
#if VKE_TERRAIN_INSTANCING_RENDERING
            _RenderInstancing( pCommandBuffer, pScene );
#else
            _Render( pCommandBuffer, pScene );
#endif
        }

        void CTerrainVertexFetchRenderer::_RenderInstancing( RenderSystem::CommandBufferPtr pCommandBuffer, CScene* pScene )
        {
#if VKE_TERRAIN_PROFILE_RENDERING
            VKE_PROFILE_SIMPLE();
#endif
#if VKE_DEBUG
            if( !g_render )
                return;
#endif
            // m_resourceIndex = m_frameCount % MAX_FRAME_COUNT;
            // m_frameCount++;
            //m_resourceIndex = 0;
            //const auto& vDrawcalls = m_pTerrain->m_QuadTree.GetLODData();
            //auto maxDrawcalls = m_pTerrain->m_Desc.maxVisibleTiles;
            //bool isPerFrameBound = false;
            RenderSystem::PipelinePtr pLastPipeline;
#if VKE_SCENE_TERRAIN_DEBUG
            RenderSystem::SDebugInfo DbgInfo;
            char text[ 1024 ];
#endif
            //auto hFrameDescSet = m_ahPerFrameDescSets[ m_resourceIndex ];
            auto hSceneBindings = pScene->GetBindings();
            const auto& vTileBindings = m_avTileBindings[ m_backBufferIndex ];
            ( void )vTileBindings;
            const bool tesselationQuads = m_pTerrain->m_Desc.Tesselation.Factors.max > 0 && m_pTerrain->m_Desc.Tesselation.quadMode;
            const uint32_t drawType = ( uint32_t )tesselationQuads * 1;
            uint32_t offsets[ 2 ] = { 0u };
            {
#if VKE_TERRAIN_PROFILE_RENDERING
                VKE_PROFILE_SIMPLE2( "draw loop" );
#endif
#if VKE_DEBUG
                if( g_draws )
#endif              
                    for( uint32_t i = 0; i < m_vInstnacingInfos.GetCount(); ++i )
                    {
                        const auto& Info = m_vInstnacingInfos[ i ];
#if VKE_SCENE_TERRAIN_DEBUG
                        vke_sprintf( text, sizeof( text ), "R:%d, B:%d, L:%d", DrawData.rootIdx, DrawData.bindingIndex,
                                     Drawcall.lod );
                        DbgInfo.pText = text;
                        pCommandBuffer->BeginDebugInfo( &DbgInfo );
#endif
                        if( Info.pPipeline->IsResourceReady() )
                        {
                            offsets[ 1 ] = Info.bufferOffset;
                            m_aDrawParams[ drawType ].Indexed.instanceCount = Info.instanceCount;
                            pCommandBuffer->Bind( Info.pPipeline );
                            pCommandBuffer->Bind( m_ahIndexBuffers[ drawType ], 0u );
                            pCommandBuffer->Bind( m_ahVertexBuffers[ drawType ], 0u );
                            pCommandBuffer->Bind( 0, hSceneBindings, 0u );
                            pCommandBuffer->Bind( 1, m_ahPerInstancedDrawDescSets[ m_lastBindingsUpdateIndex ],
                                offsets, 2u );
                            pCommandBuffer->DrawIndexed( m_aDrawParams[ drawType ] );
                        }
#if VKE_SCENE_TERRAIN_DEBUG
                        pCommandBuffer->EndDebugInfo();
#endif
                    }
            }
        }

        void CTerrainVertexFetchRenderer::_Render( RenderSystem::CommandBufferPtr pCommandBuffer, CScene* )
        {
#if VKE_TERRAIN_PROFILE_RENDERING
            VKE_PROFILE_SIMPLE();
#endif
#if VKE_DEBUG
            if( !g_render )
                return;
#endif
            //m_resourceIndex = m_frameCount % MAX_FRAME_COUNT;
            //m_frameCount++;
            //m_resourceIndex = 0;
            
            const auto& vDrawcalls = m_pTerrain->m_QuadTree.GetLODData();
            auto maxDrawcalls = m_pTerrain->m_Desc.maxVisibleTiles;
            bool isPerFrameBound = false;
            RenderSystem::PipelinePtr pLastPipeline;
#if VKE_SCENE_TERRAIN_DEBUG
            RenderSystem::SDebugInfo DbgInfo;
            char text[ 1024 ];
#endif
            auto hFrameDescSet = m_ahPerFrameDescSets[ m_backBufferIndex ];
            const auto& vTileBindings = m_avTileBindings[ m_backBufferIndex ];
            ( void )vTileBindings;
            const bool tesselationQuads =
                m_pTerrain->m_Desc.Tesselation.Factors.max > 0 && m_pTerrain->m_Desc.Tesselation.quadMode;
            const uint32_t drawType = ( uint32_t )tesselationQuads *1;
            {
#if VKE_TERRAIN_PROFILE_RENDERING
                VKE_PROFILE_SIMPLE2( "draw loop" );
#endif
#if VKE_DEBUG
                if( g_draws )
#endif
                for( uint32_t i = 0; i < vDrawcalls.GetCount() && i < maxDrawcalls; ++i )
                {
                    const auto& Drawcall = vDrawcalls[ i ];
                    const auto& DrawData = Drawcall.DrawData;
                    const auto pPipeline = DrawData.pPipeline;
#if VKE_SCENE_TERRAIN_DEBUG
                    vke_sprintf( text, sizeof( text ), "R:%d, B:%d, L:%d", DrawData.rootIdx, DrawData.bindingIndex,
                                 Drawcall.lod );
                    DbgInfo.pText = text;
                    pCommandBuffer->BeginDebugInfo( &DbgInfo );
#endif
#if VKE_DEBUG
                    if( g_pipeline )
#endif
                        if( pPipeline->IsResourceReady() )
                        {
                            if( pLastPipeline != pPipeline )
                            {
                                // printf( "new pipeline at draw: %d\n", i );
                                pLastPipeline = pPipeline;
#if VKE_DEBUG
                                if( g_bindPipeline )
#endif
                                    pCommandBuffer->Bind( DrawData.pPipeline );
#if VKE_DEBUG
                                if( g_bindBuffers )
#endif
                                    if( !isPerFrameBound )
                                    {
                                        // printf( "new buffer bindings at draw: %d\n", i );
                                        pCommandBuffer->Bind( m_ahIndexBuffers[drawType], 0u );
                                        pCommandBuffer->Bind( m_ahVertexBuffers[drawType], m_vDrawLODs[ 0 ].vertexBufferOffset );
                                        pCommandBuffer->Bind( 0, m_ahPerFrameDescSets[ m_backBufferIndex ],
                                                              m_pConstantBuffer->CalcRelativeOffset( 0, 0 ) );
                                        isPerFrameBound = true;
                                    }
                            }
                            const auto offset = m_pConstantBuffer->CalcRelativeOffset( 1u, i );
#if VKE_DEBUG
                            if( g_bindCBuffers )
#endif
                                pCommandBuffer->Bind( 1, vTileBindings[ DrawData.bindingIndex ], offset );
                        }
#if VKE_DEBUG
                    if( g_draw )
#endif
                        pCommandBuffer->DrawIndexed( m_aDrawParams[drawType] );
#if VKE_SCENE_TERRAIN_DEBUG
                    pCommandBuffer->EndDebugInfo();
#endif
                }
            }
        }

    } // Scene
} // VKE