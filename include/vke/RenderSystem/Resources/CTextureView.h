#pragma once
#include "RenderSystem/Common.h"
#if VKE_VULKAN_RENDERER
#include "RenderSystem/Vulkan/Resources/CTextureView.h"
#endif // VKE_VULKAN_RENDERER

namespace VKE
{
    namespace RenderSystem
    {
        //using TextureViewPtr = Utils::TCWeakPtr< CTextureView >;
        //using TextureViewSmartPtr = Utils::TCWeakPtr< CTextureView >;
    } // RenderSystem
} // VKE