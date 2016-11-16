#pragma once

#include "Core/VKEPreprocessor.h"
#if VKE_VULKAN_RENDERER
#include "RenderSystem/Vulkan/CRenderQueue.h"
#else
#error "implement"
#endif