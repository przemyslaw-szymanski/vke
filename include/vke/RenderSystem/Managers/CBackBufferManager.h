#pragma once

#include "RenderSystem/Common.h"
#include "Core/Utils/TCDynamicArray.h"
#include "Core/Utils/TCConstantArray.h"

namespace VKE
{
    namespace RenderSystem
    {
        class CGraphicsContext;
        class CRenderingPipeline;

        namespace Managers
        {
            struct SBackBufferManagerDesc
            {
                uint32_t    backBufferCount = 3;
            };

            template
            <
                class T,
                uint32_t MAX_BACK_BUFFER_COUNT = 4
            >
            struct TSBackBufferData
            {
                using DataVec = Utils::TCDynamicArray< T >;
                using BackBufferDataVec = Utils::TCConstantArray< DataVec, MAX_BACK_BUFFER_COUNT >;

                BackBufferDataVec   vData;
            };

            class CBackBufferManager final
            {
                friend class CGraphicsContext;

                using RenderingPipelineBuffer = TSBackBufferData< CRenderingPipeline* >;

                struct SBackBuffers
                {
                    RenderingPipelineBuffer RenderingPipelines;
                };

                struct SBackBufferDataPointers
                {
                    CRenderingPipeline*     pRenderingPipeline = nullptr;
                };

                struct SBackBufferDataHandles
                {
                    RenderingPipelineHandle hRenderingPipeline = NULL_HANDLE;
                };

                public:

                    CBackBufferManager(CGraphicsContext* pCtx);
                    ~CBackBufferManager();

                    Result Create(const SBackBufferManagerDesc& Desc);
                    void Destroy();

                    uint32_t AcquireNextBuffer();

                    void SetRenderingPipeline(const RenderingPipelineHandle& hPipeline);

                protected:

                    void _Destroy();

                protected:

                    SBackBufferManagerDesc  m_Desc;
                    CGraphicsContext*       m_pCtx = nullptr;
                    SBackBuffers            m_BackBuffers;
                    SBackBufferDataPointers m_CurrBackBufferPtrs;
                    SBackBufferDataHandles  m_CurrBackBufferHandles;
                    uint16_t                m_currBackBufferIndex = 0;
                    uint16_t                m_backBufferCount = 0;
            };
        } // Managers
    } // RenderSystem
} // VKE