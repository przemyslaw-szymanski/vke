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
                struct SRenderFrame : public Threads::ITask
                {
                    SRenderFrame()
                    {}

                    TaskState _OnStart( uint32_t threadId ) override;

                    CGraphicsContext* pCtx;
                };

                struct SPresent : public Threads::ITask
                {
                    SPresent()
                    {
                        VKE_DEBUG_CODE(
                            char buff[ 128 ];
                            sprintf_s(buff, 128, "GraphicsContext Present: %p", this);
                            this->m_strDbgName = buff;
                        );
                    }
                    CGraphicsContext* pCtx;
                    TaskState _OnStart(uint32_t threadId) override;
                };

                struct SSwapBuffers : public Threads::ITask
                {
                    SSwapBuffers()
                    {}

                    TaskState _OnStart(uint32_t threadId) override;

                    CGraphicsContext* pCtx;
                };

                SRenderFrame    RenderFrame;
                SPresent        Present;
                SSwapBuffers    SwapBuffers;
            };
        }
    }
}