#pragma once

#include "RenderSystem/Common.h"
#include "RenderSystem/Vulkan/Vulkan.h"

namespace VKE
{
    namespace RenderSystem
    {
        class CSwapChain;
        class CGraphicsQueue;
        class CDeviceContext;
        struct SInternal;

        class VKE_API CGraphicsContext
        {
            friend class CRenderSystem;
            friend class CDeviceContext;         
            struct SPrivate;

            public:

                CGraphicsContext(CDeviceContext* pCtx);
                ~CGraphicsContext();

                Result Create(const SGraphicsContextDesc& Info);
                void Destroy();

                void Resize(uint32_t width, uint32_t height);

                void RenderFrame();

                Result  CreateSwapChain(const SSwapChainDesc& Info);

                Vulkan::ICD::Device&    _GetICD() const;

                vke_force_inline
                CDeviceContext*        GetDeviceContext() const { return m_pDeviceCtx; }

                handle_t CreateGraphicsQueue(const SGraphicsQueueInfo&);

            protected:         

                Result          _CreateSwapChain(const SSwapChainDesc&);

                bool            _BeginFrame();
                void            _EndFrame();

            protected:

                SGraphicsContextDesc    m_Desc;
                CDeviceContext*         m_pDeviceCtx = nullptr;
                SPrivate*               m_pPrivate = nullptr;
        };
    } // RenderSystem
} // vke