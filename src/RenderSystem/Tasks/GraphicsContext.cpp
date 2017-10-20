#include "RenderSystem/Tasks/GraphicsContext.h"
#include "RenderSystem/CGraphicsContext.h"

namespace VKE
{
    namespace RenderSystem
    {
        namespace Tasks
        {
            static const TaskState g_aStatuses[] =
            {
                TaskStateBits::REMOVE,
                TaskStateBits::OK
            };

            TaskState SGraphicsContext::SPresent::_OnStart(uint32_t /*threadId*/)
            {
                {
                    return pCtx->_PresentFrameTask();
                }
            }

            TaskState SGraphicsContext::SBeginFrame::_OnStart(uint32_t)
            {
                {
                    return pCtx->_BeginFrameTask();
                }
                return TaskStateBits::OK;
            }

            TaskState SGraphicsContext::SEndFrame::_OnStart(uint32_t)
            {
                {
                    return pCtx->_EndFrameTask();
                }
            }

            TaskState SGraphicsContext::SSwapBuffers::_OnStart(uint32_t)
            {
                {
                    return pCtx->_SwapBuffersTask();
                }
            }
        }
    }
}