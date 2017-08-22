#include "RenderSystem/Tasks/GraphicsContext.h"
#include "RenderSystem/CGraphicsContext.h"

namespace VKE
{
    namespace RenderSystem
    {
        namespace Tasks
        {
            static const Threads::ITask::Status g_aStatuses[] =
            {
                Threads::ITask::Status::REMOVE,
                Threads::ITask::Status::OK
            };

            Threads::ITask::Status SGraphicsContext::SPresent::_OnStart(uint32_t /*threadId*/)
            {
                {
                    const bool res = pCtx->PresentFrame();
                    return g_aStatuses[res];
                }
                return Status::OK;
            }

            Threads::ITask::Status SGraphicsContext::SBeginFrame::_OnStart(uint32_t)
            {
                {
                    const bool res = pCtx->BeginFrame();
                    return g_aStatuses[res];
                }
                return Status::OK;
            }

            Threads::ITask::Status SGraphicsContext::SEndFrame::_OnStart(uint32_t)
            {
                {
                    pCtx->EndFrame();
                    return g_aStatuses[true];
                }
                return Status::OK;
            }
        }
    }
}