#include "RenderSystem/Tasks/GraphicsContext.h"
#include "RenderSystem/CGraphicsContext.h"

namespace VKE
{
    namespace RenderSystem
    {
        namespace Tasks
        {
            Threads::ITask::Status SGraphicsContext::SPresent::_OnStart(uint32_t threadId)
            {
                auto res = pCtx->PresentFrame();
                return res? Status::OK : Status::REMOVE;
            }
        }
    }
}