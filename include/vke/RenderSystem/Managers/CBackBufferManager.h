#pragma once

#include "RenderSystem/Common.h"
#include "Core/Utils/TCDynamicArray.h"
#include "Core/Utils/TCConstantArray.h"
#include "vke/Core/VKEConfig.h"

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

            class CBackBufferManager final
            {
                friend class CGraphicsContext;

                template<class T>
                using ContainerArray = Utils::TCDynamicArray< T >[ Config::MAX_BACK_BUFFER_COUNT ];

                struct SBackBuffers
                {
                    ContainerArray< CRenderingPipeline* >       aRenderingPipelines;
                    ContainerArray< void* >                     aCustomData;
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
                    
                    uint32_t GetBackBufferCount() const { return m_Desc.backBufferCount;}

                    uint32_t AddCustomData(void** apData);
                    void* GetCustomData(uint32_t idx) const;
                    void UpdateCustomData(uint32_t idx, void** apData);

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