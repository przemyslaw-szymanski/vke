#pragma once

#include "Core/Threads/Common.h"

namespace VKE::RenderSystem
{
    struct SGraphicsContextPrivateDesc
    {
        QueueRefPtr pQueue;
        handle_t    hCmdPool;
    };

    struct SSwapChainPrivateDesc : public SGraphicsContextPrivateDesc
    {
    };
} // namespace VKE::RenderSystem