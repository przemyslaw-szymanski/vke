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

            using VkImageArray = Utils::TCDynamicArray< VkImage, 8 >;
            using VkImageViewArray = Utils::TCDynamicArray< VkImageView, 8 >;
            using VkClearValueArray = Utils::TCDynamicArray< VkClearValue, 8 >;

            public:

            public:

                CRenderPass(CDeviceContext*);
                ~CRenderPass();

                Result  Create(const SRenderPassDesc& Desc);
                Result  Update(const SRenderPassDesc& Desc);
                void    Clear(const SColor& ClearColor, float clearDepth, float clearStencil);
                void    Destroy(bool destroyRenderPass = true);

                VkImage GetColorRenderTarget(uint32_t idx) const { return m_vVkImages[ idx ]; }
                VkImageView GetColorRenderTargetView(uint32_t idx) const { return m_vVkImageViews[idx]; }

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
                VkImageArray            m_vVkImages;
                VkImageViewArray        m_vVkImageViews;
                VkClearValueArray       m_vVkClearValues;
                VkFramebuffer           m_vkFramebuffer = VK_NULL_HANDLE;
                VkRenderPass            m_vkRenderPass = VK_NULL_HANDLE;
        };

    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER