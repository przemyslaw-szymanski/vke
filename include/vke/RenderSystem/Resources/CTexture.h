#pragma once
#include "RenderSystem/Common.h"
#if VKE_VULKAN_RENDERER
#include "RenderSystem/Vulkan/Resources/CTexture.h"
#endif // VKE_VULKAN_RENDERER

namespace VKE
{
    namespace RenderSystem
    {
        using TexturePtr = Utils::TCWeakPtr< CTexture >;
        using TextureSmartPtr = Utils::TCWeakPtr< CTexture >;
    } // RenderSystem
} // VKE