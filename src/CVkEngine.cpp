#include "CVkEngine.h"
#include "Core/Platform/CWindow.h"
#include "Core/Platform/CPlatform.h"
#include "RenderSystem/Vulkan/CRenderSystem.h"
#include "Core/Utils/STLUtils.h"
#include "Core/Thread/CThreadPool.h"
#include "Core/Utils/CLogger.h"
#include "Core/Memory/TCFreeListManager.h"
#include "RenderSystem/Vulkan/CContext.h"
#include "Core/Thread/ITask.h"

static VKE::CVkEngine* g_pEngine = nullptr;

VKE_API VKE::CVkEngine* VKECreate()
{
    if(g_pEngine)
        return g_pEngine;
    VKE::Platform::BeginDumpMemoryLeaks();
    g_pEngine = VKE_NEW VKE::CVkEngine;
    return g_pEngine;
}

VKE_API void VKEDestroy(VKE::CVkEngine** ppEngine)
{
    assert(g_pEngine == *ppEngine);
    VKE_DELETE( *ppEngine );
    VKE::Platform::EndDumpMemoryLeaks();
}

VKE_API VKE::CVkEngine* VKEGetEngine()
{
    assert(g_pEngine);
    return g_pEngine;
}

namespace VKE
{
    struct FreeListManagers
    {
        enum
        {
            RENDER_SYSTEM_PIPELINE,
            RENDER_SYSTEM_IMAGE,
            RENDER_SYSTEM_VERTEX_BUFFER,
            RENDER_SYSTEM_INDEX_BUFFER,
            _MAX_COUNT
        };
    };

    using WndVec = vke_vector< CWindow* >;

    namespace Task
    {
        struct CCreateWindow : public Thread::ITask
        {
            SWindowInfo* pInfo;
            CVkEngine* pEngine;
            WindowPtr pWnd;
            void _OnStart(uint32_t threadId) override
            {
                pWnd = pEngine->CreateWindow( *pInfo );
                printf( "create wnd: %p, %d\n", pWnd.Get(), threadId );
            }

            void _OnGet(void* pOut)
            {
                WindowPtr* pRes = reinterpret_cast< WindowPtr* >( pOut );
                *pRes = pWnd;
            }
        };
    } // Tasks

    struct CVkEngine::SInternal
    {
        WndVec vWindows;
    };

    CVkEngine::CVkEngine()
    {

    }

    CVkEngine::~CVkEngine()
    {
        Destroy();
    }

    void CVkEngine::Destroy()
    {
        assert(m_pInternal);
        VKE_DELETE(m_pRS);

        for (auto& pWnd : m_pInternal->vWindows)
        {
            VKE_DELETE(pWnd);
        }
        
        m_pCurrentWindow = nullptr;
        VKE_DELETE(m_pThreadPool);

        auto* pLogger = Utils::CLogger::GetSingletonPtr();
        VKE_DELETE(pLogger);

        VKE_DELETE(m_pInternal);
        VKE_DELETE_ARRAY(m_pFreeListMgr);
    }

    void CVkEngine::GetEngineLimits(SEngineLimits* pLimitsOut)
    {
        pLimitsOut = pLimitsOut;
    }

    Result CVkEngine::Init(const SEngineInfo& Info)
    {
        m_pInternal = VKE_NEW CVkEngine::SInternal;
        Result err = VKE_OK;
        m_Info = Info;
        GetEngineLimits(&m_Limits);
        
        m_pFreeListMgr = VKE_NEW Memory::CFreeListManager();

        Utils::CLogger* pLogger = VKE_NEW Utils::CLogger();
        pLogger = nullptr; // do not use this as logger is a singleton

        VKE_LOGGER.AddMode(Utils::LoggerModes::COMPILER);

        m_pThreadPool = VKE_NEW CThreadPool();
        if(VKE_FAILED(err = m_pThreadPool->Create(Info.thread)))
        {
            return err; 
        }

        m_Info.windowInfoCount = Min( Info.windowInfoCount, 128 );
        TSTaskResult<Result> aErrResults[128] = {};

        Task::CCreateWindow aCreateWndTask[ 128 ] = {};

        for( uint32_t i = 0; i < Info.windowInfoCount; ++i )
        {
            auto& Task = aCreateWndTask[ i ];
            Task.pInfo = &Info.pWindowInfos[ i ];
            Task.pEngine = this;
            this->GetThreadPool()->AddTask( Constants::Thread::ID_BALANCED, &Task );
        }

        for( uint32_t i = 0; i < Info.windowInfoCount; ++i )
        {
            auto& Task = aCreateWndTask[ i ];
            Task.Wait();
            if( Task.pWnd.IsNull() )
            {
                return VKE_FAIL;
            }
            // Update window infos after create/get OS window
            m_Info.pWindowInfos[ i ] = m_pInternal->vWindows[ i ]->GetInfo();
        }
        /*
        for(uint32_t i = 0; i < Info.windowInfoCount; ++i)
        {
            //this->CreateWindow(Info.pWindowInfos[i]);

            TSTaskParam< SWindowInfo> WindowInfoParam;
            auto& WndInfo = Info.pWindowInfos[i];
            WindowInfoParam.pData = &WndInfo;
            STaskParams Params(WindowInfoParam, &aErrResults[i]);

            m_pThreadPool->AddTask(Constants::Thread::ID_BALANCED, Params, 
                                   [this](void* p, STaskResult* r){
                TSTaskInOut< SWindowInfo, Result > InOut( p, r );
                InOut.pOutput->data = VKE_OK;
                InOut.pInput->threadId = this->GetThreadPool()->GetThisThreadID();
                auto hWnd = InOut.pInput->wndHandle;
                auto pPtr = this->CreateWindow( *InOut.pInput );
                if(pPtr.IsNull())
                {
                    InOut.pOutput->data = VKE_FAIL;
                }
                else
                {
                    if (hWnd == 0)
                    {
                        this->GetThreadPool()->AddConstantTask(InOut.pInput->threadId, (void*)pPtr.Get(),
                                                               [this](int32_t, void* pData)
                        {
                            CWindow* pWnd = reinterpret_cast<CWindow*>(pData);
                            pWnd->Update();
                            //pWnd->GetRenderingContext()->Update();
                        });
                    }
                }
            });
        }

        for(uint32_t i = 0; i < Info.windowInfoCount; ++i)
        {
            err = aErrResults[i].Get();
            if(VKE_FAILED(err))
            {
                return err;
            }
            // Update window infos after create/get OS window
            m_Info.pWindowInfos[i] = m_pInternal->vWindows[i]->GetInfo();
        }
        */
        m_Info.pRenderSystemInfo->Windows.count = m_Info.windowInfoCount;
        m_Info.pRenderSystemInfo->Windows.pData = m_Info.pWindowInfos;
        TSTaskParam<SRenderSystemInfo> RenderSystemInfoParam;
        RenderSystemInfoParam.pData = m_Info.pRenderSystemInfo;
        
        STaskParams Params;
        Params.pInputParam = RenderSystemInfoParam.pData;
        Params.inputParamSize = RenderSystemInfoParam.dataSize;
        Params.pResult = &aErrResults[0];
        m_pThreadPool->AddTask(Constants::Thread::ID_BALANCED, Params, [this](void* p, STaskResult* r){
            auto* pErr = (TSTaskResult<Result>*)r;
            pErr->data = VKE_OK;
            SRenderSystemInfo* pInfo = (SRenderSystemInfo*)p;
            if(!this->CreateRenderSystem(*pInfo))
            {
                pErr->data = VKE_FAIL;
            }
        });

        err = aErrResults[0].Get();
        if(VKE_FAILED(err))
        {
            return err;
        }

        return VKE_OK;
    }

    WindowPtr CVkEngine::CreateWindow(const SWindowInfo& Info)
    {
        auto pWnd = FindWindowTS(Info.pTitle);
        if( pWnd.IsNull() )
        {
            auto pWnd = VKE_NEW VKE::CWindow();
            if( !pWnd )
            {
                return WindowPtr();
            }
            if (VKE_FAILED(pWnd->Create(Info)))
            {
                VKE_DELETE(pWnd);
                return WindowPtr();
            }
           
            m_pInternal->vWindows.push_back(pWnd);
            pWnd->SetRenderSystem(m_pRS);

            if( m_pCurrentWindow.IsNull() )
            {
                m_currWndHandle = pWnd->GetInfo().wndHandle;
                m_pCurrentWindow = pWnd;
            }

            return WindowPtr(pWnd);
        }

        return WindowPtr();
    }

    VKE::CRenderSystem* CVkEngine::CreateRenderSystem(const SRenderSystemInfo& Info)
    {
        if( m_pRS )
            return m_pRS;
        m_pRS = VKE_NEW CRenderSystem(this);
        if( !m_pRS )
            return nullptr;
        if( VKE_FAILED( m_pRS->Create( Info ) ) )
        {
            VKE_DELETE( m_pRS );
            return nullptr;
        }
        for(auto& pWnd : m_pInternal->vWindows)
        {
            pWnd->SetRenderSystem(m_pRS);
            //m_pRS->Get
        }
        return m_pRS;
    }

    WindowPtr CVkEngine::FindWindow(cstr_t pWndName)
    {
        for(auto pWnd : m_pInternal->vWindows)
        {
            const auto& Info = pWnd->GetInfo();
            if (strcmp(Info.pTitle, pWndName) == 0)
            {
                m_pCurrentWindow = pWnd;
                m_currWndHandle = Info.wndHandle;
                return m_pCurrentWindow;
            }
        }
        return WindowPtr();
    }

    WindowPtr CVkEngine::FindWindow(const handle_t& hWnd)
    {
        if(hWnd == m_currWndHandle)
            return m_pCurrentWindow;
        for(auto pWnd : m_pInternal->vWindows)
        {
            const auto& Info = pWnd->GetInfo();
            if(Info.wndHandle == hWnd)
            {
                m_pCurrentWindow = pWnd;
                m_currWndHandle = Info.wndHandle;
                return m_pCurrentWindow;
            }
        }
        return WindowPtr();
    }

    WindowPtr CVkEngine::FindWindowTS(cstr_t pWndName)
    {
        Thread::LockGuard l(m_Mutex);
        return FindWindow(pWndName);
    }

    WindowPtr CVkEngine::FindWindowTS(const handle_t& hWnd)
    {
        Thread::LockGuard l(m_Mutex);
        return FindWindow(hWnd);
    }

    void CVkEngine::BeginFrame()
    {

    }

    void CVkEngine::EndFrame()
    {

    }

    void CVkEngine::StartRendering()
    {
        // Add task to thread pool
        bool needRender = true;
        size_t wndCount = m_pInternal->vWindows.size();
        auto threadId = std::this_thread::get_id();

        while(needRender)
        {
            size_t wndReady = wndCount;
            for(auto pWnd : m_pInternal->vWindows)
            {
                //pWnd->Update();
                wndReady -= pWnd->NeedQuit();
                //m_pRS->RenderFrame(WindowPtr(pWnd));
            }

            needRender = (wndReady > 0);
        }
    }

} // VKE