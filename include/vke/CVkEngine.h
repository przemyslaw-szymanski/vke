#pragma once

#include "Core/VKECommon.h"
#include "Core/VKEForwardDeclarations.h"
#include "Core/Platform/Common.h"
#include "RenderSystem/Common.h"
#include "Core/Threads/Common.h"
#include "Core/Utils/CLogger.h"

#if defined(CreateWindow)
#undef CreateWindow
#endif

#if defined(FindWindow)
#undef FindWindow
#endif

namespace VKE
{
    struct VKE_API SEngineInfo
    {
        cstr_t              pApplicationName = "unknown";
        cstr_t              pName = "Vulkan Engine";
        uint32_t            version = 1;
        uint32_t            applicationVersion = 0;
        SThreadPoolInfo     thread;
    };

    struct VKE_API SEngineLimits
    {
        uint16_t    maxWindowCount = 128;
    };

    namespace Core
    {
        class CFileManager;
    }

    namespace Memory
    {
        class CFreeListManager;
    }

    namespace RenderSystem
    {
        class CRenderSystem;
    }

    namespace Scene
    {
        class CWorld;
        class CScene;
    }

    struct SManagers
    {
        Core::CFileManager*     pFileMgr = nullptr;
    };

    class VKE_API CVkEngine final
    {
        struct SInternal;
        public:

                            CVkEngine();
                            ~CVkEngine();

            Result          Init(const SEngineInfo& Info);
            void            Destroy();

            void            GetEngineLimits(SEngineLimits* pLimitsOut);
            const 
            SEngineLimits&  GetEngineLimits() const { return m_Limits; }

            const
            SEngineInfo&    GetInfo() const { return m_Desc; }

            WindowPtr       CreateRenderWindow(const SWindowDesc& Desc);
            WindowPtr       _CreateWindow(const SWindowDesc& Desc);
            void            DestroyRenderWindow(WindowPtr pWnd);
            WindowPtr       GetWindow() { return m_pCurrentWindow; }
            const
            WindowPtr       GetWindow() const { return m_pCurrentWindow; }
            WindowPtr       FindWindow(cstr_t pWndName);
            WindowPtr       FindWindow(const handle_t& hWnd);
            WindowPtr       FindWindowTS(cstr_t pWndName);
            WindowPtr       FindWindowTS(const handle_t& hWnd);
            uint32_t        GetWindowCountTS();

            const SManagers&    GetManagers() const { return m_Managers; }

            CThreadPool*    GetThreadPool() const { return m_pThreadPool; }

            void            FinishTasks();
            void            WaitForTasks();

            RenderSystem::CRenderSystem*  CreateRenderSystem(const SRenderSystemDesc& Info);

            void            BeginFrame();
            void            EndFrame();
            void            StartRendering();
            void            StopRendering();

            Scene::CWorld*  World() { return m_pWorld; }

            void            SetInputListener( Input::EventListeners::IInput* pListener );

        protected:

            SEngineLimits   m_Limits;
            SEngineInfo     m_Desc;
            SInternal*      m_pPrivate = nullptr;
            WindowPtr       m_pCurrentWindow;
            handle_t        m_currWndHandle = 0;
            SManagers       m_Managers;
            Scene::CWorld*  m_pWorld;
            RenderSystem::CRenderSystem*  m_pRS = nullptr;
            CThreadPool*    m_pThreadPool = nullptr;
            std::mutex      m_Mutex;
            Threads::SyncObject m_WindowSyncObj;
            Memory::CFreeListManager*   m_pFreeListMgr = nullptr;
    };
} // VKE

extern "C"
{
    VKE_API VKE::CVkEngine* VKECreate();
    VKE_API void VKEDestroy(VKE::CVkEngine**);
    VKE_API VKE::CVkEngine* VKEGetEngine();
}
