#pragma once

#include "RenderSystem/Vulkan/CVulkanAPI.h"
#include "RenderSystem/D3D12/CD3D12API.h"
#include "RenderSystem/APITypes.h"

namespace VKE::RenderSystem
{
#if VKE_VULKAN_RENDER_SYSTEM
    using CAPI = TCAPI< CVulkanAPI >;
#elif VKE_D3D12_RENDER_SYSTEM
    using CAPI = TCAPI< CD3D12API >;
#else
#error "Unsupported 3D API"
#endif
}