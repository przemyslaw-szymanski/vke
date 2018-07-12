#pragma once
#include "RenderSystem/Common.h"
#if VKE_VULKAN_RENDERER
#include "RenderSystem/Vulkan/Vulkan.h"
#include "Core/Utils/TCDynamicArray.h"
#include "RenderSystem/Resources/CTextureView.h"
#include "Core/VKEConfig.h"

namespace VKE
{
    namespace RenderSystem
    {
        class CDeviceContext;

        struct STextureInitDesc
        {
            using ImageVec = Utils::TCDynamicArray< VkImage, 4 >;
            using TextureViewVec = Utils::TCDynamicArray< CTextureView, Config::RenderSystem::Texture::MAX_VIEW_PER_TEXTURE >;
            STextureDesc    Desc;
            CDeviceContext* pContext = nullptr;
            VkImage         hNative = VK_NULL_HANDLE;
            TextureViewVec  vTextureViews;
            TEXTURE_LAYOUT  layout = TextureLayouts::UNDEFINED;
            TextureHandle   hTexture = NULL_HANDLE;
        };

        class CTexture
        {
            friend class CGraphicsContext;
            friend class CDeviceContext;
            friend class CRenderSystem;
            friend class CResourceManager;

            public:

                            CTexture();
                            ~CTexture();

                void                    Init(const STextureInitDesc& Desc);

                const STextureInitDesc& GetDesc() const { return m_Desc; }
                STextureInitDesc&       GetDesc() { return m_Desc; }

                void                    ChangeLayout(CommandBufferPtr pCommandBuffer, TEXTURE_LAYOUT newLayout);

            protected:

                STextureInitDesc    m_Desc;
                
        };
    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER