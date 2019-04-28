#include "RenderSystem/CContextBase.h"
#include "RenderSystem/CCommandBuffer.h"
#include "RenderSystem/CDeviceContext.h"
#include "RenderSystem/Managers/CBufferManager.h"

namespace VKE
{
    namespace RenderSystem
    {
        static CCommandBufferBatch g_sDummyBatch;

        CContextBase::CContextBase( CDeviceContext* pCtx ) :
            m_DDI( pCtx->DDI() )
            , m_pDeviceCtx( pCtx )
            , m_pLastExecutedBatch( &g_sDummyBatch )
        {

        }

        Result CContextBase::Create( const SContextBaseDesc& Desc )
        {
            m_pQueue = Desc.pQueue;
            m_hCommandPool = Desc.hCommandBufferPool;

            if( m_PreparationData.pCmdBuffer == nullptr )
            {
                m_PreparationData.pCmdBuffer = _CreateCommandBuffer();
                SFenceDesc FenceDesc;
                m_PreparationData.hDDIFence = m_DDI.CreateObject( FenceDesc, nullptr );
            }

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
                Info.initComputeShader = m_initComputeShader;
                Info.initGraphicsShaders = m_initGraphicsShaders;
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

        Result CContextBase::_EndCommandBuffer( CCommandBuffer** ppInOut, COMMAND_BUFFER_END_FLAGS flags )
        {
            Result ret = VKE_OK;
            CCommandBuffer* pCb = *ppInOut;
            m_DDI.EndCommandBuffer( pCb->GetDDIObject() );

            auto pSubmitMgr = m_pQueue->_GetSubmitManager();

            if( flags & CommandBufferEndFlags::END )
            {
                pCb->m_state = CCommandBuffer::States::END;
                pSubmitMgr->Submit( m_pDeviceCtx, m_hCommandPool, pCb );
            }
            else if( flags & CommandBufferEndFlags::EXECUTE )
            {
                pCb->m_state = CCommandBuffer::States::FLUSH;
                auto pBatch = pSubmitMgr->_GetNextBatch( m_pDeviceCtx, m_hCommandPool );
                pSubmitMgr->m_signalSemaphore = ( flags & CommandBufferEndFlags::DONT_SIGNAL_SEMAPHORE ) == 0;
                pBatch->_Submit( pCb );
                ret = pSubmitMgr->ExecuteBatch( m_pQueue, &pBatch );
                if( flags & CommandBufferEndFlags::WAIT )
                {
                    if( VKE_SUCCEEDED( ret ) )
                    {
                        ret = m_DDI.WaitForFences( pBatch->m_hDDIFence, UINT64_MAX );
                    }
                }
            }

            return ret;
        }

        Result CContextBase::UpdateBuffer( const SUpdateMemoryInfo& Info, BufferPtr* ppInOut )
        {
            Result ret = VKE_FAIL;
            CBuffer* pBuffer = ( *ppInOut ).Get();
            ret = m_pDeviceCtx->m_pBufferMgr->UpdateBuffer( Info, this, &pBuffer );
            return ret;
        }

        CCommandBuffer* CContextBase::GetPreparationCommandBuffer()
        {
            if( m_PreparationData.pCmdBuffer->m_state != CCommandBuffer::States::BEGIN )
            {
                if( VKE_FAILED( BeginPreparation() ) )
                {
                    return nullptr;
                }
            }
            return m_PreparationData.pCmdBuffer;
        }

        bool CContextBase::IsPreparationDone()
        {
            return m_DDI.IsReady( m_PreparationData.hDDIFence );
        }

        Result CContextBase::BeginPreparation()
        {
            Result ret = VKE_ENOTREADY;
            if( IsPreparationDone() )
            {
                if( m_PreparationData.pCmdBuffer->m_state != CCommandBuffer::States::BEGIN )
                {
                    ret = _BeginCommandBuffer( &m_PreparationData.pCmdBuffer );
                    if( VKE_SUCCEEDED( ret ) )
                    {
                        m_PreparationData.pCmdBuffer->m_state = CCommandBuffer::States::BEGIN;
                    }
                }
                else
                {
                    ret = VKE_OK;
                }
            }
            return ret;
        }

        Result CContextBase::EndPreparation()
        {
            VKE_ASSERT( m_PreparationData.pCmdBuffer->m_state == CCommandBuffer::States::BEGIN, "" );
            Result ret = VKE_FAIL;
            DDICommandBuffer hDDICmdBuffer = m_PreparationData.pCmdBuffer->GetDDIObject();
            m_DDI.EndCommandBuffer( hDDICmdBuffer );
            m_PreparationData.pCmdBuffer->m_state = CCommandBuffer::States::END;

            SSubmitInfo Info;
            Info.commandBufferCount = 1;
            Info.hDDIFence = m_PreparationData.hDDIFence;
            Info.hDDIQueue = m_pQueue->GetDDIObject();
            Info.pDDICommandBuffers = &hDDICmdBuffer;
            Info.pDDISignalSemaphores = nullptr;
            Info.pDDIWaitSemaphores = nullptr;
            Info.signalSemaphoreCount = 0;
            Info.waitSemaphoreCount = 0;

            ret = m_pQueue->Execute( Info );
            m_PreparationData.pCmdBuffer->m_state = CCommandBuffer::States::FLUSH;
            return ret;
        }

        Result CContextBase::WaitForPreparation()
        {
            Result ret = m_DDI.WaitForFences( m_PreparationData.hDDIFence, UINT64_MAX );
            m_DDI.Reset( &m_PreparationData.hDDIFence );
            return ret;
        }

    } // RenderSystem
} // VKE