

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

        const float scale = 0.5f;
        float x = VKE::Math::ConvertToRadians( (float)Mouse.Move.x ) * scale;
        float y = VKE::Math::ConvertToRadians( (float)Mouse.Move.y ) * scale;
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
    VKE::Scene::CameraPtr pCamera;
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
        VKE::RenderSystem::SCreateShaderDesc VsDesc, PsDesc;

        VsDesc.Create.async = true;
        VsDesc.Create.stages = VKE::Core::ResourceStages::FULL_LOAD;
        VsDesc.Create.pOutput = &pVS;
        VsDesc.Shader.SetEntryPoint( "main" );
        VsDesc.Shader.Base.pFileName = "Data/Samples/Shaders/simple-mvp.vs";

        PsDesc = VsDesc;
        PsDesc.Create.pOutput = &pPS;
        PsDesc.Shader.Base.pFileName = "Data/Samples/shaders/simple.ps";

        pVS = pCtx->CreateShader( VsDesc );
        pPS = pCtx->CreateShader( PsDesc );
    }

    bool Init( VKE::RenderSystem::CDeviceContext* pCtx )
    {
        pCtx->GetRenderSystem()->GetEngine()->GetInputSystem()->SetListener( pInputListener );

        LoadShaders( pCtx );

        VKE::Scene::SSceneDesc SceneDesc;
        auto pWorld = pCtx->GetRenderSystem()->GetEngine()->World();
        pScene = pWorld->CreateScene( SceneDesc );
        pCamera = pWorld->GetCamera( 1 );

        pCamera->SetPosition( VKE::Math::CVector3( 0.0f, 0.0f, -10.0f ) );
        pCamera->Rotate( VKE::Math::ConvertToRadians( 5.0f ), VKE::Math::ConvertToRadians( 0.0f ), 0.0f );
        pCamera->Update( 0 );
        pScene->SetCamera( pCamera );
        VKE::Math::CVector3 aFrustumCorners[8];
        pCamera->GetFrustum().CalcCorners( aFrustumCorners );

        pCamera = pWorld->GetCamera( 0 );
        //pCamera->Rotate( VKE::Math::ConvertToRadians( 10.0f ), VKE::Math::ConvertToRadians( 0.0f ), 0.0f );
        pCamera->SetPosition( VKE::Math::CVector3( 0.0f, -10.0f, -15.0f ) );
        pCamera->Update( 0 );
        pInputListener->pCamera = pCamera;


        const VKE::Math::CVector3 vb[8] =
        {
            // Frustum
            aFrustumCorners[0],
            aFrustumCorners[1],
            aFrustumCorners[2],
            aFrustumCorners[3],
            aFrustumCorners[4],
            aFrustumCorners[5],
            aFrustumCorners[6],
            aFrustumCorners[7]
        };

        const uint32_t ib[24] =
        {
            VKE::Math::CFrustum::Corners::LEFT_TOP_NEAR,      VKE::Math::CFrustum::Corners::RIGHT_TOP_NEAR,
            VKE::Math::CFrustum::Corners::RIGHT_TOP_NEAR,     VKE::Math::CFrustum::Corners::RIGHT_BOTTOM_NEAR,
            VKE::Math::CFrustum::Corners::RIGHT_BOTTOM_NEAR,  VKE::Math::CFrustum::Corners::LEFT_BOTTOM_NEAR,
            VKE::Math::CFrustum::Corners::LEFT_BOTTOM_NEAR,   VKE::Math::CFrustum::Corners::LEFT_TOP_NEAR,

            VKE::Math::CFrustum::Corners::RIGHT_BOTTOM_FAR,   VKE::Math::CFrustum::Corners::RIGHT_TOP_FAR,
            VKE::Math::CFrustum::Corners::RIGHT_TOP_FAR,      VKE::Math::CFrustum::Corners::LEFT_TOP_FAR,
            VKE::Math::CFrustum::Corners::LEFT_TOP_FAR,       VKE::Math::CFrustum::Corners::LEFT_BOTTOM_FAR,
            VKE::Math::CFrustum::Corners::LEFT_BOTTOM_FAR,    VKE::Math::CFrustum::Corners::RIGHT_BOTTOM_FAR,

            VKE::Math::CFrustum::Corners::RIGHT_BOTTOM_NEAR,  VKE::Math::CFrustum::Corners::RIGHT_BOTTOM_FAR,
            VKE::Math::CFrustum::Corners::RIGHT_TOP_NEAR,     VKE::Math::CFrustum::Corners::RIGHT_TOP_FAR,
            VKE::Math::CFrustum::Corners::LEFT_BOTTOM_NEAR,   VKE::Math::CFrustum::Corners::LEFT_BOTTOM_FAR,
            VKE::Math::CFrustum::Corners::LEFT_TOP_NEAR,      VKE::Math::CFrustum::Corners::LEFT_TOP_FAR
        };

        VKE::RenderSystem::SCreateBufferDesc BuffDesc;
        BuffDesc.Create.async = false;
        BuffDesc.Buffer.usage = VKE::RenderSystem::BufferUsages::VERTEX_BUFFER | VKE::RenderSystem::BufferUsages::INDEX_BUFFER;
        BuffDesc.Buffer.memoryUsage = VKE::RenderSystem::MemoryUsages::GPU_ACCESS;
        BuffDesc.Buffer.size = sizeof( vb ) + sizeof( ib );
        BuffDesc.Buffer.indexType = VKE::RenderSystem::IndexTypes::UINT32;
        auto hVB = pCtx->CreateBuffer( BuffDesc );
        pVb = pCtx->GetBuffer( hVB );

        VKE::RenderSystem::SUpdateMemoryInfo UpdateInfo;
        UpdateInfo.pData = vb;
        UpdateInfo.dataSize = sizeof( vb );
        UpdateInfo.dstDataOffset = 0;
        pCtx->UpdateBuffer( UpdateInfo, &pVb );
        UpdateInfo.pData = ib;
        UpdateInfo.dataSize = sizeof( ib );
        UpdateInfo.dstDataOffset = sizeof( vb );
        pCtx->UpdateBuffer( UpdateInfo, &pVb );


        BuffDesc.Buffer.usage = VKE::RenderSystem::BufferUsages::CONSTANT_BUFFER;
        BuffDesc.Buffer.size = 0;
        BuffDesc.Buffer.vRegions.PushBack( VKE::RenderSystem::SBufferRegion( 3, sizeof( VKE::Math::CMatrix4x4 ) ) );
        auto hUBO = pCtx->CreateBuffer( BuffDesc );
        pUBO = pCtx->GetBuffer( hUBO );
        UBO.vData.resize( pUBO->GetSize() );

        VKE::RenderSystem::SCreateBindingDesc BindingDesc;
        BindingDesc.AddConstantBuffer( 0, VKE::RenderSystem::PipelineStages::VERTEX );
        hDescSet = pCtx->CreateResourceBindings( BindingDesc );
        VKE::RenderSystem::SUpdateBindingsInfo UpdateBindingInfo;
        const auto hBuff = VKE::RenderSystem::HandleCast<VKE::RenderSystem::BufferHandle>(  pUBO->GetHandle() );
        UpdateBindingInfo.AddBinding( 0, 0, pUBO->GetRegionElementSize( 0 ), &hBuff, 1 );
        pCtx->UpdateDescriptorSet( UpdateBindingInfo, &hDescSet );


        VKE::RenderSystem::SPipelineLayoutDesc LayoutDesc;
        LayoutDesc.vDescriptorSetLayouts.PushBack( pCtx->GetDescriptorSetLayout( hDescSet ) );
        auto pLayout = pCtx->CreatePipelineLayout( LayoutDesc );

        VKE::RenderSystem::SPipelineCreateDesc Pipeline;
    
        Pipeline.Pipeline.hLayout = pLayout->GetHandle();

        Pipeline.Pipeline.hDDIRenderPass = pCtx->GetGraphicsContext( 0 )->GetSwapChain()->GetDDIRenderPass();
        VKE::RenderSystem::SPipelineDesc::SInputLayout::SVertexAttribute VA;
        VA.format = VKE::RenderSystem::Formats::R32G32B32_SFLOAT;
        VA.location = 0;
        VA.vertexBufferBindingIndex = 0;
        VA.offset = 0;
        VA.stride = 3 * 4;
        Pipeline.Pipeline.InputLayout.vVertexAttributes.Clear();
        Pipeline.Pipeline.InputLayout.vVertexAttributes.PushBack( VA );
        Pipeline.Pipeline.Shaders.apShaders[VKE::RenderSystem::ShaderTypes::VERTEX] = pVS;
        Pipeline.Pipeline.Shaders.apShaders[VKE::RenderSystem::ShaderTypes::PIXEL] = pPS;

        auto pPipeline = pCtx->CreatePipeline( Pipeline );
        VKE_ASSERT( pPipeline.IsValid(), "" );

        //Frustum drawcall
        /*Pipeline.Pipeline.InputLayout.topology = VKE::RenderSystem::PrimitiveTopologies::LINE_LIST;
        pPipeline = pCtx->CreatePipeline( Pipeline );
        pDrawcall = pWorld->CreateDrawcall( {} );
        LOD.pPipeline = pPipeline;
        LOD.descSetOffset = pUBO->CalcOffset( 0, 2 );
        LOD.DrawParams.Indexed.indexCount = 8 + 8 + 8;
        LOD.DrawParams.Indexed.startIndex = 0;
        LOD.vertexBufferOffset = 9 * 4;
        LOD.indexBufferOffset = sizeof( vb ) + 4 * 3;
        DataInfo.canBeCulled = false;
        pDrawcall->AddLOD( LOD );
        pScene->AddObject( pDrawcall, DataInfo );*/

        VKE::Scene::STerrainDesc TerrainDesc;
        TerrainDesc.size = 100;
        TerrainDesc.Height = { -500.0f, 500.0f };
        TerrainDesc.tileRowVertexCount = 33;
        TerrainDesc.vertexDistance = 1.0f;
        TerrainDesc.lodCount = 1;
        TerrainDesc.vDDIRenderPasses.PushBack( pCtx->GetGraphicsContext( 0 )->GetSwapChain()->GetDDIRenderPass() );
        pTerrain = pScene->CreateTerrain( TerrainDesc, pCtx );

        return pVb.IsValid();
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
            pCamera->Move( pInputListener->vecDist * pInputListener->vecSpeed * pCamera->GetDirection() );
        }
        else if( InputState.Keyboard.IsKeyDown( VKE::Input::Keys::S ) )
        {
            pCamera->Move( pInputListener->vecDist * pInputListener->vecSpeed * -pCamera->GetDirection() );
        }

        pCamera->Update( 0 );
    }

    bool OnRenderFrame( VKE::RenderSystem::CGraphicsContext* pCtx ) override
    {
        UpdateCamera( pCtx );
        pCtx->BeginFrame();
        pTerrain->Update( pCtx, pCamera );
        pCtx->BindDefaultRenderPass();
        pScene->Render( pCtx );
        pTerrain->Render( pCtx, pCamera );
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