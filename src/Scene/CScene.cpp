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

            VKE_ASSERT( Desc.pDeviceContext, "DeviceContext must be set." );
            m_pDeviceCtx = Desc.pDeviceContext;

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

            _CreateDebugView();

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
            VKE_SCENE_SET_DEBUG_NAME( *pCam, dbgName );

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

            if( Info.debugView )
            {
                pDrawcall->m_hDbgView = m_pDebugView->AddBatchData( m_pDebugView->pDeviceCtx, SDebugView::BatchTypes::AABB );
            }

            return Handle.handle;
        }

        void CScene::UpdateDrawcallAABB( const handle_t& hDrawcall, const Math::CAABB& NewAABB )
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
                m_pDebugView->UpdateBatchData( SDebugView::BatchTypes::AABB, hDbgView, NewAABB );
            }
        }

        void CScene::AddDebugView( CameraPtr* pCamera )
        {
            if( m_pDebugView )
            {
                CameraPtr pTmp = *pCamera;
                //pTmp->m_hDbgView = m_pDebugView->AddBatchData( m_pDeviceCtx, SDebugView::BatchTypes::AABB );
                pTmp->m_hDbgView = m_pDebugView->AddInstancing( m_pDeviceCtx, SDebugView::InstancingTypes::AABB );
            }
        }

        void CScene::_UpdateDebugViews( RenderSystem::CGraphicsContext* pCtx )
        {
            //Math::CVector3 aCorners[8];
            for( uint32_t i = 0; i < m_vCameras.GetCount(); ++i )
            {
                auto& Curr = m_vCameras[i];
                if( Curr.m_hDbgView != UNDEFINED_U32 )
                {
                    Math::CMatrix4x4 mtxTransform;
                    Curr.GetFrustum().CalcMatrix( &mtxTransform );
                    //m_pDebugView->UpdateBatchData( SDebugView::BatchTypes::AABB, Curr.m_hDbgView, aCorners );
                    m_pDebugView->UpdateInstancing( SDebugView::InstancingTypes::AABB, Curr.m_hDbgView, mtxTransform );
                }
            }
        }

        void CScene::Render( VKE::RenderSystem::CGraphicsContext* pCtx )
        {
            m_pCurrentCamera->Update(0);
            if( m_pCurrentCamera != m_pCurrentRenderCamera )
            {
                m_pCurrentRenderCamera->Update( 0 );
            }

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
            if( m_pDebugView )
            {
                m_pDebugView->UploadInstancingConstantData( pCtx, GetRenderCamera() );
                m_pDebugView->UploadBatchData( pCtx, GetRenderCamera() );
                _UpdateDebugViews( pCtx );
            }
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

        Result CScene::_CreateDebugView()
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
                layout( location = 0 ) out vec4 oColor;

                void main()
                {
                    mat4 mtxMVP = mtxViewProj * aBuffers[gl_InstanceIndex].mtxTransform;
                    gl_Position = mtxMVP * vec4( iPosition, 1.0f );
                    oColor = aBuffers[gl_InstanceIndex].vecColor;
                }
            );

            static const cstr_t spInstancingPS = VKE_TO_STRING
            (
                #version 450 core\n

                layout( location = 0 ) in vec4 iColor;
                layout(location = 0) out vec4 oColor;

                void main()
                {
                    oColor = iColor;
                }
            );

            Result ret;
            ret = Memory::CreateObject( &HeapAllocator, &m_pDebugView );
            if( VKE_SUCCEEDED( ret ) )
            {
                m_pDebugView->pDeviceCtx = m_pDeviceCtx;

                RenderSystem::SShaderData VSData, PSData;
                VSData.type = RenderSystem::ShaderTypes::VERTEX;
                VSData.stage = RenderSystem::ShaderCompilationStages::HIGH_LEVEL_TEXT;
                VSData.pCode = (const uint8_t*)spInstancingVS;
                VSData.codeSize = (uint32_t)strlen( spInstancingVS );

                RenderSystem::SCreateShaderDesc VSDesc, PSDesc;
                VSDesc.Create.async = false;
                VSDesc.Shader.FileInfo.pName = "VKE_InstancingDebugViewVS";
                VSDesc.Shader.type = RenderSystem::ShaderTypes::VERTEX;
                VSDesc.Shader.pData = &VSData;
                VSDesc.Shader.SetEntryPoint( "main" );

                auto pVS = m_pDeviceCtx->CreateShader( VSDesc );

                PSData.type = RenderSystem::ShaderTypes::PIXEL;
                PSData.stage = RenderSystem::ShaderCompilationStages::HIGH_LEVEL_TEXT;
                PSData.pCode = (const uint8_t*)spInstancingPS;
                PSData.codeSize = (uint32_t)strlen( spInstancingPS );

                PSDesc.Create.async = true;
                PSDesc.Shader.FileInfo.pName = "VKE_InstancingDebugViewPS";
                PSDesc.Shader.type = PSData.type;
                PSDesc.Shader.pData = &PSData;
                PSDesc.Shader.SetEntryPoint( "main" );

                auto pPS = m_pDeviceCtx->CreateShader( PSDesc );

                while(pVS.IsNull() || pPS.IsNull() ) {}
                while(!pVS->IsReady() || !pPS->IsReady() ) {}

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
                VKE_RENDER_SYSTEM_SET_DEBUG_NAME( PipelineDesc, "DebugView" );

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
                BuffDesc.Create.async = false;
                BuffDesc.Buffer.memoryUsage = RenderSystem::MemoryUsages::GPU_ACCESS;
                BuffDesc.Buffer.usage = RenderSystem::BufferUsages::VERTEX_BUFFER;
                BuffDesc.Buffer.size = sizeof( aVertices );
                BuffDesc.Buffer.pData = (const void*)aVertices;
                BuffDesc.Buffer.dataSize = sizeof( aVertices );
                auto hVB = m_pDeviceCtx->CreateBuffer( BuffDesc );

                BuffDesc.Buffer.usage = RenderSystem::BufferUsages::INDEX_BUFFER;
                BuffDesc.Buffer.size = sizeof( aIndices );
                BuffDesc.Buffer.indexType = RenderSystem::IndexTypes::UINT16;
                BuffDesc.Buffer.pData = (const void*)aIndices;
                BuffDesc.Buffer.dataSize = sizeof( aIndices );
                auto hIB = m_pDeviceCtx->CreateBuffer( BuffDesc );

                m_pDebugView->hInstancingVB = HandleCast< RenderSystem::VertexBufferHandle >( hVB );
                m_pDebugView->hInstancingIB = HandleCast< RenderSystem::IndexBufferHandle >(hIB);

                auto& AABB = m_pDebugView->aInstancings[ SDebugView::InstancingTypes::AABB ];
                AABB.DrawData.DrawParams.Indexed.indexCount = 24;
                AABB.DrawData.DrawParams.Indexed.startIndex = 0;
                AABB.DrawData.DrawParams.Indexed.vertexOffset = 0;


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
        bool CScene::SDebugView::CreateConstantBuffer( RenderSystem::CDeviceContext* pCtx,
                                                       uint32_t elementCount, SConstantBuffer* pOut )
        {
            bool ret = false;
            // Create per frame constant buffer
            if( pPerFrameConstantBuffer.IsNull() )
            {
                RenderSystem::SCreateBufferDesc BuffDesc;
                BuffDesc.Create.async = false;
                BuffDesc.Buffer.memoryUsage = RenderSystem::MemoryUsages::STATIC | RenderSystem::MemoryUsages::BUFFER;
                BuffDesc.Buffer.size = sizeof( SPerFrameShaderData );
                BuffDesc.Buffer.usage = RenderSystem::BufferUsages::CONSTANT_BUFFER;
                RenderSystem::BufferHandle hPerFrameConstantBuffer = pCtx->CreateBuffer( BuffDesc );
                pPerFrameConstantBuffer = pCtx->GetBuffer( hPerFrameConstantBuffer );
            }

            // Create instancing buffer
            RenderSystem::SCreateBufferDesc BuffDesc;
            BuffDesc.Create.async = false;
            BuffDesc.Buffer.memoryUsage = RenderSystem::MemoryUsages::STATIC | RenderSystem::MemoryUsages::BUFFER;
            BuffDesc.Buffer.size = 0; // sizeof( SInstancingShaderData ) * MAX_INSTANCING_DATA_PER_BUFFER;
            BuffDesc.Buffer.usage = RenderSystem::BufferUsages::STORAGE_BUFFER;
            BuffDesc.Buffer.vRegions =
            {
                RenderSystem::SBufferRegion(MAX_INSTANCING_DATA_PER_BUFFER, sizeof( SInstancingShaderData ) )
            };
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
                        this->hPerFrameDescSet = pCtx->CreateResourceBindings( BindingDesc );
                        if( this->hPerFrameDescSet != INVALID_HANDLE )
                        {
                            pOut->hDescSet = this->hPerFrameDescSet;
                            RenderSystem::SUpdateBindingsHelper Update;
                            Update.AddBinding( 0u, 0, pPerFrameConstantBuffer->GetSize(), pPerFrameConstantBuffer->GetHandle() );
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

        uint32_t CScene::SDebugView::AddInstancing( RenderSystem::CDeviceContext* pCtx, INSTANCING_TYPE type )
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

            if( Curr.vConstantBuffers.IsEmpty() )
            {
                SConstantBuffer CB;
                if( CreateConstantBuffer( pCtx, defaultCount, &CB ) )
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
                        VKE_ASSERT( Curr.vConstantBuffers.GetCount() + 1 < 0xF, "Reached max number of constant buffers for debug view objects." );
                        SConstantBuffer TmpCB;
                        if( CreateConstantBuffer( pCtx, defaultCount, &TmpCB ) )
                        {
                            Curr.vConstantBuffers.PushBack( TmpCB );
                        }
                    }
                }
                auto& CB = Curr.vConstantBuffers.Back();
                ret = CB.drawCount++;
                VKE_ASSERT( CB.drawCount < MAX_INSTANCING_DRAW_COUNT, "Reached max number of debug view drawcalls." );
                
                if( CreateDrawcallData( pCtx, type ) )
                {
                    UInstancingHandle Handle;
                    Handle.bufferIndex = Curr.vConstantBuffers.GetCount() - 1;
                    Handle.index = ret;
                    ret = Handle.handle;
                }
                
            }
            return ret;
        }

        uint32_t CScene::SDebugView::AddDynamic( RenderSystem::CDeviceContext* pCtx, DYNAMIC_TYPE type )
        {
            uint32_t ret = UNDEFINED_U32;
            return ret;
        }

        uint32_t CScene::SDebugView::AddBatchData( RenderSystem::CDeviceContext* pCtx, BATCH_TYPE type )
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
                CreateBatch( type, pDeviceCtx );
            }
     
            {
                auto pBuffer = &Batch.vBuffers.Back();
                Handle.bufferIndex = Batch.vBuffers.GetCount() - 1;
                Handle.index = pBuffer->DrawParams.Indexed.indexCount / indexCount;

                // Out of bounds
                if( Handle.index >= MAX_INSTANCING_DATA_PER_BUFFER )
                {
                    Handle.handle = UNDEFINED_U32;

                    if( VKE_SUCCEEDED( CreateBatch( type, pDeviceCtx ) ) )
                    {
                        pBuffer = &Batch.vBuffers.Back();
                        Handle.bufferIndex = Batch.vBuffers.GetCount() - 1;
                        Handle.index = pBuffer->DrawParams.Indexed.indexCount / indexCount;
                    }
                }
            
                //const uint32_t offset = Buffer.DrawParams.Indexed.indexCount;
                pBuffer->DrawParams.Indexed.indexCount += indexCount;
                VKE_ASSERT( Handle.index < MAX_INSTANCING_DATA_PER_BUFFER, "" );
            }

            VKE_ASSERT( Handle.handle != UNDEFINED_U32, "" );
            return Handle.handle;
        }

        Result CScene::SDebugView::CreateBatch( BATCH_TYPE type, RenderSystem::CDeviceContext* pCtx )
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

            static const cstr_t spBatchVS = VKE_TO_STRING
            (
                #version 450 core\n

                layout( set = 0, binding = 0 ) uniform PerFrameConstantBuffer
                {
                    mat4    mtxViewProj;
                };

                layout( location = 0 ) in vec3 iPosition;
                //layout( location = 1 ) in vec4 iColor;
                layout( location = 0 ) out vec4 oColor;

                void main()
                {
                    mat4 mtxMVP = mtxViewProj;
                    gl_Position = mtxMVP * vec4( iPosition, 1.0f );
                    oColor = vec4(1.0, 0.3, 0.1, 1.0);
                }
            );

            static const cstr_t spBatchPS = VKE_TO_STRING
            (
                #version 450 core\n

                layout( location = 0 ) in vec4 iColor;
                layout( location = 0 ) out vec4 oColor;

                void main()
                {
                    oColor = iColor;
                }
            );

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

            RenderSystem::SCreateBufferDesc Desc;
            Desc.Create.async = false;
            Desc.Buffer.usage = RenderSystem::BufferUsages::VERTEX_BUFFER | RenderSystem::BufferUsages::INDEX_BUFFER;
            Desc.Buffer.memoryUsage = RenderSystem::MemoryUsages::GPU_ACCESS;
            Desc.Buffer.indexType = RenderSystem::IndexTypes::UINT16;
            Desc.Buffer.size = MAX_INSTANCING_DATA_PER_BUFFER * sizeof( SBatch::SVertex ) * vertexCount + MAX_INSTANCING_DATA_PER_BUFFER * sizeof( uint16_t ) * indexCount;
            auto hBuff = pCtx->CreateBuffer( Desc );
            auto pBuff = pCtx->GetBuffer( hBuff );

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
            pCtx->UpdateBuffer( UpdateInfo, &Data.pBuffer );

            if( hPerFrameDescSet == INVALID_HANDLE )
            {
                if( pPerFrameConstantBuffer.IsNull() )
                {
                    RenderSystem::SCreateBufferDesc BuffDesc;
                    BuffDesc.Create.async = false;
                    BuffDesc.Buffer.usage = RenderSystem::BufferUsages::CONSTANT_BUFFER;
                    BuffDesc.Buffer.memoryUsage = RenderSystem::MemoryUsages::GPU_ACCESS;
                    BuffDesc.Buffer.size = sizeof( SPerFrameShaderData );
                    hBuff = pCtx->CreateBuffer( BuffDesc );
                    pPerFrameConstantBuffer = pCtx->GetBuffer( hBuff );
                }
                RenderSystem::SCreateBindingDesc BindingDesc;
                BindingDesc.AddConstantBuffer( 0, RenderSystem::PipelineStages::VERTEX );
                hPerFrameDescSet = pCtx->CreateResourceBindings( BindingDesc );
                RenderSystem::SUpdateBindingsHelper UpdateHelper;
                hBuff = pPerFrameConstantBuffer->GetHandle();
                UpdateHelper.AddBinding( 0u, 0u, pPerFrameConstantBuffer->GetSize(), hBuff, RenderSystem::BindingTypes::DYNAMIC_CONSTANT_BUFFER );
                pCtx->UpdateDescriptorSet( UpdateHelper, &hPerFrameDescSet );
            }

            RenderSystem::SPipelineLayoutDesc LayoutDesc;
            LayoutDesc.vDescriptorSetLayouts = { pCtx->GetDescriptorSetLayout( hPerFrameDescSet ) };
            auto pLayout = pCtx->CreatePipelineLayout( LayoutDesc );

            RenderSystem::SShaderData VsData, PsData;
            RenderSystem::SCreateShaderDesc VsDesc, PsDesc;

            VsData.pCode = ( const uint8_t* )spBatchVS;
            VsData.stage = RenderSystem::ShaderCompilationStages::HIGH_LEVEL_TEXT;
            VsData.type = RenderSystem::ShaderTypes::VERTEX;
            VsData.codeSize = ( uint32_t )strlen( spBatchVS );
            VsDesc.Shader.FileInfo.pName = "VKE_DebugView_BatchVS";
            VsDesc.Shader.SetEntryPoint( "main" );
            VsDesc.Shader.type = VsData.type;
            VsDesc.Shader.pData = &VsData;
            auto pVS = pCtx->CreateShader( VsDesc );

            PsData.pCode = ( const uint8_t* )spBatchPS;
            PsData.stage = RenderSystem::ShaderCompilationStages::HIGH_LEVEL_TEXT;
            PsData.type = RenderSystem::ShaderTypes::PIXEL;
            PsData.codeSize = ( uint32_t )strlen( spBatchPS );
            PsDesc.Shader.FileInfo.pName = "VKE_DebugView_BatchPS";
            PsDesc.Shader.SetEntryPoint( "main" );
            PsDesc.Shader.type = PsData.type;
            PsDesc.Shader.pData = &PsData;
            auto pPS = pCtx->CreateShader( PsDesc );

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
            VKE_RENDER_SYSTEM_SET_DEBUG_NAME( Pipeline, "VKE_DebugView_Batch" );

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

        void CScene::SDebugView::UpdateBatchData( BATCH_TYPE type, const uint32_t& handle,
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

            pDeviceCtx->UpdateBuffer( UpdateInfo, &Buffer.pBuffer );
        }

        void CScene::SDebugView::UpdateBatchData( BATCH_TYPE type, const uint32_t& handle,
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
                pDeviceCtx->UpdateBuffer( UpdateInfo, &Buffer.pBuffer );
            }
        }

        void CScene::SDebugView::UpdateInstancing( INSTANCING_TYPE type, const uint32_t& handle,
            const Math::CMatrix4x4& mtxTransform )
        {
            auto& Curr = aInstancings[type];
            UInstancingHandle Handle;
            Handle.handle = handle;
            auto& CB = Curr.vConstantBuffers[Handle.bufferIndex];
            const uint32_t offset = CB.pStorageBuffer->CalcOffset( 0, Handle.index );
            auto pData = (SInstancingShaderData*)&CB.vData[ offset ];
            pData->mtxTransform = mtxTransform;
            pData->vecColor = Math::CVector4( 1.0f, 0.8f, 0.8f, 1.0f );

            Curr.UpdateBufferMask.SetBit( Handle.bufferIndex );
        }

        void CScene::SDebugView::UploadInstancingConstantData( RenderSystem::CGraphicsContext* pCtx,
            const CCamera* pCamera)
        {
            RenderSystem::SUpdateMemoryInfo UpdateInfo;

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
                        pCtx->UpdateBuffer( UpdateInfo, &CB.pConstantBuffer );

                        UpdateInfo.dataSize = CB.vData.GetCount();
                        UpdateInfo.dstDataOffset = 0;
                        UpdateInfo.pData = CB.vData.GetData();
                        pCtx->UpdateBuffer( UpdateInfo, &CB.pStorageBuffer );
                    }
                }
                Curr.UpdateBufferMask.Set( 0u );
            }
        }

        void CScene::SDebugView::UploadBatchData( RenderSystem::CGraphicsContext* pCtx,
                                                  const CCamera* pCamera )
        {
            VKE_ASSERT( pPerFrameConstantBuffer.IsValid(), "" );
            SPerFrameShaderData Data;
            Data.mtxViewProj = pCamera->GetViewProjectionMatrix();

            RenderSystem::SUpdateMemoryInfo UpdateInfo;
            UpdateInfo.pData = &Data;
            UpdateInfo.dstDataOffset = 0;
            UpdateInfo.dataSize = sizeof( SPerFrameShaderData );
            pCtx->UpdateBuffer( UpdateInfo, &pPerFrameConstantBuffer );
        }

        void CScene::SDebugView::Render( RenderSystem::CGraphicsContext* pCtx )
        {
            RenderSystem::CCommandBuffer* pCmdBuff = pCtx->GetCommandBuffer();
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
                        Batch.pPipeline = pCtx->GetDeviceContext()->CreatePipeline( BatchPipelineTemplate );
                    }

                    if( Batch.pPipeline.IsValid() && Batch.pPipeline->IsReady() )
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
                const bool needNewPipeline = InstancingPipelineTemplate.Pipeline.hDDIRenderPass != hDDICurrPass;

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
                            auto pDevCtx = pCtx->GetDeviceContext();

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

                            InstancingPipelineTemplate.Create.async = true;
                            InstancingPipelineTemplate.Pipeline.hDDIRenderPass = hDDICurrPass;
                            Curr.hDDIRenderPass = hDDICurrPass;

                            pPipeline = pDevCtx->CreatePipeline( InstancingPipelineTemplate );
                        }

                        if( pPipeline.IsValid() && pPipeline->IsReady() )
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
