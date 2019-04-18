#include "RenderSystem/CContextBase.h"
#include "RenderSystem/CCommandBuffer.h"
#include "RenderSystem/CDeviceContext.h"

namespace VKE
{
    namespace RenderSystem
    {
        Result CContextBase::Create( const SContextBaseDesc& Desc )
        {
            m_pQueue = Desc.pQueue;
            m_hCommandPool = Desc.hCommandBufferPool;

            return VKE_OK;
        }

        void CContextBase::Destroy()
        {
            if( m_pDeviceCtx != nullptr )
            {
                m_pDeviceCtx = nullptr;
                m_hCommandPool = NULL_HANDLE;
            }
        }

        CommandBufferPtr CContextBase::_CreateCommandBuffer()
        {
            CommandBufferPtr pCb;
            //m_CmdBuffMgr.CreateCommandBuffers< VKE_THREAD_SAFE >( 1, &pCb );
            Result res = m_pDeviceCtx->_CreateCommandBuffers( m_hCommandPool, 1, &pCb );
            if( VKE_SUCCEEDED( res ) )
            {
                pCb->m_pBatch = m_pQueue->_GetSubmitManager()->GetCurrentBatch( m_hCommandPool );
                pCb->m_currBackBufferIdx = m_backBufferIdx;
            }
            return pCb;
        }

    } // RenderSystem
} // VKE