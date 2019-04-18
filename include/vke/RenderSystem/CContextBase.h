#pragma once

#include "RenderSystem/Common.h"
#include "RenderSystem/CQueue.h"
#include "RenderSystem/Vulkan/Managers/CSubmitManager.h"

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
        class VKE_API CContextBase
        {
            friend class CDeviceContext;
            friend class CGraphicsContext;
            friend class CComputeContext;
            friend class CCommandBuffer;

            public:

                CContextBase( CDDI& DDI, CDeviceContext* pCtx ) :
                    m_DDI{ DDI }
                    , m_pDeviceCtx{ pCtx }
                {}

                Result                  Create(const SContextBaseDesc& Desc);
                void                    Destroy();

            protected:

                CommandBufferPtr        _CreateCommandBuffer();
                Result                  _BeginCommandBuffer( CCommandBuffer** ppInOut );
                Result                  _EndCommandBuffer( CCommandBuffer** ppInOut, COMMAND_BUFFER_END_FLAG flag );
                void                    _FlushCommandBuffer( CCommandBuffer** ppInOut );
                Result                  _ExecuteCommandBuffer( CCommandBuffer** ppInOut );

            protected:

                CDDI&               m_DDI;
                CDeviceContext*     m_pDeviceCtx;
                QueueRefPtr         m_pQueue;
                handle_t            m_hCommandPool = NULL_HANDLE;
                uint8_t             m_backBufferIdx = 0;
        };
    } // RenderSystem
} // VKE