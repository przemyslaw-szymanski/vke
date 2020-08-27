

#include "../CSampleFramework.h"

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

        const float scale = 0.0005f + 0.0f;
        float x = VKE::Math::ConvertToRadians( (float)Mouse.Move.x ) * scale;
        float y = VKE::Math::ConvertToRadians( (float)Mouse.Move.y ) * scale * 0;
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

    void LoadShaders( VKE::RenderSystem::CDeviceContext* pCtx )
    {
        /*VKE::RenderSystem::SCreateShaderDesc VsDesc, PsDesc;

        VsDesc.Create.async = true;
        VsDesc.Create.stages = VKE::Core::ResourceStages::FULL_LOAD;
        VsDesc.Create.pOutput = &pVS;
        VsDesc.Shader.SetEntryPoint( "main" );
        VsDesc.Shader.FileInfo.pFileName = "Data/Samples/Shaders/simple-mvp.vs";

        PsDesc = VsDesc;
        PsDesc.Create.pOutput = &pPS;
        PsDesc.Shader.FileInfo.pFileName = "Data/Samples/shaders/simple.ps";

        pVS = pCtx->CreateShader( VsDesc );
        pPS = pCtx->CreateShader( PsDesc );*/
    }

    bool Init( VKE::RenderSystem::CDeviceContext* pCtx )
    {
        pCtx->GetRenderSystem()->GetEngine()->GetInputSystem()->SetListener( pInputListener );

        LoadShaders( pCtx );

        VKE::Scene::SSceneDesc SceneDesc;
        SceneDesc.pDeviceContext = pCtx;
        auto pWorld = pCtx->GetRenderSystem()->GetEngine()->World();
        pScene = pWorld->CreateScene( SceneDesc );
        pCamera = pScene->CreateCamera( "Debug" );

        pCamera->SetPosition( VKE::Math::CVector3( 0.0f, 0.0f, 100.0f ) );
        pCamera->Update( 0 );
        pScene->SetCamera( pCamera );
        pScene->AddDebugView( &pCamera );

        pRenderCamera = pScene->CreateCamera( "RenderDefault" );
        pRenderCamera->SetPosition( pCamera->GetPosition() + VKE::Math::CVector3(0, -10, -20) );
        pRenderCamera->Update( 0 );
        pScene->SetRenderCamera( pRenderCamera );
        pInputListener->pCamera = pRenderCamera;


        VKE::Scene::STerrainDesc TerrainDesc;
        TerrainDesc.size = 1024;
        TerrainDesc.Height = { -50.0f, 50.0f };
        TerrainDesc.tileSize = 32;
        TerrainDesc.vertexDistance = 1.0f;
        TerrainDesc.lodCount = 7;
        TerrainDesc.Heightmap.pFileName = "data\\textures\\terrain1024.dds";
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
        if( !pCtx->GetSwapChain()->GetWindow()->HasFocus() )
        {
            return;
        }
        const auto& InputState = pCtx->GetDeviceContext()->GetRenderSystem()->GetEngine()->GetInputSystem()->GetState();

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
        vke_sprintf( pText, 128, "%.3f, %.3f, %.3f", Pos.x, Pos.y, Pos.z );
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

void Main()
{
}

int main()
{
    VKE_DETECT_MEMORY_LEAKS();
    //VKE::Platform::Debug::BreakAtAllocation( 3307 );
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
            if( pListener->Init( Sample.m_vpDeviceContexts[0] ) )
            {
                Sample.Start();
            }
        }
        VKE_DELETE( apListeners[0] );
        Sample.Destroy();
    }

    return 0;
}
