
#include "../CSampleFramework.h"
#include "Vke/Core/Managers/CImageManager.h"
struct SInputListener : public VKE::Input::EventListeners::IInput
{
    VKE::Scene::CameraPtr pCamera = nullptr;
    VKE::Math::CVector3 vecSpeed = VKE::Math::CVector3( 1.1f );
    VKE::Math::CVector3 vecDir = VKE::Math::CVector3::Z;
    VKE::Math::CVector3 vecDist = VKE::Math::CVector3( 1.0f );
    VKE::Input::MousePosition LastMousePos = { 0, 0 };
    VKE::Input::MousePosition MouseDir = { 0, 0 };
    VKE::ExtentF32 MouseAngle = { 0.0f, 0.0f };
    VKE::Math::CVector3 vecYawPitchRoll = { 0.0f };
    VKE::Math::CVector3 vecLightPos = { 2, 22, 0 };
    float lightSpeed = 10;
    bool mouseDown = false;
    void OnKeyDown( const VKE::Input::SKeyboardState& State,
                    const VKE::Input::KEY& key ) override
    {
        if( key == VKE::Input::KEY::W )
        {
            // pCamera->Move( vecDist * vecSpeed * vecDir );
        }
        if( key == VKE::Input::KEY::S )
        {
            // pCamera->Move( vecDist * vecSpeed * -vecDir );
        }
        if(key == VKE::Input::KEY::UP)
        {
            vecLightPos.z +=  lightSpeed;
        }
        if( key == VKE::Input::KEY::DOWN )
        {
            vecLightPos.z -=  lightSpeed;
        }
        if( key == VKE::Input::KEY::LEFT )
        {
            vecLightPos.x -=  lightSpeed;
        }
        if( key == VKE::Input::KEY::RIGHT )
        {
            vecLightPos.x +=  lightSpeed;
        }
        if( key == VKE::Input::KEY::PAGE_UP )
        {
            vecLightPos.y +=  lightSpeed;
        }
        if( key == VKE::Input::KEY::PAGE_DOWN )
        {
            vecLightPos.y -=  lightSpeed;
        }
    }
    void OnKeyUp( const VKE::Input::SKeyboardState& State,
                  const VKE::Input::KEY& key ) override
    {
    }
    void OnMouseButtonDown( const VKE::Input::SMouseState& Mouse ) override
    {
        mouseDown = true;
    }
    void OnMouseButtonUp( const VKE::Input::SMouseState& Mouse ) override
    {
        mouseDown = false;
    }
    void OnMouseMove( const VKE::Input::SMouseState& Mouse ) override
    {
        if( !mouseDown || ( Mouse.Move.x == 0 && Mouse.Move.y == 0 ) )
            return;
        const float scale = 0.25f + 0.0f;
        float x = VKE::Math::ConvertToRadians( ( float )-Mouse.Move.x ) * scale;
        float y =
            VKE::Math::ConvertToRadians( ( float )Mouse.Move.y ) * scale * 1;
        printf( "m %f, %f\n", x, y );
        pCamera->Rotate( x, y, 0.0f );
    }
};
struct SGfxContextListener
    : public VKE::RenderSystem::EventListeners::IGraphicsContext
{
    VKE::RenderSystem::VertexBufferRefPtr pVb;
    VKE::RenderSystem::ShaderRefPtr pVS;
    VKE::RenderSystem::ShaderRefPtr pPS;
    VKE::RenderSystem::BufferRefPtr pUBO;
    VKE::RenderSystem::DescriptorSetHandle hDescSet;
    VKE::RenderSystem::SVertexInputLayoutDesc Layout;
    VKE::Scene::CameraPtr pDebugCamera, pCamera;
    VKE::Scene::ScenePtr pScene;
    VKE::RenderSystem::IFrameGraph* pFrameGraph;
    VKE::Scene::TerrainPtr pTerrain;
    SInputListener* pInputListener;
    VKE::RenderSystem::SBeginRenderPassInfo2 m_RenderPassInfo;
    VKE::RenderSystem::RenderPassRefPtr m_pRenderPass;
    VKE::Scene::LightRefPtr m_pLight;
    VKE::WindowPtr pWindow;

    struct SUBO
    {
        vke_vector<uint8_t> vData;
    };
    SUBO UBO;
    SGfxContextListener() { pInputListener = VKE_NEW SInputListener; }
    virtual ~SGfxContextListener() { VKE_DELETE( pInputListener ); }
    void LoadTextures( VKE::RenderSystem::CDeviceContext* pCtx,
                       VKE::Scene::STerrainDesc* pDesc )
    {
        VKE::ExtentU32 TexCount;
        VKE::Scene::CTerrain::CalcTextureCount( *pDesc, &TexCount );
        pDesc->Heightmap.vvFileNames.Resize(
            TexCount.y, VKE::Scene::STerrainDesc::StringArray( TexCount.x ) );
        pDesc->Heightmap.vvNormalNames.Resize(
            TexCount.y, VKE::Scene::STerrainDesc::StringArray( TexCount.x ) );
        char buff[ 128 ];
        for( uint32_t y = 0; y < TexCount.height; ++y )
        {
            for( uint32_t x = 0; x < TexCount.width; ++x )
            {
                vke_sprintf( buff, 128,
                             "data/textures/terrain/heightmap16k_%d_%d.png", x,
                             y );
                pDesc->Heightmap.vvFileNames[ x ][ y ] = buff;

                vke_sprintf( buff, 128,
                             "data/textures/terrain/heightmap16k_normal_%d_%d.dds", x,
                             y );
                pDesc->Heightmap.vvNormalNames[ x ][ y ] = buff;
            }
        }
    }
    void SliceTextures( VKE::RenderSystem::CDeviceContext* pCtx,
                        const VKE::Scene::STerrainDesc& Desc )
    {
        if( VKE::Platform::File::Exists( "data/textures/terrain/heightmap16k_0_0.png" ) )
        {
            return;
        }
        auto pImgMgr = pCtx->GetRenderSystem()->GetEngine()->GetImageManager();
        VKE::Core::SLoadFileInfo Info;
        Info.CreateInfo.async = false;
        Info.FileInfo.pFileName = "data/textures/terrain/heightmap16k.png";
        auto hHeightmap = pImgMgr->Load( Info );
        Info.FileInfo.pFileName =
            "data/textures/terrain/heightmap16k_normal.dds";
        auto hNormal =
            pCtx->GetRenderSystem()->GetEngine()->GetImageManager()->Load(
                Info );
        VKE::ExtentU32 TexCount;
        VKE::Scene::CTerrain::CalcTextureCount( Desc, &TexCount );
        const uint32_t texCount = TexCount.width * TexCount.height;
        if( hHeightmap != VKE::INVALID_HANDLE )
        {
            VKE::Utils::TCDynamicArray<VKE::Core::ImageHandle, 128> vImages(
                texCount );
            VKE::Core::SSliceImageInfo SliceInfo;
            SliceInfo.hSrcImage = hHeightmap;
            for( uint16_t y = 0; y < TexCount.height; ++y )
            {
                for( uint16_t x = 0; x < TexCount.width; ++x )
                {
                    VKE::Core::SSliceImageInfo::SRegion Region;
                    Region.Size = { ( VKE::image_dimm_t )Desc.TileSize.max + 1u,
                                    ( VKE::image_dimm_t )Desc.TileSize.max +
                                        1u };
                    Region.Offset.x = x * Desc.TileSize.max;
                    Region.Offset.y = y * Desc.TileSize.max;
                    SliceInfo.vRegions.PushBack( Region );
                }
            }
            if( VKE_SUCCEEDED( pImgMgr->Slice( SliceInfo, &vImages[ 0 ] ) ) )
            {
                VKE::Result res;
                for( uint32_t i = 0; i < vImages.GetCount(); ++i )
                {
                    char pName[ 128 ];
                    uint32_t x, y;
                    VKE::Math::Map1DarrayIndexTo2DArrayIndex(
                        i, TexCount.width, TexCount.height, &x, &y );
                    vke_sprintf( pName, 128,
                                 "data/textures/terrain/heightmap16k_%d_%d.png",
                                 x, y );
                    VKE::Core::SSaveImageInfo SaveInfo;
                    SaveInfo.format = VKE::Core::ImageFileFormats::PNG;
                    SaveInfo.hImage = vImages[ i ];
                    SaveInfo.pFileName = pName;
                    res = pImgMgr->Save( SaveInfo );
                }
            }
        }
        if( hNormal != VKE::INVALID_HANDLE )
        {
            VKE::Utils::TCDynamicArray<VKE::Core::ImageHandle, 128> vImages(
                texCount );
            VKE::Core::SSliceImageInfo SliceInfo;
            SliceInfo.hSrcImage = hNormal;
            for( uint16_t y = 0; y < TexCount.height; ++y )
            {
                for( uint16_t x = 0; x < TexCount.width; ++x )
                {
                    VKE::Core::SSliceImageInfo::SRegion Region;
                    Region.Size = { ( VKE::image_dimm_t )Desc.TileSize.max + 1u,
                                    ( VKE::image_dimm_t )Desc.TileSize.max +
                                        1u };
                    Region.Offset.x = x * Desc.TileSize.max;
                    Region.Offset.y = y * Desc.TileSize.max;
                    SliceInfo.vRegions.PushBack( Region );
                }
            }
            if( VKE_SUCCEEDED( pImgMgr->Slice( SliceInfo, &vImages[ 0 ] ) ) )
            {
                VKE::Result res;
                for( uint32_t i = 0; i < vImages.GetCount(); ++i )
                {
                    char pName[ 128 ];
                    uint32_t x, y;
                    VKE::Math::Map1DarrayIndexTo2DArrayIndex(
                        i, TexCount.width, TexCount.height, &x, &y );
                    vke_sprintf( pName, 128,
                                 "data/textures/terrain/heightmap16k_normal_%d_%d.dds",
                                 x, y );
                    VKE::Core::SSaveImageInfo SaveInfo;
                    SaveInfo.format = VKE::Core::ImageFileFormats::DDS;
                    SaveInfo.hImage = vImages[ i ];
                    SaveInfo.pFileName = pName;
                    res = pImgMgr->Save( SaveInfo );
                }
            }
        }
    }
    bool Init( const CSampleFramework& Sample )
    {
        // pCtx->GetRenderSystem()->GetEngine()->GetInputSystem()->SetListener(
        // pInputListener );
        pWindow = Sample.m_vpWindows[ 0 ];
        pWindow->GetInputSystem().SetListener( pInputListener );
        auto pCtx = Sample.m_vpDeviceContexts[ 0 ];

        VKE::RenderSystem::SSimpleRenderPassDesc PassDesc;
        {
            VKE::RenderSystem::SRenderTargetDesc ColorRT, DepthRT;
            ColorRT.Size = pCtx->GetGraphicsContext( 0 )->GetSwapChain()->GetSize();
            ColorRT.renderPassUsage = VKE::RenderSystem::RenderTargetRenderPassOperations::COLOR_STORE;
            ColorRT.beginState = VKE::RenderSystem::TextureStates::COLOR_RENDER_TARGET;
            ColorRT.endState = VKE::RenderSystem::TextureStates::SHADER_READ;
            ColorRT.format = VKE::RenderSystem::Formats::R8G8B8A8_UNORM;
            ColorRT.memoryUsage =
                VKE::RenderSystem::MemoryUsages::TEXTURE | VKE::RenderSystem::MemoryUsages::GPU_ACCESS;
            ColorRT.type = VKE::RenderSystem::TextureTypes::TEXTURE_2D;
            ColorRT.usage = VKE::RenderSystem::TextureUsages::COLOR_RENDER_TARGET;
            ColorRT.SetDebugName( "TerrainColorRT" );
            pCtx->CreateRenderTarget( ColorRT );

            

            DepthRT = ColorRT;
            DepthRT.renderPassUsage = VKE::RenderSystem::RenderTargetRenderPassOperations::DEPTH_STENCIL_CLEAR;
            DepthRT.beginState = VKE::RenderSystem::TextureStates::DEPTH_STENCIL_RENDER_TARGET;
            DepthRT.endState = VKE::RenderSystem::TextureStates::DEPTH_STENCIL_RENDER_TARGET;
            DepthRT.format = VKE::RenderSystem::FORMAT::D24_UNORM_S8_UINT;
            DepthRT.usage = VKE::RenderSystem::TextureUsages::DEPTH_STENCIL_RENDER_TARGET;
            DepthRT.SetDebugName( "TerrainDepthRT" );
            auto hDepthRT = pCtx->CreateRenderTarget( DepthRT );
            auto pDepthRT = pCtx->GetRenderTarget( hDepthRT );
            auto pTexView = pCtx->GetTextureView( pDepthRT->GetTextureView() );

            VKE::RenderSystem::SRenderTargetInfo DepthRTInfo, ColorRTInfo;
            DepthRTInfo.ClearColor.DepthStencil = { 1, 0 };
            DepthRTInfo.hDDIView = pTexView->GetDDIObject();
            DepthRTInfo.state = VKE::RenderSystem::TextureStates::DEPTH_RENDER_TARGET;
            DepthRTInfo.renderPassOp = VKE::RenderSystem::RenderTargetRenderPassOperations::DEPTH_STENCIL_CLEAR;
            
            ColorRTInfo.ClearColor.Color = { 0, 0, 0, 1 };
            ColorRTInfo.renderPassOp = VKE::RenderSystem::RenderTargetRenderPassOperations::COLOR_CLEAR_STORE;
            ColorRTInfo.state = VKE::RenderSystem::TextureStates::COLOR_RENDER_TARGET;

            VKE::RenderSystem::SSetRenderTargetInfo RTInfo;
            RTInfo.RenderTarget = { hDepthRT, VKE::RenderSystem::RES_ID_HANDLE };
            RTInfo.ClearColor = VKE::RenderSystem::SClearValue( 1.0f, 0 );
            RTInfo.renderPassOp = VKE::RenderSystem::RenderTargetRenderPassOperations::DEPTH_STENCIL_CLEAR;
            RTInfo.state = VKE::RenderSystem::TextureStates::DEPTH_RENDER_TARGET;
            PassDesc.SetDebugName( "Terrain" );
            PassDesc.vRenderTargets.PushBack( RTInfo );
            PassDesc.RenderArea.Position = { 0, 0 };
            PassDesc.RenderArea.Size = ColorRT.Size;

            m_RenderPassInfo.SetDebugName( "Terrain" );
            m_RenderPassInfo.RenderArea.Position = { 0, 0 };
            m_RenderPassInfo.RenderArea.Size = ColorRT.Size;
            m_RenderPassInfo.DepthRenderTargetInfo = DepthRTInfo;
            m_RenderPassInfo.vColorRenderTargetInfos.PushBack( ColorRTInfo );
            auto hPass = pCtx->CreateRenderPass( PassDesc );
            m_pRenderPass = pCtx->GetRenderPass( hPass );
        }

        VKE::Scene::SSceneDesc SceneDesc;
        SceneDesc.pDeviceContext = pCtx;
        auto pWorld = pCtx->GetRenderSystem()->GetEngine()->World();
        pScene = pWorld->CreateScene( SceneDesc );
        VKE::Scene::SCameraDesc CamDesc;
        CamDesc.Name = "Debug";
        CamDesc.ClipPlanes = { 1.0f, 10000.0f };
        CamDesc.Viewport = pWindow->GetSwapChain()->GetSize();
        CamDesc.vecPosition = {0, 0.1f, 0};
        pDebugCamera = pScene->CreateCamera( CamDesc );
        {
            pDebugCamera->SetPosition( VKE::Math::CVector3( 0, 500, 0 ) );
            pDebugCamera->SetLookAt( { 0, 0, 0 } );
            pDebugCamera->Update( 0 );
            pScene->SetViewCamera( pDebugCamera );
        }
        CamDesc.Name = "Render";
        pCamera = pScene->CreateCamera( CamDesc );
        {
            pCamera->SetPosition( VKE::Math::CVector3( 0, 500, 0 ) );
            pCamera->SetLookAt( VKE::Math::CVector3( 0, 0, 0 ) );
            pCamera->Update( 0 );
            pScene->SetCamera( pCamera );
            pScene->AddDebugView( &pCamera );
        }
        pInputListener->pCamera = pDebugCamera;
        VKE::Scene::STerrainDesc TerrainDesc;
        TerrainDesc.size = 16000;
        // TerrainDesc.size = 1024;
        // TerrainDesc.size = 256;
        TerrainDesc.Height = { -200, 500 };
        TerrainDesc.TileSize = { 32, 2048 };
        TerrainDesc.vertexDistance = 1.0f;
        TerrainDesc.lodCount = 7;
        TerrainDesc.maxViewDistance = CamDesc.ClipPlanes.end;
        SliceTextures( pCtx, TerrainDesc );
        LoadTextures( pCtx, &TerrainDesc );
        /*TerrainDesc.vDDIRenderPasses.PushBack(
            pCtx->GetGraphicsContext( 0 )->GetSwapChain()->GetDDIRenderPass() );*/
        //TerrainDesc.vRenderPasses.PushBack( hPass );
        pTerrain = pScene->CreateTerrain( TerrainDesc, pCtx );

        {
            pInputListener->vecLightPos.y = 500;
            VKE::Scene::SLightDesc LightDesc;
            LightDesc.Name = "TerrainLight";
            LightDesc.vecPosition = pInputListener->vecLightPos;
            LightDesc.vecDirection = { 0, -1, 0 };
            m_pLight = pScene->CreateLight( LightDesc );
            pScene->AddDebugView( &m_pLight );
        }

        //pWindow->Update();
        return pTerrain.IsValid();
    }

    void UpdateCamera( VKE::RenderSystem::CGraphicsContext* pCtx )
    {
        /*if( !pCtx->GetSwapChain()->GetWindow()->HasFocus() )
        {
            return;
        }*/
        const auto& InputState =
            pCtx->GetSwapChain()->GetWindow()->GetInputSystem().GetState();
        if( InputState.Keyboard.IsKeyDown( VKE::Input::Keys::W ) )
        {
            pDebugCamera->Move( pInputListener->vecDist * pInputListener->vecSpeed * pDebugCamera->GetDirection() * 5 );
        }
        else if( InputState.Keyboard.IsKeyDown( VKE::Input::Keys::S ) )
        {
            pDebugCamera->Move( pInputListener->vecDist * pInputListener->vecSpeed * -pDebugCamera->GetDirection() *
                                5 );
        }
        else if( InputState.Keyboard.IsKeyDown( VKE::Input::Keys::Q ) )
        {
            pDebugCamera->Move( (pInputListener->vecDist * pInputListener->vecSpeed * (VKE::Math::CVector3::Y ) * 5) );
        }
        else if( InputState.Keyboard.IsKeyDown( VKE::Input::Keys::Z ) )
        {
            pDebugCamera->Move(  ( pInputListener->vecDist * pInputListener->vecSpeed *
                                                           (VKE::Math::CVector3::NEGATIVE_Y ) * 5 ) );
        }


        if( InputState.Keyboard.IsKeyDown( VKE::Input::Keys::R ) )
        {
            pCamera->Move( pInputListener->vecDist * pInputListener->vecSpeed *
                           pCamera->GetDirection() );
        }
        else if( InputState.Keyboard.IsKeyDown( VKE::Input::Keys::F ) )
        {
            pCamera->Move( pInputListener->vecDist * pInputListener->vecSpeed *
                           -pCamera->GetDirection() );
        }

        m_pLight->SetPosition( pInputListener->vecLightPos );
        auto pWnd = pCtx->GetDeviceContext()
                        ->GetRenderSystem()
                        ->GetEngine()
                        ->GetWindow();
        char pText[ 128 ];
        auto& Pos = pDebugCamera->GetPosition();
        auto fps = pCtx->GetDeviceContext()->GetMetrics().avgFps;
        auto fps2 = pCtx->GetDeviceContext()->GetMetrics().currentFps;
        vke_sprintf( pText, 128, "%.3f, %.3f, %.3f / %.1f - %.1f", Pos.x, Pos.y,
                     Pos.z, fps, fps2 );
        pWnd->SetText( pText );
    }
    bool OnRenderFrame( VKE::RenderSystem::CGraphicsContext* pCtx ) override
    {
        pWindow->Update();
        UpdateCamera( pCtx );
        pScene->Update( {} );
        //pTerrain->Update( pCtx );
        auto pCommandBuffer = pCtx->BeginFrame();
        pCtx->GetSwapChain()->BeginFrame( pCommandBuffer );
        //pCtx->BindDefaultRenderPass();
        //m_RenderPassInfo.vColorRenderTargetInfos[ 0 ].hView = pCtx->GetSwapChain()->GetCurrentBackBuffer().pAcquiredElement->hDDITextureView;
        auto hRT = pCtx->GetSwapChain()->GetCurrentBackBuffer().hRenderTarget;
        m_pRenderPass->SetRenderTarget( 0, VKE::RenderSystem::SSetRenderTargetInfo( hRT ) );
        pCommandBuffer->BeginRenderPass( m_pRenderPass );
        pScene->Render( pCtx );
        pTerrain->Render( pCommandBuffer );
        pScene->RenderDebug( pCommandBuffer );
        pCommandBuffer->EndRenderPass();
        pCtx->GetSwapChain()->EndFrame( pCommandBuffer );
        pCtx->GetDeviceContext()->SynchronizeTransferContext();
        pCtx->EndFrame();
        return true;
    }
};
int main()
{
    VKE_DETECT_MEMORY_LEAKS();
    // VKE::Platform::Debug::BreakAtAllocation(15874);
    {
        CSampleFramework Sample;
        SSampleCreateDesc Desc;
        VKE::RenderSystem::EventListeners::IGraphicsContext*
            apListeners[ 1 ] = { VKE_NEW SGfxContextListener() };
        Desc.ppGfxListeners = apListeners;
        Desc.gfxListenerCount = 1;
        Desc.WindowSize = { 2560, 1440 };
        if( Sample.Create( Desc ) )
        {
            SGfxContextListener* pListener =
                reinterpret_cast<SGfxContextListener*>( apListeners[ 0 ] );
            if( pListener->Init( Sample ) )
            {
                Sample.Start();
            }
        }
        VKE_DELETE( apListeners[ 0 ] );
        Sample.Destroy();
    }
    return 0;
}
