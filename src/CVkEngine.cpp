#include "CVkEngine.h"
#include "Core/Platform/CWindow.h"
#include "Core/Platform/CPlatform.h"
#include "RenderSystem/CRenderSystem.h"
#include "Core/Utils/STLUtils.h"
#include "Core/Threads/CThreadPool.h"
#include "Core/Utils/CLogger.h"
#include "Core/Memory/TCFreeListManager.h"
#include "RenderSystem/CGraphicsContext.h"
#include "Core/Threads/ITask.h"

#if defined CreateWindow
#undef CreateWindow
#endif
#if defined FindWindow
#undef FindWindow
#endif

static VKE::CVkEngine* g_pEngine = nullptr;

VKE_API VKE::CVkEngine* VKECreate()
{
    if(g_pEngine)
        return g_pEngine;
    VKE::Platform::Debug::BeginDumpMemoryLeaks();
    g_pEngine = VKE_NEW VKE::CVkEngine;
    return g_pEngine;
}

VKE_API void VKEDestroy(VKE::CVkEngine** ppEngine)
{
    assert(g_pEngine == *ppEngine);
    VKE_DELETE( *ppEngine );
    VKE::Platform::Debug::EndDumpMemoryLeaks();
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
        struct CCreateWindow : public Threads::ITask
        {
            const SWindowDesc* pDesc;
            CVkEngine* pEngine;
            WindowPtr pWnd;
            Status _OnStart(uint32_t threadId) override
            {
                pWnd = pEngine->_CreateWindow( *pDesc );
                printf( "create wnd: %p, %d\n", pWnd.Get(), threadId );
                return Status::OK;
            }

            void _OnGet(void* pOut) override
            {
                WindowPtr* pRes = reinterpret_cast< WindowPtr* >( pOut );
                *pRes = pWnd;
            }
        };

        struct SWindowUpdate : public Threads::ITask
        {
            CWindow* pWnd;
            Status _OnStart(uint32_t)
            {
                return pWnd->Update()? Status::OK : Status::REMOVE;
            }
        };
    } // Tasks

    struct CVkEngine::SInternal
    {
        using WndMap = vke_hash_map< handle_t, CWindow* >;
        using WndMap2 = vke_hash_map< std::string, CWindow* >;

        WndMap mWindows;
        WndMap2 mWindows2;
        //WndVec vWindows;
        
        struct  
        {
            Task::SWindowUpdate aWndUpdates[128];
        } Task;
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
        if( !m_pPrivate )
            return;

        //for (auto& pWnd : m_pPrivate->vWindows)
        for(auto& Pair : m_pPrivate->mWindows )
        {
            auto pWnd = Pair.second;
            pWnd->Destroy();
            m_WindowSyncObj.Lock();
            VKE_DELETE(pWnd);
            m_WindowSyncObj.Unlock();
        }
        m_WindowSyncObj.Lock();
        m_pPrivate->mWindows.clear();
        m_pCurrentWindow = nullptr;
        m_WindowSyncObj.Unlock();

        VKE_DELETE(m_pRS);

        auto* pLogger = Utils::CLogger::GetSingletonPtr();
        VKE_DELETE(m_pThreadPool);
        VKE_LOG("thread pool deleted.");
        VKE_DELETE_ARRAY(m_pFreeListMgr);
        VKE_LOG("Free list mgr deleted");
        VKE_DELETE(m_pPrivate);
        VKE_LOG("Engine deleted.");
        VKE_DELETE(pLogger);
    }

    void CVkEngine::GetEngineLimits(SEngineLimits* pLimitsOut)
    {
        pLimitsOut = pLimitsOut;
    }

    Result CVkEngine::Init(const SEngineInfo& Info)
    {
        m_pPrivate = VKE_NEW CVkEngine::SInternal;
        Result err = VKE_OK;
        m_Desc = Info;
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

        //m_Desc.windowInfoCount = Min( Info.windowInfoCount, 128 );
        //TSTaskResult<Result> aErrResults[128] = {};

        //Task::CCreateWindow aCreateWndTask[ 128 ] = {};

        //for( uint32_t i = 0; i < Info.windowInfoCount; ++i )
        //{
        //    auto& Task = aCreateWndTask[ i ];
        //    Task.pInfo = &Info.pWindowInfos[ i ];
        //    Task.pEngine = this;
        //    this->GetThreadPool()->AddTask( Constants::Threads::ID_BALANCED, &Task );
        //}

        //for( uint32_t i = 0; i < Info.windowInfoCount; ++i )
        //{
        //    auto& Task = aCreateWndTask[ i ];
        //    Task.Wait();
        //    if( Task.pWnd.IsNull() )
        //    {
        //        return VKE_FAIL;
        //    }
        //    // Update window infos after create/get OS window
        //    auto pWnd = m_pPrivate->vWindows[i];
        //    m_Desc.pWindowInfos[ i ] = pWnd->GetInfo();
        //    m_pPrivate->Task.aWndUpdates[i].pWnd = pWnd;
        //    this->GetThreadPool()->AddConstantTask(pWnd->GetThreadId(), &m_pPrivate->Task.aWndUpdates[i]);
        //}

        //m_Desc.pRenderSystemInfo->Windows.count = m_Desc.windowInfoCount;
        //m_Desc.pRenderSystemInfo->Windows.pData = m_Desc.pWindowInfos;
        //TSTaskParam<SRenderSystemDesc> RenderSystemInfoParam;
        //RenderSystemInfoParam.pData = m_Desc.pRenderSystemInfo;
        
        /*STaskParams Params;
        Params.pInputParam = RenderSystemInfoParam.pData;
        Params.inputParamSize = RenderSystemInfoParam.dataSize;
        Params.pResult = &aErrResults[0];
        m_pThreadPool->AddTask(Constants::Threads::ID_BALANCED, Params, [this](void* p, STaskResult* r){
            auto* pErr = (TSTaskResult<Result>*)r;
            pErr->data = VKE_OK;
            SRenderSystemDesc* pInfo = (SRenderSystemDesc*)p;
            if(!this->CreateRenderSystem(*pInfo))
            {
                pErr->data = VKE_FAIL;
            }
        });

        err = aErrResults[0].Get();
        if(VKE_FAILED(err))
        {
            return err;
        }*/

        return VKE_OK;
    }

    WindowPtr CVkEngine::CreateWindow(const SWindowDesc& Desc)
    {
        Task::CCreateWindow CreateWndTask;
        auto& Task = CreateWndTask;
        Task.pDesc = &Desc;
        Task.pEngine = this;
        WindowPtr pWnd;
        CThreadPool::WorkerID id = static_cast< CThreadPool::WorkerID >( m_pPrivate->mWindows.size() );
        if( VKE_FAILED(this->GetThreadPool()->AddTask(id, &Task)) )
        {
            return pWnd;
        }
        // Wait for task
        Task.Get(&pWnd);
        return pWnd;
    }

    WindowPtr CVkEngine::_CreateWindow(const SWindowDesc& Desc)
    {
        auto pWnd = FindWindow(Desc.pTitle);
        if( pWnd.IsNull() )
        {
            auto pWnd = VKE_NEW VKE::CWindow(this);
            if( !pWnd )
            {
                return WindowPtr();
            }
            if (VKE_FAILED(pWnd->Create(Desc)))
            {
                VKE_DELETE(pWnd);
                return WindowPtr();
            }

            /*m_Mutex.lock();
            const auto idx = m_pPrivate->vWindows.size();
            m_pPrivate->vWindows.push_back(pWnd);
            m_Mutex.unlock();*/
            m_WindowSyncObj.Lock();
            const auto idx = m_pPrivate->mWindows.size();
            m_pPrivate->mWindows.insert(SInternal::WndMap::value_type(pWnd->GetDesc().hWnd, pWnd));
            m_pPrivate->mWindows2.insert(SInternal::WndMap2::value_type(Desc.pTitle, pWnd));
            m_WindowSyncObj.Unlock();

            if( m_pCurrentWindow.IsNull() )
            {
                m_currWndHandle = pWnd->GetDesc().hWnd;
                m_pCurrentWindow = pWnd;
            }

            auto& WndUpdateTask = m_pPrivate->Task.aWndUpdates[ idx ];
            WndUpdateTask.pWnd = pWnd;
            this->GetThreadPool()->AddConstantTask(pWnd->GetThreadId(), &WndUpdateTask);
            return WindowPtr(pWnd);
        }

        return WindowPtr();
    }

    RenderSystem::CRenderSystem* CVkEngine::CreateRenderSystem(const SRenderSystemDesc& Info)
    {
        if( m_pRS )
            return m_pRS;
        m_pRS = VKE_NEW RenderSystem::CRenderSystem(this);
        if( !m_pRS )
            return nullptr;
        if( VKE_FAILED( m_pRS->Create( Info ) ) )
        {
            VKE_DELETE( m_pRS );
            return nullptr;
        }

        return m_pRS;
    }

    WindowPtr CVkEngine::FindWindow(cstr_t pWndName)
    {
        /*for(auto pWnd : m_pPrivate->vWindows)
        {
            const auto& Info = pWnd->GetDesc();
            if (strcmp(Info.pTitle, pWndName) == 0)
            {
                m_pCurrentWindow = pWnd;
                m_currWndHandle = Info.hWnd;
                return m_pCurrentWindow;
            }
        }*/
        const vke_string strName(pWndName);
        const auto Itr = m_pPrivate->mWindows2.find(strName);
        if( Itr != m_pPrivate->mWindows2.end() )
        {
            m_pCurrentWindow = Itr->second;
            m_currWndHandle = m_pCurrentWindow->GetDesc().hWnd;
            return m_pCurrentWindow;
        }
        return WindowPtr();
    }

    WindowPtr CVkEngine::FindWindow(const handle_t& hWnd)
    {
        if(hWnd == m_currWndHandle)
            return m_pCurrentWindow;
        assert(m_pPrivate);

        const auto Itr = m_pPrivate->mWindows.find(hWnd);
        if( Itr != m_pPrivate->mWindows.end() )
        {
            m_pCurrentWindow = Itr->second;
            m_currWndHandle = m_pCurrentWindow->GetDesc().hWnd;
            return m_pCurrentWindow;
        }
        return WindowPtr();
    }

    WindowPtr CVkEngine::FindWindowTS(cstr_t pWndName)
    {
        Threads::ScopedLock l(m_WindowSyncObj);
        return FindWindow(pWndName);
    }

    WindowPtr CVkEngine::FindWindowTS(const handle_t& hWnd)
    {
        m_WindowSyncObj.Lock();
        auto pWnd = FindWindow(hWnd);
        m_WindowSyncObj.Unlock();
        return pWnd;
    }

    void CVkEngine::DestroyWindow(WindowPtr pWnd)
    {
        const auto hWnd = pWnd->GetDesc().hWnd;
        pWnd->Destroy();
        m_WindowSyncObj.Lock();
        m_pPrivate->mWindows.erase(hWnd);
        auto pWindow = pWnd.Get();
        Memory::DestroyObject(&HeapAllocator, &pWindow);
        m_WindowSyncObj.Unlock();
    }

    void CVkEngine::BeginFrame()
    {

    }

    void CVkEngine::EndFrame()
    {

    }

    void CVkEngine::StartRendering()
    {
        bool needExit = false;
        uint32_t wndNeedQuitCount = 0;
        // Wait till all windows are not destroyed
        //auto& vWindows = m_pPrivate->vWindows;
        auto& mWindows = m_pPrivate->mWindows;
        //auto wndCount = vWindows.size();
        auto wndCount = mWindows.size();
        while( !needExit )
        {
            {
                Threads::LockGuard l(m_Mutex);
                //wndCount = vWindows.size();
                wndCount = mWindows.size();
                wndNeedQuitCount = 0;
                //for( auto pWnd : m_pPrivate->vWindows )
                for(auto& Pair : mWindows )
                {
                    auto pWnd = Pair.second;
                    if( pWnd->NeedQuit() )
                    {
                        //pWnd->Destroy();
                        wndNeedQuitCount++;
                    }
                }
            }
            needExit = wndNeedQuitCount == wndCount;
        }
        // Need end rendering loop
        // Notify all tasks to end
        FinishTasks();
        // Wait for all task to end
        WaitForTasks();
    }

    void CVkEngine::FinishTasks()
    {
        /*for( uint32_t i = 0; i < m_pPrivate->vWindows.size(); ++i )
        {
            m_pPrivate->vWindows[ i ]->NeedQuit( true );
        }*/
        for( auto& Pair : m_pPrivate->mWindows )
        {
            auto pWnd = Pair.second;
            pWnd->NeedQuit(true);
        }
    }

    void CVkEngine::WaitForTasks()
    {
        const auto count = m_pPrivate->mWindows.size();
        for( uint32_t i = 0; i < count; ++i )
        {
            m_pPrivate->Task.aWndUpdates[ i ].Wait();
        }
    }

    uint32_t CVkEngine::GetWindowCountTS()
    {
        m_WindowSyncObj.Lock();
        const uint32_t count = static_cast<uint32_t>(m_pPrivate->mWindows.size());
        m_WindowSyncObj.Unlock();
        return count;
    }

} // VKE