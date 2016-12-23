#pragma once
#if VKE_VULKAN_RENDERER
#include "RenderSystem/Vulkan/Vulkan.h"

namespace VKE
{
    namespace RenderSystem
    {
        class CSubmit;
        class CRenderPass;

        class CSubpass
        {
            friend class CRenderPass;
            public:

                Result Create(CRenderPass* pPass, const SSubpassDesc& Desc);
                void Destroy();

                void Begin();
                void End();

            protected:

                SSubpassDesc    m_Desc;
                CRenderPass*    m_pPass;
        };

        class CRenderPass final
        {
            friend class CDeviceContext;
            friend class CRenderingPipeline;
            friend class CRenderQueue;
            
            using SubpassArray = Utils::TCDynamicArray< CSubpass >;

            public:

                CRenderPass(CRenderingPipeline* pPipeline);
                ~CRenderPass();

                Result Create(const SRenderPassDesc& Desc);
                void Destroy();

                void Begin();
                void End();

                CSubpass* CreateSubpass(const SSubpassDesc& Desc);

            protected:

                SRenderPassDesc     m_Desc;
                SubpassArray        m_vSubpasses;
                CRenderingPipeline* m_pPipeline;
                CSubmit*            m_pSubmit = nullptr;
        };
    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER