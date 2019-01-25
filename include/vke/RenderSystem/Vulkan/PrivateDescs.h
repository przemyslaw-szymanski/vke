#pragma once

#include "RenderSystem/CQueue.h"
#include "Core/Threads/Common.h"

namespace VKE
{
    namespace RenderSystem
    {
        struct SGraphicsContextPrivateDesc
        {
            const VkICD::Device*        pICD;
            QueueRefPtr                 pQueue;
            VkInstance                  vkInstance;
            VkDevice                    vkDevice;
            VkPhysicalDevice            vkPhysicalDevice;
        };

        struct SSwapChainPrivateDesc : public SGraphicsContextPrivateDesc
        {
            
        };
    }
}