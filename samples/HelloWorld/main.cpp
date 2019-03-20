

#include "VKE.h"
#include "vke/Core/Utils/TCConstantArray.h"
#include "vke/Core/Memory/CMemoryPoolManager.h"
#include "vke/Core/Utils/TCString.h"

struct SGfxContextListener : public VKE::RenderSystem::EventListeners::IGraphicsContext
{
    bool initialized = false;
    VKE::RenderSystem::ShaderPtr pVertexShader;
    VKE::RenderSystem::BufferPtr pVertexBuffer;

    bool OnRenderFrame(VKE::RenderSystem::CGraphicsContext* pCtx) override
    {
        if( !initialized )
        {
            /*initialized = true;
            VKE::RenderSystem::SShaderCreateDesc ShaderDesc;
            ShaderDesc.Create.async = false;
            ShaderDesc.Shader.Base.pFileName = "data\\shaders\\test.vs";
            ShaderDesc.Shader.type = VKE::RenderSystem::ShaderTypes::VERTEX;
            VKE::RenderSystem::SCreateBufferDesc VBDesc;
            pVertexShader = pCtx->GetDeviceContext()->CreateShader( ShaderDesc );
            pVertexBuffer = pCtx->GetDeviceContext()->CreateBuffer( VBDesc );*/
        }
        VKE::RenderSystem::CommandBufferPtr pCb = pCtx->CreateCommandBuffer( VKE::RenderSystem::DDI_NULL_HANDLE );
        pCb->Begin();
        //pCb->Set( pVertexShader );
        ////pCb->SetBuffer( pVertexBuffer );
        ////pCb->Draw( 3 );
        pCb->End();
        pCb->Flush();
        return true;
    }
};

struct STest : public VKE::Core::CObject
{
    struct ST
    {
        VKE::Utils::TCDynamicArray< vke_string > vStrs;
    };
    ST s;

    STest()
    {
        s.vStrs.PushBack("a");
    }
};


void Test()
{
    /*VKE::Utils::TCString<> str1, str2;
    str1 = "abc";
    str1 += "_def";
    str1 = "ergd";
    str1.Copy(&str2);*/
    VKE::Utils::TCDynamicArray< int > vA = { 1,2,3,4,5 }, vB = {11,12,13,14};
    vA.Insert(1, 0, 3, vB.GetData());
    vB.Clear();
}

bool Main()
{
    Test();
    
    const int wndCount = 2;
    const int adapterNum = 0;

    VKE::CVkEngine* pEngine = VKECreate();
    VKE::SWindowDesc WndInfos[2];
    WndInfos[ 0 ].mode = VKE::WindowModes::WINDOW;
    WndInfos[0].vSync = false;
    WndInfos[0].hWnd = 0;
    WndInfos[0].pTitle = "main";
    WndInfos[0].Size = { 800, 600 };

    WndInfos[ 1 ].mode = VKE::WindowModes::WINDOW;
    WndInfos[1].vSync = false;
    WndInfos[1].hWnd = 0;
    WndInfos[1].pTitle = "main2";
    WndInfos[1].Size = { 800, 600 };

    VKE::SRenderSystemDesc RenderInfo;
    SGfxContextListener Listener;

    VKE::SEngineInfo EngineInfo;
    EngineInfo.thread.threadCount = VKE::Constants::Threads::COUNT_OPTIMAL;
    EngineInfo.thread.taskMemSize = 1024; // 1kb per task
    EngineInfo.thread.maxTaskCount = 1024;
    
    auto err = pEngine->Init(EngineInfo);
    if(VKE_FAILED(err))
    {
        VKEDestroy(&pEngine);
        return false;
    }

    auto pWnd1 = pEngine->CreateRenderWindow(WndInfos[ 0 ]);
    auto pWnd2 = pEngine->CreateRenderWindow(WndInfos[ 1 ]);

    VKE::RenderSystem::SRenderSystemDesc RenderSysDesc;
    
    auto pRenderSys = pEngine->CreateRenderSystem( RenderSysDesc );
    if (!pRenderSys)
    {
        return false;
    }
    const auto& vAdapters = pRenderSys->GetAdapters();
    const auto& Adapter = vAdapters[ vAdapters.GetCount() > adapterNum ? adapterNum : 0 ];

    
    // Run on first device only
    VKE::RenderSystem::SDeviceContextDesc DevCtxDesc1, DevCtxDesc2;
    DevCtxDesc1.pAdapterInfo = &Adapter;
    auto pDevCtx = pRenderSys->CreateDeviceContext(DevCtxDesc1);

    VKE::RenderSystem::CGraphicsContext* pGraphicsCtx1, *pGraphicsCtx2;
    {

        VKE::RenderSystem::SGraphicsContextDesc GraphicsDesc;
        GraphicsDesc.SwapChainDesc.pWindow = pWnd1;
        pGraphicsCtx1 = pDevCtx->CreateGraphicsContext(GraphicsDesc);
    }
    {
        if( wndCount == 2 )
        {
            VKE::RenderSystem::SGraphicsContextDesc GraphicsDesc;
            GraphicsDesc.SwapChainDesc.pWindow = pWnd2;
            pGraphicsCtx2 = pDevCtx->CreateGraphicsContext( GraphicsDesc );
            //pGraphicsCtx2->SetEventListener( &Listener );
        }
    }
    {
        
    }
    
    pWnd1->IsVisible(true);
    if( wndCount == 2 )
    {
        pWnd2->IsVisible( true );
    }
    pEngine->StartRendering();

    Listener.pVertexBuffer = nullptr;
    Listener.pVertexShader = nullptr;

    VKEDestroy(&pEngine);

    return true;
}

int main()
{
    Main();
    //VKE::Platform::DumpMemoryLeaks();
    return 0;
}