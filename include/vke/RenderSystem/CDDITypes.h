#pragma once

#if VKE_VULKAN_RENDERER
#include "RenderSystem/Vulkan/CDDITypes.h"
// VKE_VULKAN_RENDERER

#else
#   error "NO API SELECTED IN CMAKE"
#endif // VKE_VULKAN_RENDERER

namespace VKE
{
    namespace RenderSystem
    {
        namespace DDI
        {
            
        } // DDI
    } // RenderSystem
} // VKE