#pragma once

#include "RenderSystem/Vulkan/CDDITypes.h"
#include "RenderSystem/D3D12/CDDITypes.h"


namespace VKE::RenderSystem
{
#if VKE_VULKAN_RENDER_SYSTEM
    using NativeAPI = VKE::RenderSystem::Vulkan::NativeAPI;
    //using namespace VKE::RenderSystem::Vulkan;
#elif VKE_D3D12_RENDER_SYSTEM
    using NativeAPI = VKE::RenderSystem::D3D12::NativeAPI;
    //using namespace VKE::RenderSystem::D3D12;
#else
#error "Unsupported 3D API"
#endif
}