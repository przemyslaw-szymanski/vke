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

            using ImageArray = Utils::TCDynamicArray< DDIImage, 8 >;
            using ImageViewArray = Utils::TCDynamicArray< DDIImageView, 8 >;
            using ClearValueArray = Utils::TCDynamicArray< DDIClearValue, 8 >;

            VKE_ADD_DDI_OBJECT( DDIRenderPass );

            public:

                CRenderPass(CDeviceContext*);
                ~CRenderPass();

                Result  Create(const SRenderPassDesc& Desc);
                Result  Update(const SRenderPassDesc& Desc);
                void    Clear(const SColor& ClearColor, float clearDepth, float clearStencil);
                void    Destroy(bool destroyRenderPass = true);

                DDIImage GetColorRenderTarget(uint32_t idx) const { return m_vImages[ idx ]; }
                DDIImageView GetColorRenderTargetView(uint32_t idx) const { return m_vImageViews[idx]; }

                void Begin(const DDICommandBuffer& hCb);
                void End(const DDICommandBuffer& hCb);

            protected:

                const VkImageCreateInfo* _AddTextureView(TextureViewHandle hView);
                Result _CreateTextureView(const STextureDesc& Desc, TextureHandle* phTextureOut,
                    TextureViewHandle* phTextureViewOut, const VkImageCreateInfo** ppCreateInfoOut);

            protected:

                SRenderPassDesc         m_Desc;
                //VkRenderPassBeginInfo   m_vkBeginInfo;
                //Vulkan::CDeviceWrapper& m_VkDevice;
                
                CDeviceContext*         m_pCtx;
                ImageArray            m_vImages;
                ImageViewArray        m_vImageViews;
                ClearValueArray       m_vClearValues;
                DDIFramebuffer        m_hFramebuffer = DDINullHandle;
        };

    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER