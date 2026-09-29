//#include "RenderSystem/Managers/CSubmitManager.h"
//#include "RenderSystem/CDeviceContext.h"
//#include "RenderSystem/CCommandBuffer.h"
//
//namespace VKE
//{
//    namespace RenderSystem
//    {
//        void CCommandBufferBatch::operator=( const CCommandBufferBatch& Other )
//        {
//            m_hRHIFence = Other.m_hRHIFence;
//
//            m_vRHIWaitSemaphores  = Other.m_vRHIWaitSemaphores;
//            m_hRHISignalSemaphore = Other.m_hRHISignalSemaphore;
//            m_vpCommandBuffers    = Other.m_vpCommandBuffers;
//            m_vRHICommandBuffers  = Other.m_vRHICommandBuffers;
//
//            m_pMgr          = Other.m_pMgr;
//            m_currCmdBuffer = Other.m_currCmdBuffer;
//
//            m_submitted = Other.m_submitted;
//        }
//
//        void CCommandBufferBatch::operator=( CCommandBufferBatch&& Other )
//        {
//            m_hRHIFence = Other.m_hRHIFence;
//
//            m_vRHIWaitSemaphores  = std::move( Other.m_vRHIWaitSemaphores );
//            m_hRHISignalSemaphore = Other.m_hRHISignalSemaphore;
//            m_vpCommandBuffers    = std::move( Other.m_vpCommandBuffers );
//            m_vRHICommandBuffers  = std::move( Other.m_vRHICommandBuffers );
//
//            m_pMgr          = Other.m_pMgr;
//            m_currCmdBuffer = Other.m_currCmdBuffer;
//
//            m_submitted = Other.m_submitted;
//        }
//
//        bool CCommandBufferBatch::CanSubmit() const
//        {
//            return m_submitted == false;
//        }
//
//        Result CCommandBufferBatch::_Submit( CCommandBuffer* pCb )
//        {
//            m_vRHICommandBuffers.PushBack( pCb->GetRHIObject() );
//            // pCb->_SetCPUSyncObject( m_hRHIFence );
//            m_vpCommandBuffers.PushBack( pCb );
//            for( uint32_t i = 0; i < pCb->m_vRHIWaitOnSemaphores.GetCount(); ++i )
//            {
//                m_vRHIWaitSemaphores.PushBack( pCb->m_vRHIWaitOnSemaphores[ i ] );
//            }
//            return VKE_OK;
//        }
//
//        void CCommandBufferBatch::_Clear()
//        {
//            m_vpCommandBuffers.Clear();
//            m_vRHICommandBuffers.Clear();
//            m_vRHIWaitSemaphores.Clear();
//            m_submitted     = false;
//            m_currCmdBuffer = 0;
//        }
//
//        CSubmitManager::CSubmitManager()
//        {
//        }
//
//        CSubmitManager::~CSubmitManager()
//        {
//        }
//
//        void CSubmitManager::Destroy( CDeviceContext* pCtx )
//        {
//            auto& RHI = pCtx->RHI();
//            for( uint32_t i = 0; i < m_CommandBufferBatches.vSubmits.GetCount(); ++i )
//            {
//                RHI.DestroyFence( &m_CommandBufferBatches.vSubmits[ i ].m_hRHIFence, nullptr );
//                RHI.DestroySemaphore( &m_CommandBufferBatches.vSubmits[ i ].m_hRHISignalSemaphore, nullptr );
//                // RHI.DestroyObject( &m_Submits.vSubmits[i].m_hRHISignalSemaphore, nullptr );
//                // m_pCtx->_DestroyFence(&m_Submits.vSubmits[ i ].m_hRHIFence);
//                // m_pCtx->_DestroySemaphore(&m_Submits.vSubmits[ i ].m_hRHISignalSemaphore);
//            }
//            m_CommandBufferBatches.vSubmits.Clear();
//        }
//
//        void CSubmitManager::_CreateSubmits( CContextBase* pCtx, uint32_t count )
//        {
//            SFenceDesc FenceDesc;
//            FenceDesc.isSignaled = false;
//            SSemaphoreDesc SemaphoreDesc;
//
//            for( uint32_t i = count; i-- > 0; )
//            {
//                CCommandBufferBatch Tmp;
//                Tmp.m_pMgr      = this;
//                Tmp.m_hRHIFence = pCtx->GetDeviceContext()->RHI().CreateFence( FenceDesc, nullptr );
//                // pCtx->RHI().Reset( &Tmp.m_hRHIFence );
//                Tmp.m_hRHISignalSemaphore =
//                    pCtx->GetDeviceContext()->RHI().CreateSemaphore( SemaphoreDesc, nullptr );
//                m_CommandBufferBatches.vSubmits.PushBack( Tmp );
//            }
//        }
//
//        Result CSubmitManager::Create( const SSubmitManagerDesc& Desc )
//        {
//            _CreateSubmits( Desc.pCtx, SUBMIT_COUNT );
//            return VKE_OK;
//        }
//
//        CCommandBufferBatch* CSubmitManager::_GetSubmit( CContextBase* pCtx, const handle_t& hCmdPool, uint32_t idx )
//        {
//            CCommandBufferBatch* pBatch = &m_CommandBufferBatches.vSubmits[ idx ];
//            auto&                RHI    = pCtx->GetDeviceContext()->RHI();
//            if( RHI.IsSignaled( pBatch->m_hRHIFence ) )
//            {
//                RHI.Reset( &pBatch->m_hRHIFence );
//                _FreeCommandBuffers( pCtx, hCmdPool, pBatch );
//                return pBatch;
//            }
//            return nullptr;
//        }
//
//        CCommandBufferBatch* CSubmitManager::_GetNextSubmitReadySubmitFirst( CContextBase*   pCtx,
//                                                                             const handle_t& hCmdPool )
//        {
//            // Get first submit
//            CCommandBufferBatch* pBatch = nullptr;
//            auto&                RHI    = pCtx->GetDeviceContext()->RHI();
//            // If there are any submitts
//            if( !m_CommandBufferBatches.qpSubmitted.IsEmpty() )
//            {
//                pBatch = m_CommandBufferBatches.qpSubmitted.Front();
//                // Check if oldest submit is ready
//                if( RHI.IsSignaled( pBatch->m_hRHIFence ) )
//                {
//                    m_CommandBufferBatches.qpSubmitted.PopFrontFast( &pBatch );
//                    RHI.Reset( &pBatch->m_hRHIFence );
//                    if( !pBatch->m_vRHICommandBuffers.IsEmpty() && !pBatch->m_vpCommandBuffers.IsEmpty() )
//                    {
//                        _FreeCommandBuffers( pCtx, hCmdPool, pBatch );
//                    }
//                    return pBatch;
//                }
//            }
//            // If the oldest submit is not ready get next one
//            {
//                // If there are no submitted cmd buffers
//                auto& idx = m_CommandBufferBatches.currSubmitIdx;
//                if( idx < m_CommandBufferBatches.vSubmits.GetCount() )
//                {
//                    pBatch = &m_CommandBufferBatches.vSubmits[ idx++ ];
//                    // return pBatch;
//                }
//                else
//                {
//                    // ... or create new ones if no one left in the buffer
//                    _CreateSubmits( pCtx, SUBMIT_COUNT );
//                    pBatch = _GetNextSubmitReadySubmitFirst( pCtx, hCmdPool );
//                }
//            }
//            return pBatch;
//        }
//
//        CCommandBufferBatch* CSubmitManager::_GetNextSubmitFreeSubmitFirst( CContextBase*   pCtx,
//                                                                            const handle_t& hCmdPool )
//        {
//            // Get first submit
//            CCommandBufferBatch* pBatch = nullptr;
//            // Get next submit from the pool
//            auto& idx = m_CommandBufferBatches.currSubmitIdx;
//            if( idx < m_CommandBufferBatches.vSubmits.GetCount() )
//            {
//                pBatch = &m_CommandBufferBatches.vSubmits[ idx++ ];
//                assert( pBatch );
//                return pBatch;
//            }
//
//            // If there are any submitts in the pool try to get the oldest submitted if ready
//            if( !m_CommandBufferBatches.qpSubmitted.IsEmpty() )
//            {
//                pBatch    = m_CommandBufferBatches.qpSubmitted.Front();
//                auto& RHI = pCtx->GetDeviceContext()->RHI();
//                // Check if oldest submit is ready
//                if( RHI.IsSignaled( pBatch->m_hRHIFence ) )
//                {
//                    m_CommandBufferBatches.qpSubmitted.PopFrontFast( &pBatch );
//                    RHI.Reset( &pBatch->m_hRHIFence );
//                    if( !pBatch->m_vRHICommandBuffers.IsEmpty() && !pBatch->m_vpCommandBuffers.IsEmpty() )
//                    {
//                        _FreeCommandBuffers( pCtx, hCmdPool, pBatch );
//                    }
//                    return pBatch;
//                }
//            }
//            // If the oldest submit is not ready create a new one
//            _CreateSubmits( pCtx, SUBMIT_COUNT );
//            pBatch = _GetNextSubmitFreeSubmitFirst( pCtx, hCmdPool );
//
//            return pBatch;
//        }
//
//        /*CCommandBufferBatch* CSubmitManager::_GetNextBatch( CDeviceContext* pCtx, const handle_t& hCmdPool )
//        {
//            CCommandBufferBatch* pBatch = nullptr;
//            {
//                pBatch = _GetNextSubmitReadySubmitFirst( pCtx, hCmdPool );
//                assert(pBatch);
//            }
//            assert(pBatch && "No free submit batch left");
//            pBatch->_Clear();
//
//            pBatch->m_submitted = false;
//
//            return pBatch;
//        }*/
//
//        CCommandBufferBatch* CSubmitManager::_GetCurrentBatch( CContextBase* pCtx, const handle_t& hCmdPool )
//        {
//            if( m_pCurrBatch == nullptr )
//            {
//                m_pCurrBatch = _GetNextBatch< NextSubmitBatchAlgorithms::FIRST_READY >( pCtx, hCmdPool );
//            }
//            return m_pCurrBatch;
//        }
//
//        void CSubmitManager::_FreeCommandBuffers( CContextBase* pCtx, const handle_t& hPool,
//                                                  CCommandBufferBatch* pBatch )
//        {
//            auto& vCmdBuffers = pBatch->m_vpCommandBuffers;
//            // VKE_ASSERT2( hPool != INVALID_HANDLE, "CommandBufferPool handle must be valid." );
//            VKE_ASSERT2( vCmdBuffers.IsEmpty() == false, "" );
//            pCtx->_FreeCommandBuffers( vCmdBuffers.GetCount(), &vCmdBuffers[ 0 ] );
//            vCmdBuffers.Clear();
//        }
//
//        void CSubmitManager::_FreeBatch( CContextBase* pCtx, const handle_t& hCmdPool, CCommandBufferBatch** ppInOut )
//        {
//            _FreeCommandBuffers( pCtx, hCmdPool, *ppInOut );
//        }
//
//        Result CSubmitManager::_Submit( CContextBase* pCtx, const handle_t& hCmdPool, CCommandBuffer* pCb )
//        {
//            return _GetCurrentBatch( pCtx, hCmdPool )->_Submit( pCb );
//        }
//
//        Result CSubmitManager::_Submit( CContextBase* pCtx, QueuePtr pQueue, CCommandBufferBatch* pBatch )
//        {
//            RHI::GPUFence  hRHISignal          = RHI::Null;
//            uint32_t             signalCount         = 0;
//            uint32_t             waitCount           = 0;
//            RHI::GPUFence* phRHIWaitSemaphores = nullptr;
//
//            if( m_signalSemaphore )
//            {
//                signalCount = 1;
//                hRHISignal  = pBatch->m_hRHISignalSemaphore;
//            }
//
//            if( m_waitForSemaphores )
//            {
//                // pCtx->GetDeviceContext()->_GetSignaledSemaphores( &pBatch->m_vRHIWaitSemaphores );
//                waitCount           = pBatch->m_vRHIWaitSemaphores.GetCount();
//                phRHIWaitSemaphores = pBatch->m_vRHIWaitSemaphores.GetData();
//            }
//
//            SSubmitInfo Info;
//            Info.commandBufferCount   = static_cast< uint8_t >( pBatch->m_vRHICommandBuffers.GetCount() );
//            Info.pRHICommandBuffers   = pBatch->m_vRHICommandBuffers.GetData();
//            Info.hRHIFence            = pBatch->m_hRHIFence;
//            Info.signalSemaphoreCount = static_cast< uint8_t >( signalCount );
//            Info.pRHISignalSemaphores = &hRHISignal;
//            Info.waitSemaphoreCount   = static_cast< uint8_t >( waitCount );
//            Info.pRHIWaitSemaphores   = phRHIWaitSemaphores;
//            Info.hRHIQueue            = pQueue->GetRHIObject();
//
//#if 0
//            for(uint32_t i = 0; i < Info.commandBufferCount; ++i)
//            {
//                VKE_LOG( "Execute: " << Info.pRHICommandBuffers[ i ] );
//            }
//#endif
//
//            m_signalSemaphore = true; // reset signaling flag
//            Result ret        = pQueue->Execute( Info );
//            if( VKE_SUCCEEDED( ret ) )
//            {
//                pBatch->m_submitted = true;
//                // $TID _Submit: cb={si.pCommandBuffers[0]} ss={si.pSignalSemaphores[0]}, ws={si.pWaitSemaphores[0]}
//                // auto c = m_Submits.qpSubmitted.GetCount();
//
//                m_CommandBufferBatches.qpSubmitted.PushBack( pBatch );
//            }
//            else
//            {
//                ///@TODO handle if submit not succeedeed
//                VKE_ASSERT2( ret == VKE_OK, "SUBMIT NOT SUCCEEDEED. NOT HANDLED." );
//            }
//            return ret;
//        }
//
//        Result CSubmitManager::WaitForBatch( CContextBase* pCtx, const uint64_t& timeout, CCommandBufferBatch* pBatch )
//        {
//            return pCtx->GetDeviceContext()->RHI().WaitForFences( pBatch->m_hRHIFence, timeout );
//        }
//
//        void CSubmitManager::SignalSemaphore( RHI::GPUFence* phRHISemaphoreOut )
//        {
//            if( phRHISemaphoreOut != nullptr )
//            {
//                *phRHISemaphoreOut = m_pCurrBatch->m_hRHISignalSemaphore;
//                m_signalSemaphore  = true;
//            }
//            else
//            {
//                m_signalSemaphore = false;
//            }
//        }
//
//        Result CSubmitManager::ExecuteCurrentBatch( CContextBase* pCtx, QueuePtr pQueue, CCommandBufferBatch** ppOut )
//        {
//            Threads::ScopedLock l( m_CurrentBatchSyncObj );
//            VKE_ASSERT2( m_pCurrBatch != nullptr, "New batch must be set first." );
//            Result ret = VKE_FAIL;
//
//            if( m_pCurrBatch->CanSubmit() )
//            {
//                ret          = _Submit( pCtx, pQueue, m_pCurrBatch );
//                *ppOut       = m_pCurrBatch;
//                m_pCurrBatch = nullptr;
//            }
//            return ret;
//        }
//
//        Result CSubmitManager::ExecuteBatch( CContextBase* pCtx, QueuePtr pQueue, CCommandBufferBatch** ppInOut )
//        {
//            VKE_ASSERT2( ppInOut != nullptr, "" );
//            CCommandBufferBatch* pBatch = *ppInOut;
//            VKE_ASSERT2( pBatch != nullptr, "" );
//            VKE_ASSERT2( pBatch->CanSubmit(), "" );
//            Threads::ScopedLock l( pBatch->m_SyncObj );
//            return _Submit( pCtx, pQueue, pBatch );
//        }
//
//        CCommandBufferBatch* CSubmitManager::FlushCurrentBatch( CContextBase* pCtx, const handle_t& hCmdPool )
//        {
//            CCommandBufferBatch* pTmp = GetCurrentBatch( pCtx, hCmdPool );
//            Threads::SyncObject  l( m_CurrentBatchSyncObj );
//            m_pCurrBatch = nullptr;
//            return pTmp;
//        }
//
//        void CSubmitManager::SetWaitOnSemaphore( const RHI::GPUFence& hSemaphore )
//        {
//            m_hRHIWaitSemaphore = hSemaphore;
//        }
//
//    } // namespace RenderSystem
//} // namespace VKE
