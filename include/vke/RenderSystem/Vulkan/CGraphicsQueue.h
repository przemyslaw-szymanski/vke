#pragma once

#include "RenderSystem/Vulkan/Common.h"

namespace VKE
{
    namespace RenderSystem
    {
        class CContext;

        class CGraphicsQueue
        {
            public:

            CGraphicsQueue(CContext*);
            ~CGraphicsQueue();

            Result Create(const SGraphicsQueueInfo&);
            void Destroy();
        };
    } // RenderSystem
} // VKE