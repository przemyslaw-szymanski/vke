#pragma once
#if VKE_VULKAN_RENDERER
#include "RenderSystem/Vulkan/Vulkan.h"

namespace VKE
{
    namespace RenderSystem
    {
        class CRenderPass final
        {
            friend class CDeviceContext;
            friend class CRenderingPipeline;
            
            //using FramebufferArray = Utils::TCDynamicArray< VkFramebuffer, 3 >;

            public:
            protected:

                SRenderPassDesc     m_Desc;
                VkRenderPass        m_vkRenderPass = VK_NULL_HANDLE;
                VkFramebuffer       m_vkFramebuffer = VK_NULL_HANDLE;
                CRenderPass*        m_pParent = nullptr;
        };
    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER