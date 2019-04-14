#include "RenderSystem/CContextBase.h"
#include "RenderSystem/CCommandBuffer.h"
#include "RenderSystem/CDeviceContext.h"

namespace VKE
{
    namespace RenderSystem
    {
        Result CContextBase::Create( const SContextBaseDesc& Desc )
        {
            pQueue = Desc.pQueue;
            hCommandPool = Desc.hCommandBufferPool;
            SSubmitManagerDesc SMgrDesc;
            SMgrDesc.pQueue = pQueue;
            SMgrDesc.hCmdBufferPool = hCommandPool;
            Result ret = SubmitMgr.Create( SMgrDesc );

            return ret;
        }

        void CContextBase::Destroy()
        {
            if( pDeviceCtx != nullptr )
            {
                pDeviceCtx = nullptr;
                SubmitMgr.Destroy();
                hCommandPool = NULL_HANDLE;
            }
        }

        CommandBufferPtr CContextBase::CreateCommandBuffer()
        {
            CommandBufferPtr pCb;
            //m_CmdBuffMgr.CreateCommandBuffers< VKE_THREAD_SAFE >( 1, &pCb );
            Result res = pDeviceCtx->_CreateCommandBuffers( hCommandPool, 1, &pCb );
            if( VKE_SUCCEEDED( res ) )
            {
                pCb->m_pBatch = SubmitMgr.GetCurrentBatch();
                pCb->m_currBackBufferIdx = backBufferIdx;
            }
            return pCb;
        }

    } // RenderSystem
} // VKE