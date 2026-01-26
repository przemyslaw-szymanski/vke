#pragma once

#if VKE_RENDER_SYSTEM_VULKAN
#include "RenderSystem/Vulkan/CDDITypes.h"
#define VKE_ANY_API_SELECTED
// VKE_RENDER_SYSTEM_VULKAN

#elif VKE_RENDER_SYSTEM_D3D12
#include "RenderSystem/D3D12/CDDITypes.h"
#define VKE_ANY_API_SELECTED
// VKE_RENDER_SYSTEM_D3D12
#endif

#ifndef VKE_ANY_API_SELECTED
#error "NO API SELECTED IN CMAKE"
#endif // VKE_RENDER_SYSTEM_VULKAN

namespace VKE
{
    namespace RenderSystem
    {
        namespace DDI
        {

        } // namespace DDI
    } // namespace RenderSystem
} // namespace VKE