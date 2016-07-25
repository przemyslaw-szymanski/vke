
#include "VKE.h"

#include "vke/Core/Utils/TCSmartPtr.h"
#include "vke/Core/CObject.h"


bool Main()
{
    VKE::CVkEngine* pEngine = VKECreate();
    VKE::SWindowInfo WndInfos[2];
    WndInfos[0].fullScreen = false;
    WndInfos[0].vSync = false;
    WndInfos[0].wndHandle = 0;
    WndInfos[0].pTitle = "main";
    WndInfos[0].Size = { 800, 600 };

    WndInfos[1].fullScreen = false;
    WndInfos[1].vSync = false;
    WndInfos[1].wndHandle = 0;
    WndInfos[1].pTitle = "main2";
    WndInfos[1].Size = { 800, 600 };

    VKE::SRenderSystemInfo RenderInfo;

    VKE::SEngineInfo EngineInfo;
    EngineInfo.pRenderSystemInfo = &RenderInfo;
    EngineInfo.pWindowInfos = WndInfos;
    EngineInfo.windowInfoCount = 2;
    EngineInfo.thread.threadCount = VKE::Constants::Thread::COUNT_OPTIMAL;
    EngineInfo.thread.taskMemSize = 1024; // 1kb per task
    EngineInfo.thread.maxTaskCount = 1024;

    auto err = pEngine->Init(EngineInfo);
    if(VKE_FAILED(err))
    {
        VKEDestroy(&pEngine);
        return false;
    }

    pEngine->GetWindow()->Show(true);

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