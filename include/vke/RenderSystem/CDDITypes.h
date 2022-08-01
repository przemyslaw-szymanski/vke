#pragma once

#if VKE_VULKAN_RENDER_SYSTEM
#include "RenderSystem/Vulkan/CDDITypes.h"
// VKE_VULKAN_RENDER_SYSTEM

#else
#   error "NO API SELECTED IN CMAKE"
#endif // VKE_VULKAN_RENDER_SYSTEM

namespace VKE
{
    namespace RenderSystem
    {
        namespace DDI
        {
            
        } // DDI
    } // RenderSystem
} // VKE