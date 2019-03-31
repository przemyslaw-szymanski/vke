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
            //m_vCommandBuffers = Other.m_vCommandBuffers;
            //m_vDynamicCmdBuffers = Other.m_vDynamicCmdBuffers;
            //m_vStaticCmdBuffers = Other.m_vStaticCmdBuffers;
            m_hDDIFence = Other.m_hDDIFence;
            m_hDDIWaitSemaphore = Other.m_hDDIWaitSemaphore;
            m_hDDISignalSemaphore = Other.m_hDDISignalSemaphore;
            m_vCommandBuffers = Other.m_vCommandBuffers;
            m_vDDICommandBuffers = Other.m_vDDICommandBuffers;
            //m_hDDIWaitSemaphore = Other.m_hDDIWaitSemaphore;
            //m_hDDISignalSemaphore = Other.m_hDDISignalSemaphore;
            m_pMgr = Other.m_pMgr;
            m_currCmdBuffer = Other.m_currCmdBuffer;
            //m_submitCount = Other.m_submitCount;
            m_submitted = Other.m_submitted;
        }

        bool CCommandBufferBatch::CanSubmit() const
        {
            return m_submitted == false;
        }

        void CCommandBufferBatch::_Submit(CommandBufferPtr pCb)
        {
            //m_vCommandBuffers.PushBack( pCb );
            //m_vDynamicCmdBuffers.PushBack( pCb );
            m_vDDICommandBuffers.PushBack( pCb->GetDDIObject() );
            m_vCommandBuffers.PushBack( pCb );
            /*if( m_vCommandBuffers.GetCount() == m_submitCount )
            {
                m_pMgr->_Submit(this);
            }*/
        }

        /*void CCommandBufferBatch::SubmitStatic(CommandBufferPtr pCb)
        {
            m_vCommandBuffers.PushBack( pCb );
            m_vStaticCmdBuffers.PushBack( pCb );
            m_vDDICommandBuffers.PushBack( pCb->GetDDIObject() );
            if( m_vCommandBuffers.GetCount() == m_submitCount )
            {
                m_pMgr->_Submit(this);
            }
        }*/

        void CCommandBufferBatch::_Clear()
        {
            m_vCommandBuffers.Clear();
            //m_vDynamicCmdBuffers.Clear();
            //m_vStaticCmdBuffers.Clear();
            m_vDDICommandBuffers.Clear();
            //m_submitCount = 0;
            m_submitted = false;
            m_currCmdBuffer = 0;
            //m_hDDIWaitSemaphore = VK_NULL_HANDLE;
        }

        CSubmitManager::CSubmitManager(CDeviceContext* pCtx) :
            m_pCtx(pCtx)
        {}

        CSubmitManager::~CSubmitManager()
        {
            Destroy();
        }

        void CSubmitManager::Destroy()
        {
           // auto& VkDevice = m_pCtx->_GetDevice();
            auto& DDI = m_pCtx->_GetDDI();
            for( uint32_t i = 0; i < m_CommandBufferBatches.vSubmits.GetCount(); ++i )
            {
                DDI.DestroyObject( &m_CommandBufferBatches.vSubmits[i].m_hDDIFence, nullptr );
                DDI.DestroyObject( &m_CommandBufferBatches.vSubmits[i].m_hDDISignalSemaphore, nullptr );
                //DDI.DestroyObject( &m_Submits.vSubmits[i].m_hDDISignalSemaphore, nullptr );
                //m_pCtx->_DestroyFence(&m_Submits.vSubmits[ i ].m_hDDIFence);
                //m_pCtx->_DestroySemaphore(&m_Submits.vSubmits[ i ].m_hDDISignalSemaphore);
            }
            m_CommandBufferBatches.vSubmits.Clear();
        }

        void CSubmitManager::_CreateSubmits(uint32_t count)
        {
            SFenceDesc FenceDesc;
            FenceDesc.isSignaled = false;
            SSemaphoreDesc SemaphoreDesc;

            for( uint32_t i = count; i-- > 0; )
            {
                CCommandBufferBatch Tmp;
                Tmp.m_pMgr = this;
                Tmp.m_hDDIFence = m_pCtx->_GetDDI().CreateObject( FenceDesc, nullptr );
                Tmp.m_hDDISignalSemaphore = m_pCtx->_GetDDI().CreateObject( SemaphoreDesc, nullptr );
                m_CommandBufferBatches.vSubmits.PushBack( Tmp );
            }
        }

        Result CSubmitManager::Create(const SSubmitManagerDesc& Desc)
        {
            VKE_ASSERT( Desc.pQueue.IsValid(), "Initialize queue." );
            m_Desc = Desc;
            _CreateSubmits(SUBMIT_COUNT);
            return VKE_OK;
        }

        CCommandBufferBatch* CSubmitManager::_GetSubmit(uint32_t idx)
        {
            CCommandBufferBatch* pSubmit = &m_CommandBufferBatches.vSubmits[idx];
            if( m_pCtx->_GetDDI().IsReady( pSubmit->m_hDDIFence ) )
            {
                m_pCtx->_GetDDI().Reset( &pSubmit->m_hDDIFence );
                _FreeCommandBuffers( pSubmit );
                return pSubmit;
            }
            return nullptr;
        }

        CCommandBufferBatch* CSubmitManager::_GetNextSubmit()
        {
            return _GetNextSubmitFreeSubmitFirst();
        }

        CCommandBufferBatch* CSubmitManager::_GetNextSubmitReadySubmitFirst()
        {
            // Get first submit
            CCommandBufferBatch* pSubmit = nullptr;
            // If there are any submitts
            if( !m_CommandBufferBatches.qpSubmitted.IsEmpty() )
            {
                pSubmit = m_CommandBufferBatches.qpSubmitted.Front();
                // Check if oldest submit is ready
                if( m_pCtx->_GetDDI().IsReady( pSubmit->m_hDDIFence ) )
                {
                    m_CommandBufferBatches.qpSubmitted.PopFrontFast( &pSubmit );
                    m_pCtx->_GetDDI().Reset( &pSubmit->m_hDDIFence );
                    if( !pSubmit->m_vDDICommandBuffers.IsEmpty() )
                    {
                        _FreeCommandBuffers( pSubmit );
                    }
                    return pSubmit;
                }
            }
            // If the oldest submit is not ready get next one
            {
                // If there are no submitted cmd buffers
                auto& idx = m_CommandBufferBatches.currSubmitIdx;
                if( idx < m_CommandBufferBatches.vSubmits.GetCount() )
                {
                    pSubmit = &m_CommandBufferBatches.vSubmits[idx++];
                    //return pSubmit;
                }
                else
                {
                    // ... or create new ones if no one left in the buffer
                    _CreateSubmits( SUBMIT_COUNT );
                    pSubmit = _GetNextSubmitReadySubmitFirst();
                }
            }
            return pSubmit;
        }

        CCommandBufferBatch* CSubmitManager::_GetNextSubmitFreeSubmitFirst()
        {
            // Get first submit
            CCommandBufferBatch* pSubmit = nullptr;
            // Get next submit from the pool
            auto& idx = m_CommandBufferBatches.currSubmitIdx;
            if( idx < m_CommandBufferBatches.vSubmits.GetCount() )
            {
                pSubmit = &m_CommandBufferBatches.vSubmits[ idx++ ];
                assert(pSubmit);
                return pSubmit;
            }

            // If there are any submitts in the pool try to get the oldest submitted if ready
            if( !m_CommandBufferBatches.qpSubmitted.IsEmpty() )
            {
                pSubmit = m_CommandBufferBatches.qpSubmitted.Front();
                // Check if oldest submit is ready
                if( m_pCtx->_GetDDI().IsReady( pSubmit->m_hDDIFence ) )
                {
                    m_CommandBufferBatches.qpSubmitted.PopFrontFast( &pSubmit );
                    m_pCtx->_GetDDI().Reset( &pSubmit->m_hDDIFence );
                    if( !pSubmit->m_vDDICommandBuffers.IsEmpty() )
                    {
                        _FreeCommandBuffers( pSubmit );
                    }
                    return pSubmit;
                }
            }
            // If the oldest submit is not ready create a new one
            _CreateSubmits( SUBMIT_COUNT );
            pSubmit = _GetNextSubmitFreeSubmitFirst();
           
            return pSubmit;
        }

        CCommandBufferBatch* CSubmitManager::_GetNextBatch()
        {
            CCommandBufferBatch* pSubmit = nullptr;
            {
                pSubmit = _GetNextSubmitFreeSubmitFirst();
                assert(pSubmit);
            }
            assert(pSubmit && "No free submit batch left");
            pSubmit->_Clear();
            //pSubmit->m_hDDIWaitSemaphore = vkWaitSemaphore;
            //pSubmit->m_submitCount = cmdBufferCount;
            pSubmit->m_submitted = false;
            pSubmit->m_hDDIWaitSemaphore = m_hDDIWaitSemaphore;
            m_hDDIWaitSemaphore = DDI_NULL_HANDLE;
            // $TID GetNextSubmit: pCurr={(void*)m_pCurrSubmit}, pNext={(void*)pSubmit}
            //m_pCurrBatch = pSubmit;
            return pSubmit;
        }

        CCommandBufferBatch* CSubmitManager::GetCurrentBatch()
        {
            Threads::ScopedLock l( m_CurrentBatchSyncObj );
            if( m_pCurrBatch == nullptr )
            {
                m_pCurrBatch = _GetNextBatch();
            }
            return m_pCurrBatch;
        }

        void CSubmitManager::_FreeCommandBuffers(CCommandBufferBatch* pSubmit)
        {
            auto& vCmdBuffers = pSubmit->m_vCommandBuffers;
            VKE_ASSERT( m_Desc.hCmdBufferPool != 0, "CommandBufferPool handle must be valid." );
            m_pCtx->_FreeCommandBuffers( m_Desc.hCmdBufferPool, vCmdBuffers.GetCount(), &vCmdBuffers[0] );
            vCmdBuffers.Clear();
        }

        //void CSubmitManager::_CreateCommandBuffers(CCommandBufferBatch* pSubmit, uint32_t count)
        //{
        //    pSubmit->m_vDynamicCmdBuffers.Resize( count );
        //    m_pCtx->_CreateCommandBuffers( count, &pSubmit->m_vCommandBuffers[0] );
        //    //auto pCb = pSubmit->m_vCommandBuffers[ 0 ];
        //    // $TID CreateCommandBuffers: pSubmit={(void*)this}, cb={pCb}
        //}

        Result CSubmitManager::_Submit(CCommandBufferBatch* pSubmit)
        {
            /*const auto& DDI = m_pCtx->_GetDDI();
            static const VkPipelineStageFlags vkWaitMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            VkSubmitInfo si = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
            
            if( !pSubmit->m_vDDISignalSemaphores.IsEmpty() )
            {
                si.pSignalSemaphores = &pSubmit->m_vDDISignalSemaphores[0];
                si.signalSemaphoreCount = pSubmit->m_vDDISignalSemaphores.GetCount();
            }
            if( !pSubmit->m_vDDIWaitSemaphores.IsEmpty() )
            {
                si.pWaitSemaphores = &pSubmit->m_vDDIWaitSemaphores[0];
                si.waitSemaphoreCount = pSubmit->m_vDDIWaitSemaphores.GetCount();
            }
            si.pWaitDstStageMask = &vkWaitMask;
            si.commandBufferCount = pSubmit->m_vDDICommandBuffers.GetCount();
            si.pCommandBuffers = &pSubmit->m_vDDICommandBuffers[ 0 ];*/
            //VK_ERR( m_pQueue->Submit( ICD, si, pSubmit->m_hDDIFence ) );

            DDISemaphore hDDISignal = DDI_NULL_HANDLE;
            uint32_t signalCount = 0;
            if( m_signalSemaphore )
            {
                signalCount = 1;
                hDDISignal = pSubmit->m_hDDISignalSemaphore;
            }

            SSubmitInfo Info;
            Info.commandBufferCount = pSubmit->m_vDDICommandBuffers.GetCount();
            Info.pDDICommandBuffers = pSubmit->m_vDDICommandBuffers.GetData();
            Info.hDDIFence = pSubmit->m_hDDIFence;
            Info.signalSemaphoreCount = signalCount;
            Info.pDDISignalSemaphores = &hDDISignal;
            Info.waitSemaphoreCount = pSubmit->m_hDDIWaitSemaphore != DDI_NULL_HANDLE ? 1 : 0;
            Info.pDDIWaitSemaphores = &pSubmit->m_hDDIWaitSemaphore;
            Info.hDDIQueue = m_Desc.pQueue->GetDDIObject();

            m_signalSemaphore = true; // reset signaling flag
            Result ret = m_Desc.pQueue->Submit( Info );
            if( VKE_SUCCEEDED( ret ) )
            {
                pSubmit->m_submitted = true;
                // $TID _Submit: cb={si.pCommandBuffers[0]} ss={si.pSignalSemaphores[0]}, ws={si.pWaitSemaphores[0]}
                //auto c = m_Submits.qpSubmitted.GetCount();

                m_CommandBufferBatches.qpSubmitted.PushBack( pSubmit );
            }
            else
            {
                ///@TODO handle if submit not succeedeed
                VKE_ASSERT( ret == VKE_OK, "SUBMIT NOT SUCCEEDEED. NOT HANDLED." );
            }
            return ret;
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

        Result CSubmitManager::ExecuteCurrentBatch( CCommandBufferBatch** ppOut )
        {
            Threads::ScopedLock l( m_CurrentBatchSyncObj );
            VKE_ASSERT( m_pCurrBatch != nullptr, "New batch must be set first." );
            Result ret = VKE_FAIL;
            if( m_pCurrBatch->CanSubmit() )
            {
                ret = _Submit( m_pCurrBatch );
                *ppOut = m_pCurrBatch;
                m_pCurrBatch = nullptr;
            }
            return ret;
        }

        void CSubmitManager::SetWaitOnSemaphore( const DDISemaphore& hSemaphore )
        {
            m_hDDIWaitSemaphore = hSemaphore;
        }
   
    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER