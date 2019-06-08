

#include "../CSampleFramework.h"

struct SInputListener : public VKE::Input::EventListeners::IInput
{
    VKE::Scene::CameraPtr pCamera;
    VKE::Math::CVector3 vecSpeed = VKE::Math::CVector3( 0.5f );
    VKE::Math::CVector3 vecDir = VKE::Math::CVector3::Z;
    VKE::Input::MousePosition LastMousePos = { 0,0 };
    VKE::Input::MousePosition MouseDir = { 0,0 };
    VKE::ExtentF32  MouseAngle = { 0.0f, 0.0f };
    VKE::Math::CVector3 vecYawPitchRoll = { 0.0f };
    bool mouseDown = false;

    void OnKeyDown(const VKE::Input::KEY& key) override
    {
        switch( key )
        {
            case VKE::Input::Keys::CAPITAL_W:
            case VKE::Input::Keys::W:
            {
                pCamera->Move( vecSpeed * vecDir );
                pCamera->SetLookAt( pCamera->GetPosition() + VKE::Math::CVector3::ONE * vecDir );
            }
            break;
            case VKE::Input::Keys::CAPITAL_S:
            case VKE::Input::Keys::S:
            {
                pCamera->Move( vecSpeed * -vecDir );
                pCamera->SetLookAt( pCamera->GetPosition() + VKE::Math::CVector3::ONE * vecDir );
            }
            break;
        };
    }

    void OnMouseButtonDown( const VKE::Input::MOUSE_BUTTON& button, const VKE::Input::MousePosition& Pos ) override
    {
        mouseDown = true;
        LastMousePos = Pos;
    }

    void OnMouseButtonUp( const VKE::Input::MOUSE_BUTTON& button, const VKE::Input::MousePosition& Pos ) override
    {
        mouseDown = false;
        LastMousePos = Pos;
    }

    void OnMouseMove( const VKE::Input::MousePosition& Position ) override
    {
        if( !mouseDown )
            return;

        MouseDir.x = Position.x - LastMousePos.x;
        MouseDir.y = Position.y - LastMousePos.y;

        if( MouseDir.x == 0 && MouseDir.y == 0 )
            return;
        
        vecYawPitchRoll.x += MouseDir.x;
        vecYawPitchRoll.y += MouseDir.y;
        
        const float scale = 0.1f;
        float x = VKE::Math::ConvertToRadians( MouseDir.x ) * scale;
        float y = VKE::Math::ConvertToRadians( MouseDir.y ) * scale;
        //pCamera->RotateX( VKE::Math::ConvertToRadians( -x ) );
        //pCamera->RotateY( VKE::Math::ConvertToRadians( y ) );
        pCamera->Rotate( x, y, 0.0f );
        //pCamera->Rotate( VKE::Math::CVector3::X, x );
        //pCamera->Rotate( VKE::Math::CVector3::Y, y );
        LastMousePos = Position;
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

    SInputListener* pInputListener;

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
        VsDesc.Create.stages = VKE::Resources::StageBits::FULL_LOAD;
        VsDesc.Create.pOutput = &pVS;
        VsDesc.Shader.Base.pFileName = "Data/Samples/Shaders/simple-mvp.vs";
        
        PsDesc = VsDesc;
        PsDesc.Create.pOutput = &pPS;
        PsDesc.Shader.Base.pFileName = "Data/Samples/shaders/simple.ps";

        pVS = pCtx->CreateShader( VsDesc );
        pPS = pCtx->CreateShader( PsDesc );
    }

    bool Init( VKE::RenderSystem::CDeviceContext* pCtx )
    {
        pCtx->GetRenderSystem()->GetEngine()->SetInputListener( pInputListener );

        LoadShaders( pCtx );

        const VKE::Math::CVector3 vb[3] =
        {
            { 0.0f,   0.5f,   0.0f },
            { -0.5f, -0.5f,   0.0f },
            { 0.5f,  -0.5f,   0.0f }
        };

        const uint32_t ib[3] =
        {
            0, 1, 2
        };

        VKE::RenderSystem::SCreateBufferDesc BuffDesc;
        BuffDesc.Create.async = false;
        BuffDesc.Buffer.usage = VKE::RenderSystem::BufferUsages::VERTEX_BUFFER | VKE::RenderSystem::BufferUsages::INDEX_BUFFER;
        BuffDesc.Buffer.memoryUsage = VKE::RenderSystem::MemoryUsages::GPU_ACCESS;
        BuffDesc.Buffer.size = sizeof( vb ) + sizeof(ib);
        BuffDesc.Buffer.indexType = VKE::RenderSystem::IndexTypes::UINT32;
        pVb = pCtx->CreateBuffer( BuffDesc );
        
        VKE::RenderSystem::SUpdateMemoryInfo UpdateInfo;
        UpdateInfo.pData = vb;
        UpdateInfo.dataSize = sizeof( vb );
        UpdateInfo.dstDataOffset = 0;
        pCtx->UpdateBuffer( UpdateInfo, &pVb );
        UpdateInfo.pData = ib;
        UpdateInfo.dataSize = sizeof( ib );
        UpdateInfo.dstDataOffset = sizeof( vb );
        pCtx->UpdateBuffer( UpdateInfo, &pVb );

        Layout.vAttributes =
        {
            { "Position", VKE::RenderSystem::VertexAttributeTypes::POSITION3 }
        };

        VKE::Scene::SSceneDesc SceneDesc;
        SceneDesc.graphSystem = VKE::Scene::GraphSystems::OCTREE;
        pScene = pCtx->GetRenderSystem()->GetEngine()->World()->CreateScene( SceneDesc );
        pCamera = pCtx->GetRenderSystem()->GetEngine()->World()->GetCamera( 0 );
        pScene->SetCamera( pCamera );
        pInputListener->pCamera = pCamera;

        //pCamera->SetLookAt( VKE::Math::CVector3( 0.0f, 0.0f, 0.0f ) );
        pCamera->SetPosition( VKE::Math::CVector3( 0.0f, 0.0f, -10.0f ) );
        pCamera->Rotate( VKE::Math::CVector3::Y, VKE::Math::ConvertToRadians( 15.0f ) );
        pCamera->Rotate( VKE::Math::CVector3::X, VKE::Math::ConvertToRadians( 15.0f ) );
        pCamera->Update(0);
        
        VKE::Math::CMatrix4x4 Model, MVP;
        VKE::Math::CMatrix4x4::Translate( VKE::Math::CVector3( 0.0f, 0.0f, 0.0f ), &Model );
        VKE::Math::CMatrix4x4::Mul( Model, pCamera->GetViewProjectionMatrix(), &MVP );

        BuffDesc.Buffer.usage = VKE::RenderSystem::BufferUsages::UNIFORM_BUFFER;
        BuffDesc.Buffer.size = sizeof( VKE::Math::CMatrix4x4 );
        pUBO = pCtx->CreateBuffer( BuffDesc );
        UpdateInfo.pData = &MVP;
        UpdateInfo.dataSize = sizeof( VKE::Math::CMatrix4x4 );
        pCtx->UpdateBuffer( UpdateInfo, &pUBO );

        VKE::RenderSystem::SCreateBindingDesc BindingDesc;
        BindingDesc.AddBuffer( 0, VKE::RenderSystem::PipelineStages::VERTEX );
        hDescSet = pCtx->CreateResourceBindings( BindingDesc );
        VKE::RenderSystem::SUpdateBindingsInfo UpdateBindingInfo;
        const auto hBuff = VKE::RenderSystem::BufferHandle{ pUBO->GetHandle() };
        UpdateBindingInfo.AddBinding( 0, 0, UpdateInfo.dataSize, &hBuff, 1 );
        pCtx->UpdateDescriptorSet( UpdateBindingInfo, &hDescSet );

        while( pPS.IsNull() )
        {

        }

        VKE::Scene::CDrawcall Drawcall;
        VKE::Scene::CDrawcall::LOD LOD;
        LOD.DrawParams.indexCount = 3;
        LOD.DrawParams.instanceCount = 1;
        LOD.DrawParams.startIndex = 0;
        LOD.DrawParams.startInstance = 0;
        LOD.DrawParams.vertexOffset = 0;
        LOD.hDescSet = hDescSet;
        LOD.hVertexBuffer.handle = pVb->GetHandle();
        LOD.vertexBufferOffset = 0;
        LOD.hIndexBuffer.handle = pVb->GetHandle();
        LOD.indexBufferOffset = sizeof( vb );
        LOD.InputLayout = Layout;
        LOD.topology = VKE::RenderSystem::PrimitiveTopologies::TRIANGLE_LIST;
        LOD.ppVertexShader = &pVS;
        LOD.ppPixelShader = &pPS;
        Drawcall.AddLOD( LOD );
        VKE::Scene::SDrawcallDataInfo DataInfo;
        VKE::Math::CAABB::Transform( 1.0f, VKE::Math::CVector3( 0.0f, 0.0f, 0.0f ), &DataInfo.AABB );
        pScene->AddObject( &Drawcall, DataInfo );
        

        return pVb.IsValid();
    }

    void UpdateUBO( VKE::RenderSystem::CGraphicsContext* pCtx )
    {
        VKE::Math::CMatrix4x4 Model, MVP;
        VKE::Math::CMatrix4x4::Translate( VKE::Math::CVector3( 0.0f, 0.0f, 0.0f ), &Model );
        VKE::Math::CMatrix4x4::Mul( Model, pCamera->GetViewProjectionMatrix(), &MVP );
        VKE::RenderSystem::SUpdateMemoryInfo UpdateInfo;
        UpdateInfo.pData = &MVP;
        UpdateInfo.dataSize = sizeof( VKE::Math::CMatrix4x4 );
        UpdateInfo.dstDataOffset = 0;
        pCtx->UpdateBuffer( UpdateInfo, &pUBO );
    }

    bool OnRenderFrame(VKE::RenderSystem::CGraphicsContext* pCtx) override
    {
        pCtx->BeginFrame();
        UpdateUBO( pCtx );
        pCtx->BindDefaultRenderPass();
        pScene->Render( pCtx );
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