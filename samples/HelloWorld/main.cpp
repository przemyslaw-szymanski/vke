
#include "VKE.h"

#include "vke/Core/Utils/TCSmartPtr.h"
#include "vke/Core/CObject.h"
#include "vke/Core/Math/Math.h"
#include "vke/Core/Utils/TCDynamicArray.h"
#include "vke/Core/Utils/CTimer.h"

struct Policy
{
    struct Resize
    {
        static uint32_t Calc(uint32_t c) { return VKE::Utils::DynamicArrayDefaultPolicy::Resize::Calc(c); }
    };

    struct Reserve
    {
        static uint32_t Calc(uint32_t c) { return VKE::Utils::DynamicArrayDefaultPolicy::Reserve::Calc(c); }
    };

    struct PushBack
    {
        static uint32_t Calc(uint32_t c) { return c * 2; }
    };
};

bool Main()
{
    VKE::Utils::TCArrayContainer<int> a;
    a.Resize(10);
    a.Reserve(100);

    VKE::Utils::TCDynamicArray<int, 32, VKE::Memory::CHeapAllocator, Policy> v(5); 
    std::vector<int> v2;

    VKE::Utils::CTimer tm1, tm2;
    auto count = 100000;
    v.Reserve(count);
    v2.reserve(count);
    tm1.Start();
    for (uint32_t i = 0; i < count; ++i)
    {
        v.PushBack(i);
    }
    auto diff1 = tm1.GetElapsedTime<VKE::TimeUnits::Milliseconds>();

    v.Remove(3);

    tm2.Start();
    for (uint32_t i = 0; i < count; ++i)
    {
        v2.push_back(i);
    }

    auto diff2 = tm2.GetElapsedTime<VKE::TimeUnits::Milliseconds>();


    return true;

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

    pEngine->FindWindow("main")->IsVisibleAsync(true);
    pEngine->FindWindow("main2")->IsVisibleAsync(true);

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