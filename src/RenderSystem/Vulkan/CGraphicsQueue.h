#pragma once

#include "RenderSystem/Vulkan/Common.h"

namespace VKE
{
    namespace RenderSystem
    {
        class CContext;
        class CCommandBuffer;

        class CGraphicsQueue
        {
            public:

            CGraphicsQueue(CContext*);
            ~CGraphicsQueue();

            Result Create(const SGraphicsQueueInfo&);
            void Destroy();

            CCommandBuffer* GetCommandBuffer();

            Result Submit();
        };
    } // RenderSystem
} // VKE