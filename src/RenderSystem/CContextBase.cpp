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

        CCommandBuffer* CContextBase::_CreateCommandBuffer()
        {
            CCommandBuffer* pCb;
            //m_CmdBuffMgr.CreateCommandBuffers< VKE_THREAD_SAFE >( 1, &pCb );
            Result res = m_pDeviceCtx->_CreateCommandBuffers( m_hCommandPool, 1, &pCb );
            if( VKE_SUCCEEDED( res ) )
            {
                SCommandBufferInitInfo Info;
                Info.pBaseCtx = this;
                Info.backBufferIdx = m_backBufferIdx;

                pCb->Init( Info );
            }
            return pCb;
        }

        Result CContextBase::_BeginCommandBuffer( CCommandBuffer** ppInOut )
        {
            Result ret = VKE_OK;
            CCommandBuffer* pCb = *ppInOut;
            VKE_ASSERT( pCb && pCb->m_pBaseCtx, "" );

            m_pDeviceCtx->DDI().Reset( pCb->GetDDIObject() );
            m_pDeviceCtx->_GetDDI().BeginCommandBuffer( pCb->GetDDIObject() );

            pCb->m_currBackBufferIdx = m_backBufferIdx;

            return ret;
        }

        Result CContextBase::_EndCommandBuffer( CCommandBuffer** ppInOut, COMMAND_BUFFER_END_FLAG flag )
        {
            Result ret = VKE_OK;
            CCommandBuffer* pCb = *ppInOut;
            m_DDI.EndCommandBuffer( pCb->GetDDIObject() );

            auto pSubmitMgr = m_pQueue->_GetSubmitManager();

            switch( flag )
            {
                case CommandBufferEndFlags::END:
                {
                    pCb->m_state = CCommandBuffer::States::END;
                    pSubmitMgr->Submit( m_pDeviceCtx, m_hCommandPool, pCb );
                }
                break;
                case CommandBufferEndFlags::EXECUTE:
                {
                    // If a command buffer needs to be executed immediately
                    // generate a temporary batch and execute it
                    pCb->m_state = CCommandBuffer::States::FLUSH;
                    auto pBatch = pSubmitMgr->_GetNextBatch( m_pDeviceCtx, m_hCommandPool );
                    pBatch->_Submit( pCb );
                    ret = pSubmitMgr->ExecuteBatch( m_pQueue, &pBatch );
                }
                break;
                case CommandBufferEndFlags::EXECUTE_AND_WAIT:
                {
                    pCb->m_state = CCommandBuffer::States::FLUSH;
                    auto pBatch = pSubmitMgr->_GetNextBatch( m_pDeviceCtx, m_hCommandPool );
                    pBatch->_Submit( pCb );
                    ret = pSubmitMgr->ExecuteBatch( m_pQueue, &pBatch );
                    if( VKE_SUCCEEDED( ret ) )
                    {
                        ret = m_DDI.WaitForFences( pBatch->m_hDDIFence, UINT64_MAX );
                    }
                }
                break;
            };

            return ret;
        }

    } // RenderSystem
} // VKE