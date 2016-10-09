#pragma once

#include "RenderSystem/Vulkan/Common.h"

namespace VKE
{
    namespace RenderSystem
    {
        class CSwapChain;
        class CDeviceContext;
        class CGraphicsQueue;
        struct SInternal;

        class CContext
        {
            friend class CRenderSystem;
            friend class CDevice;         

            public:

				CContext(CDevice* pDevice);
				~CContext();

				Result Create(const SContextInfo& Info);
				void Destroy();

				void Resize(uint32_t width, uint32_t height);

				void RenderFrame();

				Result  CreateSwapChain(const SSwapChainInfo& Info);

				vke_force_inline
				CDevice*        GetDevice() const { return m_pDevice; }

				vke_force_inline
				CDeviceContext* GetDeviceContext() const { return m_pDeviceCtx; }

				handle_t CreateGraphicsQueue(const SGraphicsQueueInfo&);

            protected:

                Result          _CreateDevices();
                Result          _CreateDevice(const SDeviceInfo& Info);

                Result          _CreateSwapChain(const SSwapChainInfo&);

                void*           _GetInstance() const;

                const void*     _GetInstanceFunctions() const;
                const void*     _GetGlobalFunctions() const;

                bool            _BeginFrame();
                void            _EndFrame();

            protected:

                SContextInfo        m_Info;
                CDevice*            m_pDevice = nullptr;
                CDeviceContext*     m_pDeviceCtx = nullptr;
                SInternal*          m_pInternal = nullptr;
        };
    } // RenderSystem
} // vke