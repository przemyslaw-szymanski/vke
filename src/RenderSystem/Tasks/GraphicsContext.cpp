#include "RenderSystem/Tasks/GraphicsContext.h"
#include "RenderSystem/CGraphicsContext.h"

namespace VKE
{
    namespace RenderSystem
    {
        namespace Tasks
        {
            static const Threads::ITask::Result g_aStatuses[] =
            {
                Threads::ITask::Result::REMOVE,
                Threads::ITask::Result::OK
            };

            Threads::ITask::Result SGraphicsContext::SPresent::_OnStart(uint32_t /*threadId*/)
            {
                {
                    return pCtx->_PresentFrameTask();
                }
            }

            Threads::ITask::Result SGraphicsContext::SBeginFrame::_OnStart(uint32_t)
            {
                {
                    return pCtx->_BeginFrameTask();
                }
                return Result::OK;
            }

            Threads::ITask::Result SGraphicsContext::SEndFrame::_OnStart(uint32_t)
            {
                {
                    return pCtx->_EndFrameTask();
                }
            }

            Threads::ITask::Result SGraphicsContext::SSwapBuffers::_OnStart(uint32_t)
            {
                {
                    return pCtx->_SwapBuffersTask();
                }
            }
        }
    }
}