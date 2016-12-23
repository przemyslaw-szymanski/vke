

#include "VKE.h"

#include <windows.h>
#include <crtdbg.h>

bool Main()
{
    //Test();

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

    VKE::SEngineInfo EngineInfo;
    /*EngineInfo.pRenderSystemInfo = &RenderInfo;
    EngineInfo.pWindowInfos = WndInfos;
    EngineInfo.windowInfoCount = 2;*/
    EngineInfo.thread.threadCount = VKE::Constants::Threads::COUNT_OPTIMAL;
    EngineInfo.thread.taskMemSize = 1024; // 1kb per task
    EngineInfo.thread.maxTaskCount = 1024;
    
    auto err = pEngine->Init(EngineInfo);
    if(VKE_FAILED(err))
    {
        VKEDestroy(&pEngine);
        return false;
    }

    auto pWnd1 = pEngine->CreateWindow(WndInfos[ 0 ]);
    //auto pWnd2 = pEngine->CreateWindow(WndInfos[ 1 ]);

    VKE::RenderSystem::SRenderSystemDesc RenderSysDesc;
    
    auto pRenderSys = pEngine->CreateRenderSystem( RenderSysDesc );
    const auto& vAdapters = pRenderSys->GetAdapters();
    const auto& Adapter = vAdapters[ 0 ];

    // Run on first device only
    VKE::RenderSystem::SDeviceContextDesc DevCtxDesc1, DevCtxDesc2;
    DevCtxDesc1.pAdapterInfo = &Adapter;
    auto pDevCtx = pRenderSys->CreateDeviceContext(DevCtxDesc1);

    VKE::RenderSystem::CGraphicsContext* pGraphicsCtx1, *pGraphicsCtx2;
    {
        VKE::RenderSystem::SGraphicsContextDesc GraphicsDesc;
        GraphicsDesc.SwapChainDesc.hProcess = pWnd1->GetDesc().hProcess;
        GraphicsDesc.SwapChainDesc.hWnd = pWnd1->GetDesc().hWnd;
        pGraphicsCtx1 = pDevCtx->CreateGraphicsContext(GraphicsDesc);
    }
    {
        /*VKE::RenderSystem::SGraphicsContextDesc GraphicsDesc;
        GraphicsDesc.SwapChainDesc.hProcess = pWnd2->GetDesc().hProcess;
        GraphicsDesc.SwapChainDesc.hWnd = pWnd2->GetDesc().hWnd;
        pGraphicsCtx2 = pDevCtx->CreateGraphicsContext(GraphicsDesc);*/
    }
    
    pWnd1->IsVisible(true);
    //pWnd2->IsVisible(true);

    pEngine->StartRendering();

    VKEDestroy(&pEngine);

    return true;
}

int main()
{
    Main();
    //VKE::Platform::DumpMemoryLeaks();
    return 0;
}