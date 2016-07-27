#pragma once

#include "RenderSystem/Vulkan/Common.h"

namespace VKE
{
    namespace RenderSystem
    {
        class CSwapChain;
        class CDeviceContext;
        struct SInternal;

        class CContext
        {
            friend class CRenderSystem;
            friend class CDevice;

            using SwapChainVec = vke_vector< CSwapChain* >;

            public:

            CContext(CDevice* pDevice);
            ~CContext();

            Result Create(const SContextInfo& Info);
            void Destroy();

            void RenderFrame(const handle_t& hSwapChain);

            Result  CreateSwapChain(const SSwapChainInfo& Info);

            vke_force_inline
            CDevice*        GetDevice() const { return m_pDevice; }

            vke_force_inline
            CDeviceContext* GetDeviceContext() const { return m_pDeviceCtx; }

            protected:

            Result          _CreateDevices();
            Result          _CreateDevice(const SDeviceInfo& Info);

            Result          _CreateSwapChain(const SSwapChainInfo&);
            
            void*           _GetInstance() const;

            const
            void*   _GetInstanceFunctions() const;
            const
            void*     _GetGlobalFunctions() const;

            protected:

                SContextInfo    m_Info;
                CDevice*        m_pDevice = nullptr;
                CDeviceContext* m_pDeviceCtx = nullptr;
                SwapChainVec    m_vpSwapChains;
                SInternal*      m_pInternal = nullptr;
        };
    } // RenderSystem
} // vke