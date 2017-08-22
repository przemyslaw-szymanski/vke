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

            struct STexture
            {
                TextureHandle   hTex = NULL_HANDLE;
                bool            created = false;

                STexture() {}
                STexture(TextureHandle hTex, bool cr = false) :
                    hTex(hTex),
                    created(cr) {}
            };

            struct STextureView
            {
                TextureViewHandle   hView = NULL_HANDLE;
                bool                created = false;
                STextureView() {}
                STextureView(TextureViewHandle hView, bool cr = false) :
                    hView(hView),
                    created(cr) {}
            };

            enum class State
            {
                UNDEFINED,
                BEGIN,
                END
            };

            using ImageViewArray = Utils::TCDynamicArray< VkImageView, 8 >;
            using TextureArray = Utils::TCDynamicArray< STexture, 8 >;
            using TextureViewArray = Utils::TCDynamicArray< STextureView, 8 >;
            using ClearValueArray = Utils::TCDynamicArray< VkClearValue, 8 >;
            using AttachmentDescArray = Utils::TCDynamicArray< VkAttachmentDescription, 8 >;
            using AttachmentRefArray = Utils::TCDynamicArray< VkAttachmentReference, 8 >;

            struct SInputAttachments
            {
                VkImageView vkDepthStencilView = VK_NULL_HANDLE;
            };

            public:

                CRenderPass(CDeviceContext*);
                ~CRenderPass();

                Result  Create(const SRenderTargetDesc& Desc);
                Result  Update(const SRenderTargetDesc& Desc);
                void    Clear(const SColor& ClearColor, float clearDepth, float clearStencil);
                void    Destroy(bool destroyRenderPass = true);

                TextureHandle   AddWriteTexture(const STextureDesc& Desc, RENDER_TARGET_WRITE_ATTACHMENT_USAGE usage,
                    const SColor& ClearColor);
                Result          AddReadTexture(const TextureViewHandle& hView, RENDER_TARGET_READ_ATTACHMENT_USAGE usage);
                TextureHandle   AddWriteDepthStencil();
                Result          AddReadDepthStencil(const TextureViewHandle& hTextureView);
                Result          AddSubpass();
                Result          Create();

                void Begin(const VkCommandBuffer& vkCb);
                void End(const VkCommandBuffer& vkEnd);

            protected:

                const VkImageCreateInfo* _AddTextureView(TextureViewHandle hView);
                Result _CreateTextureView(const STextureDesc& Desc, TextureHandle* phTextureOut,
                    TextureViewHandle* phTextureViewOut, const VkImageCreateInfo** ppCreateInfoOut);

            protected:

                SRenderTargetDesc       m_Desc;
                VkRenderPassBeginInfo   m_vkBeginInfo;
                ClearValueArray         m_vClearValues;
                ImageViewArray          m_vImgViews;
                TextureViewArray        m_vTextureViewHandles;
                TextureArray            m_vTextureHandles;
                AttachmentDescArray     m_vAttachmentDescriptions;
                AttachmentRefArray      m_vAttachmentReferences;
                SInputAttachments       m_InputAttachments;
                CDeviceContext*         m_pCtx;
                VkFramebuffer           m_vkFramebuffer = VK_NULL_HANDLE;
                VkRenderPass            m_vkRenderPass = VK_NULL_HANDLE;
                State                   m_state = State::UNDEFINED;
        };

    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER