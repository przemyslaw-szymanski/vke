

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

    void OnKeyDown(const VKE::Input::SKeyboardState& State, const VKE::Input::KEY& key ) override
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
        
        /*const float scale = 0.5f;
        float x = VKE::Math::ConvertToRadians( (float)Mouse.Move.x ) * scale;
        float y = VKE::Math::ConvertToRadians( (float)Mouse.Move.y ) * scale;*/
        //pCamera->RotateX( VKE::Math::ConvertToRadians( -x ) );
        //pCamera->RotateY( VKE::Math::ConvertToRadians( y ) );
        //pCamera->Rotate( x, y, 0.0f );
        //pCamera->Rotate( VKE::Math::CVector3::X, x );
        //pCamera->Rotate( VKE::Math::CVector3::Y, y );

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
        pCtx->GetRenderSystem()->GetEngine()->GetInputSystem()->SetListener( pInputListener );

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

    VKE::ExtentI16 LastMove = { 0,0 };
    VKE::ExtentU16 LastPos = { 0,0 };

    void UpdateCamera( VKE::RenderSystem::CGraphicsContext* pCtx )
    {
        const auto& InputState = pCtx->GetDeviceContext()->GetRenderSystem()->GetEngine()->GetInputSystem()->GetState();

        if( InputState.Mouse.IsButtonDown( VKE::Input::MouseButtons::LEFT ) )
        {
            if( LastPos.x == 0 && LastPos.y == 0 )
            {
                LastPos = InputState.Mouse.Position;
            }

            VKE::ExtentI16 Delta = InputState.Mouse.Move - LastMove;
            //VKE::ExtentI16 Delta;
            Delta.x = InputState.Mouse.Position.x - LastPos.x;
            Delta.y = InputState.Mouse.Position.y - LastPos.y;
            LastPos = InputState.Mouse.Position;

            if( Delta.x != 0 || Delta.y != 0 )
            {
                const float scale = 0.5f;
                float x = VKE::Math::ConvertToRadians( (float)Delta.x );
                float y = VKE::Math::ConvertToRadians( (float)Delta.y );

                x = VKE::Math::Clamp( x, -1.0f, 1.0f );
                y = VKE::Math::Clamp( y, -1.0f, 1.0f );
                x *= scale;
                y *= scale;

                LastMove = InputState.Mouse.Move;
                
                char buff[128];
                vke_sprintf( buff, 128, "%d, %d / %d, %d",
                    InputState.Mouse.Move.x, InputState.Mouse.Move.y,
                    LastMove.x, LastMove.y );
                pCtx->GetSwapChain()->GetWindow()->SetText( buff );
                pCamera->Rotate( x, y, 0.0f );
            }
        }

        if( InputState.Keyboard.IsKeyDown( VKE::Input::Keys::W ) )
        {
            pCamera->Move( pInputListener->vecDist * pInputListener->vecSpeed * pInputListener->vecDir );
        }
        else if( InputState.Keyboard.IsKeyDown( VKE::Input::Keys::S ) )
        {
            pCamera->Move( pInputListener->vecDist * pInputListener->vecSpeed * -pInputListener->vecDir );
        }
    }

    bool OnRenderFrame(VKE::RenderSystem::CGraphicsContext* pCtx) override
    {
        UpdateCamera( pCtx );
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
