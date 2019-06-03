

#include "../CSampleFramework.h"

struct SKeyboardListener : public VKE::EventListeners::IUserInput
{
    void OnKeyDown(const VKE::Input::Key& key) override
    {

    }

    void OnMouseMove( const VKE::Input::MousePosition& Position ) override
    {

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

        pCamera->SetLookAt( VKE::Math::CVector3( 0.0f, 0.0f, 0.0f ) );
        pCamera->SetPosition( VKE::Math::CVector3( 0.0f, 0.0f, -5.0f ) );
        pCamera->Update();
        
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
        UpdateBindingInfo.AddBinding( 0, 0, UpdateInfo.dataSize, &VKE::RenderSystem::BufferHandle{ pUBO->GetHandle() }, 1 );
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
        LOD.pVertexShader = pVS;
        LOD.pPixelShader = pPS;
        Drawcall.AddLOD( LOD );
        VKE::Scene::SDrawcallDataInfo DataInfo;
        VKE::Math::CAABB::Transform( 1.0f, VKE::Math::CVector3( 0.0f, 0.0f, 0.0f ), &DataInfo.AABB );
        pScene->AddObject( &Drawcall, DataInfo );
        

        return pVb.IsValid();
    }

    bool OnRenderFrame(VKE::RenderSystem::CGraphicsContext* pCtx) override
    {
        pCtx->BeginFrame();
        pScene->Render( pCtx );
        pCtx->EndFrame();
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