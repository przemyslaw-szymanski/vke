#pragma once

#include "VKECommon.h"
#include "VKEForwardDeclarations.h"
#include "Platform/Common.h"
#include "RenderSystem/Common.h"
#include "Thread/Common.h"

#if defined(CreateWindow)
#undef CreateWindow
#endif

namespace VKE
{
    struct VKE_API SEngineInfo
    {
        SWindowInfo*        pWindowInfos = nullptr;
        SRenderSystemInfo*  pRenderSystemInfo = nullptr;
        cstr_t              pApplicationName = "unknown";
        cstr_t              pName = "Vulkan Engine";
        uint32_t            windowInfoCount = 0;
        uint32_t            version = 1;
        uint32_t            applicationVersion = 0;
        SThreadPoolInfo     thread;
    };

    struct VKE_API SEngineLimits
    {
        uint16_t    maxWindowCount = 128;
    };

    namespace Memory
    {
        class CFreeListManager;
    }

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
            SEngineInfo&    GetInfo() const { return m_Info; }

            WindowPtr       CreateWindow(const SWindowInfo& Info);
            WindowPtr       GetWindow() { return m_pCurrentWindow; }
            const
            WindowPtr       GetWindow() const { return m_pCurrentWindow; }
            WindowPtr       GetWindow(cstr_t pWndName);
            WindowPtr       GetWindow(const handle_t& hWnd);

            CRenderSystem*  CreateRenderSystem(const SRenderSystemInfo& Info);

            void            BeginFrame();
            void            EndFrame();
            void            StartRendering();

        protected:

            SEngineLimits   m_Limits;
            SEngineInfo     m_Info;
            SInternal*      m_pInternal = nullptr;
            WindowPtr       m_pCurrentWindow;
            handle_t        m_currWndHandle = NULL_HANDLE;
            CRenderSystem*  m_pRS = nullptr;
            CThreadPool*    m_pThreadPool = nullptr;
            Memory::CFreeListManager*   m_pFreeListMgr = nullptr;
    };
} // VKE

extern "C"
{
    VKE_API VKE::CVkEngine* VKECreate();
    VKE_API void VKEDestroy(VKE::CVkEngine**);
    VKE_API VKE::CVkEngine* VKEGetEngine();
}
