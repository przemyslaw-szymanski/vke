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
#include "Core/Managers/CFileManager.h"
#include "Core/Managers/CImageManager.h"
#include "RenderSystem/Managers/CImageManager.h"

#include "Core/Input/CInputSystem.h"

#include "Scene/CWorld.h"

#if defined CreateWindow
#undef CreateWindow
#endif
#if defined FindWindow
#undef FindWindow
#endif

static VKE::CVkEngine* g_pEngine = nullptr;

VKE_API VKE::CVkEngine* VKECreate()
{
    if (g_pEngine)
        return g_pEngine;
    //VKE::Platform::Debug::BeginDumpMemoryLeaks();
    g_pEngine = VKE_NEW VKE::CVkEngine;
    return g_pEngine;
}

VKE_API void VKEDestroy(VKE::CVkEngine** ppEngine)
{
    assert(g_pEngine == *ppEngine);
    VKE_DELETE(*ppEngine);
    //VKE::Platform::Debug::EndDumpMemoryLeaks();
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
        struct SCreateWindow : public Threads::ITask
        {
            const SWindowDesc* pDesc;
            CVkEngine* pEngine;
            WindowPtr pWnd;
            TaskState _OnStart(uint32_t threadId) override
            {
                pWnd = pEngine->_CreateWindow(*pDesc);
                printf("create wnd: %p, %d\n", pWnd.Get(), threadId);
                return TaskStateBits::OK;
            }

            void _OnGet(void** ppOut) override
            {
                WindowPtr* ppRes = reinterpret_cast<WindowPtr*>(ppOut);
                *ppRes = pWnd;
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
            Tasks::Window::SUpdate aWndUpdates[128];
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
        if (!m_pPrivate)
            return;

        //for (auto& pWnd : m_pPrivate->vWindows)
        for (auto& Pair : m_pPrivate->mWindows)
        {
            auto pWnd = Pair.second;
            pWnd->Destroy();
            m_WindowSyncObj.Lock();
            VKE_DELETE(pWnd);
            m_WindowSyncObj.Unlock();
        }

        if (m_pWorld)
        {
            m_pWorld->_Destroy();
            Memory::DestroyObject(&HeapAllocator, &m_pWorld);
        }

        Memory::DestroyObject(&HeapAllocator, &m_Managers.pImgMgr);
        Memory::DestroyObject(&HeapAllocator, &m_Managers.pFileMgr);

        m_WindowSyncObj.Lock();
        m_pPrivate->mWindows.clear();
        m_pCurrentWindow = nullptr;
        m_WindowSyncObj.Unlock();

        VKE_DELETE(m_pRS);

        VKE_DELETE(m_pThreadPool);
        //VKE_LOG("thread pool deleted.");
        VKE_DELETE_ARRAY(m_pFreeListMgr);
        //VKE_LOG("Free list mgr deleted");
        VKE_DELETE(m_pPrivate);
        //VKE_LOG("Engine deleted.");
        //auto* pLogger = Utils::CLogger::GetSingletonPtr();
        //VKE_DELETE( m_pLogger );
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

        VKE_LOG_PROG("VKEngine initialization");

        VKE_LOGGER.AddMode(Utils::LoggerModes::COMPILER);

        m_pThreadPool = VKE_NEW Threads::CThreadPool();
        if (VKE_FAILED(err = m_pThreadPool->Create(Info.thread)))
        {
            return err;
        }
        VKE_LOG_PROG("VKEngine thread pool created");
        {
            if (VKE_SUCCEEDED(err = Memory::CreateObject(&HeapAllocator, &m_Managers.pFileMgr)))
            {
                Core::SFileManagerDesc Desc;
                Desc.maxFileCount = Config::Resource::File::DEFAULT_COUNT;
                if (VKE_FAILED(err = m_Managers.pFileMgr->Create(Desc)))
                {
                    goto ERR;
                }
            }
            else
            {
                VKE_LOG_ERR("Unable to allocate memory for CFileManager.");
                goto ERR;
            }
        }
        VKE_LOG_PROG("VKEngine file manager created");

        {
            if (VKE_SUCCEEDED(err = Memory::CreateObject(&HeapAllocator, &m_Managers.pImgMgr)))
            {
                Core::SImageManagerDesc Desc;
                Desc.pFileMgr = m_Managers.pFileMgr;
                if (VKE_FAILED(err = m_Managers.pImgMgr->_Create(Desc)))
                {
                    goto ERR;
                }
            }
            else
            {
                VKE_LOG_ERR("Unable to allocate memory for CFileManager.");
                goto ERR;
            }
        }
        VKE_LOG_PROG("VKEngine file manager created");

        {
            if (VKE_FAILED(Memory::CreateObject(&HeapAllocator, &m_pWorld)))
            {
                VKE_LOG_ERR("Unable to create memory for CWorld.");
                goto ERR;
            }
            Scene::CWorld::SDesc WorldDesc;
            if (VKE_FAILED(m_pWorld->_Create(WorldDesc)))
            {
                goto ERR;
            }
        }

        return VKE_OK;
    ERR:
        Destroy();
        return VKE_FAIL;
    }

    WindowPtr CVkEngine::CreateRenderWindow(const SWindowDesc& Desc)
    {
        Task::SCreateWindow CreateWndTask;
        auto& Task = CreateWndTask;
        Task.pDesc = &Desc;
        Task.pEngine = this;
        Task.SetName( VKE_FUNCTION );
        WindowPtr pWnd;
        const Threads::CThreadPool::WorkerID id = static_cast<const Threads::CThreadPool::WorkerID>(static_cast<int32_t>(m_pPrivate->mWindows.size()));
        if (VKE_FAILED(this->GetThreadPool()->AddTask(id, &Task)))
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
        if (pWnd.IsNull())
        {
            pWnd = WindowPtr(VKE_NEW VKE::CWindow(this));
            if (pWnd.IsNull())
            {
                return WindowPtr();
            }
            if (VKE_FAILED(pWnd->Create(Desc)))
            {
                CWindow* pTmp = pWnd.Release();
                VKE_DELETE(pTmp);
                return WindowPtr();
            }

            /*m_Mutex.lock();
            const auto idx = m_pPrivate->vWindows.size();
            m_pPrivate->vWindows.push_back(pWnd);
            m_Mutex.unlock();*/
            m_WindowSyncObj.Lock();
            const auto idx = m_pPrivate->mWindows.size();
            m_pPrivate->mWindows.insert(SInternal::WndMap::value_type(pWnd->GetDesc().hWnd, pWnd.Get()));
            m_pPrivate->mWindows2.insert(SInternal::WndMap2::value_type(Desc.pTitle, pWnd.Get()));
            m_WindowSyncObj.Unlock();

            if (m_pCurrentWindow.IsNull())
            {
                m_currWndHandle = pWnd->GetDesc().hWnd;
                m_pCurrentWindow = pWnd;
            }

            auto& WndUpdateTask = m_pPrivate->Task.aWndUpdates[idx];
            WndUpdateTask.pWnd = pWnd.Get();
            WndUpdateTask.Flags |= Threads::TaskFlags::RENDER_THREAD | Threads::TaskFlags::HIGH_PRIORITY;
            Threads::CThreadPool::NativeThreadID ID = Threads::CThreadPool::NativeThreadID(pWnd->GetThreadId());
            this->GetThreadPool()->AddConstantTask(ID, &WndUpdateTask, TaskStateBits::OK);
            WndUpdateTask.IsActive(true);
        }

        return pWnd;
    }

    RenderSystem::CRenderSystem* CVkEngine::CreateRenderSystem(const SRenderSystemDesc& Info)
    {
        if (m_pRS)
            return m_pRS;
        m_pRS = VKE_NEW RenderSystem::CRenderSystem(this);
        if (!m_pRS)
            return nullptr;
        if (VKE_FAILED(m_pRS->Create(Info)))
        {
            VKE_DELETE(m_pRS);
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
        if (Itr != m_pPrivate->mWindows2.end())
        {
            m_pCurrentWindow = Itr->second;
            m_currWndHandle = m_pCurrentWindow->GetDesc().hWnd;
            return m_pCurrentWindow;
        }
        return WindowPtr();
    }

    WindowPtr CVkEngine::FindWindow(const handle_t& hWnd)
    {
        if (hWnd == m_currWndHandle)
            return m_pCurrentWindow;
        assert(m_pPrivate);

        const auto Itr = m_pPrivate->mWindows.find(hWnd);
        if (Itr != m_pPrivate->mWindows.end())
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

    void CVkEngine::DestroyRenderWindow(WindowPtr pWnd)
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
        uint32_t visibleWindowCount = 0;
        // Wait till all windows are not destroyed
        //auto& vWindows = m_pPrivate->vWindows;
        auto& mWindows = m_pPrivate->mWindows;
        //auto wndCount = vWindows.size();
        auto wndCount = mWindows.size();
        ///TODO fix this loop. needExit should not depends on window.
        while (!needExit)
        {
            {
                Threads::LockGuard l(m_Mutex);
                //wndCount = vWindows.size();
                wndCount = mWindows.size();
                wndNeedQuitCount = visibleWindowCount = 0;
                //for( auto pWnd : m_pPrivate->vWindows )
                for (auto& Pair : mWindows)
                {
                    auto pWnd = Pair.second;
                    if (pWnd->NeedQuit())
                    {
                        //pWnd->Destroy();
                        wndNeedQuitCount++;
                    }
                    else if (pWnd->IsVisible() && pWnd->GetSwapChain())
                    {
                        m_pRS->RenderFrame(WindowPtr(pWnd));
                    }
                    visibleWindowCount += pWnd->IsVisible();
                }
            }
            needExit = wndNeedQuitCount == visibleWindowCount;
        }
        // Need end rendering loop
        // Notify all tasks to end
        FinishTasks();
        // Wait for all task to end
        WaitForTasks();
    }

    void CVkEngine::StopRendering()
    {
        /*FinishTasks();
        WaitForTasks();
        m_pThreadPool->Destroy();*/
    }

    void CVkEngine::FinishTasks()
    {
        /*for( uint32_t i = 0; i < m_pPrivate->vWindows.size(); ++i )
        {
            m_pPrivate->vWindows[ i ]->NeedQuit( true );
        }*/
        for (auto& Pair : m_pPrivate->mWindows)
        {
            auto pWnd = Pair.second;
            pWnd->Close();
        }
    }

    void CVkEngine::WaitForTasks()
    {
        const auto count = m_pPrivate->mWindows.size();
        for (uint32_t i = 0; i < count; ++i)
        {
            m_pPrivate->Task.aWndUpdates[i].Wait();
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