#pragma once

#include "RenderSystem/Common.h"
#include "VKEForwardDeclarations.h"
#include "ThirdParty/math/DirectXMath.h"

namespace VKE
{
    namespace RenderSystem
    {
        class CCommandBuffer;
        class CContext;
        class CDevice;

        class CRenderQueue
        {
            using MatrixVec = vke_vector< DirectX::XMMATRIX >;
            struct SDrawData
            {

            };

            struct SCalcData
            {
                vke_vector< bool >  aVisibles;
            };

            public:

            CRenderQueue(CDevice* pCtx);
            ~CRenderQueue();

            Result Create(const SRenderQueueInfo& Info);
            void Destroy();

            void Begin();
            void Draw();
            void End();

            protected:

            SRenderQueueInfo    m_Info;
            CDevice*            m_pDevice;
            CommandBufferPtr    m_pCmdBuff;
            SCalcData           m_CalcData;
        };
    }
} // VKE
