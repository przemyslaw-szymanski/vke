#pragma once

#include "RenderSystem/Vulkan/CVulkanAPI.h"
#include "RenderSystem/D3D12/CD3D12API.h"

namespace VKE::RenderSystem
{
#if VKE_RENDER_SYSTEM == VKE_VULKAN
    using CRHI = Vulkan::CVulkanAPI;
#elif VKE_RENDER_SYSTEM == VKE_D3D12
    using CRHI = D3D12::CD3D12API;
#else
#error "Unsupported 3D API"
#endif
}