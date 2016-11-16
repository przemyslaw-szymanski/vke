#pragma once

#include "Vulkan.h"
#include "Core/Threads/Common.h"

namespace VKE
{
    namespace RenderSystem
    {
        struct SQueue
        {
            VkQueue             vkQueue;
            uint32_t            familyIndex;
        };

        struct SGraphicsContextPrivateDesc
        {
            Vulkan::ICD::Device*    pICD;
            SQueue                  Queue;
            VkInstance              vkInstance;
            VkDevice                vkDevice;
            VkPhysicalDevice        vkPhysicalDevice;
        };

        struct SSwapChainPrivateDesc : public SGraphicsContextPrivateDesc
        {
            
        };
    }
}