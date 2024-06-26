#include "RenderSystem/Tasks/GraphicsContext.h"
#include "RenderSystem/CGraphicsContext.h"

namespace VKE
{
    namespace RenderSystem
    {
        namespace Tasks
        {
            TaskState SGraphicsContext::SPresent::_OnStart(uint32_t /*threadId*/)
            {
                {
                    //return pCtx->_PresentFrameTask();
                    return TaskStateBits::OK;
                }
            }

            TaskState SGraphicsContext::SRenderFrame::_OnStart(uint32_t)
            {
                {
                    //return pCtx->_RenderFrameTask();
                    return TaskStateBits::OK;
                }
            }

            TaskState SGraphicsContext::SSwapBuffers::_OnStart(uint32_t)
            {
                {
                    //return pCtx->_SwapBuffersTask();
                    return TaskStateBits::OK;
                }
            }

            TaskState SGraphicsContext::SExecuteCommandBuffers::_OnStart( uint32_t )
            {
                {
                    //return pCtx->_ExecuteCommandBuffersTask();
                    return TaskStateBits::OK;
                }
            }
        }
    }
}