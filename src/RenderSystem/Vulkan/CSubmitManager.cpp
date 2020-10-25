#if VKE_VULKAN_RENDERER
#include "RenderSystem/Vulkan/Managers/CSubmitManager.h"
#include "RenderSystem/CDeviceContext.h"
#include "RenderSystem/Vulkan/CCommandBuffer.h"

namespace VKE
{
    namespace RenderSystem
    {
        void CCommandBufferBatch::operator=(const CCommandBufferBatch& Other)
        {
            m_hDDIFence = Other.m_hDDIFence;

            m_vDDIWaitSemaphores = Other.m_vDDIWaitSemaphores;
            m_hDDISignalSemaphore = Other.m_hDDISignalSemaphore;
            m_vpCommandBuffers = Other.m_vpCommandBuffers;
            m_vDDICommandBuffers = Other.m_vDDICommandBuffers;

            m_pMgr = Other.m_pMgr;
            m_currCmdBuffer = Other.m_currCmdBuffer;

            m_submitted = Other.m_submitted;
        }

        void CCommandBufferBatch::operator =( CCommandBufferBatch&& Other )
        {
            m_hDDIFence = Other.m_hDDIFence;

            m_vDDIWaitSemaphores = std::move( Other.m_vDDIWaitSemaphores );
            m_hDDISignalSemaphore = Other.m_hDDISignalSemaphore;
            m_vpCommandBuffers = std::move( Other.m_vpCommandBuffers );
            m_vDDICommandBuffers = std::move( Other.m_vDDICommandBuffers );

            m_pMgr = Other.m_pMgr;
            m_currCmdBuffer = Other.m_currCmdBuffer;

            m_submitted = Other.m_submitted;
        }

        bool CCommandBufferBatch::CanSubmit() const
        {
            return m_submitted == false;
        }

        void CCommandBufferBatch::_Submit( CCommandBuffer* pCb)
        {
            m_vDDICommandBuffers.PushBack( pCb->GetDDIObject() );
            pCb->_SetFence( m_hDDIFence );
            m_vpCommandBuffers.PushBack( pCb );
            for( uint32_t i = 0; i < pCb->m_vDDIWaitOnSemaphores.GetCount(); ++i )
            {
                m_vDDIWaitSemaphores.PushBack( pCb->m_vDDIWaitOnSemaphores[i] );
            }
        }

        void CCommandBufferBatch::_Clear()
        {
            m_vpCommandBuffers.Clear();
            m_vDDICommandBuffers.Clear();
            m_vDDIWaitSemaphores.Clear();
            m_submitted = false;
            m_currCmdBuffer = 0;
        }

        CSubmitManager::CSubmitManager()
        {}

        CSubmitManager::~CSubmitManager()
        {

        }

        void CSubmitManager::Destroy( CDeviceContext* pCtx )
        {
            auto& DDI = pCtx->_GetDDI();
            for( uint32_t i = 0; i < m_CommandBufferBatches.vSubmits.GetCount(); ++i )
            {
                DDI.DestroyFence( &m_CommandBufferBatches.vSubmits[i].m_hDDIFence, nullptr );
                DDI.DestroySemaphore( &m_CommandBufferBatches.vSubmits[i].m_hDDISignalSemaphore, nullptr );
                //DDI.DestroyObject( &m_Submits.vSubmits[i].m_hDDISignalSemaphore, nullptr );
                //m_pCtx->_DestroyFence(&m_Submits.vSubmits[ i ].m_hDDIFence);
                //m_pCtx->_DestroySemaphore(&m_Submits.vSubmits[ i ].m_hDDISignalSemaphore);
            }
            m_CommandBufferBatches.vSubmits.Clear();
        }

        void CSubmitManager::_CreateSubmits(CDeviceContext* pCtx, uint32_t count)
        {
            SFenceDesc FenceDesc;
            FenceDesc.isSignaled = false;
            SSemaphoreDesc SemaphoreDesc;

            for( uint32_t i = count; i-- > 0; )
            {
                CCommandBufferBatch Tmp;
                Tmp.m_pMgr = this;
                Tmp.m_hDDIFence = pCtx->_GetDDI().CreateFence( FenceDesc, nullptr );
                //pCtx->DDI().Reset( &Tmp.m_hDDIFence );
                Tmp.m_hDDISignalSemaphore = pCtx->_GetDDI().CreateSemaphore( SemaphoreDesc, nullptr );
                m_CommandBufferBatches.vSubmits.PushBack( Tmp );
            }
        }

        Result CSubmitManager::Create(const SSubmitManagerDesc& Desc)
        {
            _CreateSubmits( Desc.pCtx, SUBMIT_COUNT );
            return VKE_OK;
        }

        CCommandBufferBatch* CSubmitManager::_GetSubmit( CDeviceContext* pCtx, const handle_t& hCmdPool, uint32_t idx )
        {
            CCommandBufferBatch* pBatch = &m_CommandBufferBatches.vSubmits[idx];
            if( pCtx->_GetDDI().IsReady( pBatch->m_hDDIFence ) )
            {
                pCtx->_GetDDI().Reset( &pBatch->m_hDDIFence );
                _FreeCommandBuffers( pCtx, hCmdPool, pBatch );
                return pBatch;
            }
            return nullptr;
        }

        CCommandBufferBatch* CSubmitManager::_GetNextSubmit( CDeviceContext* pCtx, const handle_t& hCmdPool )
        {
            return _GetNextSubmitFreeSubmitFirst( pCtx, hCmdPool );
        }

        CCommandBufferBatch* CSubmitManager::_GetNextSubmitReadySubmitFirst( CDeviceContext* pCtx, const handle_t& hCmdPool )
        {
            // Get first submit
            CCommandBufferBatch* pBatch = nullptr;
            // If there are any submitts
            if( !m_CommandBufferBatches.qpSubmitted.IsEmpty() )
            {
                pBatch = m_CommandBufferBatches.qpSubmitted.Front();
                // Check if oldest submit is ready
                if( pCtx->_GetDDI().IsReady( pBatch->m_hDDIFence ) )
                {
                    m_CommandBufferBatches.qpSubmitted.PopFrontFast( &pBatch );
                    pCtx->_GetDDI().Reset( &pBatch->m_hDDIFence );
                    if( !pBatch->m_vDDICommandBuffers.IsEmpty() )
                    {
                        _FreeCommandBuffers( pCtx, hCmdPool, pBatch );
                    }
                    return pBatch;
                }
            }
            // If the oldest submit is not ready get next one
            {
                // If there are no submitted cmd buffers
                auto& idx = m_CommandBufferBatches.currSubmitIdx;
                if( idx < m_CommandBufferBatches.vSubmits.GetCount() )
                {
                    pBatch = &m_CommandBufferBatches.vSubmits[idx++];
                    //return pBatch;
                }
                else
                {
                    // ... or create new ones if no one left in the buffer
                    _CreateSubmits( pCtx, SUBMIT_COUNT );
                    pBatch = _GetNextSubmitReadySubmitFirst( pCtx, hCmdPool );
                }
            }
            return pBatch;
        }

        CCommandBufferBatch* CSubmitManager::_GetNextSubmitFreeSubmitFirst( CDeviceContext* pCtx, const handle_t& hCmdPool )
        {
            // Get first submit
            CCommandBufferBatch* pBatch = nullptr;
            // Get next submit from the pool
            auto& idx = m_CommandBufferBatches.currSubmitIdx;
            if( idx < m_CommandBufferBatches.vSubmits.GetCount() )
            {
                pBatch = &m_CommandBufferBatches.vSubmits[ idx++ ];
                assert(pBatch);
                return pBatch;
            }

            // If there are any submitts in the pool try to get the oldest submitted if ready
            if( !m_CommandBufferBatches.qpSubmitted.IsEmpty() )
            {
                pBatch = m_CommandBufferBatches.qpSubmitted.Front();
                // Check if oldest submit is ready
                if( pCtx->_GetDDI().IsReady( pBatch->m_hDDIFence ) )
                {
                    m_CommandBufferBatches.qpSubmitted.PopFrontFast( &pBatch );
                    pCtx->_GetDDI().Reset( &pBatch->m_hDDIFence );
                    if( !pBatch->m_vDDICommandBuffers.IsEmpty() )
                    {
                        _FreeCommandBuffers( pCtx, hCmdPool, pBatch );
                    }
                    return pBatch;
                }
            }
            // If the oldest submit is not ready create a new one
            _CreateSubmits( pCtx, SUBMIT_COUNT );
            pBatch = _GetNextSubmitFreeSubmitFirst( pCtx, hCmdPool );
           
            return pBatch;
        }

        CCommandBufferBatch* CSubmitManager::_GetNextBatch( CDeviceContext* pCtx, const handle_t& hCmdPool )
        {
            CCommandBufferBatch* pBatch = nullptr;
            {
                //pBatch = _GetNextSubmitFreeSubmitFirst( pCtx, hCmdPool );
                pBatch = _GetNextSubmitReadySubmitFirst( pCtx, hCmdPool );
                assert(pBatch);
            }
            assert(pBatch && "No free submit batch left");
            pBatch->_Clear();

            pBatch->m_submitted = false;
            //pBatch->m_hDDIWaitSemaphore = m_hDDIWaitSemaphore;
            //m_hDDIWaitSemaphore = DDI_NULL_HANDLE;

            return pBatch;
        }

        CCommandBufferBatch* CSubmitManager::_GetCurrentBatch( CDeviceContext* pCtx, const handle_t& hCmdPool )
        {
            if( m_pCurrBatch == nullptr )
            {
                m_pCurrBatch = _GetNextBatch( pCtx, hCmdPool );
            }
            return m_pCurrBatch;
        }

        void CSubmitManager::_FreeCommandBuffers( CDeviceContext* pCtx, const handle_t& hPool, CCommandBufferBatch* pBatch)
        {
            auto& vCmdBuffers = pBatch->m_vpCommandBuffers;
            VKE_ASSERT( hPool != 0, "CommandBufferPool handle must be valid." );
            pCtx->_FreeCommandBuffers( hPool, vCmdBuffers.GetCount(), &vCmdBuffers[0] );
            vCmdBuffers.Clear();
        }

        void CSubmitManager::_Submit( CDeviceContext* pCtx, const handle_t& hCmdPool, CCommandBuffer* pCb )
        {
            _GetCurrentBatch( pCtx, hCmdPool )->_Submit( pCb );
        }

        Result CSubmitManager::_Submit( CDeviceContext* pCtx, QueuePtr pQueue, CCommandBufferBatch* pBatch)
        {
            DDISemaphore hDDISignal = DDI_NULL_HANDLE;
            uint32_t signalCount = 0;
            uint32_t waitCount = 0;
            DDISemaphore* phDDIWaitSemaphores = nullptr;   
            
            if( m_signalSemaphore )
            {
                signalCount = 1;
                hDDISignal = pBatch->m_hDDISignalSemaphore;
            }

            if( m_waitForSemaphores )
            {
                pCtx->_GetSignaledSemaphores( &pBatch->m_vDDIWaitSemaphores );
                waitCount = pBatch->m_vDDIWaitSemaphores.GetCount();
                phDDIWaitSemaphores = pBatch->m_vDDIWaitSemaphores.GetData();
            }

            SSubmitInfo Info;
            Info.commandBufferCount = static_cast< uint8_t >( pBatch->m_vDDICommandBuffers.GetCount() );
            Info.pDDICommandBuffers = pBatch->m_vDDICommandBuffers.GetData();
            Info.hDDIFence = pBatch->m_hDDIFence;
            Info.signalSemaphoreCount = static_cast< uint8_t >( signalCount );
            Info.pDDISignalSemaphores = &hDDISignal;
            Info.waitSemaphoreCount = static_cast< uint8_t >( waitCount );
            Info.pDDIWaitSemaphores = phDDIWaitSemaphores;
            Info.hDDIQueue = pQueue->GetDDIObject();

            m_signalSemaphore = true; // reset signaling flag
            Result ret = pQueue->Execute( Info );
            if( VKE_SUCCEEDED( ret ) )
            {
                pBatch->m_submitted = true;
                // $TID _Submit: cb={si.pCommandBuffers[0]} ss={si.pSignalSemaphores[0]}, ws={si.pWaitSemaphores[0]}
                //auto c = m_Submits.qpSubmitted.GetCount();

                m_CommandBufferBatches.qpSubmitted.PushBack( pBatch );
            }
            else
            {
                ///@TODO handle if submit not succeedeed
                VKE_ASSERT( ret == VKE_OK, "SUBMIT NOT SUCCEEDEED. NOT HANDLED." );
            }
            return ret;
        }

        Result CSubmitManager::WaitForBatch( CDeviceContext* pCtx, const uint64_t& timeout, CCommandBufferBatch* pBatch )
        {
            return pCtx->DDI().WaitForFences( pBatch->m_hDDIFence, timeout );
        }

        void CSubmitManager::SignalSemaphore( DDISemaphore* phDDISemaphoreOut )
        {
            if( phDDISemaphoreOut != nullptr )
            {
                *phDDISemaphoreOut = m_pCurrBatch->m_hDDISignalSemaphore;
                m_signalSemaphore = true;
            }
            else
            {
                m_signalSemaphore = false;
            }
        }

        Result CSubmitManager::ExecuteCurrentBatch( CDeviceContext* pCtx, QueuePtr pQueue, CCommandBufferBatch** ppOut )
        {
            VKE_ASSERT( m_pCurrBatch != nullptr, "New batch must be set first." );
            Result ret = VKE_FAIL;
            Threads::ScopedLock l( m_CurrentBatchSyncObj );
            if( m_pCurrBatch->CanSubmit() )
            {
                ret = _Submit( pCtx, pQueue, m_pCurrBatch );
                *ppOut = m_pCurrBatch;
                m_pCurrBatch = nullptr;
            }
            return ret;
        }

        Result CSubmitManager::ExecuteBatch( CDeviceContext* pCtx, QueuePtr pQueue, CCommandBufferBatch** ppInOut )
        {
            VKE_ASSERT( ppInOut != nullptr, "" );
            CCommandBufferBatch* pBatch = *ppInOut;
            VKE_ASSERT( pBatch != nullptr, "" );
            VKE_ASSERT( pBatch->CanSubmit(), "" );
            Threads::ScopedLock l( pBatch->m_SyncObj );
            return _Submit( pCtx, pQueue, pBatch );
        }

        CCommandBufferBatch* CSubmitManager::FlushCurrentBatch( CDeviceContext* pCtx, const handle_t& hCmdPool )
        {
            CCommandBufferBatch* pTmp = GetCurrentBatch( pCtx, hCmdPool );
            Threads::SyncObject l( m_CurrentBatchSyncObj );
            m_pCurrBatch = nullptr;
            return pTmp;
        }

        void CSubmitManager::SetWaitOnSemaphore( const DDISemaphore& hSemaphore )
        {
            m_hDDIWaitSemaphore = hSemaphore;
        }
   
    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER