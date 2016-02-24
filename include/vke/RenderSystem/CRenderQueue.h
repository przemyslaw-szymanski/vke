#pragma once

#include "RenderSystem/Common.h"
#include "VKEForwardDeclarations.h"

namespace VKE
{
    namespace RenderSystem
    {
        class CCommandBuffer;
        class CContext;
        class CDevice;

        class CRenderQueue
        {
            public:

            CRenderQueue(CDevice* pCtx);
            ~CRenderQueue();

            Result Create(const SRenderQueueInfo& Info);
            void Destroy();

            void Begin();
            void End();

            protected:

            SRenderQueueInfo    m_Info;
            CDevice*            m_pDevice;
            CommandBufferPtr    m_pCmdBuff;
        };
    }
} // VKE
