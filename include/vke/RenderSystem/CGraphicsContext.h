#pragma once

#include "RenderSystem/Common.h"

namespace VKE
{
    namespace RenderSystem
    {
        class CSwapChain;
        class CGraphicsQueue;
        struct SInternal;

        class VKE_API CGraphicsContext
        {
            friend class CRenderSystem;
            friend class CDevice;         

            public:

                CGraphicsContext(CDevice* pDevice);
                ~CGraphicsContext();

                Result Create(const SGraphicsContextDesc& Info);
                void Destroy();

                void Resize(uint32_t width, uint32_t height);

                void RenderFrame();

                Result  CreateSwapChain(const SSwapChainInfo& Info);

                

                vke_force_inline
                CDevice*        GetDevice() const { return m_pDevice; }

                handle_t CreateGraphicsQueue(const SGraphicsQueueInfo&);

            protected:

                Result          _CreateDevices();
                Result          _CreateDevice(const SDeviceContextDesc& Info);

                Result          _CreateSwapChain(const SSwapChainInfo&);

                void*           _GetInstance() const;

                const void*     _GetInstanceFunctions() const;
                const void*     _GetGlobalFunctions() const;

                bool            _BeginFrame();
                void            _EndFrame();

            protected:

                SGraphicsContextDesc        m_Info;
                CDevice*            m_pDevice = nullptr;
                SInternal*          m_pInternal = nullptr;
        };
    } // RenderSystem
} // vke