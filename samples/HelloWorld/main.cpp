
#include "VKE.h"

#include "vke/Core/Utils/TCSmartPtr.h"
#include "vke/Core/CObject.h"
#include "vke/Core/Math/Math.h"

union DirtyFlags
{
	DirtyFlags() {}

	DirtyFlags(int b1, int b2, int b3, int b4, int b5, int b6) :
		bit1(b1),
		bit2(b2),
		bit3(b3),
		bit4(b4),
		bit5(b5),
		bit6(b6)
	{}

	DirtyFlags(uint64_t v0, uint64_t v1) :
		Value0(v0),
		Value1(v1)
	{}

	DirtyFlags(const DirtyFlags& Other) :
		DirtyFlags(Other.Value0, Other.Value1)
	{}

	uint32_t ConstantRegistersMask;

	struct
	{
		// QW0, DW0
		uint32_t bit1 : 1;
		uint32_t bit2 : 2;
		uint32_t bit3 : 29;
		// QW0, DW1
		uint32_t bit4 : 32;
		// QW1, DW0
		uint32_t bit5 : 32;
		// QW1, DW1
		uint32_t bit6 : 32;
	};
	
	struct
	{
		uint64_t Value0;
		uint64_t Value1;
	};
};

const DirtyFlags g_cSyncBtpVs = DirtyFlags
(
	// QW0, DW0
	0, // field desc BITMASK(0), etc
	1,
	2345,
	235235,
	23424,
	234234
);


bool Main()
{
	static DirtyFlags BtpDirtyFlags[] =
	{
		g_cSyncBtpVs
		// ...
	};

	DirtyFlags pDirtyFlags;
	pDirtyFlags.Value0 |= BtpDirtyFlags[0 /*shaderType*/].Value0;

    VKE::Math::CVector v1, v2, v3;

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