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
            using Desc = STextureViewInitDesc;
            public:

                void            Init(const STextureViewInitDesc& Desc);

                Desc&           GetDesc() { return m_Desc; }
                const Desc&     GetDesc() const { return m_Desc; }

            protected:

                Desc    m_Desc;
        };
    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER