

#include "VKE.h"
#include "vke/Core/Utils/TCConstantArray.h"
#include "vke/Core/Memory/CPoolMemoryManager.h"

#include <windows.h>
#include <crtdbg.h>

struct SGfxContextListener : public VKE::RenderSystem::EventListeners::IGraphicsContext
{
    bool OnRenderFrame(VKE::RenderSystem::CGraphicsContext* pCtx) override
    {
        return true;
    }
};

namespace VKE
{
    template
    <
        typename T,
        uint32_t DEFAULT_COUNT = 1024,
        typename HandleType = uint32_t,
        class DataContainerType = VKE::Utils::TCDynamicArray< T, DEFAULT_COUNT >,
        class HandleContainerType = VKE::Utils::TCDynamicArray< HandleType, DEFAULT_COUNT >
    >
    class TCHandledBuffer
    {
        public:

            using HandleBuffer = Utils::TCDynamicArray< HandleContainerType >;
            using FreeHandleBuffer = Utils::TCDynamicArray< HandleType, DEFAULT_COUNT >;

        public:

            TCHandledBuffer()
            {
                // Add null handle
                PushBack( T() );
            }

            ~TCHandledBuffer()
            {
            }

            HandleType PushBack(const T& Data)
            {
                HandleType idx;
                if( m_vFreeHandles.PopBack( &idx ) )
                {
                    m_vBuffer[ idx ] = Data;
                    m_vHandleMap[ idx ] = idx;
                }
                else
                {
                    uint32_t handleIdx = m_vBuffer.PushBack( Data );
                    idx = m_vHandleMap.PushBack( handleIdx );
                }
                return idx;
            }

            void Free(const HandleType& idx)
            {
                m_vFreeHandles.PushBack( idx );
            }

            void Remove(const HandleType& idx)
            {
                m_vBuffer.Remove( idx );
                m_vHandleMap.Remove( idx );
                for( uint32_t i = idx; i < m_vHandleMap.GetCount(); ++i )
                {
                    m_vHandleMap[ i ] = i;
                }
            }

            T& At(const HandleType& idx)
            {
                return m_vBuffer[ _GetDataIdx( idx ) ];
            }

            const T& At(const HandleType& idx) const
            {
                return m_vBuffer[ _GetDataIdx( idx ) ];
            }

        protected:

            vke_force_inline
            HandleType _GetDataIdx(const HandleType& idx) const
            {
                return m_vHandleMap[ idx ];
            }

        protected:

            FreeHandleBuffer    m_vFreeHandles;
            DataContainerType   m_vBuffer;
            HandleBuffer        m_vHandleMap;
    };
}

void Test()
{
    VKE::Memory::CPoolMemoryManager Mgr;
}

bool Main()
{
    Test();

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
    if (!pRenderSys)
    {
        return false;
    }
    const auto& vAdapters = pRenderSys->GetAdapters();
    const auto& Adapter = vAdapters[ 0 ];

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
        VKE::RenderSystem::SGraphicsContextDesc GraphicsDesc;
        GraphicsDesc.SwapChainDesc.pWindow = pWnd2;
        pGraphicsCtx2 = pDevCtx->CreateGraphicsContext(GraphicsDesc);
    }
    
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