#pragma once
#if VKE_VULKAN_RENDERER

#include "RenderSystem/Vulkan/Vulkan.h"
#include "RenderSystem/Common.h"
#include "Core/VKEForwardDeclarations.h"

namespace VKE
{
    namespace RenderSystem
    {
        class CGraphicsContext;
        class CDeviceContext;

        class VKE_API CRenderPass final : public Core::CObject
        {
            friend class CGraphicsContext;
            friend class CDeviceContext;
            friend class CRenderingPipeline;
            friend class CSwapChain;

            using VkImageViewArray = Utils::TCDynamicArray< VkImageView >;
            using VkClearValueArray = Utils::TCDynamicArray< VkClearValue >;

            public:

                CRenderPass(CDeviceContext*);
                ~CRenderPass();

                Result  Create(const SRenderPassDesc& Desc);
                Result  Update(const SRenderPassDesc& Desc);
                void    Clear(const SColor& ClearColor, float clearDepth, float clearStencil);
                void    Destroy(bool destroyRenderPass = true);

                void Begin(const VkCommandBuffer& vkCb);
                void End(const VkCommandBuffer& vkEnd);

            protected:

                const VkImageCreateInfo* _AddTextureView(TextureViewHandle hView);
                Result _CreateTextureView(const STextureDesc& Desc, TextureHandle* phTextureOut,
                    TextureViewHandle* phTextureViewOut, const VkImageCreateInfo** ppCreateInfoOut);

            protected:

                SRenderPassDesc         m_Desc;
                VkRenderPassBeginInfo   m_vkBeginInfo;
                Vulkan::CDeviceWrapper& m_VkDevice;
                
                CDeviceContext*         m_pCtx;
                VkImageViewArray        m_vVkImageViews;
                VkClearValueArray       m_vVkClearValues;
                VkFramebuffer           m_vkFramebuffer = VK_NULL_HANDLE;
                VkRenderPass            m_vkRenderPass = VK_NULL_HANDLE;
        };

    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER