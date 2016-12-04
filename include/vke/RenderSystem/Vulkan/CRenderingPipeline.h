#pragma once
#if VKE_VULKAN_RENDERER
#include "RenderSystem/Vulkan/Vulkan.h"
namespace VKE
{
    namespace RenderSystem
    {
        class CRenderPass;
        class CRenderingPipeline final
        {
            friend class CRenderPass;
            public:

            CRenderingPipeline(CDeviceContext* pCtx);
            ~CRenderingPipeline();

            Result Create(const SRenderingPipelineDesc& Desc);
            void Destroy();

            protected:

                uint32_t _IsTextureUsedInNextPass(CRenderPass* pPass, const handle_t& hTex);

            protected:

                SRenderingPipelineDesc      m_Desc;
                CDeviceContext*             m_pCtx;
        };
    }
}
#endif // VKE_VULKAN_RENDERER