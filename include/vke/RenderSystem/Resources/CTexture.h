#pragma once
#include "RenderSystem/Common.h"
#if VKE_VULKAN_RENDER_SYSTEM
#include "RenderSystem/Vulkan/Resources/CTexture.h"
#endif // VKE_VULKAN_RENDER_SYSTEM

namespace VKE
{
    namespace RenderSystem
    {
        using TexturePtr = Utils::TCWeakPtr< CTexture >;
        using TextureSmartPtr = Utils::TCWeakPtr< CTexture >;
    } // RenderSystem
} // VKE