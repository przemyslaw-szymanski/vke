#include "Scene/Terrain/CTerrainVertexFetchRenderer.h"
#include "Scene/Terrain/CTerrain.h"
#include "Scene/CScene.h"

#include "RenderSystem/CDeviceContext.h"
#include "RenderSystem/CGraphicsContext.h"

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
            uint32_t vertexCount = Desc.vertexCountPerTile * Desc.vertexCountPerTile;
            vVertices.Resize( vertexCount );

            Math::CVector3 vecCurr = Math::CVector3::ZERO;
            float step = Desc.vertexDistance;
            uint32_t idx = 0;

            for( uint32_t z = 0; z < Desc.vertexCountPerTile; ++z )
            {
                vecCurr.z = step * z;
                for( uint32_t x = 0; x < Desc.vertexCountPerTile; ++x )
                {
                    vecCurr.x = step * x;
                    vVertices[idx++] = vecCurr;
                }
            }
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

        Utils::TCDynamicArray< uint16_t, 1 > CreateIndices( const STerrainDesc& Desc, uint8_t lod )
        {
            Utils::TCDynamicArray< uint16_t, 1 > vIndices;
            uint32_t vertexCount = Desc.vertexCountPerTile;
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

            for( uint32_t y = 0; y < Desc.vertexCountPerTile - 1; ++y )
            {
                for( uint32_t x = 0; x < Desc.vertexCountPerTile - 1; ++x )
                {
#if CCW
                    /*
                    *---*   (0,0)----(1,0)
                    | /       |   /    |
                    *   *   (0,1)----(1,1)
                    */
                    auto a = CALC_IDX_00( x, y, Desc.vertexCountPerTile );
                    auto b = CALC_IDX_01( x, y, Desc.vertexCountPerTile );
                    auto c = CALC_IDX_10( x, y, Desc.vertexCountPerTile );
                    vIndices[currIdx++] = a;
                    vIndices[currIdx++] = b;
                    vIndices[currIdx++] = c;
                    /*
                    *   *   (0,0)----(1,0)
                      / |     |   /    |
                    *---*   (0,1)----(1,1)
                    */
                    a = CALC_IDX_10( x, y, Desc.vertexCountPerTile );
                    b = CALC_IDX_01( x, y, Desc.vertexCountPerTile );
                    c = CALC_IDX_11( x, y, Desc.vertexCountPerTile );
                    vIndices[currIdx++] = a;
                    vIndices[currIdx++] = b;
                    vIndices[currIdx++] = c;
#else
                    /*
                    *---*   (0,0)----(1,0)
                    | /       |   /    |
                    *   *   (0,1)----(1,1)
                    */
                    vIndices[currIdx++] = CALC_IDX_00( x, y, Desc.vertexCountPerTile );
                    vIndices[currIdx++] = CALC_IDX_10( x, y, Desc.vertexCountPerTile );
                    vIndices[currIdx++] = CALC_IDX_01( x, y, Desc.vertexCountPerTile );
                    /*
                    *   *   (0,0)----(1,0)
                    / |     |   /    |
                    *---*   (0,1)----(1,1)
                    */
                    vIndices[currIdx++] = CALC_IDX_01( x, y, Desc.vertexCountPerTile );
                    vIndices[currIdx++] = CALC_IDX_10( x, y, Desc.vertexCountPerTile );
                    vIndices[currIdx++] = CALC_IDX_11( x, y, Desc.vertexCountPerTile );
#endif
                }
            }
            VKE_ASSERT( vIndices.GetCount() == currIdx, "" );
            return vIndices;
        }

        Result CTerrainVertexFetchRenderer::_Create( const STerrainDesc& Desc, RenderSystem::CDeviceContext* pCtx )
        {
            Result ret = VKE_FAIL;

            m_maxVisibleTiles = 1000;

            SDrawcallDesc DrawcallDesc;
            DrawcallDesc.type = RenderSystem::DrawcallTypes::STATIC_OPAQUE;
            m_pDrawcall = m_pTerrain->GetScene()->CreateDrawcall( DrawcallDesc );
            
            const uint32_t totalIndexCount = CalcTotalIndexCount( Desc.vertexCountPerTile, Desc.lodCount );
            Utils::TCDynamicArray< uint16_t, 1 > vIndices;
            vIndices.Reserve( totalIndexCount );

            const auto vVertices = CreateVertices( Desc );
            RenderSystem::SCreateBufferDesc BuffDesc;
            BuffDesc.Create.async = false;
            BuffDesc.Buffer.memoryUsage = RenderSystem::MemoryUsages::STATIC | RenderSystem::MemoryUsages::BUFFER;
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
                LOD.indexBufferOffset = vIndices.GetCount() * sizeof( uint16_t );
           
                LOD.DrawParams.Indexed.indexCount = vLODIndices.GetCount();
                LOD.DrawParams.Indexed.instanceCount = 1;
                LOD.DrawParams.Indexed.startIndex = 0;
                LOD.DrawParams.Indexed.startInstance = 0;
                LOD.DrawParams.Indexed.vertexOffset = 0;

                m_pDrawcall->AddLOD( LOD );
                vIndices.Append( vLODIndices );
            }

            BuffDesc.Buffer.usage = RenderSystem::BufferUsages::INDEX_BUFFER;
            BuffDesc.Buffer.size = vIndices.GetCount() * sizeof( uint16_t );
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

            ret = VKE_OK;

            return ret;

        }

        Result CTerrainVertexFetchRenderer::_CreateConstantBuffers( RenderSystem::CDeviceContext* pCtx )
        {
            Result ret = VKE_FAIL;

            RenderSystem::SCreateBufferDesc Desc;
            Desc.Create.async = false;
            Desc.Buffer.memoryUsage = RenderSystem::MemoryUsages::BUFFER | RenderSystem::MemoryUsages::STATIC;
            Desc.Buffer.usage = RenderSystem::BufferUsages::CONSTANT_BUFFER;
            Desc.Buffer.size = 0;
            Desc.Buffer.vRegions =
            {
                RenderSystem::SBufferRegion( 1u, (uint16_t)sizeof( SPerFrameConstantBuffer ) ),
                RenderSystem::SBufferRegion( m_maxVisibleTiles, (uint16_t)sizeof( SPerDrawConstantBuffer ) )
            };

            auto hBuffer = pCtx->CreateBuffer( Desc );
            if( hBuffer != NULL_HANDLE )
            {
                m_pConstantBuffer = pCtx->GetBuffer( hBuffer );
                RenderSystem::SCreateBindingDesc BindingDesc;
                BindingDesc.AddBuffer( 0, RenderSystem::PipelineStages::VERTEX | RenderSystem::PipelineStages::PIXEL );
                m_hPerTileDescSet = pCtx->CreateResourceBindings( BindingDesc );
                m_hPerFrameDescSet = pCtx->CreateResourceBindings( BindingDesc );
                if( m_hPerFrameDescSet != NULL_HANDLE && m_hPerFrameDescSet != NULL_HANDLE )
                {
                    RenderSystem::SUpdateBindingsInfo UpdateInfo;
                    UpdateInfo.AddBinding( 0, m_pConstantBuffer->CalcOffset( 1, 0 ), m_pConstantBuffer->GetRegionSize( 1 ),
                        &hBuffer, 1u );
                    pCtx->UpdateDescriptorSet( UpdateInfo, &m_hPerTileDescSet );
                    UpdateInfo.Reset();
                    UpdateInfo.AddBinding( 0, m_pConstantBuffer->CalcOffset( 0, 0 ), m_pConstantBuffer->GetRegionSize( 0 ),
                        &hBuffer, 1u );
                    pCtx->UpdateDescriptorSet( UpdateInfo, &m_hPerFrameDescSet );
                }
            }

            m_hDDISets[0] = pCtx->GetDescriptorSet( m_hPerFrameDescSet );
            m_hDDISets[1] = pCtx->GetDescriptorSet( m_hPerTileDescSet );

            return ret;
        }

        static cstr_t g_pTerrainVS = VKE_TO_STRING
        (
            #version 450 core\n

            layout( set = 0, binding = 0 ) uniform PerFrameTerrainConstantBuffer
            {
                mat4    mtxViewProj;
            };

            layout( set = 1, binding = 0 ) uniform PerTileConstantBuffer
            {
                mat4    mtxTransform;
            };

            layout( location = 0 ) in vec3 iPosition;

            void main()
            {
                gl_Position = mtxViewProj * vec4( iPosition, 1.0 ) + vec4( 0.01, 0.0, 0, 0 );
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

            PipelineDesc.Pipeline.hLayout = pLayout->GetHandle();

            PipelineDesc.Pipeline.InputLayout.topology = RenderSystem::PrimitiveTopologies::TRIANGLE_LIST;
            PipelineDesc.Pipeline.InputLayout.vVertexAttributes =
            {
                RenderSystem::SPipelineDesc::SInputLayout::SVertexAttribute( "POSITION",
                RenderSystem::Formats::R32G32B32_SFLOAT, 0 )
            };

            auto& DS = PipelineDesc.Pipeline.DepthStencil;
            DS.Depth.compareFunc = RenderSystem::CompareFunctions::GREATER_EQUAL;
            DS.Depth.enable = true;
            DS.Depth.enableTest = true;
            DS.Depth.enableWrite = true;
            auto& Blend = PipelineDesc.Pipeline.Blending;
            Blend.enable = false;
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

        void CTerrainVertexFetchRenderer::Render( RenderSystem::CGraphicsContext* pCtx )
        {
            RenderSystem::CCommandBuffer* pCb = pCtx->GetCommandBuffer();
            const auto& LOD = m_pDrawcall->GetLOD( 0 );
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
                m_BindingTables[1].firstSet = 0;
                m_BindingTables[1].pCmdBuffer = pCb;
                m_BindingTables[1].pPipelineLayout = pPipeline->GetLayout().Get();
                m_BindingTables[1].setCount = 1;
                m_BindingTables[1].type = RenderSystem::PipelineTypes::GRAPHICS;
                pCb->Bind( m_BindingTables[1] );



                pCb->Bind( LOD.hVertexBuffer, LOD.vertexBufferOffset );
                pCb->Bind( LOD.hIndexBuffer, LOD.indexBufferOffset );

                pCb->DrawIndexed( LOD.DrawParams );
            }
        }

    } // Scene
} // VKE