#pragma once

#include "Vulkan.h"
#include "Core/Threads/Common.h"

namespace VKE
{
    namespace RenderSystem
    {
        struct SGraphicsContextPrivateDesc
        {
            const Vulkan::ICD::Device*  pICD;
            Vulkan::SQueue*             pQueue;
            VkInstance                  vkInstance;
            VkDevice                    vkDevice;
            VkPhysicalDevice            vkPhysicalDevice;
        };

        struct SSwapChainPrivateDesc : public SGraphicsContextPrivateDesc
        {
            
        };
    }
}