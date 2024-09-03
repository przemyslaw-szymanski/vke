
#include "../CSampleFramework.h"
#include "Vke/Core/Managers/CImageManager.h"

#define HEIGHTMAP_2PIX_BIGGER 1

struct SInputListener : public VKE::Input::EventListeners::IInput
{
    VKE::Scene::CameraPtr pCamera = nullptr;
    const float meterPerSecond = 0.2777f;

    VKE::Math::CVector3 vecSpeed = VKE::Math::CVector3( 10.0f * meterPerSecond );
    VKE::Math::CVector3 vecDir = VKE::Math::CVector3::Z;
    VKE::Math::CVector3 vecDist = VKE::Math::CVector3( 1.0f );
    VKE::Input::MousePosition LastMousePos = { 0, 0 };
    VKE::Input::MousePosition MouseDir = { 0, 0 };
    VKE::ExtentF32 MouseAngle = { 0.0f, 0.0f };
    VKE::Math::CVector3 vecYawPitchRoll = { 0.0f };
    VKE::Math::CVector3 vecLightPos = { 2, 22, 0 };
    float dbgCameraSpeed = 1;
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
        float x = VKE::Math::ConvertToRadians( ( float )Mouse.Move.x ) * scale;
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
    //VKE::RenderSystem::IFrameGraph* pFrameGraph;
    VKE::RenderSystem::CFrameGraph* pFrameGraph;
    VKE::Scene::TerrainPtr pTerrain;
    SInputListener* pInputListener;
    VKE::RenderSystem::SBeginRenderPassInfo2 m_RenderPassInfo;
    VKE::RenderSystem::RenderPassRefPtr m_pRenderPass;
    VKE::Scene::LightRefPtr m_pLight;
    VKE::WindowPtr pWindow;
    VKE::Utils::CTimer Timer;
    float frameTime = 0;

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
        auto TexCount = VKE::Scene::CTerrain::CalcTextureCount( *pDesc );
        /*pDesc->Heightmap.vTextures.Resize(
            TexCount.y, VKE::Scene::STerrainDesc::StringArray( TexCount.x ) );
        pDesc->Heightmap.vt.Resize(
            TexCount.y, VKE::Scene::STerrainDesc::StringArray( TexCount.x ) );*/
        pDesc->vTileTextures.Resize( TexCount.x * TexCount.y );
        char buff[ 128 ];
        for( uint32_t y = 0; y < TexCount.height; ++y )
        {
            for( uint32_t x = 0; x < TexCount.width; ++x )
            {
                vke_sprintf( buff, 128,
                             "data/textures/terrain/heightmap16k_%d_%d.png", x,
                             y );
                //pDesc->Heightmap.vvFileNames[ x ][ y ] = buff;
                auto& Textures = pDesc->vTileTextures[VKE::Math::Map2DArrayIndexTo1DArrayIndex(x, y, TexCount.width)];
                Textures.Heightmap.Format( "data/textures/terrain/heightmap16k_%d_%d.png", x, y );
                //Textures.HeightmapNormal.Format( "data/textures/terrain/heightmap16k_%d_%d.png", x, y );
                Textures.vSplatmaps.Resize( 1 );
                Textures.vSplatmaps[0].Format( "data/textures/terrain/splat01_%d_%d.dds", x, y );

                vke_sprintf( buff, 128,
                             "data/textures/terrain/heightmap16k_normal_%d_%d.dds", x,
                             y );
                //pDesc->Heightmap.vvNormalNames[ x ][ y ] = buff;
            }
        }

    }
    void SliceTextures( VKE::RenderSystem::CDeviceContext* pCtx,
                        const VKE::Scene::STerrainDesc& Desc,
        const char* pFileName, const char* pFileToCheckExists,
        const char* pSlicedFileNameFormat,
        uint32_t sliceRegionSize, uint16_t sliceRegionBias)
    {

        if( VKE::Platform::File::Exists( pFileToCheckExists ) )
        {
            return;
        }
        if( !VKE::Platform::File::Exists( pFileName ) )
        {
            return;
        }
        auto pImgMgr = pCtx->GetRenderSystem()->GetEngine()->GetImageManager();
        VKE::Core::SLoadFileInfo Info;
        Info.CreateInfo.flags = VKE::Core::CreateResourceFlags::DEFAULT;
        Info.FileInfo.FileName = pFileName;
        VKE::Core::ImageHandle hHeightmap;
        auto res = pImgMgr->Load( Info, &hHeightmap );
        //Info.FileInfo.pFileName = "data/textures/terrain/heightmap16k_normal.dds";
        //res = pCtx->GetRenderSystem()->GetEngine()->GetImageManager()->Load( Info, &hNormal );
        auto TexCount = VKE::Scene::CTerrain::CalcTextureCount( Desc );
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
                    Region.Size = { ( VKE::image_dimm_t )sliceRegionSize,
                                    ( VKE::image_dimm_t )sliceRegionSize };
                    Region.Offset.x = x * Desc.TileSize.max - ((x == 0)? 0 : 1 * sliceRegionBias);
                    Region.Offset.y = y * Desc.TileSize.max - ( ( y == 0 ) ? 0 : 1 * sliceRegionBias );
                    SliceInfo.vRegions.PushBack( Region );
                }
            }
            if( VKE_SUCCEEDED( pImgMgr->Slice( SliceInfo, &vImages[ 0 ] ) ) )
            {
                for( uint32_t i = 0; i < vImages.GetCount(); ++i )
                {
                    char pName[ 128 ];
                    uint32_t x, y;
                    VKE::Math::Map1DarrayIndexTo2DArrayIndex( i, TexCount.width, TexCount.height, &x, &y );
                    vke_sprintf( pName, 128, pSlicedFileNameFormat, x, y );
                    VKE::Core::SSaveImageInfo SaveInfo;
                    //SaveInfo.format = VKE::Core::ImageFileFormats::PNG;
                    auto pImg = pImgMgr->GetImage( vImages[ i ] );
                    SaveInfo.format = pImg->GetDesc().fileFormat;
                    SaveInfo.hImage = vImages[ i ];
                    SaveInfo.pFileName = pName;
                    res = pImgMgr->Save( SaveInfo );
                }
            }
        }
        /*if( hNormal != VKE::INVALID_HANDLE )
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
        }*/
    }
    bool Init( const CSampleFramework& Sample )
    {
        // pCtx->GetRenderSystem()->GetEngine()->GetInputSystem()->SetListener(
        // pInputListener );
        pWindow = Sample.m_vpWindows[ 0 ];
        pWindow->GetInputSystem().SetListener( pInputListener );
        auto pDevice = Sample.m_vpDeviceContexts[ 0 ];
        auto pCtx = pDevice->GetGraphicsContext( 0 );
        auto pCmdBuffer = pCtx->GetCommandBuffer();

        VKE::RenderSystem::SSimpleRenderPassDesc PassDesc;
        {
            VKE::RenderSystem::SRenderTargetDesc ColorRT, DepthRT;
            ColorRT.Size = pCtx->GetSwapChain()->GetSize();
            ColorRT.renderPassUsage = VKE::RenderSystem::RenderTargetRenderPassOperations::COLOR_STORE;
            ColorRT.beginState = VKE::RenderSystem::TextureStates::COLOR_RENDER_TARGET;
            ColorRT.endState = VKE::RenderSystem::TextureStates::SHADER_READ;
            ColorRT.format = VKE::RenderSystem::Formats::R8G8B8A8_UNORM;
            ColorRT.memoryUsage =
                VKE::RenderSystem::MemoryUsages::TEXTURE | VKE::RenderSystem::MemoryUsages::GPU_ACCESS;
            ColorRT.type = VKE::RenderSystem::TextureTypes::TEXTURE_2D;
            ColorRT.usage = VKE::RenderSystem::TextureUsages::COLOR_RENDER_TARGET;
            ColorRT.SetDebugName( "TerrainColorRT" );
            ColorRT.Name = "TerrainColorRT";
            pDevice->CreateRenderTarget( ColorRT );

            

            DepthRT = ColorRT;
            DepthRT.renderPassUsage = VKE::RenderSystem::RenderTargetRenderPassOperations::DEPTH_STENCIL_CLEAR;
            DepthRT.beginState = VKE::RenderSystem::TextureStates::DEPTH_STENCIL_RENDER_TARGET;
            DepthRT.endState = VKE::RenderSystem::TextureStates::DEPTH_STENCIL_RENDER_TARGET;
            DepthRT.format = VKE::RenderSystem::FORMAT::D24_UNORM_S8_UINT;
            DepthRT.usage = VKE::RenderSystem::TextureUsages::DEPTH_STENCIL_RENDER_TARGET;
            DepthRT.SetDebugName( "TerrainDepthRT" );
            DepthRT.Name = "TerrainDepthRT";
            auto hDepthRT = pDevice->CreateRenderTarget( DepthRT );
            //auto pDepthRT = pDevice->GetRenderTarget( hDepthRT );
            //auto pTexView = pDevice->GetTextureView( pDepthRT->GetTextureView() );

            /*VKE::RenderSystem::SRenderTargetInfo DepthRTInfo, ColorRTInfo;
            DepthRTInfo.ClearColor.DepthStencil = { 1, 0 };
            DepthRTInfo.hDDIView = pTexView->GetDDIObject();
            DepthRTInfo.state = VKE::RenderSystem::TextureStates::DEPTH_RENDER_TARGET;
            DepthRTInfo.renderPassOp = VKE::RenderSystem::RenderTargetRenderPassOperations::DEPTH_STENCIL_CLEAR;*/
            
            /*ColorRTInfo.ClearColor.Color = { 0.5, 0.5, 0.5, 1 };
            ColorRTInfo.renderPassOp = VKE::RenderSystem::RenderTargetRenderPassOperations::COLOR_CLEAR_STORE;
            ColorRTInfo.state = VKE::RenderSystem::TextureStates::COLOR_RENDER_TARGET;*/

            VKE::RenderSystem::SSetRenderTargetInfo RTInfo;
            RTInfo.RenderTarget = { hDepthRT, VKE::RenderSystem::RES_ID_HANDLE };
            RTInfo.ClearColor = VKE::RenderSystem::SClearValue( 1.0f, 0 );
            RTInfo.renderPassOp = VKE::RenderSystem::RenderTargetRenderPassOperations::DEPTH_STENCIL_CLEAR;
            RTInfo.state = VKE::RenderSystem::TextureStates::DEPTH_RENDER_TARGET;
            PassDesc.SetDebugName( "Terrain" );
            PassDesc.vRenderTargets.PushBack( RTInfo );
            PassDesc.RenderArea.Position = { 0, 0 };
            PassDesc.RenderArea.Size = ColorRT.Size;
            PassDesc.Name = "Terrain";

            //m_RenderPassInfo.SetDebugName( "Terrain" );
            ////m_RenderPassInfo.
            //m_RenderPassInfo.RenderArea.Position = { 0, 0 };
            //m_RenderPassInfo.RenderArea.Size = ColorRT.Size;
            //m_RenderPassInfo.DepthRenderTargetInfo = DepthRTInfo;
            //m_RenderPassInfo.vColorRenderTargetInfos.PushBack( ColorRTInfo );
            auto hPass = pDevice->CreateRenderPass( PassDesc );
            m_pRenderPass = pDevice->GetRenderPass( hPass );
        }

        VKE::Scene::SSceneDesc SceneDesc;
        //SceneDesc.pDeviceContext = pDevice;
        SceneDesc.pCommandBuffer = pCmdBuffer;
        auto pWorld = pDevice->GetRenderSystem()->GetEngine()->World();
        pScene = pWorld->CreateScene( SceneDesc );
        VKE::Scene::SCameraDesc CamDesc;
        CamDesc.Name = "Debug";
        CamDesc.ClipPlanes = { 1.0f, 10000.0f };
        CamDesc.Viewport = pWindow->GetSwapChain()->GetSize();
        CamDesc.vecPosition = {0, 0.1f, 0};
        pDebugCamera = pScene->CreateCamera( CamDesc );
        {
            pDebugCamera->SetPosition( VKE::Math::CVector3( 30, 255, -0 ) );
            pDebugCamera->SetLookAt( { 0, 0, 0 } );
            pDebugCamera->Update( 0 );
            pScene->SetViewCamera( pDebugCamera );
        }
        CamDesc.Name = "Render";
        pCamera = pScene->CreateCamera( CamDesc );
        {
            pCamera->SetPosition( VKE::Math::CVector3( 0, 290, 0 ) );
            pCamera->SetLookAt( VKE::Math::CVector3( 0, 0, 0 ) );
            pCamera->Update( 0 );
            pScene->SetCamera( pCamera );
            pScene->AddDebugView( pCmdBuffer, &pCamera );
        }
        pInputListener->pCamera = pDebugCamera;
        VKE::Scene::STerrainDesc TerrainDesc;
        {
            TerrainDesc.size = 16000;
            // TerrainDesc.size = 1024;
            // TerrainDesc.size = 256;
            TerrainDesc.Height = { -200, 500 };
            TerrainDesc.TileSize = { 32, 2048 };
            TerrainDesc.vertexDistance = 1.0f;
            //TerrainDesc.lodCount = 3;
            TerrainDesc.maxViewDistance = CamDesc.ClipPlanes.end;
            TerrainDesc.HeightmapOffset = { HEIGHTMAP_2PIX_BIGGER, HEIGHTMAP_2PIX_BIGGER };
            TerrainDesc.lodTreshold = 20;
            TerrainDesc.Tesselation.Factors = { 0, 0 };
            TerrainDesc.Tesselation.maxDistance = 2048;
            TerrainDesc.Tesselation.quadMode = true;
            uint32_t sliceRegionSize = TerrainDesc.TileSize.max + 1u + HEIGHTMAP_2PIX_BIGGER * 2;
            SliceTextures( pDevice, TerrainDesc,
                "data/textures/terrain/heightmap16k.png",
                           "data/textures/terrain/heightmap16k_0_0.png",
                           "data/textures/terrain/heightmap16k_%d_%d.png",
                sliceRegionSize,
                HEIGHTMAP_2PIX_BIGGER );
            SliceTextures( pDevice, TerrainDesc,
                "data/textures/terrain/splat01.dds",
                           "data/textures/terrain/splat01_0_0.dds",
                "data/textures/terrain/splat01_%d_%d.dds",
                TerrainDesc.TileSize.max, 0 );
            //LoadTextures( pDevice, &TerrainDesc );
            /*TerrainDesc.vDDIRenderPasses.PushBack(
                pCtx->GetGraphicsContext( 0 )->GetSwapChain()->GetDDIRenderPass() );*/
            // TerrainDesc.vRenderPasses.PushBack( hPass );
            
        }
        {
            pTerrain = pScene->CreateTerrain( TerrainDesc, pCmdBuffer );
            uint16_t w = ( uint16_t )( TerrainDesc.size / TerrainDesc.TileSize.max ) + ((TerrainDesc.size % TerrainDesc.TileSize.max) > 0);
            uint16_t h = w;
            for(uint16_t y = 0; y < h; y++)
            {
                for(uint16_t x = 0; x < w; ++x)
                {
                    VKE::Scene::SLoadTerrainTileInfo Info;
                    Info.Heightmap.Format(  "data/textures/terrain/heightmap16k_%d_%d.png", x, y );
                    //Info.HeightmapNormal.Format();
                    Info.vSplatmaps.Resize( 1 );
                    Info.vSplatmaps[ 0 ].Format( "data/textures/terrain/splat01_%d_%d.dds", x, y );
                    Info.Position = { x, y };
                    Info.vDiffuseTextures =
                    { 
                        "data/textures/terrain/snow_02_4k/textures/snow_02_diff_4k.jpg",
                        "data/textures/terrain/rock_01_4k/textures/rock_01_diff_4k.jpg",
                        "data/textures/terrain/coral_mud_01_1k.blend/textures/coral_mud_01_diff_1k.jpg",
                        "data/textures/terrain/forest_leaves_02_1k/textures/forest_leaves_02_diffuse_1k.jpg",
                    };
                    
                    //pTerrain->LoadTile( Info, pCmdBuffer );
                }
            }
        }
        {
            pInputListener->vecLightPos.y = 500;
            VKE::Scene::SLightDesc LightDesc;
            LightDesc.Name = "TerrainLight";
            LightDesc.vecPosition = pInputListener->vecLightPos;
            LightDesc.vecDirection = { 0, -1, 0 };
            m_pLight = pScene->CreateLight( LightDesc );
            pScene->AddDebugView( pCmdBuffer, &m_pLight );
        }

        VKE::RenderSystem::SFrameGraphDesc FrameGraphDesc;
        FrameGraphDesc.Name = "Simple";
        FrameGraphDesc.pDevice = Sample.m_vpDeviceContexts[ 0 ];
        FrameGraphDesc.apContexts[ VKE::RenderSystem::ContextTypes::GENERAL ] = Sample.m_vpGraphicsContexts[ 0 ];
        FrameGraphDesc.apContexts[ VKE::RenderSystem::ContextTypes::TRANSFER ]
            = Sample.m_vpDeviceContexts[ 0 ]->GetTransferContext();

        pFrameGraph = Sample.m_pRenderSystem->CreateFrameGraph( FrameGraphDesc );
        auto pSwapBufferPass = pFrameGraph->CreatePass( { .pName = "SwapBuffers",  } );
        auto pBeginFramePass = pFrameGraph->CreatePass( { .pName = "BeginFrame", } );
        auto pEndFramePass = pFrameGraph->CreatePass( { .pName = "EndFrame", } );
        auto pExecuteFrame = pFrameGraph->CreateExecutePass( { .pName = "ExecuteFrame", .pThread = "ExecuteFrame" } );
        auto pPresent = pFrameGraph->CreatePresentPass( { .pName = "PresentFrame", .pThread = "PresentFrame" } );
        auto pTextureLoadPass = pFrameGraph->CreatePass( { .pName = "TextureLoad",  } );
        auto pBufferLoadPass = pFrameGraph->CreatePass( { .pName = "BufferLoad", } );
        auto pBufferUploadPass = pFrameGraph->CreatePass( { .pName = "BufferUpload", .pCommandBuffer = "Upload" } );
        auto pCompileShaderPass = pFrameGraph->CreatePass( { .pName = "CompileShader", } );
        auto pTextureUploadPass = pFrameGraph->CreatePass( { .pName = "TextureUpload", .pCommandBuffer = "Upload" } );
        auto pTextureGenMipmapPass = pFrameGraph->CreatePass( { .pName = "GenMipmaps" } );
        auto pLoadDataPass = pFrameGraph->CreatePass( { .pName = "LoadData" } );
        auto pUploadDataPass = pFrameGraph->CreatePass( { .pName = "UploadData" } );
        auto pSceneUpdatePass = pFrameGraph->CreatePass( { .pName = "SceneUpdate" } );
        auto pUpdatePass = pFrameGraph->CreatePass( { .pName = "Update" } );
        auto pExecuteUploadPass = pFrameGraph->CreateExecutePass( { .pName = "ExecuteUpload" } );
        auto pExecuteUpdatePass = pFrameGraph->CreateExecutePass( { .pName = "ExecuteUpdate" } );
        auto pRenderFramePass = pFrameGraph->CreatePass( { .pName = "RenderFrame" } );
        auto pFinishFramePass = pFrameGraph->CreatePass( { .pName = "FinishFrame" } );
        //auto pCreateResourcePass
          //  = pFrameGraph->CreateCustomPass<VKE::RenderSystem::CFrameGraphMultiWorkloadNode>( { .pName = "CreateResource" }, nullptr );

        pFrameGraph->SetRootNode( pSwapBufferPass )
            ->AddNext( pBeginFramePass )
            ->AddNext( pLoadDataPass )
                ->AddSubpass( pTextureLoadPass )
                ->AddSubpass( pBufferLoadPass )
                ->AddSubpass( pCompileShaderPass )
            ->AddNext(pUploadDataPass)
                ->AddSubpass( pTextureUploadPass )
                ->AddSubpass( pBufferUploadPass )
                ->AddSubpass( pTextureGenMipmapPass )
            ->AddNext( pUpdatePass )
                ->AddSubpass( pSceneUpdatePass )
            ->AddNext( pRenderFramePass )
            ->AddNext( pEndFramePass )
                ->AddSubpass( pExecuteUploadPass )
                ->AddSubpass( pExecuteUpdatePass )
                ->AddSubpass( pExecuteFrame )
            ->AddNext( pPresent )
            ->AddNext( pFinishFramePass );

        pExecuteUploadPass
            ->AddToExecute( pTextureUploadPass )
            ->AddToExecute( pBufferUploadPass );

        pExecuteUpdatePass->AddToExecute( pUpdatePass );

        pRenderFramePass->WaitFor( { .pNode = pUpdatePass, .WaitOn = VKE::RenderSystem::WaitOnBits::THREAD } );

        pExecuteFrame->WaitFor( { .pNode = pSwapBufferPass, .WaitOn = VKE::RenderSystem::WaitOnBits::GPU } );
        pExecuteFrame->WaitFor( {.pNode = pExecuteUpdatePass, .WaitOn = VKE::RenderSystem::WaitOnBits::GPU } );
        pExecuteFrame->AddToExecute( pBeginFramePass );
        pExecuteFrame->AddToExecute( pTextureGenMipmapPass );
        pExecuteFrame->AddToExecute( pRenderFramePass );
        pExecuteFrame->AddToExecute( pEndFramePass );

        pPresent->WaitFor( {
            .pNode = pExecuteFrame,
            .frame = VKE::RenderSystem::WaitForFrames::CURRENT,
            .WaitOn = VKE::RenderSystem::WaitOnBits::GPU | VKE::RenderSystem::WaitOnBits::THREAD
            } );

        pBeginFramePass->WaitFor( { .pNode = pFinishFramePass,
                                    .frame = VKE::RenderSystem::WaitForFrames::LAST,
                                    .WaitOn = VKE::RenderSystem::WaitOnBits::THREAD } );
        pFinishFramePass->WaitFor( { .pNode = pEndFramePass, .WaitOn = VKE::RenderSystem::WaitOnBits::THREAD } );

        pSwapBufferPass->SetWorkload( [ & ]( VKE::RenderSystem::CFrameGraphNode* const pPass, uint8_t backBufferIdx )
        {
            VKE::Result ret = pPass->OnWorkloadBegin( backBufferIdx );
            if( VKE_SUCCEEDED( ret ) )
            {
                auto pCtx = pPass->GetContext()->Reinterpret<VKE::RenderSystem::CGraphicsContext>();
                auto pSwpChain = pCtx->GetSwapChain();
                ret = pSwpChain->SwapBuffers( pPass->GetGPUFence( backBufferIdx ), VKE::RenderSystem::NativeAPI::Null );
            }
            ret = pPass->OnWorkloadEnd( ret );
            return ret;
        } );
        pBeginFramePass->SetWorkload( [ & ]( VKE::RenderSystem::CFrameGraphNode* const pPass, uint8_t backBufferIdx )
                {
            VKE::Result ret = pPass->OnWorkloadBegin( backBufferIdx );
            if( VKE_SUCCEEDED( ret ) )
            {
                auto pCtx = pPass->GetContext()->Reinterpret<VKE::RenderSystem::CGraphicsContext>();
                auto pCmdBuffer = pPass->GetCommandBuffer();
                auto pSwpChain = pCtx->GetSwapChain();
                // pCmdBuffer->Begin();
                VKE::RenderSystem::STextureBarrierInfo Barrier;
                pSwpChain->GetBackBufferTexture()->SetState( VKE::RenderSystem::TextureStates::COLOR_RENDER_TARGET,
                                                             &Barrier );
                pCmdBuffer->Barrier( Barrier );
                pPass->AddSynchronization( pSwpChain->GetBackBufferGPUFence() );
            }
            ret = pPass->OnWorkloadEnd( ret );
                    return VKE::VKE_OK;
                });
        pEndFramePass->SetWorkload( [ & ]( VKE::RenderSystem::CFrameGraphNode* const pPass, uint8_t backBufferIdx )
                                {
            VKE::Result ret = pPass->OnWorkloadBegin( backBufferIdx );
            if( VKE_SUCCEEDED( ret ) )
            {
                auto pCtx = pPass->GetContext()->Reinterpret<VKE::RenderSystem::CGraphicsContext>();
                auto pCmdBuffer = pPass->GetCommandBuffer();
                // pCtx->GetSwapChain()->EndFrame( pPass->GetCommandBuffer() );
                // pFrameGraph->SetupPresent( pCtx->GetSwapChain() );
                auto pSwpChain = pCtx->GetSwapChain();
                VKE::RenderSystem::STextureBarrierInfo Barrier;
                pSwpChain->GetBackBufferTexture()->SetState( VKE::RenderSystem::TextureStates::PRESENT, &Barrier );
                pCmdBuffer->Barrier( Barrier );
                 ret = pFrameGraph->EndFrame();
            }
            ret = pPass->OnWorkloadEnd( ret );

                                    //pSwpChain->SwapBuffers();
            return ret;
                                });
        pFinishFramePass->SetWorkload( [ & ]( VKE::RenderSystem::CFrameGraphNode* const pPass, uint8_t backBufferIdx )
            {
            VKE::Result ret = pPass->OnWorkloadBegin( backBufferIdx );
                pPass->GetFrameGraph()->UpdateCounters();
            ret = pPass->OnWorkloadEnd( ret );
                return ret;
            } );

        //pFrameGraph->AddPass( { 
        //    .Name = "SwapBuffers",
        //    .ParentName = "Root",
        //    .ThreadName = "Main",
        //    .Workload = [&](VKE::RenderSystem::CFrameGraphNode* const pPass)
        //    {
        //        auto pCtx = pPass->GetContext()->Reinterpret<VKE::RenderSystem::CGraphicsContext>();
        //        auto pSwpChain = pCtx->GetSwapChain();
        //        auto pBackBuffer = pSwpChain->SwapBuffers(pPass->GetGPUFence());
        //        return pBackBuffer == nullptr? VKE::VKE_FAIL : VKE::VKE_OK;
        //    }
        //    } );
        //pFrameGraph->AddPass( 
        //    { 
        //        .Name = "BeginFrame",
        //        .ParentName = "Root",
        //        .CommandBufferName = "Main",
        //        .ExecutionName = "Main",
        //        .ThreadName = "Main",
        //        .contextType = VKE::RenderSystem::ContextTypes::GENERAL,
        //        .Flags = VKE::RenderSystem::FrameGraphNodeFlagBits::SIGNAL_GPU_FENCE,
        //        .Workload = [ & ]( VKE::RenderSystem::CFrameGraphNode* const pPass )
        //        {
        //            auto pCtx = pPass->GetContext()->Reinterpret<VKE::RenderSystem::CGraphicsContext>();
        //            auto pCmdBuffer = pPass->GetCommandBuffer();
        //            /*auto pBackBuffer = pCtx->GetSwapChain()->SwapBuffers( false );
        //            VKE_ASSERT( pBackBuffer != nullptr );
        //            if(pBackBuffer != nullptr)
        //            {
        //                auto pCmdBuffer = pPass->GetCommandBuffer();
        //                pCmdBuffer->Begin();
        //                pCtx->GetSwapChain()->BeginFrame( pCmdBuffer );
        //                pCtx->GetSwapChain()->EndFrame( pCmdBuffer );
        //                VKE_LOG( "begin frame" );
        //            }*/
        //            auto pSwpChain = pCtx->GetSwapChain();
        //            pCmdBuffer->Begin();
        //            VKE::RenderSystem::STextureBarrierInfo Barrier;
        //            pSwpChain->GetBackBufferTexture()->SetState( VKE::RenderSystem::TextureStates::COLOR_RENDER_TARGET,
        //                                                         &Barrier );
        //            pCmdBuffer->Barrier( Barrier );
        //            pPass->AddSynchronization( pSwpChain->GetBackBufferGPUFence() );
        //            return VKE::VKE_OK;
        //        }
        //    } );
        //pFrameGraph->AddPass( { .Name = "EndFrame",
        //                        .ParentName = "BeginFrame",
        //                        .CommandBufferName = "Main",
        //                        .ExecutionName = "Main",
        //                        .ThreadName = "Main",
        //                        .contextType = VKE::RenderSystem::ContextTypes::GENERAL,
        //                        .Workload = [ & ]( VKE::RenderSystem::CFrameGraphNode* const pPass )
        //                        {
        //                            auto pCtx = pPass->GetContext()->Reinterpret<VKE::RenderSystem::CGraphicsContext>();
        //                            auto pCmdBuffer = pPass->GetCommandBuffer();
        //                            //pCtx->GetSwapChain()->EndFrame( pPass->GetCommandBuffer() );
        //                            //pFrameGraph->SetupPresent( pCtx->GetSwapChain() );
        //                            auto pSwpChain = pCtx->GetSwapChain();
        //                            VKE::RenderSystem::STextureBarrierInfo Barrier;
        //                            pSwpChain->GetBackBufferTexture()->SetState(
        //                                VKE::RenderSystem::TextureStates::PRESENT, &Barrier );
        //                            pCmdBuffer->Barrier( Barrier );
        //                            

        //                            pSwpChain->SwapBuffers();
        //                            return pFrameGraph->EndFrame();
        //                        } } );
        //pFrameGraph->AddExecutePass(
        //    { 
        //        .Name = "ExecuteFrame",
        //        .ParentName = "EndFrame",
        //        .ExecutionName = "Main",
        //        .ThreadName = "Main",
        //        .WaitOnGPU = { "SwapBuffers" },
        //        .contextType = VKE::RenderSystem::ContextTypes::GENERAL
        //    } );
        //pFrameGraph->AddPresentPass(
        //    {
        //        .Name = "Present",
        //        .ParentName = "ExecuteFrame",
        //        .ThreadName = "Main",
        //        .WaitForPassName = "ExecuteFrame"
        //    } );
        pFrameGraph->Build();
        //pCmdBuffer->End( VKE::RenderSystem::ExecuteCommandBufferFlags::END, nullptr );
        Timer.Start();
        return pTerrain.IsValid();
    }

    float GetFrameTimeSeconds()
    {
        return Timer.GetElapsedTime<VKE::Utils::CTimer::Seconds>();
    }

    void UpdateCamera( VKE::RenderSystem::CGraphicsContext* pCtx )
    {
        /*if( !pCtx->GetSwapChain()->GetWindow()->HasFocus() )
        {
            return;
        }*/
        const auto& InputState =
            pCtx->GetSwapChain()->GetWindow()->GetInputSystem().GetState();
        //const float frameTime = GetFrameTimeSeconds();
        const VKE::Math::CVector3 vecCamMoveDistance = { pInputListener->vecSpeed * frameTime *
                                                         pInputListener->dbgCameraSpeed };

        if( InputState.Keyboard.IsKeyDown( VKE::Input::Keys::W ) )
        {
            pDebugCamera->Move( vecCamMoveDistance * pDebugCamera->GetDirection() );
        }
        else if( InputState.Keyboard.IsKeyDown( VKE::Input::Keys::S ) )
        {
            pDebugCamera->Move( vecCamMoveDistance * -pDebugCamera->GetDirection() );
        }
        else if( InputState.Keyboard.IsKeyDown( VKE::Input::Keys::Q ) )
        {
            pDebugCamera->Move( ( vecCamMoveDistance * 100 * ( VKE::Math::CVector3::Y ) ) );
        }
        else if( InputState.Keyboard.IsKeyDown( VKE::Input::Keys::Z ) )
        {
            pDebugCamera->Move( ( vecCamMoveDistance * 100 * (VKE::Math::CVector3::NEGATIVE_Y ) ) );
        }


        if( InputState.Keyboard.IsKeyDown( VKE::Input::Keys::I ) )
        {
            pCamera->Move( pInputListener->vecDist * pInputListener->vecSpeed * VKE::Math::CVector3::Z );
        }
        else if( InputState.Keyboard.IsKeyDown( VKE::Input::Keys::K ) )
        {
            pCamera->Move( pInputListener->vecDist * pInputListener->vecSpeed * VKE::Math::CVector3::NEGATIVE_Z );
        }
        else if( InputState.Keyboard.IsKeyDown( VKE::Input::Keys::L ) )
        {
            pCamera->Move( pInputListener->vecDist * pInputListener->vecSpeed * VKE::Math::CVector3::X );
        }
        else if( InputState.Keyboard.IsKeyDown( VKE::Input::Keys::J ) )
        {
            pCamera->Move( pInputListener->vecDist * pInputListener->vecSpeed * VKE::Math::CVector3::NEGATIVE_X );
        }
        else
        if( InputState.Keyboard.IsKeyDown( VKE::Input::Keys::U ) )
        {
            pCamera->Move( pInputListener->vecDist * pInputListener->vecSpeed  *
                           VKE::Math::CVector3::Y );
        }
        else if( InputState.Keyboard.IsKeyDown( VKE::Input::Keys::H ) )
        {
            pCamera->Move( pInputListener->vecDist * pInputListener->vecSpeed *
                           VKE::Math::CVector3::NEGATIVE_Y );
        }

        if( InputState.Keyboard.IsKeyDown(VKE::Input::KEY::NUMPAD_PLUS) )
        {
            pInputListener->dbgCameraSpeed += 10;
        }
        else if( InputState.Keyboard.IsKeyDown( VKE::Input::KEY::NUMPAD_MINUS ) )
        {
            pInputListener->dbgCameraSpeed -= 10;
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
        fps = pCtx->GetDeviceContext()->GetRenderSystem()->GetFrameGraph()->GetCounter( VKE::RenderSystem::FrameGraphCounterTypes::CPU_FRAME_TIME ).Avg.f32;
        auto fps3 = pCtx->GetDeviceContext()
                  ->GetRenderSystem()
                  ->GetFrameGraph()
                  ->GetCounter( VKE::RenderSystem::FrameGraphCounterTypes::CPU_FPS )
                  .Avg.f32;
        ( void )fps2;
        vke_sprintf( pText, 128, "%.3f, %.3f, %.3f / %.3f - %.3f", Pos.x, Pos.y,
                     Pos.z, fps, fps3 );
        pWnd->SetText( pText );
    }
    bool OnRenderFrame( VKE::RenderSystem::CGraphicsContext* pCtx ) override
    {
        frameTime = GetFrameTimeSeconds();
        Timer.Start();
        pWindow->Update();
        UpdateCamera( pCtx );

        pFrameGraph->Run();
        //auto pCommandBuffer = pCtx->BeginFrame();
        //pCtx->GetSwapChain()->BeginFrame( pCommandBuffer );

        //VKE::Scene::SUpdateSceneInfo UpdateSceneInfo;
        //UpdateSceneInfo.pCommandBuffer = pCommandBuffer;
        //pScene->Update( UpdateSceneInfo );
        //
        ////pCtx->BindDefaultRenderPass();
        ////m_RenderPassInfo.vColorRenderTargetInfos[ 0 ].hView = pCtx->GetSwapChain()->GetCurrentBackBuffer().pAcquiredElement->hDDITextureView;
        //auto hRT = pCtx->GetSwapChain()->GetCurrentBackBuffer().hRenderTarget;
        //m_pRenderPass->SetRenderTarget( 0, VKE::RenderSystem::SSetRenderTargetInfo( hRT, {0.5f, 0.5f, 0.5f, 1.0f} ) );
        //pCommandBuffer->BeginRenderPass( m_pRenderPass );
        //pScene->Render( pCommandBuffer );
      
        ////pTerrain->Render( pCommandBuffer );
        //pFrameGraph->Run();
        //pScene->RenderDebug( pCommandBuffer );
        //pCommandBuffer->EndRenderPass();
        //pCtx->GetSwapChain()->EndFrame( pCommandBuffer );
        //pCtx->EndFrame();
        return true;
    }
};
int main()
{
    VKE_DETECT_MEMORY_LEAKS();
    //VKE::Platform::Debug::BreakAtAllocation( 277595 );
    {
        CSampleFramework Sample;
        SSampleCreateDesc Desc;
        VKE::RenderSystem::EventListeners::IGraphicsContext*
            apListeners[ 1 ] = { VKE_NEW SGfxContextListener() };
        Desc.ppGfxListeners = apListeners;
        Desc.gfxListenerCount = 1;
        //Desc.WindowSize = { 1920, 1080 };
        Desc.WindowSize = { 800, 600 };
        Desc.enableRendererDebug = VKE_DEBUG;
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
