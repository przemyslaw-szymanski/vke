#pragma once
#include "Core/VKEPreprocessor.h"
#if VKE_VULKAN_RENDERER
#include "RenderSystem/Common.h"
#include "Core/Utils/TCSmartPtr.h"
#include "RenderSystem/Vulkan/Vulkan.h"

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

        class VKE_API CRenderSystem
        {
            friend class CGraphicsContext;
            friend class CDeviceContext;
            using FreeListVec = vke_vector< Memory::CFreeListPool* >;
            using ContextVec = vke_vector< RenderSystem::CGraphicsContext* >;
            using DeviceVec = Utils::TCDynamicArray< RenderSystem::CDeviceContext*, 4 >;

        public:

            using AdapterVec = vke_vector< RenderSystem::SAdapterInfo >;

        public:

            CRenderSystem(CVkEngine* pEngine);
            ~CRenderSystem();

            Result Create(const SRenderSystemDesc& Info);
            void Destroy();

            vke_force_inline
                CVkEngine*                  GetEngine() const { return m_pEngine; }

            void                        RenderFrame(const WindowPtr pWnd);

            CPipeline*    CreatePipeline();
            handle_t                    CreateFramebuffer(const SFramebufferDesc& Info);

            Result                      MakeCurrent(RenderSystem::CGraphicsContext* pCtx, CONTEXT_SCOPE scope = ContextScopes::ALL);

            const AdapterVec&           GetAdapters() const;

            CGraphicsContext*               GetCurrentContext(CONTEXT_SCOPE scope);

            CDeviceContext*     CreateDeviceContext(const SDeviceContextDesc& Desc);
            void                DestroyDeviceContext(CDeviceContext** ppOut);

        protected:

            Result      _AllocMemory(SRenderSystemDesc* pInfoOut);
            Result      _InitAPI();
            Result      _CreateDevices();
            Result      _CreateFreeListMemory(uint32_t id, uint16_t* pElemCountOut, uint16_t defaultElemCount, size_t memSize);
            const
            void*       _GetICD() const;

            VkInstance  _GetVkInstance() const;

        protected:

            SRenderSystemDesc       m_Desc;
            SRSInternal*            m_pPrivate = nullptr;
            CVkEngine*              m_pEngine = nullptr;
            FreeListVec             m_vpFreeLists;
            DeviceVec               m_vpDevices;
        };
    } // RenderSystem

} // VKE
#endif // VKE_VULKAN_RENDERER