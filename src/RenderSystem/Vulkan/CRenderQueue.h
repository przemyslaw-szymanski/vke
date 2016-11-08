#pragma once

#include "RenderSystem/Vulkan/Common.h"
#include "Core/VKEForwardDeclarations.h"
#include "ThirdParty/math/DirectX/DirectXMath.h"

namespace VKE
{
    namespace RenderSystem
    {
        class CCommandBuffer;
        class CGraphicsContext;
        class CDevice;

        class CRenderQueue
        {
            using MatrixVec = vke_vector< DirectX::XMMATRIX >;
            static const uint32_t COMMAND_BUFFER_COUNT = 3;

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

            Result Create(const SGraphicsQueueInfo& Info);
            void Destroy();

            void Begin();
            void Draw();
            void End();
            void Submit();

            vke_force_inline
            CommandBufferPtr    GetCommandBuffer() { return m_pCmdBuffers[ m_currCmdBuffId ]; }

            CommandBufferPtr    GetNextCommandBuffer();

            protected:

            SGraphicsQueueInfo    m_Desc;
            CDevice*            m_pDevice;
            CommandBufferPtr    m_pCmdBuffers[COMMAND_BUFFER_COUNT];
            uint32_t            m_currCmdBuffId = 0;
            SCalcData           m_CalcData;
            uint16_t            m_type = 0;
            uint16_t            m_id = 0;
            uint16_t            m_priority = 0;
        };
    }
} // VKE
