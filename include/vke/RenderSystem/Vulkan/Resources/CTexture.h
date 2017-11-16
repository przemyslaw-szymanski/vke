#pragma once
#include "RenderSystem/Common.h"
#if VKE_VULKAN_RENDERER
#include "RenderSystem/Vulkan/Vulkan.h"
#include "Core/Utils/TCDynamicArray.h"
#include "RenderSystem/Resources/CTextureView.h"

namespace VKE
{
    namespace RenderSystem
    {
        class CDeviceContext;

        struct STextureInitDesc
        {
            using ImageVec = Utils::TCDynamicArray< VkImage, 4 >;
            using TextureViewVec = Utils::TCDynamicArray< TextureViewPtr, 4 >;
            CDeviceContext* pContext = nullptr;
            VkImage         vkImage = VK_NULL_HANDLE;
            TextureViewVec  vTextureViews;
            TEXTURE_LAYOUT  layout = TextureLayouts::UNDEFINED;
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

                void        Init(const STextureInitDesc& Desc);
                void        ChangeLayout(TEXTURE_LAYOUT newLayout);

            protected:

                STextureInitDesc    m_Desc;
                
        };
    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER