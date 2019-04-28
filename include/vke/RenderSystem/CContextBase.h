#pragma once

#include "RenderSystem/Common.h"
#include "RenderSystem/CQueue.h"
#include "RenderSystem/Vulkan/Managers/CSubmitManager.h"
#include "RenderSystem/CCommandBuffer.h"

namespace VKE
{
    namespace RenderSystem
    {
        class CDeviceContext;

        struct SContextBaseDesc
        {
            QueueRefPtr         pQueue;
            handle_t            hCommandBufferPool;
        };

        // Implementation in CDeviceContext.cpp
        class VKE_API CContextBase : public virtual CCommandBufferContext
        {
            friend class CDeviceContext;
            friend class CGraphicsContext;
            friend class CComputeContext;
            friend class CCommandBuffer;
            friend class CBufferManager;

            struct SPreparationData 
            {
                CCommandBuffer* pCmdBuffer = nullptr;
                DDIFence        hDDIFence = DDI_NULL_HANDLE;
            };

            public:

                CContextBase( CDeviceContext* pCtx );

                Result                  Create(const SContextBaseDesc& Desc);
                void                    Destroy();

                CCommandBuffer*         GetPreparationCommandBuffer();
                Result                  BeginPreparation();
                Result                  EndPreparation();
                Result                  WaitForPreparation();
                bool                    IsPreparationDone();

                DDISemaphore            GetSignaledSemaphore() const { return _GetLastExecutedBatch()->GetSignaledSemaphore(); }

                Result                  UpdateBuffer( const SUpdateMemoryInfo& Info, BufferPtr* ppInOut );

            protected:

                CCommandBuffer*         _CreateCommandBuffer();
                Result                  _BeginCommandBuffer( CCommandBuffer** ppInOut );
                Result                  _EndCommandBuffer( CCommandBuffer** ppInOut, COMMAND_BUFFER_END_FLAGS flags );
                void                    _FlushCommandBuffer( CCommandBuffer** ppInOut );
                Result                  _ExecuteCommandBuffer( CCommandBuffer** ppInOut );

                CCommandBufferBatch*    _GetLastExecutedBatch() const { return m_pLastExecutedBatch; }

            protected:

                CDDI&                   m_DDI;
                CDeviceContext*         m_pDeviceCtx;
                QueueRefPtr             m_pQueue;
                handle_t                m_hCommandPool = NULL_HANDLE;
                CCommandBufferBatch*    m_pLastExecutedBatch;
                SPreparationData        m_PreparationData;
                uint8_t                 m_backBufferIdx = 0;
                bool                    m_initComputeShader = false;
                bool                    m_initGraphicsShaders = false;
        };
    } // RenderSystem
} // VKE