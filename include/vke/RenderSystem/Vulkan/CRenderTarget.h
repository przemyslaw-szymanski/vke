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

        class VKE_API CRenderTarget final : public Core::CObject
        {
            friend class CGraphicsContext;
            friend class CDeviceContext;
            friend class CRenderingPipeline;
            friend class CSwapChain;

            using ImageViewArray = Utils::TCDynamicArray< VkImageView, 8 >;
            using TextureArray = Utils::TCDynamicArray< TextureHandle, 8 >;
            using TextureViewArray = Utils::TCDynamicArray< TextureViewHandle, 8 >;

            struct SInputAttachments
            {
                VkImageView vkDepthStencilView = VK_NULL_HANDLE;
            };

            public:

                CRenderTarget(CDeviceContext*);
                ~CRenderTarget();

                /*Result Create(const SRenderTargetDesc& Desc);
                void Destroy();*/

            protected:

                SRenderTargetDesc   m_Desc;
                ImageViewArray      m_vImgViews;
                TextureViewArray    m_vTextureViewHandles;
                TextureArray        m_vTextureHandles;
                SInputAttachments   m_InputAttachments;
                CDeviceContext*     m_pCtx;
                VkFramebuffer       m_vkFramebuffer = VK_NULL_HANDLE;
                VkRenderPass        m_vkRenderPass = VK_NULL_HANDLE;
        };

    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER