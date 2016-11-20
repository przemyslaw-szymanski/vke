

#include "VKE.h"
#include "vke/Core/Utils/TCDynamicArray.h"

void Test()
{
    VKE::Platform::Thread::CSpinlock lock;
    VKE::Utils::TCDynamicArray<int> arr;
    
    std::thread threads[ 3 ];
    threads[ 0 ] = std::thread([ & ]()
    {
        lock.Lock();
        for( uint32_t i = 0; i < 10; ++i )
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            
            arr.PushBack(i);
            
        }lock.Unlock();
    });

    threads[ 1 ] = std::thread([ & ]()
    {
        for( uint32_t i = 10; i < 20; ++i )
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            lock.Lock();
            arr.PushBack(i);
            lock.Unlock();
        }
    });

    threads[ 2 ] = std::thread([ & ]()
    {
        for( uint32_t i = 30; i < 40; ++i )
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(15));
            lock.Lock();
            arr.PushBack(i);
            lock.Unlock();
        }
    });

    /*threads[ 0 ].detach();
    threads[ 1 ].detach();
    threads[ 2 ].detach();*/
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    arr.GetCount();
}

bool Main()
{
    //Test();

    VKE::CVkEngine* pEngine = VKECreate();
    VKE::SWindowDesc WndInfos[2];
    WndInfos[ 0 ].mode = VKE::WindowModes::WINDOW;
    WndInfos[0].vSync = false;
    WndInfos[0].wndHandle = 0;
    WndInfos[0].pTitle = "main";
    WndInfos[0].Size = { 800, 600 };

    WndInfos[ 1 ].mode = VKE::WindowModes::WINDOW;
    WndInfos[1].vSync = false;
    WndInfos[1].wndHandle = 0;
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
    auto pWnd2 = pEngine->CreateWindow(WndInfos[ 1 ]);

    VKE::RenderSystem::SRenderSystemDesc RenderSysDesc;
    
    auto pRenderSys = pEngine->CreateRenderSystem( RenderSysDesc );
    const auto& vAdapters = pRenderSys->GetAdapters();
    const auto& Adapter = vAdapters[ 0 ];

    VKE::RenderSystem::SDeviceContextDesc DevCtxDesc1, DevCtxDesc2;
    DevCtxDesc1.pAdapterInfo = &Adapter;
    DevCtxDesc2.pAdapterInfo = &Adapter;
    auto pDevCtx1 = pRenderSys->CreateDeviceContext(DevCtxDesc1);
    auto pDevCtx2 = pRenderSys->CreateDeviceContext(DevCtxDesc2);
    
    pWnd1->IsVisible(true);
    pWnd2->IsVisible(true);

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