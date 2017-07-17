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
                if (_IsTaskToWaitForFinished())
                {
                    const bool res = pCtx->PresentFrame();
                    printf("present\n");
                    return g_aStatuses[res];
                }
                return Status::OK;
            }

            Threads::ITask::Status SGraphicsContext::SBeginFrame::_OnStart(uint32_t)
            {
                if (_IsTaskToWaitForFinished())
                {
                    const bool res = pCtx->BeginFrame();
                    printf("begin\n");
                    return g_aStatuses[res];
                }
                return Status::OK;
            }

            Threads::ITask::Status SGraphicsContext::SEndFrame::_OnStart(uint32_t)
            {
                if (_IsTaskToWaitForFinished())
                {
                    pCtx->EndFrame();
                    printf("end\n");
                    return g_aStatuses[true];
                }
                return Status::OK;
            }
        }
    }
}