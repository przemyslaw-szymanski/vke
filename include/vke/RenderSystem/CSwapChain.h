#pragma once

#include "Core/VKECommon.h"
#if VKE_VULKAN_RENDERER
#include "RenderSystem/Vulkan/CSwapChain.h"
#else
#error "implement"
#endif