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
        class CContextBase
        {
            friend class CDeviceContext;
            friend class CGraphicsContext;
            friend class CComputeContext;
            friend class CCommandBuffer;

            public:

                CContextBase( CDDI& DDI, CDeviceContext* pCtx ) :
                    DDI{ DDI }
                    , pDeviceCtx{ pCtx }
                    , SubmitMgr{ pCtx }
                {}

                Result                  Create(const SContextBaseDesc& Desc);
                void                    Destroy();

                CommandBufferPtr        CreateCommandBuffer();
                Result                  BeginCommandBuffer( CCommandBuffer** ppInOut );
                Result                  EndCommandBuffer( CCommandBuffer** ppInOut, COMMAND_BUFFER_END_FLAG flag );

            protected:

                CDDI&               DDI;
                CDeviceContext*     pDeviceCtx;
                QueueRefPtr         pQueue;
                CSubmitManager      SubmitMgr;
                handle_t            hCommandPool = NULL_HANDLE;
                uint8_t             backBufferIdx = 0;
        };
    } // RenderSystem
} // VKE