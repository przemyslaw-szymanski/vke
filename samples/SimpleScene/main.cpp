

#include "../CSampleFramework.h"

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

    SGfxContextListener()
    {

    }

    virtual ~SGfxContextListener()
    {

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

        pCtx->CreateShader( VsDesc );
        pCtx->CreateShader( PsDesc );
    }

    bool Init( VKE::RenderSystem::CDeviceContext* pCtx )
    {
        LoadShaders( pCtx );

        VKE::RenderSystem::SCreateBufferDesc BuffDesc;
        BuffDesc.Create.async = false;
        BuffDesc.Buffer.usage = VKE::RenderSystem::BufferUsages::VERTEX_BUFFER;
        BuffDesc.Buffer.memoryUsage = VKE::RenderSystem::MemoryUsages::GPU_ACCESS;
        BuffDesc.Buffer.size = ( sizeof( float ) * 4 ) * 3;
        pVb = pCtx->CreateBuffer( BuffDesc );
        const float vb[4 * 3] =
        {
            0.0f,   0.5f,   0.0f,   1.0f,
            -0.5f, -0.5f,   0.0f,   1.0f,
            0.5f,  -0.5f,   0.0f,   1.0f
        };
        VKE::RenderSystem::SUpdateMemoryInfo UpdateInfo;
        UpdateInfo.pData = vb;
        UpdateInfo.dataSize = sizeof( vb );
        UpdateInfo.dstDataOffset = 0;
        pCtx->UpdateBuffer( UpdateInfo, &pVb );
        

        Layout.vAttributes =
        {
            { "Position", VKE::RenderSystem::VertexAttributeTypes::POSITION }
        };

        VKE::Scene::SSceneDesc SceneDesc;
        pScene = pCtx->GetRenderSystem()->GetEngine()->World()->CreateScene( SceneDesc );
        pCamera = pCtx->GetRenderSystem()->GetEngine()->World()->GetCamera( 0 );
        pScene->SetCamera( pCamera );
        
        VKE::Scene::SModelDesc;

        pCtx->GetRenderSystem()->GetEngine()->World()->CreateModel( ModelDesc );

        pCamera->SetLookAt( VKE::Math::CVector( 0.0f, 0.0f, 1.0f ) );
        pCamera->SetPosition( VKE::Math::CVector( 0.0f, 0.0f, -1.0f ) );
        pCamera->Update();
        
        VKE::Math::CMatrix4x4 Model, MVP;
        VKE::Math::CMatrix4x4::Translate( VKE::Math::CVector( 0.1f, 0.1f, 0.1f ), &Model );
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
        UpdateBindingInfo.AddBinding( 0, 0, UpdateInfo.dataSize, &VKE::RenderSystem::BufferHandle{ pUBO->GetHandle() }, 1 );
        pCtx->UpdateDescriptorSet( UpdateBindingInfo, &hDescSet );

        while( pPS.IsNull() )
        {

        }

        

        return pVb.IsValid();
    }

    bool OnRenderFrame(VKE::RenderSystem::CGraphicsContext* pCtx) override
    {
        /*pCtx->BeginFrame();
        pCtx->SetState( Layout );
        pCtx->Bind( pVb );
        pCtx->SetState( pVS );
        pCtx->SetState( pPS );
        pCtx->Bind( hDescSet );
        pCtx->Draw( 3 );
        pCtx->EndFrame();*/
        pScene->Render( pCtx );
        return true;
    }
};

void Main()
{
    int* a = VKE_NEW int;
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