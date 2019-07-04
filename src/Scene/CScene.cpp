#include "Scene/CScene.h"
#include "Scene/CCamera.h"

#include "Scene/COctree.h"
#include "Scene/CBVH.h"
#include "Scene/CQuadTree.h"

#include "RenderSystem/CGraphicsContext.h"
#include "RenderSystem/CDeviceContext.h"
#include "RenderSystem/FrameGraph/CForwardRenderer.h"

#include "Scene/CWorld.h"
#include "Scene/Terrain/CTerrain.h"

namespace VKE
{
    namespace Scene
    {
        Result CScene::_Create( const SSceneDesc& Desc)
        {
            Result ret = VKE_FAIL;
            SOctreeDesc OctDesc;
            SSceneGraphDesc SceneGraphDesc = Desc.SceneGraphDesc;
            RenderSystem::SFrameGraphDesc FrameGraphDesc = Desc.FrameGraphDesc;

            if( SceneGraphDesc.pDesc == nullptr )
            {
                SceneGraphDesc.pDesc = &OctDesc;
                SceneGraphDesc.pName = SCENE_GRAPH_OCTREE_NAME;
            }
            
            if( strcmp( SceneGraphDesc.pName, SCENE_GRAPH_OCTREE_NAME ) == 0 )
            {
                ret = Memory::CreateObject( &HeapAllocator, &m_pOctree, this );
                if( VKE_SUCCEEDED( ret ) )
                {
                    SOctreeDesc* pDesc = reinterpret_cast< SOctreeDesc* >( SceneGraphDesc.pDesc );
                    ret = m_pOctree->_Create( *pDesc );
                }
            }

            RenderSystem::SForwardRendererDesc FwdDesc;
            if( FrameGraphDesc.pDesc == nullptr )
            {
                FrameGraphDesc.pDesc = &FwdDesc;
                FrameGraphDesc.pName = RenderSystem::FRAME_GRAPH_FORWARD_RENDERER_NAME;
            }

            if( strcmp( FrameGraphDesc.pName, RenderSystem::FRAME_GRAPH_FORWARD_RENDERER_NAME ) == 0 )
            {
                m_pFrameGraph = VKE_NEW RenderSystem::CForwardRenderer();
            }
            VKE_ASSERT( m_pFrameGraph != nullptr, "" );
            m_pFrameGraph->SetScene( this );

            m_vDrawLayers.Resize( 31 );
            m_vpVisibleLayerDrawcalls.Resize( 31 );

            m_vpDrawcalls.PushBack( {} );
            for( uint32_t i = 0; i < m_vDrawLayers.GetCount(); ++i )
            {
                m_vDrawLayers[ i ].Add( {} );
            }

            return ret;
        }

        void CScene::_Destroy()
        {
            _DestroyDebugView();
            if( m_pTerrain.IsValid() )
            {
                m_pTerrain->_Destroy();
                auto pTmp = m_pTerrain.Release();
                VKE_DELETE( pTmp );
            }
            if( m_pOctree )
            {
                m_pOctree->_Destroy();
                Memory::DestroyObject( &HeapAllocator, &m_pOctree );
            }
            if( m_pFrameGraph )
            {
                m_pFrameGraph->_Destroy();
                VKE_DELETE( m_pFrameGraph );
            }

            m_vpDrawcalls.Clear();
        }

        RenderSystem::DrawcallPtr CScene::CreateDrawcall( const Scene::SDrawcallDesc& Desc )
        {
            auto pDrawcall = m_pWorld->CreateDrawcall( Desc );

            return pDrawcall;
        }

        TerrainPtr CScene::CreateTerrain( const STerrainDesc& Desc, RenderSystem::CDeviceContext* pCtx )
        {
            if( m_pTerrain.IsValid() )
            {
                DestroyTerrain( &m_pTerrain );
            }
            CTerrain* pTerrain;
            if( VKE_SUCCEEDED( Memory::CreateObject( &HeapAllocator, &pTerrain, this ) ) )
            {
                m_pTerrain = TerrainPtr{ pTerrain };
                if( VKE_FAILED( pTerrain->_Create( Desc, pCtx ) ) )
                {
                    goto ERR;
                }
            }
            else
            {
                VKE_LOG_ERR( "Unable to create memory for CTerrain object." );
                goto ERR;
            }
            _CreateDebugView( pCtx );
            return m_pTerrain;
        ERR:
            if( m_pTerrain.IsValid() )
            {
                DestroyTerrain( &m_pTerrain );
            }
            return m_pTerrain;
        }

        void CScene::DestroyTerrain( TerrainPtr* ppInOut )
        {
            CTerrain* pTerrain = ppInOut->Release();
            pTerrain->_Destroy();
            Memory::DestroyObject( &HeapAllocator, &pTerrain );
        }

        CameraPtr CScene::CreateCamera( cstr_t dbgName )
        {
            const uint32_t hCam = m_vCameras.PushBack( {} );
            CCamera* pCam = &m_vCameras[ hCam ];
 
            return CameraPtr{ pCam };
        }

        handle_t CScene::AddObject( RenderSystem::DrawcallPtr pDrawcall, const SDrawcallDataInfo& Info )
        {
            auto handle2 = m_vpDrawcalls.PushBack( pDrawcall );
            auto handle = m_vDrawLayers[Info.layer].Add( Info );
            VKE_ASSERT( handle == handle2, "" );
            RenderSystem::UObjectHandle Handle;
            RenderSystem::UDrawcallHandle hDrawcall;
            hDrawcall.reserved1 = handle;
            Handle.index = handle;
            Handle.layer = Info.layer;
            
            //auto pBits = &m_DrawData.GetBits( handle );
            auto& AABB = m_vDrawLayers[Info.layer].GetAABB( handle );
            if( Info.canBeCulled )
            {
                COctree::UObjectHandle hNodeObj = m_pOctree->AddObject( AABB, Handle );
                pDrawcall->m_hSceneGraph = hNodeObj.handle;
            }
            else
            {
                m_vpAlwaysVisibleDrawcalls.PushBack( pDrawcall );
            }
            pDrawcall->m_hObj = Handle;
            
            return Handle.handle;
        }

        void CScene::UpdateDrawcallAABB( const handle_t& hDrawcall, const Math::CAABB& NewAABB )
        {
            RenderSystem::UObjectHandle hObj;
            hObj.handle = hDrawcall;

            m_vDrawLayers[hObj.layer].Update( hObj.index, NewAABB );
            if( m_pOctree )
            {
                auto pDrawcall = m_vpDrawcalls[hObj.index];
                const auto& hSceneGraph = pDrawcall->m_hSceneGraph;
                pDrawcall->m_hSceneGraph = m_pOctree->_UpdateObject( hSceneGraph, hObj, NewAABB ).handle;
            }
        }

        void CScene::Render( VKE::RenderSystem::CGraphicsContext* pCtx )
        {
            m_pCurrentCamera->Update(0);
            const Math::CFrustum& Frustum = m_pCurrentCamera->GetFrustum();
            _FrustumCullDrawcalls( Frustum );
            _Draw( pCtx );
        }

        void CScene::_FrustumCullDrawcalls( const Math::CFrustum& Frustum )
        {
            /*const auto& vAABBs = m_DrawData.vAABBs;
            const uint32_t count = vAABBs.GetCount();
            for( uint32_t i = 0; i < count; ++i )
            {
                m_DrawData.vBits[ i ].visible = true;
            }*/
            if( m_pOctree )
            {
                m_pOctree->FrustumCull( Frustum );
            }
 
            for( uint32_t layer = 0; layer < m_vDrawLayers.GetCount(); ++layer )
            {
                auto& Curr = m_vDrawLayers[layer];
                m_vpVisibleLayerDrawcalls[ layer ].Clear();

                for( uint32_t i = 0; i < Curr.vVisibles.GetCount(); ++i )
                {
                    if( Curr.vVisibles[i] )
                    {
                        auto pCurr = m_vpDrawcalls[i];
                        m_vpVisibleLayerDrawcalls[layer].PushBack( pCurr );
                    }
                }
            }
            for( uint32_t i = 0; i < m_vpAlwaysVisibleDrawcalls.GetCount(); ++i )
            {
                m_vpVisibleLayerDrawcalls[0].PushBack( m_vpAlwaysVisibleDrawcalls[ i ] );
            }
        }

        void CScene::_SortDrawcalls( const Math::CFrustum& Frustum )
        {

        }

        void CScene::_Draw( VKE::RenderSystem::CGraphicsContext* pCtx )
        {
            //RenderSystem::CCommandBuffer* pCmdBuffer = pCtx->GetCommandBuffer();
            //static uint32_t c = 0;
            //c++;
            //for( uint32_t i = 0; i < m_DrawData.vVisibles.GetCount(); ++i )
            //{
            //    //const auto& Bits = m_DrawData.vBits[i];
            //    //if( Bits.visible )
            //    if( m_DrawData.vVisibles[ i ] )
            //    {
            //        // Load this drawcall == cache miss
            //        const RenderSystem::DrawcallPtr pDrawcall = m_vpDrawcalls[ i ];
            //        const auto& LOD = pDrawcall->m_vLODs[ pDrawcall->m_currLOD ];

            //        pCmdBuffer->Bind( LOD.hVertexBuffer, LOD.vertexBufferOffset );
            //        pCmdBuffer->Bind( LOD.hIndexBuffer, LOD.indexBufferOffset );
            //        pCmdBuffer->Bind( LOD.hDescSet, LOD.descSetOffset );
            //        /*pCmdBuffer->SetState( LOD.InputLayout );
            //        pCmdBuffer->SetState( *(LOD.ppVertexShader) );
            //        pCmdBuffer->SetState( *(LOD.ppPixelShader) );
            //        pCmdBuffer->SetState( LOD.InputLayout.topology );*/
            //        pCmdBuffer->Bind( LOD.pPipeline );
            //        pCmdBuffer->DrawIndexed( LOD.DrawParams );
            //    }
            //}
            m_pFrameGraph->Render( pCtx );
            if( m_pDebugView )
            {
                m_pDebugView->Render( pCtx );
            }
        }

        handle_t CScene::_CreateSceneNode(const uint32_t idx)
        {
            UObjectHandle Ret;
            Ret.handle = 0;
            return Ret.handle;
        }

        Result CScene::_CreateDebugView(RenderSystem::CDeviceContext* pCtx)
        {
            static const cstr_t spInstancingVS = VKE_TO_STRING
            (
                #version 450 core\n

                layout( set = 0, binding = 0 ) uniform PerFrameConstantBuffer
                {
                    mat4    mtxViewProj;
                };

                struct SPerInstanceConstantBuffer
                {
                    mat4    mtxTransform;
                    vec4    vecColor;
                };

                layout(set = 0, binding = 1 ) readonly buffer PerInstanceConstantBuffer
                {
                    SPerInstanceConstantBuffer[] aBuffers;
                };

                layout( location = 0 ) in vec3 iPosition;

                void main()
                {
                    mat4 mtxMVP = mtxViewProj * aBuffers[gl_InstanceIndex].mtxTransform;
                    gl_Position = mtxMVP * vec4( iPosition, 1.0f );
                }
            );

            static const cstr_t spInstancingPS = VKE_TO_STRING
            (
                #version 450 core\n

                layout(location = 0) out vec4 oColor;

                void main()
                {
                    oColor = vec4(1.0);
                }
            );

            Result ret;
            ret = Memory::CreateObject( &HeapAllocator, &m_pDebugView );
            if( VKE_SUCCEEDED( ret ) )
            {
                RenderSystem::SShaderData VSData, PSData;
                VSData.type = RenderSystem::ShaderTypes::VERTEX;
                VSData.stage = RenderSystem::ShaderCompilationStages::HIGH_LEVEL_TEXT;
                VSData.pCode = (const uint8_t*)spInstancingVS;
                VSData.codeSize = (uint32_t)strlen( spInstancingVS );

                RenderSystem::SCreateShaderDesc VSDesc, PSDesc;
                VSDesc.Create.async = false;
                VSDesc.Shader.Base.pName = "VKE_InstancingDebugViewVS";
                VSDesc.Shader.type = RenderSystem::ShaderTypes::VERTEX;
                VSDesc.Shader.pData = &VSData;
                VSDesc.Shader.SetEntryPoint( "main" );

                auto pVS = pCtx->CreateShader( VSDesc );

                PSData.type = RenderSystem::ShaderTypes::PIXEL;
                PSData.stage = RenderSystem::ShaderCompilationStages::HIGH_LEVEL_TEXT;
                PSData.pCode = (const uint8_t*)spInstancingPS;
                PSData.codeSize = (uint32_t)strlen( spInstancingPS );

                PSDesc.Create.async = true;
                PSDesc.Shader.Base.pName = "VKE_InstancingDebugViewPS";
                PSDesc.Shader.type = PSData.type;
                PSDesc.Shader.pData = &PSData;
                PSDesc.Shader.SetEntryPoint( "main" );

                auto pPS = pCtx->CreateShader( PSDesc );

                while(pVS.IsNull() || pPS.IsNull() ) {}
                while(!pVS->IsReady() || !pPS->IsReady() ) {}

                RenderSystem::SCreateBindingDesc BindingDesc;
                BindingDesc.AddConstantBuffer( 0, RenderSystem::PipelineStages::VERTEX );
                BindingDesc.AddStorageBuffer( 1, RenderSystem::PipelineStages::VERTEX );
                auto hDescSet = pCtx->CreateResourceBindings( BindingDesc );

                if( hDescSet != NULL_HANDLE )
                {
                    RenderSystem::SPipelineLayoutDesc LayoutDesc;
                    LayoutDesc.vDescriptorSetLayouts = { pCtx->GetDescriptorSetLayout( hDescSet ) };
                    RenderSystem::PipelineLayoutPtr pLayout = pCtx->CreatePipelineLayout( LayoutDesc );

                    RenderSystem::SPipelineDesc::SInputLayout::SVertexAttribute VA;
                    VA.pName = "POSITION";
                    VA.format = RenderSystem::Formats::R32G32B32_SFLOAT;
                    VA.vertexBufferBindingIndex = 0;
                    VA.stride = 3 * 4;

                    auto& PipelineDesc = m_pDebugView->InstancingPipelineTemplate.Pipeline;
                    PipelineDesc.hLayout = pLayout->GetHandle();
                    PipelineDesc.InputLayout.topology = RenderSystem::PrimitiveTopologies::LINE_LIST;
                    PipelineDesc.InputLayout.vVertexAttributes =
                    {
                        { "POSITION", RenderSystem::Formats::R32G32B32_SFLOAT, 0u }
                    };
                    PipelineDesc.Shaders.apShaders[RenderSystem::ShaderTypes::VERTEX] = pVS;
                    PipelineDesc.Shaders.apShaders[RenderSystem::ShaderTypes::PIXEL] = pPS;
                    VKE_RENDER_SYSTEM_SET_DEBUG_NAME( PipelineDesc, "DebugView" );
                }
                else
                {
                    ret = VKE_FAIL;
                }
            }
            return ret;
        }

        void CScene::_DestroyDebugView()
        {
            Memory::DestroyObject( &HeapAllocator, &m_pDebugView );
        }

        void CScene::_RenderDebugView(RenderSystem::CGraphicsContext* pCtx)
        {
            VKE_ASSERT( m_pDebugView != nullptr, "" );
            m_pDebugView->Render( pCtx );
        }

    } // Scene
    // Debug view
    namespace Scene
    {
        void CScene::SDebugView::Render( RenderSystem::CGraphicsContext* pCtx )
        {
            RenderSystem::CCommandBuffer* pCmdBuff = pCtx->GetCommandBuffer();
            auto hDDICurrPass = pCmdBuff->GetCurrentDDIRenderPass();
            // AABB
            auto& Curr = aInstancings[InstancingTypes::AABB];
            if( Curr.pPipeline.IsNull() || Curr.hDDIRenderPass != hDDICurrPass )
            {
                InstancingPipelineTemplate.Create.async = true;
                InstancingPipelineTemplate.Pipeline.hDDIRenderPass = hDDICurrPass;
                Curr.hDDIRenderPass = hDDICurrPass;

                Curr.pPipeline = pCtx->GetDeviceContext()->CreatePipeline( InstancingPipelineTemplate );
            }

            if( Curr.pPipeline.IsValid() && Curr.pPipeline->IsReady() )
            {

            }
        }
    } // Scene
} // VKE
