#include "RenderSystem/Vulkan/Resources/CTexture.h"
#if VKE_VULKAN_RENDERER
namespace VKE
{
    namespace RenderSystem
    {
        CTexture::CTexture()
        {}

        CTexture::~CTexture()
        {}

        void CTexture::Init(const STextureInitDesc& Desc)
        {
            m_Desc = Desc;
        }

        void CTexture::ChangeLayout(TEXTURE_LAYOUT newLayout)
        {

        }
    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER