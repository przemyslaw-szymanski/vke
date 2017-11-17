#pragma once
#if VKE_VULKAN_RENDERER
namespace VKE
{
    namespace RenderSystem
    {
        struct STextureViewInitDesc
        {
            STextureViewDesc    Desc;
            TextureViewHandle   hView;
            VkImageView         hNative;
            TexturePtr          pTexture;
        };
        class CTextureView
        {
            public:

                void        Init(const STextureViewInitDesc& Desc);

            protected:

                STextureViewInitDesc    m_Desc;
        };
    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER