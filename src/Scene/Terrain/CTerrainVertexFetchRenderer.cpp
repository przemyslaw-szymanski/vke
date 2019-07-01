#include "Scene/Terrain/CTerrainVertexFetchRenderer.h"
#include "Scene/Terrain/CTerrain.h"
#include "Scene/CScene.h"

#include "RenderSystem/CDeviceContext.h"
#include "RenderSystem/CGraphicsContext.h"

#include "RenderSystem/IFrameGraph.h"

namespace VKE
{
    namespace Scene
    {
        void CTerrainVertexFetchRenderer::_Destroy()
        {

        }

        Utils::TCDynamicArray< Math::CVector3, 1 > CreateVertices( const STerrainDesc& Desc )
        {
            Utils::TCDynamicArray< Math::CVector3, 1 > vVertices;
            uint32_t vertexCount = Desc.tileRowVertexCount * Desc.tileRowVertexCount;
            vVertices.Resize( vertexCount );

            Math::CVector3 vecCurr = Math::CVector3::ZERO;
            float step = Desc.vertexDistance;
            uint32_t idx = 0;

            for( uint32_t z = 0; z < Desc.tileRowVertexCount; ++z )
            {
                vecCurr.z = step * z;
                for( uint32_t x = 0; x < Desc.tileRowVertexCount; ++x )
                {
                    vecCurr.x = step * x;
                    vVertices[idx++] = vecCurr;
                }
            }

            /*vVertices =
            {
                { 0.0f,   0.5f,   0.0f },
                { -0.5f, -0.5f,   0.0f },
                { 0.5f,  -0.5f,   0.0f }
            };

            vVertices =
            {
                { 0.0f,   0.0f,   100.0f },
                { -100.0f, -0.0f,   -100.0f },
                { 100.0f,  -0.0f,   -100.0f }
            };*/
            return vVertices;
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
        IndexBuffer CreateIndices( const STerrainDesc& Desc, uint8_t lod )
        {
            IndexBuffer vIndices;
            uint32_t vertexCount = Desc.tileRowVertexCount;
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
#define CCW 1
#define CALC_XY(_x, _y, _w) (uint16_t)( (_x) + (_w) * (_y) )
#define CALC_IDX_00(_x, _y, _w) CALC_XY( _x, _y, _w ) 
#define CALC_IDX_01(_x, _y, _w) CALC_XY( _x, _y + 1, _w )
#define CALC_IDX_10(_x, _y, _w) CALC_XY( _x + 1, _y, _w )
#define CALC_IDX_11(_x, _y, _w) CALC_XY( _x + 1, _y + 1, _w )

            for( uint16_t y = 0; y < Desc.tileRowVertexCount - 1; ++y )
            {
                for( uint16_t x = 0; x < Desc.tileRowVertexCount - 1; ++x )
                {
#if CCW
                    /*
                    *---*   (0,0)----(1,0)
                    | /       |   /    |
                    *   *   (0,1)----(1,1)
                    */
                    auto a = CALC_IDX_00( x, y, Desc.tileRowVertexCount );
                    auto b = CALC_IDX_01( x, y, Desc.tileRowVertexCount );
                    auto c = CALC_IDX_10( x, y, Desc.tileRowVertexCount );
                    vIndices[currIdx++] = a;
                    vIndices[currIdx++] = b;
                    vIndices[currIdx++] = c;
                    /*
                    *   *   (0,0)----(1,0)
                      / |     |   /    |
                    *---*   (0,1)----(1,1)
                    */
                    a = CALC_IDX_10( x, y, Desc.tileRowVertexCount );
                    b = CALC_IDX_01( x, y, Desc.tileRowVertexCount );
                    c = CALC_IDX_11( x, y, Desc.tileRowVertexCount );
                    vIndices[currIdx++] = a;
                    vIndices[currIdx++] = b;
                    vIndices[currIdx++] = c;
#else
                    /*
                    *---*   (0,0)----(1,0)
                    | /       |   /    |
                    *   *   (0,1)----(1,1)
                    */
                    vIndices[currIdx++] = CALC_IDX_00( x, y, Desc.tileRowVertexCount );
                    vIndices[currIdx++] = CALC_IDX_10( x, y, Desc.tileRowVertexCount );
                    vIndices[currIdx++] = CALC_IDX_01( x, y, Desc.tileRowVertexCount );
                    /*
                    *   *   (0,0)----(1,0)
                    / |     |   /    |
                    *---*   (0,1)----(1,1)
                    */
                    vIndices[currIdx++] = CALC_IDX_01( x, y, Desc.tileRowVertexCount );
                    vIndices[currIdx++] = CALC_IDX_10( x, y, Desc.tileRowVertexCount );
                    vIndices[currIdx++] = CALC_IDX_11( x, y, Desc.tileRowVertexCount );
#endif
                }
            }
            VKE_ASSERT( vIndices.GetCount() == currIdx, "" );
            //vIndices = {0,1,2};
            return vIndices;
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

        Result CTerrainVertexFetchRenderer::_Create( const STerrainDesc& Desc,
                                                     RenderSystem::CDeviceContext* pCtx )
        {
            Result ret = VKE_FAIL;

            m_maxVisibleTiles = CalcMaxVisibleTiles( Desc );

            SDrawcallDesc DrawcallDesc;
            DrawcallDesc.type = RenderSystem::DrawcallTypes::STATIC_OPAQUE;
            m_pDrawcall = m_pTerrain->GetScene()->CreateDrawcall( DrawcallDesc );
            
            const uint32_t totalIndexCount = CalcTotalIndexCount( Desc.tileRowVertexCount, Desc.lodCount );
            IndexBuffer vIndices;
            vIndices.Reserve( totalIndexCount );

            const auto vVertices = CreateVertices( Desc );
            RenderSystem::SCreateBufferDesc BuffDesc;
            BuffDesc.Create.async = false;
            BuffDesc.Buffer.memoryUsage = RenderSystem::MemoryUsages::GPU_ACCESS;
            BuffDesc.Buffer.usage = RenderSystem::BufferUsages::VERTEX_BUFFER;
            BuffDesc.Buffer.size = vVertices.GetCount() * sizeof( Math::CVector3 );
            m_hVertexBuffer = pCtx->CreateBuffer( BuffDesc );
            RenderSystem::SUpdateMemoryInfo UpdateInfo;
            UpdateInfo.dataSize = BuffDesc.Buffer.size;
            UpdateInfo.dstDataOffset = 0;
            UpdateInfo.pData = vVertices.GetData();
            pCtx->UpdateBuffer( UpdateInfo, &m_hVertexBuffer );

            for( uint32_t i = 0; i < Desc.lodCount; ++i )
            {
                RenderSystem::CDrawcall::LOD LOD;
                const auto vLODIndices = CreateIndices( Desc, 0 );
                LOD.indexBufferOffset = vIndices.GetCount() * sizeof( IndexType );
           
                LOD.DrawParams.Indexed.indexCount = vLODIndices.GetCount();
                LOD.DrawParams.Indexed.instanceCount = 1;
                LOD.DrawParams.Indexed.startIndex = 0;
                LOD.DrawParams.Indexed.startInstance = 0;
                LOD.DrawParams.Indexed.vertexOffset = 0;

                m_pDrawcall->AddLOD( LOD );
                vIndices.Append( vLODIndices );
            }

            BuffDesc.Buffer.usage = RenderSystem::BufferUsages::INDEX_BUFFER;
            BuffDesc.Buffer.size = vIndices.GetCount() * sizeof( IndexType );
            BuffDesc.Buffer.indexType = RenderSystem::IndexTypes::UINT16;

            m_hIndexBuffer = pCtx->CreateBuffer( BuffDesc );
            UpdateInfo.dataSize = BuffDesc.Buffer.size;
            UpdateInfo.pData = vIndices.GetData();
            pCtx->UpdateBuffer( UpdateInfo, &m_hIndexBuffer );

            _CreateConstantBuffers( pCtx );

            for( uint8_t i = 0; i < Desc.lodCount; ++i )
            {
                auto& LOD = m_pDrawcall->GetLOD( i );
                LOD.hIndexBuffer = RenderSystem::HandleCast<RenderSystem::IndexBufferHandle>( m_hIndexBuffer );
                LOD.hVertexBuffer = RenderSystem::HandleCast<RenderSystem::VertexBufferHandle>( m_hVertexBuffer );
                _CreatePipeline( Desc, i, pCtx, &LOD );
                LOD.vertexBufferOffset = 0;
                LOD.hDescSet = m_hPerTileDescSet;
                LOD.descSetOffset = 0;
            }

            auto pData = m_vConstantBufferData.GetData() + m_pConstantBuffer->CalcOffset( 0, 0 );
            auto pPerFrameData = ( SPerFrameConstantBuffer* )pData;
            pPerFrameData->mtxViewProj = Math::CMatrix4x4::IDENTITY;
            pPerFrameData->Height = Desc.Height;
            pPerFrameData->TerrainSize = Desc.Size;
            pPerFrameData->vertexDistance = Desc.vertexDistance;
            pPerFrameData->tileRowVertexCount = Desc.tileRowVertexCount;

            UpdateInfo.dataSize = sizeof( SPerFrameConstantBuffer );
            UpdateInfo.dstDataOffset = m_pConstantBuffer->CalcOffset( 0, 0 );
            UpdateInfo.pData = pData;
            ret = pCtx->UpdateBuffer( UpdateInfo, &m_pConstantBuffer );

            if( VKE_SUCCEEDED( ret ) )
            {
                ret = _CreateDrawcalls( Desc );
            }

            return ret;

        }

        Result CTerrainVertexFetchRenderer::_CreateConstantBuffers( RenderSystem::CDeviceContext* pCtx )
        {
            Result ret = VKE_FAIL;

            RenderSystem::SCreateBufferDesc Desc;
            Desc.Create.async = false;
            Desc.Buffer.memoryUsage = RenderSystem::MemoryUsages::GPU_ACCESS;
            Desc.Buffer.usage = RenderSystem::BufferUsages::CONSTANT_BUFFER;
            Desc.Buffer.size = 0;
            Desc.Buffer.vRegions =
            {
                RenderSystem::SBufferRegion( 1u, (uint16_t)sizeof( SPerFrameConstantBuffer ) ),
                RenderSystem::SBufferRegion( 1u, (uint16_t)sizeof( SPerDrawConstantBuffer ) )
            };

            auto hBuffer = pCtx->CreateBuffer( Desc );
            if( hBuffer != NULL_HANDLE )
            {
                m_pConstantBuffer = pCtx->GetBuffer( hBuffer );
                RenderSystem::SCreateBindingDesc BindingDesc;
                BindingDesc.AddBuffer( 0, RenderSystem::PipelineStages::VERTEX );
                m_hPerTileDescSet = pCtx->CreateResourceBindings( BindingDesc );
                m_hPerFrameDescSet = pCtx->CreateResourceBindings( BindingDesc );
                if( m_hPerFrameDescSet != NULL_HANDLE && m_hPerFrameDescSet != NULL_HANDLE )
                {
                    RenderSystem::SUpdateBindingsInfo UpdateInfo;
                    UpdateInfo.AddBinding( 0, m_pConstantBuffer->CalcOffset( 1, 0 ), m_pConstantBuffer->GetRegionElementSize( 1 ),
                        &hBuffer, 1u );
                    pCtx->UpdateDescriptorSet( UpdateInfo, &m_hPerTileDescSet );
                    UpdateInfo.Reset();
                    UpdateInfo.AddBinding( 0, m_pConstantBuffer->CalcOffset( 0, 0 ), m_pConstantBuffer->GetRegionElementSize( 0 ),
                        &hBuffer, 1u );
                    pCtx->UpdateDescriptorSet( UpdateInfo, &m_hPerFrameDescSet );
                }
            }

            m_hDDISets[0] = pCtx->GetDescriptorSet( m_hPerFrameDescSet );
            m_hDDISets[1] = pCtx->GetDescriptorSet( m_hPerTileDescSet );

            m_vConstantBufferData.Resize( m_pConstantBuffer->GetSize() );

            

            return ret;
        }

        static cstr_t g_pTerrainVS = VKE_TO_STRING
        (
            #version 450 core\n

            layout( set = 0, binding = 0 ) uniform PerFrameTerrainConstantBuffer
            {
                mat4    mtxViewProj;
                vec2    vec2TerrainSize;
                vec2    vec2TerrainHeight;
                float   vertexDistance;
                uint    tileRowVertexCount;
            };

            layout( set = 1, binding = 0 ) uniform PerTileConstantBuffer
            {
                vec4    vec4Position;
            };

            layout( location = 0 ) in vec3 iPosition;

            void main()
            {
                mat4 mtxMVP = mtxViewProj;
                gl_Position = mtxMVP * vec4( iPosition + vec4Position.xyz, 1.0 );
            }
        );

        static cstr_t g_pTerrainPS = VKE_TO_STRING
        (
            #version 450 core\n

            layout( location = 0 ) out vec4 oColor;

            void main()
            {
                oColor = vec4(1,1,1,1);
            }
        );

        Result CTerrainVertexFetchRenderer::_CreatePipeline( const STerrainDesc& Desc, uint8_t lod,
            RenderSystem::CDeviceContext* pCtx, RenderSystem::CDrawcall::LOD* pInOut )
        {
            auto& LOD = *pInOut;

            Result ret = VKE_OK;

            RenderSystem::SShaderData VsData, PsData;
            VsData.pCode = (uint8_t*)g_pTerrainVS;
            VsData.codeSize = (uint32_t)strlen( g_pTerrainVS );
            VsData.state = RenderSystem::ShaderStates::HIGH_LEVEL_TEXT;
            VsData.type = RenderSystem::ShaderTypes::VERTEX;

            RenderSystem::SCreateShaderDesc VsDesc, PsDesc;
            VsDesc.Create.async = true;
            VsDesc.Shader.Base.pName = "VertexFetchTerrainVS";
            VsDesc.Shader.pData = &VsData;
            VsDesc.Shader.pEntryPoint = "main";
            VsDesc.Shader.type = RenderSystem::ShaderTypes::VERTEX;

            auto pVs = pCtx->CreateShader( VsDesc );

            PsData.pCode = (uint8_t*)g_pTerrainPS;
            PsData.codeSize = (uint32_t)strlen( g_pTerrainPS );
            PsData.state = RenderSystem::ShaderStates::HIGH_LEVEL_TEXT;
            PsData.type = RenderSystem::ShaderTypes::PIXEL;

            char aPsEntryPointName[128];
            //vke_sprintf( aPsEntryPointName, 128, "main_lod%d", lod );
            vke_sprintf( aPsEntryPointName, 128, "main" );

            PsDesc.Create.async = true;
            PsDesc.Shader.Base.pName = "VertexFetchTerrianPS";
            PsDesc.Shader.pEntryPoint = aPsEntryPointName;
            PsDesc.Shader.pData = &PsData;
            PsDesc.Shader.type = RenderSystem::ShaderTypes::PIXEL;

            auto pPs = pCtx->CreateShader( PsDesc );

            RenderSystem::SPipelineLayoutDesc LayoutDesc;
            LayoutDesc.vDescriptorSetLayouts =
            {
                pCtx->GetDescriptorSetLayout( m_hPerTileDescSet ),
                pCtx->GetDescriptorSetLayout( m_hPerFrameDescSet )
            };
            auto pLayout = pCtx->CreatePipelineLayout( LayoutDesc );
            auto ddi = pLayout->GetDDIObject();
            auto hdsl = pCtx->GetDescriptorSetLayout( m_hPerFrameDescSet );
            auto ddidsl = pCtx->GetDescriptorSetLayout( hdsl );
            ddidsl = ddidsl;
            ddi = ddi;
            RenderSystem::SPipelineCreateDesc PipelineDesc;
            PipelineDesc.Create.async = true;
            auto& Pipeline = PipelineDesc.Pipeline;

            PipelineDesc.Pipeline.hLayout = pLayout->GetHandle();

            PipelineDesc.Pipeline.InputLayout.topology = RenderSystem::PrimitiveTopologies::TRIANGLE_LIST;
            VKE::RenderSystem::SPipelineDesc::SInputLayout::SVertexAttribute VA;
            VA.format = VKE::RenderSystem::Formats::R32G32B32_SFLOAT;
            VA.location = 0;
            VA.vertexBufferBindingIndex = 0;
            VA.offset = 0;
            VA.stride = 3 * 4;
            PipelineDesc.Pipeline.InputLayout.vVertexAttributes.PushBack( VA );
            /*auto& DS = PipelineDesc.Pipeline.DepthStencil;
            DS.Depth.compareFunc = RenderSystem::CompareFunctions::GREATER_EQUAL;
            DS.Depth.enable = false;
            DS.Depth.enableTest = true;
            DS.Depth.enableWrite = true;*/
            /*auto& Blend = PipelineDesc.Pipeline.Blending;
            Blend.enable = false;*/
            Pipeline.Rasterization.Polygon.mode = RenderSystem::PolygonModes::WIREFRAME;
            PipelineDesc.Pipeline.Shaders.apShaders[RenderSystem::ShaderTypes::VERTEX] = pVs;
            PipelineDesc.Pipeline.Shaders.apShaders[RenderSystem::ShaderTypes::PIXEL] = pPs;

            for( uint32_t i = 0; i < Desc.vDDIRenderPasses.GetCount(); ++i )
            {
                PipelineDesc.Pipeline.hDDIRenderPass = Desc.vDDIRenderPasses[i];
                auto pPipeline = pCtx->CreatePipeline( PipelineDesc );
                if( pPipeline.IsNull() )
                {
                    ret = VKE_FAIL;
                }
                else
                {
                    LOD.vpPipelines.PushBack( pPipeline );
                }
            }
            return ret;
        }

        Result CTerrainVertexFetchRenderer::_CreateDrawcalls( const STerrainDesc& Desc )
        {
            Result ret = VKE_OK;
            CScene* pScene = m_pTerrain->GetScene();

            Math::CVector3 vecAABBSize( Desc.vertexDistance * Desc.tileRowVertexCount );

            SDrawcallDesc DrawcallDesc;
            DrawcallDesc.type = RenderSystem::DrawcallTypes::STATIC_OPAQUE;
            RenderSystem::CDrawcall::LOD LOD = m_pDrawcall->GetLOD(0);
            SDrawcallDataInfo Info;
            Info.AABB = Math::CAABB( Math::CVector3::ZERO, vecAABBSize );

            m_vpDrawcalls.Reserve( m_maxVisibleTiles );
 
            for( uint32_t i = 0; i < m_maxVisibleTiles; ++i )
            {
                auto pDrawcall = pScene->CreateDrawcall( DrawcallDesc );
                
                for( uint32_t l = 0; l < Desc.lodCount; ++l )
                {
                    pDrawcall->AddLOD( LOD );
                }
                pDrawcall->DisableFrameGraphRendering( true );
                m_vpDrawcalls.PushBack( pDrawcall );
                pScene->AddObject( pDrawcall, Info );
            }
            return ret;
        }

        void CTerrainVertexFetchRenderer::Update( RenderSystem::CGraphicsContext* pCtx, CCamera* pCamera )
        {
            _UpdateConstantBuffers( pCtx, pCamera );
            _UpdateDrawcalls( pCamera );
        }

        void CTerrainVertexFetchRenderer::_UpdateConstantBuffers( RenderSystem::CGraphicsContext* pCtx, CCamera* pCamera )
        {
            auto pData = m_vConstantBufferData.GetData() + m_pConstantBuffer->CalcOffset( 0, 0 );
            auto pPerFrameData = (SPerFrameConstantBuffer*)pData;
            pPerFrameData->mtxViewProj = pCamera->GetViewProjectionMatrix();

            pData = m_vConstantBufferData.GetData() + m_pConstantBuffer->CalcOffset( 1, 0 );
            auto pPerTileData = (SPerDrawConstantBuffer*)pData;
            pPerTileData->vecPosition = Math::CVector4( -33.0f, 0, 0, 0 );
            //pPerTileData->mtxTransform = Math::CMatrix4x4::IDENTITY;
            //Math::CMatrix4x4::Translate( Math::CVector3( 0.0f, 0.0f, 0.0f ), &pPerTileData->mtxTransform );
            //Math::CMatrix4x4::Mul( pPerTileData->mtxTransform, pPerFrameData->mtxViewProj, &pPerTileData->mtxTransform );

            RenderSystem::SUpdateMemoryInfo UpdateInfo;
            UpdateInfo.dataSize = m_vConstantBufferData.GetCount();
            UpdateInfo.dstDataOffset = 0;
            UpdateInfo.pData = m_vConstantBufferData.GetData();
            pCtx->UpdateBuffer( UpdateInfo, &m_pConstantBuffer );
        }

        // Calculates tile center position and top-left corner position
        void vke_force_inline CalcTilePositions( float tileSize, const ExtentU32& HalfTileCount, const ExtentU32& TileIndex,
            Math::CVector3* pvecCenterOut, Math::CVector3* pvecTopLeftCorner )
        {
            int32_t x = TileIndex.x - HalfTileCount.x;
            int32_t y = TileIndex.y - HalfTileCount.y;
            pvecTopLeftCorner->x = tileSize * x;
            pvecTopLeftCorner->y = 0;
            pvecTopLeftCorner->z = tileSize * y;
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
            VKE_ASSERT( level > 0, "" );
            *colCount = ((level * 2) + 1);
            *rowCount = (*colCount - 2);
            uint32_t ret = *colCount * 2 + *rowCount * 2;
            return ret;
        }

        // Clamps (-float, +float) to (0, float*2)
        void vke_force_inline Convert3DPositionToTileSpace( const ExtentU32& HalfExtents, const Math::CVector3& vecPos,
            ExtentU32* pOut )
        {
            pOut->x = (uint32_t)vecPos.x + HalfExtents.width;
            pOut->y = (uint32_t)vecPos.z + HalfExtents.height;
        }

        void vke_force_inline CalcTileIndex( float tileSize, const ExtentU32& Position, ExtentU32* pIndexOut )
        {
            pIndexOut->x = (uint32_t)(Position.x / tileSize);
            pIndexOut->y = (uint32_t)(Position.y / tileSize);
        }

        void CTerrainVertexFetchRenderer::_UpdateDrawcalls( CCamera* pCamera )
        {
            // Position each AABB
            const auto& Desc = m_pTerrain->GetDesc();
            const float tileSize = Desc.vertexDistance * Desc.tileRowVertexCount;

            const Math::CVector3 vecTileSize( tileSize );
            const Math::CVector3 vecMax = Math::CVector3( Desc.Size.width * 0.5f, 0, Desc.Size.height * 0.5f );
            const Math::CVector3 vecMin = Math::CVector3( -vecMax.z, 0, -vecMax.z );
            const ExtentU32 TileCount( (uint32_t)(Desc.Size.width / tileSize), (uint32_t)(Desc.Size.height / tileSize) );
            const ExtentU32 HalfTileCount( TileCount.x / 2, TileCount.y / 2 );
            const Math::CVector3& vecCamPos = pCamera->GetPosition();
            const ExtentU32 HalfSize( (uint32_t)Desc.Size.width / 2, (uint32_t)Desc.Size.height / 2 );

            CScene* pScene = m_pTerrain->GetScene();

            // Calc tile where camera is on
            ExtentU32 CurrTileIndex;
            Math::CVector3 vecCurrTileCenter, vecCurrTileCorner;
            Math::CAABB CurrTileAABB( Math::CVector3::ZERO, vecTileSize );

            // Calc center tile
            ExtentU32 CenterTileIndex;
            Convert3DPositionToTileSpace( HalfSize, vecCamPos, &CenterTileIndex );
            CalcTileIndex( tileSize, CenterTileIndex, &CenterTileIndex );
            CalcTilePositions( tileSize, HalfTileCount, CenterTileIndex, &vecCurrTileCenter, &vecCurrTileCorner );
            CurrTileAABB = Math::CAABB( vecCurrTileCenter, vecTileSize );
            pScene->UpdateDrawcallAABB( m_vpDrawcalls[0]->GetHandle(), CurrTileAABB );

            uint32_t processedDrawcallCount = 0;
            // Update in circle, starting from the camera position
            for( uint32_t i = 1, circleLevel = 1; i < m_vpDrawcalls.GetCount(); i += processedDrawcallCount, ++circleLevel )
            {
                uint32_t rowCount, colCount;
                processedDrawcallCount = CalcCircuitDrawcallCount( circleLevel, &colCount, &rowCount );
                // Calc top row
                CurrTileIndex.y = CenterTileIndex.y + (rowCount / 2 + 1);
                CurrTileIndex.x = CenterTileIndex.x - (rowCount / 2 + 1);
                for( uint32_t c = 0; c < colCount; ++c )
                {
                    auto& pCurr = m_vpDrawcalls[i + c];
                    CalcTilePositions( tileSize, HalfTileCount, CurrTileIndex, &vecCurrTileCenter, &vecCurrTileCorner );
                    CurrTileAABB = Math::CAABB( vecCurrTileCenter, vecTileSize );
                    pScene->UpdateDrawcallAABB( pCurr->GetHandle(), CurrTileAABB );
                    CurrTileIndex.x++;
                }
                // Calc bottom row
                CurrTileIndex.y = CenterTileIndex.y - (rowCount / 2 + 1);
                CurrTileIndex.x = CenterTileIndex.x - (rowCount / 2 + 1);
                for( uint32_t c = 0; c < colCount; ++c )
                {
                    auto& pCurr = m_vpDrawcalls[i + c];
                    CalcTilePositions( tileSize, HalfTileCount, CurrTileIndex, &vecCurrTileCorner, &vecCurrTileCorner );
                    CurrTileAABB = Math::CAABB( vecCurrTileCenter, vecTileSize );
                    pScene->UpdateDrawcallAABB( pCurr->GetHandle(), CurrTileAABB );
                    CurrTileIndex.x++;
                }
                // Calc left row
                CurrTileIndex.x = CenterTileIndex.x - (rowCount / 2 + 1);
                CurrTileIndex.y = CenterTileIndex.y - (rowCount / 2 + 1) + 1;
                for( uint32_t r = 0; r < rowCount; ++r )
                {
                    auto& pCurr = m_vpDrawcalls[i + r];
                    CalcTilePositions( tileSize, HalfTileCount, CurrTileIndex, &vecCurrTileCorner, &vecCurrTileCorner );
                    CurrTileAABB = Math::CAABB( vecCurrTileCenter, vecTileSize );
                    pScene->UpdateDrawcallAABB( pCurr->GetHandle(), CurrTileAABB );
                    CurrTileIndex.y++;
                }
                // Calc right row
                CurrTileIndex.x = CenterTileIndex.x + (rowCount / 2 + 1);
                CurrTileIndex.y = CenterTileIndex.y - (rowCount / 2 + 1) + 1;
                for( uint32_t r = 0; r < rowCount; ++r )
                {
                    auto& pCurr = m_vpDrawcalls[i + r];
                    CalcTilePositions( tileSize, HalfTileCount, CurrTileIndex, &vecCurrTileCorner, &vecCurrTileCorner );
                    CurrTileAABB = Math::CAABB( vecCurrTileCenter, vecTileSize );
                    pScene->UpdateDrawcallAABB( pCurr->GetHandle(), CurrTileAABB );
                    CurrTileIndex.y++;
                }
            }
        }

        void CTerrainVertexFetchRenderer::Render( RenderSystem::CGraphicsContext* pCtx, CCamera* pCamera )
        {
            RenderSystem::CCommandBuffer* pCb = pCtx->GetCommandBuffer();
            CScene* pScene = m_pTerrain->GetScene();

            for( uint32_t i = 0; i < m_vpDrawcalls.GetCount(); ++i )
            {
                //if(pScene-> )
            }
            /*const auto& LOD = m_pDrawcall->GetLOD( 0 );
            auto pPipeline = LOD.vpPipelines[0];
            if( pPipeline->IsReady() )
            {
                pCb->Bind( pPipeline );

                uint32_t offset = 0;
                m_BindingTables[0].aDDISetHandles = &m_hDDISets[0];
                m_BindingTables[0].aDynamicOffsets = &offset;
                m_BindingTables[0].dynamicOffsetCount = 1;
                m_BindingTables[0].firstSet = 0;
                m_BindingTables[0].pCmdBuffer = pCb;
                m_BindingTables[0].pPipelineLayout = pPipeline->GetLayout().Get();
                m_BindingTables[0].setCount = 1;
                m_BindingTables[0].type = RenderSystem::PipelineTypes::GRAPHICS;

                pCb->Bind( m_BindingTables[0] );

                m_BindingTables[1].aDDISetHandles = &m_hDDISets[1];
                m_BindingTables[1].aDynamicOffsets = &LOD.descSetOffset;
                m_BindingTables[1].dynamicOffsetCount = 1;
                m_BindingTables[1].firstSet = 1;
                m_BindingTables[1].pCmdBuffer = pCb;
                m_BindingTables[1].pPipelineLayout = pPipeline->GetLayout().Get();
                m_BindingTables[1].setCount = 1;
                m_BindingTables[1].type = RenderSystem::PipelineTypes::GRAPHICS;
                pCb->Bind( m_BindingTables[1] );



                pCb->Bind( LOD.hVertexBuffer, LOD.vertexBufferOffset );
                pCb->Bind( LOD.hIndexBuffer, LOD.indexBufferOffset );

                pCb->DrawIndexed( LOD.DrawParams );
            }*/
        }

    } // Scene
} // VKE