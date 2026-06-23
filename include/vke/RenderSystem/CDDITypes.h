#pragma once

#if VKE_COMPILE_VULKAN_RHI
#include "RenderSystem/Vulkan/CDDITypes.h"
#define VKE_ANY_API_SELECTED
// VKE_COMPILE_VULKAN_RHI

#elif VKE_COMPILE_D3D12_RHI
#include "RenderSystem/D3D12/CDDITypes.h"
#define VKE_ANY_API_SELECTED
// VKE_COMPILE_D3D12_RHI
#endif

#ifndef VKE_ANY_API_SELECTED
#error "NO API SELECTED IN CMAKE"
#endif // VKE_COMPILE_VULKAN_RHI

namespace VKE
{
    namespace RenderSystem
    {
        namespace DDI
        {

        } // namespace DDI
    } // namespace RenderSystem
} // namespace VKE