#pragma once
#include "RenderSystem/Common.h"
#if VKE_VULKAN_RENDER_SYSTEM
#include "RenderSystem/Vulkan/Resources/CTextureView.h"
#endif // VKE_VULKAN_RENDER_SYSTEM

namespace VKE
{
    namespace RenderSystem
    {
        //using TextureViewPtr = Utils::TCWeakPtr< CTextureView >;
        //using TextureViewSmartPtr = Utils::TCWeakPtr< CTextureView >;
    } // RenderSystem
} // VKE