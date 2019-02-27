#pragma once

#include "RenderSystem/CQueue.h"
#include "Core/Threads/Common.h"

namespace VKE
{
    namespace RenderSystem
    {
        struct SGraphicsContextPrivateDesc
        {
            QueueRefPtr pQueue;
            handle_t    hCmdPool;
        };

        struct SSwapChainPrivateDesc : public SGraphicsContextPrivateDesc
        {
            
        };
    }
}