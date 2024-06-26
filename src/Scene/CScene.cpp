#include "Scene/CScene.h"
#include "Scene/CCamera.h"

#include "Scene/COctree.h"
#include "Scene/CBVH.h"
#include "Scene/CQuadTree.h"

#include "RenderSystem/CGraphicsContext.h"
#include "RenderSystem/CDeviceContext.h"
#include "RenderSystem/CCommandBuffer.h"
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
            RenderSystem::SFrameGraphDesc2 FrameGraphDesc = Desc.FrameGraphDesc;

            VKE_ASSERT2( Desc.pCommandBuffer.IsValid(), "DeviceContext must be set." );
            auto pCtx = Desc.pCommandBuffer->GetContext();
            m_pDeviceCtx = pCtx->GetDeviceContext();

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
            VKE_ASSERT2( m_pFrameGraph != nullptr, "" );
            m_pFrameGraph->SetScene( this );

            m_vDrawLayers.Resize( 31 );
            m_vpVisibleLayerDrawcalls.Resize( 31 );

            _CreateDebugView( Desc.pCommandBuffer );

            m_vpDrawcalls.PushBack( {} );
            for( uint32_t i = 0; i < m_vDrawLayers.GetCount(); ++i )
            {
                m_vDrawLayers[ i ].Add( {} );
            }

            ret = _CreateConstantBuffers();

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

            _DestroyLights();
        }

        void CScene::_DestroyLights()
        {
            for( uint32_t i = 0; i < LightTypes::_MAX_COUNT; ++i )
            {
                SLights& Lights = m_Lights[ i ];
                for(uint32_t l = 0 ; l < Lights.vpLights.GetCount(); ++l)
                {
                    CLight* pLight = Lights.vpLights[ l ].Release();
                    Memory::DestroyObject( &HeapAllocator, &pLight );
                }
                Lights.vpLights.Clear();
                Lights.vFreeIndices.Clear();
                Lights.vColors.Clear();
                Lights.vDbgViews.Clear();
                Lights.vDirections.Clear();
                Lights.vEnableds.clear();
                Lights.vNeedUpdates.clear();
                Lights.vPositions.Clear();
                Lights.vRadiuses.Clear();
                Lights.vSortedLightData.Clear();
                Lights.vStrengths.Clear();
            }
        }

        Result CScene::_CreateConstantBuffers()
        {
            Result ret = VKE_FAIL;
            {
                RenderSystem::SCreateBufferDesc BuffDesc;
                BuffDesc.Create.flags = Core::CreateResourceFlags::DEFAULT;
                BuffDesc.Buffer.size = sizeof( SConstantBuffer );
                BuffDesc.Buffer.usage = RenderSystem::BufferUsages::CONSTANT_BUFFER;
                BuffDesc.Buffer.memoryUsage = RenderSystem::MemoryUsages::STATIC_BUFFER;
                BuffDesc.Buffer.SetDebugName( "VKE_Scene_ConstantBuffer" );
                auto hCB = m_pDeviceCtx->CreateBuffer( BuffDesc );
                if( hCB != INVALID_HANDLE )
                {
                    m_pConstantBufferGPU = m_pDeviceCtx->GetBuffer( hCB );
                }
            }
            auto swapchainElCount = m_pDeviceCtx->GetGraphicsContext( 0 )->GetSwapChain()->GetBackBufferCount();
            {
                RenderSystem::SCreateBufferDesc BuffDesc;
                BuffDesc.Buffer.size = 0;
                BuffDesc.Buffer.vRegions.Resize( swapchainElCount + 1, RenderSystem::SBufferRegion(1, sizeof(SConstantBuffer)) );
                BuffDesc.Buffer.memoryUsage = RenderSystem::MemoryUsages::STAGING_BUFFER;
                BuffDesc.Buffer.usage = RenderSystem::BufferUsages::UPLOAD;
                BuffDesc.Buffer.SetDebugName( "VKE_Scene_StagingConstantBuffer" );
                auto hBuffer = m_pDeviceCtx->CreateBuffer( BuffDesc );
                if( hBuffer != INVALID_HANDLE )
                {
                    m_pConstantBufferCPU = m_pDeviceCtx->GetBuffer( hBuffer );
                }
            }
            if( m_pConstantBufferCPU.IsValid() && m_pConstantBufferGPU.IsValid() )
            {
                ret = VKE_OK;
            }
            if( VKE_SUCCEEDED( ret ) )
            {
                VKE_ASSERT2( Config::RenderSystem::SwapChain::MAX_BACK_BUFFER_COUNT >= swapchainElCount, "" );
                RenderSystem::SCreateBindingDesc BindingDesc;
                BindingDesc.SetDebugName( "VKE_Scene_ConstantBuffer" );
                BindingDesc.LayoutDesc.SetDebugName( BindingDesc.GetDebugName() );
                BindingDesc.AddConstantBuffer( 0, RenderSystem::PipelineStages::ALL );
                uint32_t cbSize = m_pConstantBufferGPU->GetSize();
                for (uint32_t i = 0; i < swapchainElCount + 1; ++i)
                {
                    m_ahBindings[i] = m_pDeviceCtx->CreateResourceBindings( BindingDesc );
                    if( m_ahBindings[ i ] != INVALID_HANDLE )
                    {
                        RenderSystem::SUpdateBindingsHelper UpdateInfo;
                        UpdateInfo.AddBinding( 0u, 0u, cbSize, m_pConstantBufferGPU->GetHandle(),
                                               RenderSystem::BindingTypes::DYNAMIC_CONSTANT_BUFFER );
                        m_pDeviceCtx->UpdateDescriptorSet( UpdateInfo, &m_ahBindings[i] );
                    }
                    else
                    {
                        ret = VKE_FAIL;
                        break;
                    }
                }
            }
            if( VKE_SUCCEEDED( ret ) )
            {
                m_hCurrentBindings = m_ahBindings[ 0 ];
            }
            return ret;
        }

        void CScene::_UpdateConstantBuffers(RenderSystem::CommandBufferPtr pCmdBuffer)
        {
            auto backBufferIndex = pCmdBuffer->GetBackBufferIndex();
            //const auto& LightData = GetLight( LightTypes::DIRECTIONAL, 0 )->GetData();
            const auto& LightDesc = GetLight( LightTypes::DIRECTIONAL, 0 )->GetDesc();

            void* pData = m_pConstantBufferCPU->MapRegion( backBufferIndex, 0 );
            RenderSystem::SBufferWriter Builder( pData, m_pConstantBufferCPU->GetRegionSize(backBufferIndex) );
            Builder.Write(
                m_pViewCamera->GetViewProjectionMatrix(),
                m_pViewCamera->GetPosition(),
                0.0f, // pad
                m_pViewCamera->GetDirection(),
                0.0f, // pad
                LightDesc.vecPosition,
                LightDesc.radius,
                LightDesc.vecDirection,
                LightDesc.attenuation,
                LightDesc.Color);

            m_pConstantBufferCPU->Unmap();
            RenderSystem::SCopyBufferInfo CopyInfo;
            CopyInfo.hDDISrcBuffer = m_pConstantBufferCPU->GetDDIObject();
            CopyInfo.hDDIDstBuffer = m_pConstantBufferGPU->GetDDIObject();
            CopyInfo.Region.dstBufferOffset = 0;
            CopyInfo.Region.srcBufferOffset = m_pConstantBufferCPU->CalcAbsoluteOffset( backBufferIndex, 0 );
            CopyInfo.Region.size = Builder.GetWrittenSize();
            auto s = sizeof( SConstantBuffer );
            VKE_ASSERT2( CopyInfo.Region.size == s, "" );
            pCmdBuffer->Copy( CopyInfo );
            m_hCurrentBindings = m_ahBindings[ backBufferIndex ];
        }

        RenderSystem::DrawcallPtr CScene::CreateDrawcall( const Scene::SDrawcallDesc& Desc )
        {
            auto pDrawcall = m_pWorld->CreateDrawcall( Desc );

            return pDrawcall;
        }

        TerrainPtr CScene::CreateTerrain( const STerrainDesc& Desc, RenderSystem::CommandBufferPtr pCmdBuff )
        {
            if( m_pTerrain.IsValid() )
            {
                DestroyTerrain( &m_pTerrain );
            }
            CTerrain* pTerrain;
            if( VKE_SUCCEEDED( Memory::CreateObject( &HeapAllocator, &pTerrain, this ) ) )
            {
                m_pTerrain = TerrainPtr{ pTerrain };
                if( VKE_FAILED( pTerrain->_Create( Desc, pCmdBuff ) ) )
                {
                    goto ERR;
                }
            }
            else
            {
                VKE_LOG_ERR( "Unable to create memory for CTerrain object." );
                goto ERR;
            }

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

        CameraPtr CScene::CreateCamera( const SCameraDesc& Desc )
        {
            const uint32_t hCam = m_vCameras.PushBack( {} );
            CCamera* pCam = &m_vCameras[ hCam ];
            pCam->_Init( Desc );

            return CameraPtr{ pCam };
        }

        handle_t CScene::AddObject( RenderSystem::CommandBufferPtr pCmdbuff, RenderSystem::DrawcallPtr pDrawcall,
            const SDrawcallDataInfo& Info )
        {
            const auto handle2 = m_vpDrawcalls.PushBack( pDrawcall );
            const auto handle = m_vDrawLayers[Info.layer].Add( Info );
            VKE_ASSERT2( handle == handle2, "" );
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

            if( Info.debugView )
            {
                pDrawcall->m_hDbgView = m_pDebugView->AddBatchData( pCmdbuff, SDebugView::BatchTypes::AABB );
            }

            return Handle.handle;
        }

        void CScene::UpdateDrawcallAABB( RenderSystem::CommandBufferPtr pCmdBuffer, const handle_t& hDrawcall,
                                         const Math::CAABB& NewAABB )
        {
            RenderSystem::UObjectHandle hObj;
            hObj.handle = hDrawcall;

            m_vDrawLayers[hObj.layer].Update( hObj.index, NewAABB );
            auto pDrawcall = m_vpDrawcalls[hObj.index];

            if( m_pOctree )
            {
                const auto& hSceneGraph = pDrawcall->m_hSceneGraph;
                pDrawcall->m_hSceneGraph = m_pOctree->_UpdateObject( hSceneGraph, hObj, NewAABB ).handle;
            }
            if( m_pDebugView )
            {
                const auto& hDbgView = pDrawcall->m_hDbgView;
                /*Math::CMatrix4x4 mtxTransform;
                mtxTransform.Transform( Math::CVector4( NewAABB.Extents * 2 ), Math::CVector4::ZERO,
                    Math::CQuaternion::UNIT, Math::CVector4( NewAABB.Center ) );
                m_pDebugView->UpdateInstancing( SDebugView::INSTANCING_TYPE::AABB, hDbgView, mtxTransform );*/
                m_pDebugView->UpdateBatchData( pCmdBuffer, SDebugView::BatchTypes::AABB, hDbgView, NewAABB );
            }
        }

        void CScene::AddDebugView( RenderSystem::CommandBufferPtr pCmdBuff, CameraPtr* pCamera )
        {
            if( m_pDebugView )
            {
                CameraPtr pTmp = *pCamera;
                //pTmp->m_hDbgView = m_pDebugView->AddBatchData( m_pDeviceCtx, SDebugView::BatchTypes::AABB );
                pTmp->m_hDbgView = m_pDebugView->AddInstancing( pCmdBuff, SDebugView::InstancingTypes::AABB );
            }
        }

        void CScene::AddDebugView( RenderSystem::CommandBufferPtr pCmdBuff, LightPtr* ppLight )
        {
            if( m_pDebugView )
            {
                auto hView = m_pDebugView->AddInstancing( pCmdBuff, SDebugView::InstancingTypes::AABB );
                ( *ppLight )->_SetDebugView( hView );
            }
        }

        void CScene::_UpdateDebugViews( RenderSystem::CommandBufferPtr pCmdbuff )
        {
            //Math::CVector3 aCorners[8];
            for( uint32_t i = 0; i < m_vCameras.GetCount(); ++i )
            {
                auto& Curr = m_vCameras[i];
                if( Curr.m_hDbgView != UNDEFINED_U32 )
                {
                    SDebugView::SInstancingShaderData Data;
                    Curr.GetFrustum().CalcMatrix( &Data.mtxTransform );
                    Data.vecColor = { 1, 0.8f, 0.7f, 1 };
                    m_pDebugView->UpdateInstancing( pCmdbuff, SDebugView::InstancingTypes::AABB, Curr.m_hDbgView, Data );
                }
            }
            
            for( uint32_t lt = 0; lt < LightTypes::_MAX_COUNT; ++lt )
            {
                const auto& Lights = m_Lights[ lt ];
                for( uint32_t l = 0; l < Lights.vDbgViews.GetCount(); ++l )
                {
                    const auto hDbgView = Lights.vDbgViews[ l ];
                    if( Lights.vDbgViews[ l ] != UNDEFINED_U32 )
                    {
                        SDebugView::SInstancingShaderData Data;
                        const auto& pLight = Lights.vpLights[ l ];
                        pLight->CalcMatrix( &Data.mtxTransform );
                        Data.vecColor = { 0, 1, 0, 1 };
                        m_pDebugView->UpdateInstancing( pCmdbuff, SDebugView::InstancingTypes::AABB, hDbgView, Data );
                    }
                }
            }
        }

        void CScene::SetCamera( CameraPtr pCam )
        {
            m_pCurrentCamera = pCam;
            if( m_pViewCamera == nullptr )
            {
                m_pViewCamera = pCam;
            }
        }

        void CScene::Update( const SUpdateSceneInfo& Info )
        {
            VKE_ASSERT2( Info.pCommandBuffer.IsValid(), "Command buffer must be a valid pointer." );
            m_pCurrentCamera->Update( 0 );
            if( m_pCurrentCamera != m_pViewCamera )
            {
                m_pViewCamera->Update( 0 );
            }
            _UpdateConstantBuffers( Info.pCommandBuffer );
            const Math::CFrustum& Frustum = m_pCurrentCamera->GetFrustum();
            _FrustumCullDrawcalls( Frustum );

            m_pTerrain->Update( Info.pCommandBuffer );
        }

        void CScene::Render( VKE::RenderSystem::CommandBufferPtr pCmdBuff )
        {
            _Draw( pCmdBuff );
        }

        void CScene::RenderDebug( RenderSystem::CommandBufferPtr pCmdBuffer )
        {
            if( m_pDebugView )
            {
                m_pDebugView->Render( pCmdBuffer );
            }
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

        void CScene::_Draw( VKE::RenderSystem::CommandBufferPtr pCmdBuff )
        {
            if( m_pDebugView )
            {
                m_pDebugView->UploadInstancingConstantData( pCmdBuff, GetViewCamera() );
                m_pDebugView->UploadBatchData( pCmdBuff, GetViewCamera() );
                _UpdateDebugViews( pCmdBuff );
            }
            m_pFrameGraph->Render( pCmdBuff );
            if( m_pDebugView )
            {
                m_pDebugView->Render( pCmdBuff );
            }
        }

        handle_t CScene::_CreateSceneNode(const uint32_t idx)
        {
            UObjectHandle Ret;
            Ret.handle = 0;
            return Ret.handle;
        }

        Result CScene::_CreateDebugView(RenderSystem::CommandBufferPtr pCmdBuff)
        {
            static const cstr_t spGLSLInstancingVS = VKE_TO_STRING
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
                layout( location = 0 ) out vec4 oColor;

                void main()
                {
                    mat4 mtxMVP = mtxViewProj * aBuffers[gl_InstanceIndex].mtxTransform;
                    gl_Position = mtxMVP * vec4( iPosition, 1.0f );
                    oColor = aBuffers[gl_InstanceIndex].vecColor;
                }
            );

            static cstr_t spHLSLInstancingVS = VKE_TO_STRING
            (
                struct SPerFrameConstants
                {
                    float4x4 mtxViewProj;
                };
                ConstantBuffer<SPerFrameConstants> PerFrameConstants : register(b0, space0);

                struct SInstanceData
                {
                    float4x4    mtxTransform;
                    float4      f4Color;
                };
                StructuredBuffer<SInstanceData> InstanceData : register(t1, space0);

                struct SIn
                {
                    float3  f3Position : SV_POSITION;
                    uint    instanceID : SV_InstanceID;
                };
                struct SOut
                {
                    float4 f4Position : SV_POSITION;
                    float4 f4Color : COLOR0;
                };
                void main(in SIn IN, out SOut OUT)
                {
                    SInstanceData Data = InstanceData[IN.instanceID];
                    //SInstanceData Data = InstanceData[0];
                    float4x4 mtxMVP = mul( PerFrameConstants.mtxViewProj, Data.mtxTransform );
                    OUT.f4Position = mul( mtxMVP, float4( IN.f3Position, 1 ) );
                    OUT.f4Color = Data.f4Color;
                }
            );

            cstr_t pInstancingVS = VKE_USE_HLSL_SYNTAX ? spHLSLInstancingVS : spGLSLInstancingVS;

            static const cstr_t spGLSLInstancingPS = VKE_TO_STRING
            (
                #version 450 core\n

                layout( location = 0 ) in vec4 iColor;
                layout(location = 0) out vec4 oColor;

                void main()
                {
                    oColor = iColor;
                }
            );

            static cstr_t spHLSLInstancingPS = VKE_TO_STRING
            (
                float4 main(in float4 color : COLOR0) : SV_TARGET0 { return color; }
            );

            cstr_t pGLSLInstancingPS = VKE_USE_HLSL_SYNTAX ? spHLSLInstancingPS : spGLSLInstancingPS;

            Result ret;
            ret = Memory::CreateObject( &HeapAllocator, &m_pDebugView );
            if( VKE_SUCCEEDED( ret ) )
            {
                m_pDebugView->pDeviceCtx = m_pDeviceCtx;

                RenderSystem::SShaderData VSData, PSData;
                VSData.type = RenderSystem::ShaderTypes::VERTEX;
                VSData.stage = RenderSystem::ShaderCompilationStages::HIGH_LEVEL_TEXT;
                VSData.pCode = (const uint8_t*)pInstancingVS;
                VSData.codeSize = (uint32_t)strlen(pInstancingVS);

                RenderSystem::SCreateShaderDesc VSDesc, PSDesc;
                VSDesc.Create.flags = Core::CreateResourceFlags::DEFAULT;
                //VSDesc.Shader.FileInfo.pName = "VKE_InstancingDebugViewVS";
                VSDesc.Shader.type = RenderSystem::ShaderTypes::VERTEX;
                VSDesc.Shader.pData = &VSData;
                VSDesc.Shader.EntryPoint = "main";
                VSDesc.Shader.Name = "VKE_InstancingDebugViewVS";

                auto pVS = m_pDeviceCtx->CreateShader( VSDesc );

                PSData.type = RenderSystem::ShaderTypes::PIXEL;
                PSData.stage = RenderSystem::ShaderCompilationStages::HIGH_LEVEL_TEXT;
                PSData.pCode = (const uint8_t*)pGLSLInstancingPS;
                PSData.codeSize = (uint32_t)strlen(pGLSLInstancingPS);

                PSDesc.Create.flags = Core::CreateResourceFlags::DEFAULT;
                //PSDesc.Shader.FileInfo.pName = "VKE_InstancingDebugViewPS";
                PSDesc.Shader.type = PSData.type;
                PSDesc.Shader.pData = &PSData;
                PSDesc.Shader.EntryPoint = "main";
                PSDesc.Shader.Name = "VKE_InstancingDebugViewPS";

                auto pPS = m_pDeviceCtx->CreateShader( PSDesc );

                while(pVS.IsNull() || pPS.IsNull() ) {}
                while( !pVS->IsResourceReady() || !pPS->IsResourceReady() )
                {
                }

                RenderSystem::SPipelineDesc::SInputLayout::SVertexAttribute VA;
                VA.pName = "POSITION";
                VA.format = RenderSystem::Formats::R32G32B32_SFLOAT;
                VA.vertexBufferBindingIndex = 0;
                VA.stride = 3 * 4;

                auto& PipelineDesc = m_pDebugView->InstancingPipelineTemplate.Pipeline;
                PipelineDesc.InputLayout.topology = RenderSystem::PrimitiveTopologies::LINE_LIST;
                PipelineDesc.InputLayout.vVertexAttributes =
                {
                    { "POSITION", RenderSystem::Formats::R32G32B32_SFLOAT, 0u }
                };
                PipelineDesc.Shaders.apShaders[RenderSystem::ShaderTypes::VERTEX] = pVS;
                PipelineDesc.Shaders.apShaders[RenderSystem::ShaderTypes::PIXEL] = pPS;
                //VKE_RENDER_SYSTEM_SET_DEBUG_NAME( PipelineDesc, "DebugView" );
                PipelineDesc.SetDebugName( "DebugView" );
                {
                    PipelineDesc.DepthStencil.Depth.write = false;
                    PipelineDesc.DepthStencil.Depth.test = false;
                    PipelineDesc.DepthStencil.Depth.compareFunc = RenderSystem::CompareFunctions::NEVER;
                }

                // Create vertex buffer for AABB
                const Math::CVector3 aVertices[ 8 ] =
                {
                    // AABB
                    // Front side
                    { 0.0f, 1.0f, 0.0f },   { 1.0f, 1.0f, 0.0f },
                    { 0.0f, 0.0f, 0.0f },   { 1.0f, 0.0f, 0.0f },

                    // Back side
                    { 0.0f, 1.0f, 1.0f },   { 1.0f, 1.0f, 1.0f },
                    { 0.0f, 0.0f, 1.0f },   { 1.0f, 0.0f, 1.0f },
                };

                const uint16_t aIndices[24] =
                {
                    // AABB
                    // Front size
                    0, 1, // top left - top right
                    1, 3, // top right - bottom right
                    3, 2, // bottom right - bottom left
                    2, 0, // bottom left - top left
                    // Back side
                    4, 5,
                    5, 7,
                    7, 6,
                    6, 4,
                    // Top side
                    0, 4,
                    1, 5,
                    // bottom side
                    2, 6,
                    3, 7
                };

                RenderSystem::SCreateBufferDesc BuffDesc;
                BuffDesc.Create.flags = Core::CreateResourceFlags::DEFAULT;
                BuffDesc.Buffer.memoryUsage = RenderSystem::MemoryUsages::GPU_ACCESS | RenderSystem::MemoryUsages::BUFFER;
                BuffDesc.Buffer.usage = RenderSystem::BufferUsages::VERTEX_BUFFER;
                BuffDesc.Buffer.size = sizeof( aVertices );
                BuffDesc.Buffer.SetDebugName( "VKE_Scene_DebugView" );
                auto hVB = m_pDeviceCtx->CreateBuffer( BuffDesc );
                VKE_ASSERT2( hVB != INVALID_HANDLE, "" );
                BuffDesc.Buffer.usage = RenderSystem::BufferUsages::INDEX_BUFFER;
                BuffDesc.Buffer.size = sizeof( aIndices );
                BuffDesc.Buffer.indexType = RenderSystem::IndexTypes::UINT16;
                BuffDesc.Buffer.SetDebugName( "VKE_Scene_DebugView" );
                auto hIB = m_pDeviceCtx->CreateBuffer( BuffDesc );
                VKE_ASSERT2( hIB != INVALID_HANDLE, "" );
                m_pDebugView->hInstancingVB = HandleCast< RenderSystem::VertexBufferHandle >( hVB );
                m_pDebugView->hInstancingIB = HandleCast< RenderSystem::IndexBufferHandle >(hIB);

                auto& AABB = m_pDebugView->aInstancings[ SDebugView::InstancingTypes::AABB ];
                AABB.DrawData.DrawParams.Indexed.indexCount = 24;
                AABB.DrawData.DrawParams.Indexed.startIndex = 0;
                AABB.DrawData.DrawParams.Indexed.vertexOffset = 0;

                auto pCtx = pCmdBuff->GetContext();
                RenderSystem::SUpdateMemoryInfo UpdateMemInfo;
                UpdateMemInfo.dataSize = sizeof( aVertices );
                UpdateMemInfo.pData = ( const void* )aVertices;
                pCtx->UpdateBuffer( pCmdBuff, UpdateMemInfo, &hVB );
                UpdateMemInfo.dataSize = sizeof( aIndices );
                UpdateMemInfo.pData = ( const void* )aIndices;
                pCtx->UpdateBuffer( pCmdBuff, UpdateMemInfo, &hIB );
            }
            return ret;
        }

        void CScene::_DestroyDebugView()
        {
            Memory::DestroyObject( &HeapAllocator, &m_pDebugView );
        }

        void CScene::_RenderDebugView(RenderSystem::CommandBufferPtr pCmdBuff)
        {
            VKE_ASSERT2( m_pDebugView != nullptr, "" );
            m_pDebugView->Render( pCmdBuff );
        }

        LightRefPtr CScene::CreateLight( const SLightDesc& Desc )
        {
            // Get free
            uint32_t idx;
            LightRefPtr pRet;
            SLights& Lights = m_Lights[ Desc.type ];
            
            if( Lights.vFreeIndices.PopBack( &idx ) )
            {
                pRet = Lights.vpLights[ idx ];
                Lights.vColors[ idx ] = Desc.Color;
                Lights.vDirections[idx] = Desc.vecDirection;
                Lights.vNeedUpdates[ idx ] = true;
                Lights.vPositions[ idx ] = Desc.vecPosition;
                Lights.vRadiuses[ idx ] = Desc.radius;
                Lights.vStrengths[ idx ] = Desc.attenuation;
                Lights.vDbgViews[ idx ] = UNDEFINED_U32;
                pRet->_Create( Desc, &Lights, idx );
            }
            else
            {
                CLight* pLight;
                if( VKE_SUCCEEDED( Memory::CreateObject( &HeapAllocator, &pLight ) ) )
                {
                    idx = Lights.vpLights.PushBack( LightRefPtr{ pLight } );
                    pLight->m_index = idx;
                    pLight->_Create( Desc, &Lights, idx );
                    Lights.vColors.PushBack( Desc.Color );
                    Lights.vDirections.PushBack( Desc.vecDirection );
                    Lights.vPositions.PushBack( Desc.vecPosition );
                    Lights.vRadiuses.PushBack( Desc.radius );
                    Lights.vStrengths.PushBack( Desc.attenuation );
                    Lights.vNeedUpdates.push_back( true );
                    Lights.vDbgViews.PushBack( UNDEFINED_U32 );
                    pRet = Lights.vpLights[ idx ];
                }
            }
            return pRet;
        }

        void CScene::_SortLights(LIGHT_TYPE type)
        {
            SLights& Lights = m_Lights[ type ];
            Lights.vSortedLightData.Clear();

            for( uint32_t i = 0; i < Lights.vEnableds.size(); ++i )
            {
                if( Lights.vEnableds[ i ] )
                {
                    CLight* pLight = Lights.vpLights[ i ].Get();
                    const SLightDesc& Desc = pLight->GetDesc();
                    auto idx = Lights.vSortedLightData.PushBack( {} );
                    auto& Data = Lights.vSortedLightData[ idx ];
                    Data.radius = Desc.radius;
                    Data.attenuation = Desc.attenuation;
                    Data.vecColor = { Desc.Color.r, Desc.Color.g, Desc.Color.b };
                    Data.vecDir = Desc.vecDirection;
                    Data.vecPos = Desc.vecPosition;
                }
            }
        }

        void CScene::_SortLights()
        {
            for( int i = 0; i < LightTypes::_MAX_COUNT; ++i )
            {
                _SortLights( (LIGHT_TYPE)i );
            }
        }

    } // Scene
    // Debug view
    namespace Scene
    {
        bool CScene::SDebugView::CreateConstantBuffer( RenderSystem::CDeviceContext* pCtx,
                                                       uint32_t elementCount, SConstantBuffer* pOut )
        {
            bool ret = false;
            // Create per frame constant buffer
            if( pPerFrameConstantBuffer.IsNull() )
            {
                RenderSystem::SCreateBufferDesc BuffDesc;
                BuffDesc.Create.flags = Core::CreateResourceFlags::DEFAULT;
                BuffDesc.Buffer.memoryUsage = RenderSystem::MemoryUsages::STATIC | RenderSystem::MemoryUsages::BUFFER;
                BuffDesc.Buffer.size = sizeof( SPerFrameShaderData );
                BuffDesc.Buffer.usage = RenderSystem::BufferUsages::CONSTANT_BUFFER;
                BuffDesc.Buffer.SetDebugName( "VKE_Scene_DebugView" );
                RenderSystem::BufferHandle hPerFrameConstantBuffer = pCtx->CreateBuffer( BuffDesc );
                pPerFrameConstantBuffer = pCtx->GetBuffer( hPerFrameConstantBuffer );
            }

            // Create instancing buffer
            RenderSystem::SCreateBufferDesc BuffDesc;
            BuffDesc.Create.flags = Core::CreateResourceFlags::DEFAULT;
            BuffDesc.Buffer.memoryUsage = RenderSystem::MemoryUsages::STATIC | RenderSystem::MemoryUsages::BUFFER;
            BuffDesc.Buffer.size = 0; // sizeof( SInstancingShaderData ) * MAX_INSTANCING_DATA_PER_BUFFER;
            BuffDesc.Buffer.usage = RenderSystem::BufferUsages::STORAGE_BUFFER;
            BuffDesc.Buffer.vRegions =
            {
                RenderSystem::SBufferRegion(MAX_INSTANCING_DATA_PER_BUFFER, sizeof( SInstancingShaderData ) )
            };
            BuffDesc.Buffer.SetDebugName( "VKE_Scene_DebugView" );
            RenderSystem::BufferHandle hInstanceDataBuffer = pCtx->CreateBuffer( BuffDesc );

            if( pPerFrameConstantBuffer.IsValid() && hInstanceDataBuffer != INVALID_HANDLE )
            {
                //auto pCBuffer = pCtx->GetBuffer( hPerFrameConstantBuffer );
                auto pSBuffer = pCtx->GetBuffer( hInstanceDataBuffer );

                if( pOut->vData.Resize( pSBuffer->GetSize() ) )
                {
                    pOut->pConstantBuffer = pPerFrameConstantBuffer;
                    pOut->pStorageBuffer = pSBuffer;
                    if( this->hPerFrameDescSet == INVALID_HANDLE )
                    {
                        RenderSystem::SCreateBindingDesc BindingDesc;
                        BindingDesc.AddConstantBuffer( 0, RenderSystem::PipelineStages::VERTEX );
                        BindingDesc.AddStorageBuffer( 1, RenderSystem::PipelineStages::VERTEX, 1u );
                        BindingDesc.LayoutDesc.SetDebugName( "VKE_Scene_DebugView" );
                        BindingDesc.SetDebugName( "VKE_Scene_DebugView" );
                        this->hPerFrameDescSet = pCtx->CreateResourceBindings( BindingDesc );
                        if( this->hPerFrameDescSet != INVALID_HANDLE )
                        {
                            pOut->hDescSet = this->hPerFrameDescSet;
                            RenderSystem::SUpdateBindingsHelper Update;
                            Update.AddBinding( 0u, 0, pPerFrameConstantBuffer->GetSize(), pPerFrameConstantBuffer->GetHandle(), RenderSystem::BindingTypes::DYNAMIC_CONSTANT_BUFFER );
                            Update.AddBinding( 1u, 0, pSBuffer->GetSize(), hInstanceDataBuffer, RenderSystem::BindingTypes::DYNAMIC_STORAGE_BUFFER );
                            pCtx->UpdateDescriptorSet( Update, &this->hPerFrameDescSet );
                            ret = true;
                        }
                        else
                        {
                            pCtx->DestroyBuffer( &pSBuffer );
                        }
                    }
                }
                else
                {
                    pCtx->DestroyBuffer( &pSBuffer );
                }
            }

            if( this->InstancingPipelineTemplate.Pipeline.hLayout == INVALID_HANDLE )
            {
                RenderSystem::SPipelineLayoutDesc LayoutDesc;
                LayoutDesc.vDescriptorSetLayouts = { pCtx->GetDescriptorSetLayout( hPerFrameDescSet ) };
                auto pLayout = pCtx->CreatePipelineLayout( LayoutDesc );
                auto& Pipeline = this->InstancingPipelineTemplate.Pipeline;
                Pipeline.hLayout = pLayout->GetHandle();
            }

            return ret;
        }

        uint32_t CScene::SDebugView::AddInstancing( RenderSystem::CommandBufferPtr pCmdBuff, INSTANCING_TYPE type )
        {
            uint32_t ret = UNDEFINED_U32;
            auto& Curr = aInstancings[type];

            uint32_t defaultCount = 0;
            switch( type )
            {
                case InstancingTypes::AABB: defaultCount = Config::Scene::Debug::DEFAULT_AABB_VIEW_COUNT; break;
                case InstancingTypes::SPHERE: defaultCount = Config::Scene::Debug::DEFAULT_SPHERE_VIEW_COUNT; break;
                case InstancingTypes::FRUSTUM: defaultCount = Config::Scene::Debug::DEFAULT_FRUSTUM_VIEW_COUNT; break;
            }

            auto pCtx = pCmdBuff->GetContext();
            auto pDevice = pCtx->GetDeviceContext();

            if( Curr.vConstantBuffers.IsEmpty() )
            {
                SConstantBuffer CB;
                if( CreateConstantBuffer( pDevice, defaultCount, &CB ) )
                {
                    Curr.vConstantBuffers.PushBack( CB );
                }
            }
            if( !Curr.vConstantBuffers.IsEmpty() )
            {
                // Create new constant buffer if there is no space left in current one
                {
                    auto& CB = Curr.vConstantBuffers.Back();
                    const uint32_t offset = (CB.drawCount + 1) * sizeof( SInstancingShaderData );
                    if( offset >= CB.vData.GetCount() )
                    {
                        VKE_ASSERT2( Curr.vConstantBuffers.GetCount() + 1 < 0xF, "Reached max number of constant buffers for debug view objects." );
                        SConstantBuffer TmpCB;
                        if( CreateConstantBuffer( pDevice, defaultCount, &TmpCB ) )
                        {
                            Curr.vConstantBuffers.PushBack( TmpCB );
                        }
                    }
                }
                auto& CB = Curr.vConstantBuffers.Back();
                ret = CB.drawCount++;
                VKE_ASSERT2( CB.drawCount < MAX_INSTANCING_DRAW_COUNT, "Reached max number of debug view drawcalls." );

                if( CreateDrawcallData( pDevice, type ) )
                {
                    UInstancingHandle Handle;
                    Handle.bufferIndex = Curr.vConstantBuffers.GetCount() - 1;
                    Handle.index = ret;
                    ret = Handle.handle;
                }

            }
            return ret;
        }

        uint32_t CScene::SDebugView::AddDynamic( RenderSystem::CommandBufferPtr, DYNAMIC_TYPE )
        {
            uint32_t ret = UNDEFINED_U32;
            return ret;
        }

        uint32_t CScene::SDebugView::AddBatchData( RenderSystem::CommandBufferPtr pCmdBuff, BATCH_TYPE type )
        {
            UInstancingHandle Handle;
            Handle.handle = UNDEFINED_U32;


            //static const SBatch::SVertex aVertexTemplate[8] =
            //{
            //    // AABB
            //    // Front side
            //    { aPositionTemplate[0], DefaultColor }, { aPositionTemplate[1], DefaultColor },
            //    { aPositionTemplate[2], DefaultColor }, { aPositionTemplate[3], DefaultColor },

            //    // Back side
            //    { aPositionTemplate[4], DefaultColor }, { aPositionTemplate[5], DefaultColor },
            //    { aPositionTemplate[6], DefaultColor }, { aPositionTemplate[7], DefaultColor }
            //};

            //static const uint16_t aIndexTemplate[24] =
            //{
            //    // AABB
            //    // Front size
            //    0, 1, // top left - top right
            //    1, 3, // top right - bottom right
            //    3, 2, // bottom right - bottom left
            //    2, 0, // bottom left - top left
            //    // Back side
            //    4, 5,
            //    5, 7,
            //    7, 6,
            //    6, 4,
            //    // Top side
            //    0, 4,
            //    1, 5,
            //    // bottom side
            //    2, 6,
            //    3, 7
            //};






            uint16_t vertexCount = 0;
            uint16_t indexCount = 0;

            switch( type )
            {
                case BatchTypes::AABB:
                {
                    vertexCount = 8;
                    indexCount = 24;
                }
                break;
            }

            auto& Batch = aBatches[type];
            if( Batch.vBuffers.IsEmpty() )
            {
                CreateBatch( type, pCmdBuff );
            }

            {
                auto pBuffer = &Batch.vBuffers.Back();
                Handle.bufferIndex = Batch.vBuffers.GetCount() - 1;
                Handle.index = pBuffer->DrawParams.Indexed.indexCount / indexCount;

                // Out of bounds
                if( Handle.index >= MAX_INSTANCING_DATA_PER_BUFFER )
                {
                    Handle.handle = UNDEFINED_U32;

                    if( VKE_SUCCEEDED( CreateBatch( type, pCmdBuff ) ) )
                    {
                        pBuffer = &Batch.vBuffers.Back();
                        Handle.bufferIndex = Batch.vBuffers.GetCount() - 1;
                        Handle.index = pBuffer->DrawParams.Indexed.indexCount / indexCount;
                    }
                }

                //const uint32_t offset = Buffer.DrawParams.Indexed.indexCount;
                pBuffer->DrawParams.Indexed.indexCount += indexCount;
                VKE_ASSERT2( Handle.index < MAX_INSTANCING_DATA_PER_BUFFER, "" );
            }

            VKE_ASSERT2( Handle.handle != UNDEFINED_U32, "" );
            return Handle.handle;
        }

        Result CScene::SDebugView::CreateBatch( BATCH_TYPE type, RenderSystem::CommandBufferPtr pCmdBuff )
        {
            Result ret = VKE_OK;

            // Create vertex buffer for AABB
            static const Math::CVector3 aPositionTemplate[8] =
            {
                // AABB
                // Front side
                { 0.0f, 1.0f, 0.0f },{ 1.0f, 1.0f, 0.0f },
                { 0.0f, 0.0f, 0.0f },{ 1.0f, 0.0f, 0.0f },

                // Back side
                { 0.0f, 1.0f, 1.0f },{ 1.0f, 1.0f, 1.0f },
                { 0.0f, 0.0f, 1.0f },{ 1.0f, 0.0f, 1.0f }
            };

            static const Math::CVector4 DefaultColor = Math::CVector4::ONE;

            //static const cstr_t spBatchVS = VKE_TO_STRING
            //(
            //    #version 450 core\n

            //    layout( set = 0, binding = 0 ) uniform PerFrameConstantBuffer
            //    {
            //        mat4    mtxViewProj;
            //    };

            //    layout( location = 0 ) in vec3 iPosition;
            //    //layout( location = 1 ) in vec4 iColor;
            //    layout( location = 0 ) out vec4 oColor;

            //    void main()
            //    {
            //        mat4 mtxMVP = mtxViewProj;
            //        gl_Position = mtxMVP * vec4( iPosition, 1.0f );
            //        oColor = vec4(1.0, 0.3, 0.1, 1.0);
            //    }
            //);

            //static const cstr_t spBatchPS = VKE_TO_STRING
            //(
            //    #version 450 core\n

            //    layout( location = 0 ) in vec4 iColor;
            //    layout( location = 0 ) out vec4 oColor;

            //    void main()
            //    {
            //        oColor = iColor;
            //    }
            //);
            static const cstr_t spBatchVS = R"(
                struct SPerFrameConstantBuffer
                {
                    float4x4 mtxViewProj;
                };
                ConstantBuffer<SPerFrameConstantBuffer> FrameData : register(b0, space0);
                struct SIn
                {
                    float3 f3Position : SV_Position
                };
                struct SOut
                {
                    float4 f4Position : SV_Position;
                };
                void main(in SIn IN, out SOut OUT)
                {
                    OUT out;
                    out.f4Position = FrameData.mtxViewProj * float4( IN.f3Position, 1.0f );
                }
                )";

            static const cstr_t spBatchPS = R"(
                struct SIn
                {
                    float4 f4Position : POSITION0;
                };
                float4 main(in SIn IN) : SV_Target
                {
                    return float4( 1.0, 0.3, 0.1, 1.0 );
                }
                )";

            uint16_t vertexCount = 0;
            uint16_t indexCount = 0;

            switch( type )
            {
                case BatchTypes::AABB:
                {
                    vertexCount = 8;
                    indexCount = 24;
                }
                break;
            };
            using AABBCorners = Math::CAABB::Corners;
            static const uint16_t aIndexTemplate[ 24 ] =
            {
                // AABB, Frustum
                AABBCorners::LEFT_TOP_NEAR,     AABBCorners::RIGHT_TOP_NEAR,
                AABBCorners::RIGHT_TOP_NEAR,    AABBCorners::RIGHT_BOTTOM_NEAR,
                AABBCorners::RIGHT_BOTTOM_NEAR, AABBCorners::LEFT_BOTTOM_NEAR,
                AABBCorners::LEFT_BOTTOM_NEAR,  AABBCorners::LEFT_TOP_NEAR,

                AABBCorners::LEFT_TOP_FAR,      AABBCorners::RIGHT_TOP_FAR,
                AABBCorners::RIGHT_TOP_FAR,     AABBCorners::RIGHT_BOTTOM_FAR,
                AABBCorners::RIGHT_BOTTOM_FAR,  AABBCorners::LEFT_BOTTOM_FAR,
                AABBCorners::LEFT_BOTTOM_FAR,   AABBCorners::LEFT_TOP_FAR,

                AABBCorners::LEFT_TOP_NEAR,     AABBCorners::LEFT_TOP_FAR,
                AABBCorners::RIGHT_TOP_NEAR,    AABBCorners::RIGHT_TOP_FAR,

                AABBCorners::LEFT_BOTTOM_NEAR,  AABBCorners::LEFT_BOTTOM_FAR,
                AABBCorners::RIGHT_BOTTOM_NEAR, AABBCorners::RIGHT_BOTTOM_FAR
            };

            const uint32_t indexBufferOffset = MAX_INSTANCING_DATA_PER_BUFFER * sizeof( SBatch::SVertex ) * vertexCount;

            auto& Batch = aBatches[ type ];

            Batch.objectIndexCount = indexCount;
            Batch.objectVertexCount = vertexCount;
            Batch.indexBufferOffset = indexBufferOffset;

            auto pCtx = pCmdBuff->GetContext();
            auto pDevice = pCtx->GetDeviceContext();

            RenderSystem::SCreateBufferDesc Desc;
            Desc.Create.flags = Core::CreateResourceFlags::DEFAULT;
            Desc.Buffer.usage = RenderSystem::BufferUsages::VERTEX_BUFFER | RenderSystem::BufferUsages::INDEX_BUFFER;
            Desc.Buffer.memoryUsage = RenderSystem::MemoryUsages::GPU_ACCESS;
            Desc.Buffer.indexType = RenderSystem::IndexTypes::UINT16;
            Desc.Buffer.size = MAX_INSTANCING_DATA_PER_BUFFER * sizeof( SBatch::SVertex ) * vertexCount + MAX_INSTANCING_DATA_PER_BUFFER * sizeof( uint16_t ) * indexCount;
            auto hBuff = pDevice->CreateBuffer( Desc );
            auto pBuff = pDevice->GetBuffer( hBuff );

            SBatch::SBuffer Data;
            Data.pBuffer = pBuff;
            Data.DrawParams.Indexed.indexCount = 0;
            Data.DrawParams.Indexed.instanceCount = 1;
            Data.DrawParams.Indexed.startIndex = 0;
            Data.DrawParams.Indexed.startInstance = 0;
            Data.DrawParams.Indexed.vertexOffset = 0;
            Batch.vBuffers.PushBack( Data );


            // Create indices for all possible drawcalls
            Utils::TCDynamicArray< uint16_t, 1 > vIndices( indexCount * MAX_INSTANCING_DATA_PER_BUFFER );
            uint16_t offset = 0;
            for( uint32_t i = 0; i < vIndices.GetCount(); i += indexCount )
            {
                for( uint32_t j = 0; j < indexCount; ++j )
                {
                    vIndices[ i + j ] = aIndexTemplate[ j ] + offset;
                }
                offset += vertexCount;
            }

            RenderSystem::SUpdateMemoryInfo UpdateInfo;
            UpdateInfo.pData = vIndices.GetData();
            UpdateInfo.dstDataOffset = indexBufferOffset;
            UpdateInfo.dataSize = sizeof( uint16_t ) * vIndices.GetCount();
            pCtx->UpdateBuffer( pCmdBuff, UpdateInfo, &Data.pBuffer );

            if( hPerFrameDescSet == INVALID_HANDLE )
            {
                if( pPerFrameConstantBuffer.IsNull() )
                {
                    RenderSystem::SCreateBufferDesc BuffDesc;
                    BuffDesc.Create.flags = Core::CreateResourceFlags::DEFAULT;
                    BuffDesc.Buffer.usage = RenderSystem::BufferUsages::CONSTANT_BUFFER;
                    BuffDesc.Buffer.memoryUsage = RenderSystem::MemoryUsages::GPU_ACCESS;
                    BuffDesc.Buffer.size = sizeof( SPerFrameShaderData );
                    hBuff = pDevice->CreateBuffer( BuffDesc );
                    pPerFrameConstantBuffer = pDevice->GetBuffer( hBuff );
                }
                RenderSystem::SCreateBindingDesc BindingDesc;
                BindingDesc.AddConstantBuffer( 0, RenderSystem::PipelineStages::VERTEX );
                hPerFrameDescSet = pDevice->CreateResourceBindings( BindingDesc );
                RenderSystem::SUpdateBindingsHelper UpdateHelper;
                hBuff = pPerFrameConstantBuffer->GetHandle();
                UpdateHelper.AddBinding( 0u, 0u, pPerFrameConstantBuffer->GetSize(), hBuff, RenderSystem::BindingTypes::DYNAMIC_CONSTANT_BUFFER );
                pDevice->UpdateDescriptorSet( UpdateHelper, &hPerFrameDescSet );
            }

            RenderSystem::SPipelineLayoutDesc LayoutDesc;
            LayoutDesc.vDescriptorSetLayouts = { pDevice->GetDescriptorSetLayout( hPerFrameDescSet ) };
            auto pLayout = pDevice->CreatePipelineLayout( LayoutDesc );

            RenderSystem::SShaderData VsData, PsData;
            RenderSystem::SCreateShaderDesc VsDesc, PsDesc;

            VsData.pCode = ( const uint8_t* )spBatchVS;
            VsData.stage = RenderSystem::ShaderCompilationStages::HIGH_LEVEL_TEXT;
            VsData.type = RenderSystem::ShaderTypes::VERTEX;
            VsData.codeSize = ( uint32_t )strlen( spBatchVS );
            VsDesc.Shader.Name = "VKE_DebugView_BatchVS";
            VsDesc.Shader.EntryPoint = "main";
            VsDesc.Shader.type = VsData.type;
            VsDesc.Shader.pData = &VsData;
            auto pVS = pDevice->CreateShader( VsDesc );

            PsData.pCode = ( const uint8_t* )spBatchPS;
            PsData.stage = RenderSystem::ShaderCompilationStages::HIGH_LEVEL_TEXT;
            PsData.type = RenderSystem::ShaderTypes::PIXEL;
            PsData.codeSize = ( uint32_t )strlen( spBatchPS );
            PsDesc.Shader.Name = "VKE_DebugView_BatchPS";
            PsDesc.Shader.EntryPoint = "main";
            PsDesc.Shader.type = PsData.type;
            PsDesc.Shader.pData = &PsData;
            auto pPS = pDevice->CreateShader( PsDesc );

            auto& Pipeline = BatchPipelineTemplate.Pipeline;
            Pipeline.hLayout = pLayout->GetHandle();
            Pipeline.InputLayout.topology = RenderSystem::PrimitiveTopologies::LINE_LIST;
            Pipeline.InputLayout.vVertexAttributes =
            {
                RenderSystem::SPipelineDesc::SInputLayout::SVertexAttribute( "POSITION",    RenderSystem::Formats::R32G32B32_SFLOAT,    0 ),
                //RenderSystem::SPipelineDesc::SInputLayout::SVertexAttribute( "COLOR",       RenderSystem::Formats::R32G32B32A32_SFLOAT, 1 )
            };
            Pipeline.Shaders.apShaders[ RenderSystem::ShaderTypes::VERTEX ] = pVS;
            Pipeline.Shaders.apShaders[ RenderSystem::ShaderTypes::PIXEL ] = pPS;
            //VKE_RENDER_SYSTEM_SET_DEBUG_NAME( Pipeline, "VKE_DebugView_Batch" );
            Pipeline.SetDebugName( "VKE_DebugView_Batch" );

            {
                Pipeline.DepthStencil.Depth.enable = false;
            }

            return ret;
        }

        bool CScene::SDebugView::CreateDrawcallData( RenderSystem::CDeviceContext* pCtx,
                                                     INSTANCING_TYPE type )
        {
            bool ret = true;



            return ret;
        }

        void CScene::SDebugView::CalcCorners( const Math::CAABB& AABB, CScene::SDebugView::SBatch::SVertex* pOut )
        {
#if VKE_USE_DIRECTX_MATH
            // Create vertex buffer for AABB
            const DirectX::XMVECTORF32 aPositionTemplate[8] =
            {
                // AABB
                // Front side
                { -1.0f, 1.0f, -1.0f, 0.0f },     { 1.0f, 1.0f, -1.0f, 0.0f },
                { -1.0f, -1.0f, -1.0f, 0.0f },    { 1.0f, -1.0f, -1.0f, 0.0f },

                // Back side
                { -1.0f, 1.0f, 1.0f, 0.0f },      { 1.0f, 1.0f, 1.0f, 0.0f },
                { -1.0f, -1.0f, 1.0f, 0.0f },     { 1.0f, -1.0f, 1.0f, 0.0f }
            };

            /*DirectX::XMVECTOR Extents = DirectX::XMLoadFloat3( &AABB._Native.Extents );
            DirectX::XMVECTOR Center = DirectX::XMLoadFloat3( &AABB._Native.Center );

            for( uint32_t i = 0; i < 8; ++i )
            {
                DirectX::XMVECTOR v = DirectX::XMVectorMultiplyAdd( aPositionTemplate[i], Extents, Center );
                DirectX::XMStoreFloat3( &(pOut[i].vecPosition._Native), v );
            }*/
            DirectX::XMFLOAT3 aCorners[8];
            AABB._Native.GetCorners( aCorners );
            for( uint32_t i = 0; i < 8; ++i )
            {
                pOut[i].vecPosition._Native = aCorners[i];
            }
#else
#error "Implement"
#endif
        }

        void CScene::SDebugView::UpdateBatchData( RenderSystem::CommandBufferPtr pCmdBuff, BATCH_TYPE type,
                                                  const uint32_t& handle,
                                                  const Math::CAABB& AABB )
        {
            auto& Curr = aBatches[type];
            UInstancingHandle Handle;
            Handle.handle = handle;
            auto& Buffer = Curr.vBuffers[Handle.bufferIndex];
            const uint32_t vertexOffset = Handle.index * Curr.objectVertexCount * sizeof( SBatch::SVertex );

            // Create vertex buffer for AABB
            static const Math::CVector4 aPositionTemplate[8] =
            {
                // AABB
                // Front side
                { -1.0f, 1.0f, -1.0f, 0.0f },     { 1.0f, 1.0f, -1.0f, 0.0f },
                { -1.0f, -1.0f, -1.0f, 0.0f },    { 1.0f, -1.0f, -1.0f, 0.0f },

                // Back side
                { -1.0f, 1.0f, 1.0f, 0.0f },      { 1.0f, 1.0f, 1.0f, 0.0f },
                { -1.0f, -1.0f, 1.0f, 0.0f },     { 1.0f, -1.0f, 1.0f, 0.0f }
            };

            static const Math::CVector4 DefaultColor = Math::CVector4::ONE;

            SBatch::SVertex aVertices[8];
            CalcCorners( AABB, aVertices );

            RenderSystem::SUpdateMemoryInfo UpdateInfo;
            UpdateInfo.dataSize = sizeof( SBatch::SVertex ) * Curr.objectVertexCount;
            UpdateInfo.pData = &aVertices;
            UpdateInfo.dstDataOffset = vertexOffset;

            pCmdBuff->GetContext()->UpdateBuffer( pCmdBuff, UpdateInfo, &Buffer.pBuffer );
        }

        void CScene::SDebugView::UpdateBatchData( RenderSystem::CommandBufferPtr pCmdBuff, BATCH_TYPE type,
                                                  const uint32_t& handle,
            const Math::CVector3* aCorners )
        {
            auto& Curr = aBatches[type];
            if( !Curr.vBuffers.IsEmpty() )
            {
                UInstancingHandle Handle;
                Handle.handle = handle;
                auto& Buffer = Curr.vBuffers[ Handle.bufferIndex ];
                const uint32_t vertexOffset = Handle.index * Curr.objectVertexCount * sizeof( SBatch::SVertex );

                SBatch::SVertex aVertices[ 8 ];
                for( uint32_t i = 0; i < 8; ++i )
                {
                    aVertices[ i ].vecPosition = aCorners[ i ];
                }

                RenderSystem::SUpdateMemoryInfo UpdateInfo;
                UpdateInfo.dataSize = sizeof( SBatch::SVertex ) * Curr.objectVertexCount;
                UpdateInfo.pData = &aVertices;
                UpdateInfo.dstDataOffset = vertexOffset;
                pCmdBuff->GetContext()->UpdateBuffer( pCmdBuff, UpdateInfo, &Buffer.pBuffer );
            }
        }

        void CScene::SDebugView::UpdateInstancing( RenderSystem::CommandBufferPtr, INSTANCING_TYPE type,
                                                   const uint32_t handle,
                                                   const SInstancingShaderData& Data )
        {
            auto& Curr = aInstancings[type];
            UInstancingHandle Handle;
            Handle.handle = handle;
            auto& CB = Curr.vConstantBuffers[Handle.bufferIndex];
            const uint32_t offset = CB.pStorageBuffer->CalcAbsoluteOffset( 0, Handle.index );
            auto pData = (SInstancingShaderData*)&CB.vData[ offset ];
            Memory::Copy( pData, &Data );

            Curr.UpdateBufferMask.SetBit( Handle.bufferIndex );
        }

        void CScene::SDebugView::UploadInstancingConstantData( RenderSystem::CommandBufferPtr pCmdBuffer,
            const CCamera* pCamera)
        {
            RenderSystem::SUpdateMemoryInfo UpdateInfo;
            auto pCtx = pCmdBuffer->GetContext();
            //auto pDevice = pCtx->GetDeviceContext();

            for( uint32_t i = 0; i < InstancingTypes::_MAX_COUNT; ++i )
            {
                auto& Curr = aInstancings[i];
                for( uint8_t b = 0; b < Curr.vConstantBuffers.GetCount(); ++b )
                {
                    if( Curr.UpdateBufferMask.IsBitSet( b ) )
                    {
                        auto& CB = Curr.vConstantBuffers[b];

                        SPerFrameShaderData PerFrameShaderData;
                        PerFrameShaderData.mtxViewProj = pCamera->GetViewProjectionMatrix();

                        UpdateInfo.dataSize = sizeof( SPerFrameShaderData );
                        UpdateInfo.dstDataOffset = 0;
                        UpdateInfo.pData = &PerFrameShaderData;
                        pCtx->UpdateBuffer( pCmdBuffer, UpdateInfo, &CB.pConstantBuffer );

                        UpdateInfo.dataSize = CB.vData.GetCount();
                        UpdateInfo.dstDataOffset = 0;
                        UpdateInfo.pData = CB.vData.GetData();
                        pCtx->UpdateBuffer( pCmdBuffer, UpdateInfo, &CB.pStorageBuffer );
                    }
                }
                Curr.UpdateBufferMask.Set( 0u );
            }
        }

        void CScene::SDebugView::UploadBatchData( RenderSystem::CommandBufferPtr pCmdBuffer,
                                                  const CCamera* pCamera )
        {
            VKE_ASSERT2( pPerFrameConstantBuffer.IsValid(), "" );
            SPerFrameShaderData Data;
            Data.mtxViewProj = pCamera->GetViewProjectionMatrix();

            RenderSystem::SUpdateMemoryInfo UpdateInfo;
            UpdateInfo.pData = &Data;
            UpdateInfo.dstDataOffset = 0;
            UpdateInfo.dataSize = sizeof( SPerFrameShaderData );
            pCmdBuffer->GetContext()->UpdateBuffer( pCmdBuffer, UpdateInfo, &pPerFrameConstantBuffer );
        }

        void CScene::SDebugView::Render( RenderSystem::CommandBufferPtr pCmdBuff )
        {
            //RenderSystem::CCommandBuffer* pCmdBuff = pCtx->GetCommandBuffer().Get();
            const auto& hDDICurrPass = pCmdBuff->GetCurrentDDIRenderPass();

            // Batch
            {
                const bool needNewPipeline = BatchPipelineTemplate.Pipeline.hDDIRenderPass != hDDICurrPass;
                if( BatchPipelineTemplate.Pipeline.hLayout != INVALID_HANDLE )
                {
                    auto& Batch = aBatches[ BatchTypes::AABB ];
                    if( Batch.pPipeline.IsNull() || needNewPipeline )
                    {
                        BatchPipelineTemplate.Pipeline.hDDIRenderPass = hDDICurrPass;
                        Batch.pPipeline = pCmdBuff->GetContext()->GetDeviceContext()->CreatePipeline( BatchPipelineTemplate );
                    }

                    if( Batch.pPipeline.IsValid() && Batch.pPipeline->IsResourceReady() )
                    {
                        pCmdBuff->Bind( Batch.pPipeline );
                        const uint32_t descSetOffset = 0;
                        pCmdBuff->Bind( 0, hPerFrameDescSet, &descSetOffset, 1 );

                        for( uint32_t i = 0; i < Batch.vBuffers.GetCount(); ++i )
                        {
                            const auto& Buffer = Batch.vBuffers[ i ];
                            const auto hVB = HandleCast< RenderSystem::VertexBufferHandle >( Buffer.pBuffer->GetHandle() );
                            const auto hIB = HandleCast< RenderSystem::IndexBufferHandle >( hVB );
                            pCmdBuff->Bind( hVB, 0 );
                            pCmdBuff->Bind( hIB, Batch.indexBufferOffset );
                            pCmdBuff->DrawIndexed( Buffer.DrawParams );
                        }
                    }
                }
            }
            // Instancing
            {
                //const bool needNewPipeline = InstancingPipelineTemplate.Pipeline.hDDIRenderPass != hDDICurrPass;

                pCmdBuff->Bind( hInstancingVB, 0 );
                pCmdBuff->Bind( hInstancingIB, 0 );
                // AABB
                {
                    auto& Curr = aInstancings[InstancingTypes::AABB];
                    if( !Curr.vConstantBuffers.IsEmpty() )
                    {
                        auto& DrawData = Curr.DrawData;
                        auto& pPipeline = DrawData.pPipeline;

                        if( pPipeline.IsNull() || Curr.hDDIRenderPass != hDDICurrPass )
                        {
                            auto pDevCtx = pCmdBuff->GetContext()->GetDeviceContext();

                            if( InstancingPipelineTemplate.Pipeline.hLayout == INVALID_HANDLE )
                            {
                                // Get any desc set as they all are the same
                                auto hDescSet = Curr.vConstantBuffers.Back().hDescSet;
                                RenderSystem::SPipelineLayoutDesc LayoutDesc;
                                LayoutDesc.vDescriptorSetLayouts = { pDevCtx->GetDescriptorSetLayout( hDescSet ) };
                                const auto& pLayout = pDevCtx->CreatePipelineLayout( LayoutDesc );
                                if( pLayout.IsValid() )
                                {
                                    InstancingPipelineTemplate.Pipeline.hLayout = pLayout->GetHandle();
                                }
                            }

                            InstancingPipelineTemplate.Create.flags = Core::CreateResourceFlags::DEFAULT;
                            InstancingPipelineTemplate.Pipeline.hDDIRenderPass = hDDICurrPass;
                            Curr.hDDIRenderPass = hDDICurrPass;

                            pPipeline = pDevCtx->CreatePipeline( InstancingPipelineTemplate );
                        }

                        if( pPipeline.IsValid() && pPipeline->IsResourceReady() )
                        {
                            pCmdBuff->Bind( pPipeline );

                            for( uint32_t b = 0; b < Curr.vConstantBuffers.GetCount(); ++b )
                            {
                                const auto& CB = Curr.vConstantBuffers[b];
                                static const uint32_t aOffsets[2] = { 0u, 0u };
                                pCmdBuff->Bind( 0, CB.hDescSet, aOffsets, 2 );
                                DrawData.DrawParams.Indexed.instanceCount = CB.drawCount;
                                pCmdBuff->DrawIndexed( DrawData.DrawParams );
                            }
                        }
                    }
                }
            }
        }
    } // Scene
} // VKE
