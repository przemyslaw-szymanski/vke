#pragma once

#if VKE_VULKAN_RENDERER
#include "RenderSystem/Vulkan/CDDITypes.h"
#elif // VKE_VULKAN_RENDERER

#else
#   error "NO API SELECTED IN CMAKE"
#endif // VKE_VULKAN_RENDERER