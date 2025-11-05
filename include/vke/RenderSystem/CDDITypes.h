#pragma once

#if VKE_VULKAN_RENDER_SYSTEM
#include "RenderSystem/Vulkan/CDDITypes.h"
#define VKE_ANY_API_SELECTED
// VKE_VULKAN_RENDER_SYSTEM
#endif

#if VKE_D3D12_RENDER_SYSTEM
#include "RenderSystem/D3D12/CDDITypes.h"
#define VKE_ANY_API_SELECTED
// VKE_D3D12_RENDER_SYSTEM
#endif

#ifndef VKE_ANY_API_SELECTED
#error "NO API SELECTED IN CMAKE"
#endif // VKE_VULKAN_RENDER_SYSTEM

namespace VKE
{
    namespace RenderSystem
    {
        namespace DDI
        {

        } // namespace DDI
    } // namespace RenderSystem
} // namespace VKE