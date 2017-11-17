#include "RenderSystem/Resources/CTexture.h"
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

        void CTexture::ChangeLayout(CommandBufferPtr pCommandBuffer, TEXTURE_LAYOUT newLayout)
        {

        }
    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER