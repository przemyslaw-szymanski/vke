

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
    VKE::SWindowInfo WndInfos[2];
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

    VKE::SRenderSystemInfo RenderInfo;

    VKE::SEngineInfo EngineInfo;
    EngineInfo.pRenderSystemInfo = &RenderInfo;
    EngineInfo.pWindowInfos = WndInfos;
    EngineInfo.windowInfoCount = 2;
    EngineInfo.thread.threadCount = VKE::Constants::Threads::COUNT_OPTIMAL;
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