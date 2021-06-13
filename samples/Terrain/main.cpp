

#include "../CSampleFramework.h"
#include "Vke/Core/Managers/CImageManager.h"

struct SInputListener : public VKE::Input::EventListeners::IInput
{
    VKE::Scene::CameraPtr pCamera;
    VKE::Math::CVector3 vecSpeed = VKE::Math::CVector3( 0.1f );
    VKE::Math::CVector3 vecDir = VKE::Math::CVector3::Z;
    VKE::Math::CVector3 vecDist = VKE::Math::CVector3( 1.0f );
    VKE::Input::MousePosition LastMousePos = { 0,0 };
    VKE::Input::MousePosition MouseDir = { 0,0 };
    VKE::ExtentF32  MouseAngle = { 0.0f, 0.0f };
    VKE::Math::CVector3 vecYawPitchRoll = { 0.0f };
    bool mouseDown = false;

    void OnKeyDown( const VKE::Input::SKeyboardState& State, const VKE::Input::KEY& key ) override
    {
        if( key == VKE::Input::KEY::W )
        {
            //pCamera->Move( vecDist * vecSpeed * vecDir );
        }
        if( key == VKE::Input::KEY::S )
        {
            //pCamera->Move( vecDist * vecSpeed * -vecDir );
        }
    }

    void OnKeyUp( const VKE::Input::SKeyboardState& State, const VKE::Input::KEY& key ) override
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
        if( !mouseDown || (Mouse.Move.x == 0 && Mouse.Move.y == 0) )
            return;

        const float scale = 0.25f + 0.0f;

        float x = VKE::Math::ConvertToRadians( (float)Mouse.Move.x ) * scale;
        float y = VKE::Math::ConvertToRadians( (float)Mouse.Move.y ) * scale * 1;
        //printf( "m %f, %f\n", x, y );
        pCamera->Rotate( x, y, 0.0f );
    }
};

struct SGfxContextListener : public VKE::RenderSystem::EventListeners::IGraphicsContext
{
    VKE::RenderSystem::VertexBufferRefPtr pVb;
    VKE::RenderSystem::ShaderRefPtr pVS;
    VKE::RenderSystem::ShaderRefPtr pPS;
    VKE::RenderSystem::BufferRefPtr pUBO;
    VKE::RenderSystem::DescriptorSetHandle hDescSet;
    VKE::RenderSystem::SVertexInputLayoutDesc Layout;
    VKE::Scene::CameraPtr pCamera, pRenderCamera;
    VKE::Scene::ScenePtr pScene;
    VKE::RenderSystem::IFrameGraph* pFrameGraph;
    VKE::Scene::TerrainPtr pTerrain;

    SInputListener* pInputListener;

    struct SUBO
    {
        vke_vector<uint8_t> vData;
    };

    SUBO UBO;

    SGfxContextListener()
    {
        pInputListener = VKE_NEW SInputListener;
    }

    virtual ~SGfxContextListener()
    {
        VKE_DELETE( pInputListener );
    }

    void LoadTextures( VKE::RenderSystem::CDeviceContext* pCtx,
                       VKE::Scene::STerrainDesc* pDesc )
    {
        VKE::ExtentU32 TexCount;
        VKE::Scene::CTerrain::CalcTextureCount( *pDesc, &TexCount );
        pDesc->Heightmap.vvFileNames.Resize( TexCount.y, VKE::Scene::STerrainDesc::StringArray( TexCount.x ) );

        char buff[ 128 ];
        for( uint32_t y = 0; y < TexCount.height; ++y )
        {
            for( uint32_t x = 0; x < TexCount.width; ++x )
            {
                vke_sprintf( buff, 128, "data/textures/terrain/heightmap16k_%d_%d.png", x, y );
                pDesc->Heightmap.vvFileNames[ x ][ y ] = buff;
            }
        }
    }

    void SliceTextures( VKE::RenderSystem::CDeviceContext* pCtx,
                        const VKE::Scene::STerrainDesc& Desc )
    {
        auto pImgMgr = pCtx->GetRenderSystem()->GetEngine()->GetImageManager();
        VKE::Core::SLoadFileInfo Info;
        Info.CreateInfo.async = false;
        Info.FileInfo.pFileName = "data/textures/terrain/heightmap16k.png";
        auto hHeightmap = pImgMgr->Load( Info );
        Info.FileInfo.pFileName = "data/textures/terrain/heightmap16k_normal.dds";
        auto hNormal = pCtx->GetRenderSystem()->GetEngine()->GetImageManager()->Load( Info );

        VKE::ExtentU32 TexCount;
        VKE::Scene::CTerrain::CalcTextureCount( Desc, &TexCount );
        const uint32_t texCount = TexCount.width * TexCount.height;

        if( hHeightmap != VKE::INVALID_HANDLE )
        {
            VKE::Utils::TCDynamicArray< VKE::Core::ImageHandle, 128 > vImages( texCount );
            VKE::Core::SSliceImageInfo SliceInfo;
            SliceInfo.hSrcImage = hHeightmap;
            
            for( uint16_t y = 0; y < TexCount.height; ++y )
            {
                for( uint16_t x = 0; x < TexCount.width; ++x )
                {
                    VKE::Core::SSliceImageInfo::SRegion Region;
                    Region.Size = { (VKE::image_dimm_t)Desc.TileSize.max + 1u, ( VKE::image_dimm_t )Desc.TileSize.max + 1u };
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
                    VKE::Math::Map1DarrayIndexTo2DArrayIndex( i, TexCount.width, TexCount.height, &x, &y );
                    vke_sprintf( pName, 128, "data/textures/terrain/heightmap16k_%d_%d.png", x, y );
                    VKE::Core::SSaveImageInfo SaveInfo;
                    SaveInfo.format = VKE::Core::ImageFileFormats::PNG;
                    SaveInfo.hImage = vImages[ i ];
                    SaveInfo.pFileName = pName;
                    res = pImgMgr->Save( SaveInfo );
                }
            }
        }
    }

    bool Init( const CSampleFramework& Sample )
    {
        //pCtx->GetRenderSystem()->GetEngine()->GetInputSystem()->SetListener( pInputListener );
        Sample.m_vpWindows[0]->GetInputSystem().SetListener(pInputListener);
        auto pCtx = Sample.m_vpDeviceContexts[0];

        VKE::Scene::SSceneDesc SceneDesc;
        SceneDesc.pDeviceContext = pCtx;
        auto pWorld = pCtx->GetRenderSystem()->GetEngine()->World();
        pScene = pWorld->CreateScene( SceneDesc );
        pCamera = pScene->CreateCamera( "Debug" );

        pCamera->SetPosition( VKE::Math::CVector3( 128.0f, 0.0f, 100.0f ) );
        pCamera->Update( 0 );
        pScene->SetCamera( pCamera );
        pScene->AddDebugView( &pCamera );

        pRenderCamera = pScene->CreateCamera( "RenderDefault" );
        pRenderCamera->SetPosition( pCamera->GetPosition() + VKE::Math::CVector3(0, 20, -0) );
        pRenderCamera->SetLookAt(VKE::Math::CVector3(0, 0, 0));
        pRenderCamera->Update( 0 );
        pScene->SetRenderCamera( pRenderCamera );
        pInputListener->pCamera = pRenderCamera;

        VKE::Scene::STerrainDesc TerrainDesc;
        TerrainDesc.size = 16000;
        //TerrainDesc.size = 1024;
        //TerrainDesc.size = 256;
        TerrainDesc.Height = { -200.0f, 200.0f };
        TerrainDesc.TileSize = {32, 2048};
        TerrainDesc.vertexDistance = 1.0f;
        TerrainDesc.lodCount = 7;
        TerrainDesc.maxViewDistance = 10000;

        SliceTextures( pCtx, TerrainDesc );

        LoadTextures( pCtx, &TerrainDesc );
        
        TerrainDesc.vDDIRenderPasses.PushBack( pCtx->GetGraphicsContext( 0 )->GetSwapChain()->GetDDIRenderPass() );
        pTerrain = pScene->CreateTerrain( TerrainDesc, pCtx );

        return pTerrain.IsValid();
    }

    void UpdateUBO( VKE::RenderSystem::CGraphicsContext* pCtx )
    {
        VKE::Math::CMatrix4x4 Model, MVP, *pMVP;
        VKE::Math::CMatrix4x4::Translate( VKE::Math::CVector3( 0.0f, 0.0f, 0.0f ), &Model );
        pMVP = (VKE::Math::CMatrix4x4*)&UBO.vData[pUBO->CalcOffset( 0, 0 )];
        VKE::Math::CMatrix4x4::Mul( Model, pCamera->GetViewProjectionMatrix(), pMVP );

        VKE::Math::CMatrix4x4::Translate( VKE::Math::CVector3( 0.5f, 0.5f, 0.0f ), &Model );
        pMVP = (VKE::Math::CMatrix4x4*)&UBO.vData[pUBO->CalcOffset( 0, 1 )];
        VKE::Math::CMatrix4x4::Mul( Model, pCamera->GetViewProjectionMatrix(), pMVP );
        // Frustum
        VKE::Math::CMatrix4x4::Translate( VKE::Math::CVector3( 0.0f, 0.0f, 0.0f ), &Model );
        pMVP = (VKE::Math::CMatrix4x4*)&UBO.vData[pUBO->CalcOffset( 0, 2 )];
        VKE::Math::CMatrix4x4::Mul( Model, pCamera->GetViewProjectionMatrix(), pMVP );

        VKE::RenderSystem::SUpdateMemoryInfo UpdateInfo;
        UpdateInfo.pData = &UBO.vData[0];
        UpdateInfo.dataSize = (uint32_t)UBO.vData.size();
        UpdateInfo.dstDataOffset = 0;
        pCtx->UpdateBuffer( UpdateInfo, &pUBO );
    }

    void UpdateCamera( VKE::RenderSystem::CGraphicsContext* pCtx )
    {
        /*if( !pCtx->GetSwapChain()->GetWindow()->HasFocus() )
        {
            return;
        }*/
        const auto& InputState = pCtx->GetSwapChain()->GetWindow()->GetInputSystem().GetState();

        if( InputState.Keyboard.IsKeyDown( VKE::Input::Keys::W ) )
        {
            pRenderCamera->Move( pInputListener->vecDist * pInputListener->vecSpeed * pRenderCamera->GetDirection() * 5 );
        }
        else if( InputState.Keyboard.IsKeyDown( VKE::Input::Keys::S ) )
        {
            pRenderCamera->Move( pInputListener->vecDist * pInputListener->vecSpeed * -pRenderCamera->GetDirection() * 5 );
        }

        if( InputState.Keyboard.IsKeyDown( VKE::Input::Keys::R ) )
        {
            pCamera->Move( pInputListener->vecDist * pInputListener->vecSpeed * pCamera->GetDirection() );
        }
        else if( InputState.Keyboard.IsKeyDown( VKE::Input::Keys::F ) )
        {
            pCamera->Move( pInputListener->vecDist * pInputListener->vecSpeed * -pCamera->GetDirection() );
        }

        pCamera->Update( 0 );
        pRenderCamera->Update( 0 );
        auto pWnd = pCtx->GetDeviceContext()->GetRenderSystem()->GetEngine()->GetWindow();
        char pText[ 128 ];
        auto& Pos = pCamera->GetPosition();
        auto fps = pCtx->GetDeviceContext()->GetMetrics().avgFps;
        auto fps2 = pCtx->GetDeviceContext()->GetMetrics().currentFps;
        vke_sprintf( pText, 128, "%.3f, %.3f, %.3f / %.1f - %.1f", Pos.x, Pos.y, Pos.z, fps, fps2 );
        pWnd->SetText( pText );
    }

    bool OnRenderFrame( VKE::RenderSystem::CGraphicsContext* pCtx ) override
    {
        UpdateCamera( pCtx );
        pTerrain->Update( pCtx );

        pCtx->BeginFrame();
        pCtx->BindDefaultRenderPass();
        pScene->Render( pCtx );
        pTerrain->Render( pCtx );
        pCtx->GetDeviceContext()->SynchronizeTransferContext();
        pCtx->EndFrame();
        return true;
    }
};

int main()
{
    VKE_DETECT_MEMORY_LEAKS();
    //VKE::Platform::Debug::BreakAtAllocation(15874);
    {
        CSampleFramework Sample;
        SSampleCreateDesc Desc;
        VKE::RenderSystem::EventListeners::IGraphicsContext* apListeners[1] =
        {
            VKE_NEW SGfxContextListener()
        };
        Desc.ppGfxListeners = apListeners;
        Desc.gfxListenerCount = 1;

        if( Sample.Create( Desc ) )
        {
            SGfxContextListener* pListener = reinterpret_cast<SGfxContextListener*>(apListeners[0]);
            if( pListener->Init( Sample ) )
            {
                Sample.Start();
            }
        }
        VKE_DELETE( apListeners[0] );
        Sample.Destroy();
    }

    return 0;
}
