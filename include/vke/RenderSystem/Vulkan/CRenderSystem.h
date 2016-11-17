#pragma once

#include "RenderSystem/Common.h"
#include "Core/Utils/TCSmartPtr.h"

namespace VKE
{
    namespace Memory
    {
        class CFreeListPool;
    }

    namespace RenderSystem
    {
        class CDeviceContext;
        class CPipeline;
        class CDevice;
        class CGraphicsContext;
        class CSwapChain;
        class CGraphicsContext;
    }

    class CVkEngine;
    class CWindow;

    using WindowPtr = Utils::TCWeakPtr< CWindow >;
    
    namespace RenderSystem
    {
        struct SRSInternal;

        class CRenderSystem
        {
            friend class CGraphicsContext;
            friend class CDeviceContext;
            using FreeListVec = vke_vector< Memory::CFreeListPool* >;
            using ContextVec = vke_vector< RenderSystem::CGraphicsContext* >;
            using DeviceVec = vke_vector< RenderSystem::CDeviceContext* >;

        public:

            using AdapterVec = vke_vector< RenderSystem::SAdapterInfo >;

        public:

            CRenderSystem(CVkEngine* pEngine);
            ~CRenderSystem();

            Result Create(const SRenderSystemInfo& Info);
            void Destroy();

            vke_force_inline
                CVkEngine*                  GetEngine() const { return m_pEngine; }

            void                        RenderFrame(const WindowPtr pWnd);

            CPipeline*    CreatePipeline();
            handle_t                    CreateFramebuffer(const SFramebufferDesc& Info);

            Result                      MakeCurrent(RenderSystem::CGraphicsContext* pCtx, CONTEXT_SCOPE scope = ContextScopes::ALL);

            const AdapterVec&           GetAdapters() const;

            CGraphicsContext*               GetCurrentContext(CONTEXT_SCOPE scope);

        protected:

            Result      _AllocMemory(SRenderSystemInfo* pInfoOut);
            Result      _InitAPI();
            Result      _CreateDevices();
            Result      _CreateDevice(const SAdapterInfo& Info);
            Result      _CreateFreeListMemory(uint32_t id, uint16_t* pElemCountOut, uint16_t defaultElemCount, size_t memSize);
            const
            void*       _GetICD() const;

            //VkInstance  _GetInstance() const;

        protected:

            SRenderSystemInfo       m_Desc;
            SRSInternal*            m_pPrivate = nullptr;
            CVkEngine*              m_pEngine = nullptr;
            FreeListVec             m_vpFreeLists;
            DeviceVec               m_vpDevices;
        };
    } // RenderSystem

} // VKE
