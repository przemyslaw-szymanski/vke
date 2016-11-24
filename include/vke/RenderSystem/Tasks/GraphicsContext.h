#pragma once

#include "Core/Threads/ITask.h"

namespace VKE
{
    namespace RenderSystem
    {
        class CGraphicsContext;

        namespace Tasks
        {
            struct SGraphicsContext
            {
                struct SPresent : public Threads::ITask
                {
                    CGraphicsContext* pCtx;
                    Status _OnStart(uint32_t threadId) override;
                };

                SPresent    Present;
            };
        }
    }
}