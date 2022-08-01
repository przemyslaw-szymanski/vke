#pragma once

#include "Core/VKEPreprocessor.h"
#if VKE_VULKAN_RENDER_SYSTEM
#include "RenderSystem/Vulkan/CRenderQueue.h"
#else
#error "implement"
#endif