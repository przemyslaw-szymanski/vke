#pragma once

#include "RenderSystem/Common.h"
#include "RenderSystem/CQueue.h"
#include "RenderSystem/Vulkan/Managers/CSubmitManager.h"

namespace VKE
{
    namespace RenderSystem
    {
        class CDeviceContext;

        struct SContextCommonDesc
        {
            QueueRefPtr         pQueue;
            handle_t            hCommandBufferPool;
            SSubmitManagerDesc  SubmitMgrDesc;
        };

        // Implementation in CDeviceContext.cpp
        struct SCommonContext
        {
            friend class CDeviceContext;
            friend class CGraphicsContext;
            friend class CComputeContext;
            friend class CCommandBuffer;

            SCommonContext( CDDI& DDI, CDeviceContext* pCtx ) :
                DDI{ DDI }
                , pDeviceCtx{ pCtx }
                , SubmitMgr{ pCtx }
            {}

            Result                  Init(const SContextCommonDesc& Desc);
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