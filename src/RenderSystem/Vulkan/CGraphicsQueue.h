#pragma once

#include "RenderSystem/Vulkan/Common.h"

namespace VKE
{
    namespace RenderSystem
    {
        class CGraphicsContext;
        class CCommandBuffer;

        class CGraphicsQueue
        {
            public:

            CGraphicsQueue(CGraphicsContext*);
            ~CGraphicsQueue();

            Result Create(const SGraphicsQueueInfo&);
            void Destroy();

            CCommandBuffer* GetCommandBuffer();

            Result Submit();
        };
    } // RenderSystem
} // VKE