#pragma once

#include "RenderSystem/Common.h"
#include "Utils/TCSmartPtr.h"

namespace VKE
{
    namespace Memory
    {
        class CFreeListPool;
    }

    namespace RenderSystem
    {
        class CDevice;
        class CPipeline;
        class CDevice;
        class CContext;
        class CSwapChain;
    }

    class CVkEngine;
    class CWindow;

    using WindowPtr = Utils::TCWeakPtr< CWindow >;
    
    struct SRSInternal;

    class CRenderSystem
    {
        friend class RenderSystem::CContext;
        friend class RenderSystem::CDevice;
        using FreeListVec = vke_vector< Memory::CFreeListPool* >;
        using ContextVec = vke_vector< RenderSystem::CContext* >;
        using DeviceVec = vke_vector< RenderSystem::CDevice* >;

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

            RenderSystem::CPipeline*    CreatePipeline();

            const AdapterVec&           GetAdapters() const;

        protected:

            Result      _AllocMemory(SRenderSystemInfo* pInfoOut);
            Result      _InitAPI();
            Result      _CreateContexts();
            Result      _CreateFreeListMemory(uint32_t id, uint16_t* pElemCountOut, uint16_t defaultElemCount, size_t memSize);
            const
            void*       _GetGlobalFunctions() const;
            const
            void*       _GetInstanceFunctions() const;

        protected:

            SRenderSystemInfo   m_Info;
            SRSInternal*        m_pInternal = nullptr;
            CVkEngine*          m_pEngine = nullptr;
            FreeListVec         m_vpFreeLists;
            DeviceVec           m_vpDevices;
    };
} // VKE
