#pragma once
#if VKE_VULKAN_RENDERER
#include "RenderSystem/Vulkan/Vulkan.h"

namespace VKE
{
    namespace RenderSystem
    {
        class CRenderPass;
        class CGraphicsContext;
        class CDeviceContext;

        class CRenderingPipeline final
        {
            friend class CRenderPass;
            using RenderPassArray = Utils::TCDynamicArray< CRenderPass*, 128 >;

            public:

            CRenderingPipeline(CDeviceContext* pCtx);
            ~CRenderingPipeline();

            Result Create(const SRenderingPipelineDesc& Desc);
            void Destroy();
            void Begin();
            void Render();
            void End();

            CRenderPass* CreatePass(const SRenderPassDesc& Desc);

            protected:

                SRenderingPipelineDesc  m_Desc;
                RenderPassArray         m_vpRenderPasses;
                CDeviceContext*         m_pCtx;
        };
    }
}
#endif // VKE_VULKAN_RENDERER