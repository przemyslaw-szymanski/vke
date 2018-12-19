#if VKE_VULKAN_RENDERER
#include "RenderSystem/Vulkan/Managers/CSubmitManager.h"
#include "RenderSystem/CDeviceContext.h"
#include "RenderSystem/Vulkan/CCommandBuffer.h"

namespace VKE
{
    namespace RenderSystem
    {
        void CSubmit::operator=(const CSubmit& Other)
        {
            m_vCommandBuffers = Other.m_vCommandBuffers;
            m_vDynamicCmdBuffers = Other.m_vDynamicCmdBuffers;
            m_vStaticCmdBuffers = Other.m_vStaticCmdBuffers;
            m_hDDIFence = Other.m_hDDIFence;
            m_hDDIWaitSemaphore = Other.m_hDDIWaitSemaphore;
            m_hDDISignalSemaphore = Other.m_hDDISignalSemaphore;
            m_pMgr = Other.m_pMgr;
            m_currCmdBuffer = Other.m_currCmdBuffer;
            m_submitCount = Other.m_submitCount;
            m_submitted = Other.m_submitted;
        }

        void CSubmit::Submit(CommandBufferPtr pCb)
        {
            pCb->Flush();
            m_vCommandBuffers.PushBack( pCb );
            m_vDynamicCmdBuffers.PushBack( pCb );
            m_vDDICommandBuffers.PushBack( pCb->GetNative() );
            if( m_vCommandBuffers.GetCount() == m_submitCount )
            {
                m_pMgr->_Submit(this);
            }
        }

        void CSubmit::SubmitStatic(CommandBufferPtr pCb)
        {
            m_vCommandBuffers.PushBack( pCb );
            m_vStaticCmdBuffers.PushBack( pCb );
            m_vDDICommandBuffers.PushBack( pCb->GetNative() );
            if( m_vCommandBuffers.GetCount() == m_submitCount )
            {
                m_pMgr->_Submit(this);
            }
        }

        void CSubmit::_Clear()
        {
            m_vCommandBuffers.Clear();
            m_vDynamicCmdBuffers.Clear();
            m_vStaticCmdBuffers.Clear();
            m_vDDICommandBuffers.Clear();
            m_submitCount = 0;
            m_submitted = false;
            m_currCmdBuffer = 0;
            m_hDDIWaitSemaphore = VK_NULL_HANDLE;
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
            for( uint32_t i = 0; i < m_Submits.vSubmits.GetCount(); ++i )
            {
                DDI.DestroyObject( &m_Submits.vSubmits[i].m_hDDIFence );
                DDI.DestroyObject( &m_Submits.vSubmits[i].m_hDDISignalSemaphore );
                //m_pCtx->_DestroyFence(&m_Submits.vSubmits[ i ].m_hDDIFence);
                //m_pCtx->_DestroySemaphore(&m_Submits.vSubmits[ i ].m_hDDISignalSemaphore);
            }
            m_Submits.vSubmits.Clear();
        }

        void CSubmitManager::_CreateSubmits(uint32_t count)
        {
            SFenceDesc FenceDesc;
            FenceDesc.isSignaled = false;
            SSemaphoreDesc SemaphoreDesc;

            for( uint32_t i = count; i-- > 0; )
            {
                CSubmit Tmp;
                Tmp.m_pMgr = this;
                Tmp.m_hDDIFence = m_pCtx->_GetDDI().CreateObject( FenceDesc );
                Tmp.m_hDDISignalSemaphore = m_pCtx->_GetDDI().CreateObject( SemaphoreDesc );
                m_Submits.vSubmits.PushBack( Tmp );
            }
        }

        Result CSubmitManager::Create(const SSubmitManagerDesc& Desc)
        {
            VKE_ASSERT( Desc.pQueue.IsValid(), "Initialize queue." );
            m_pQueue = Desc.pQueue;
            _CreateSubmits(SUBMIT_COUNT);
            m_pCurrSubmit = _GetNextSubmit();
            return VKE_OK;
        }

        CSubmit* CSubmitManager::_GetSubmit(uint32_t idx)
        {
            CSubmit* pSubmit = &m_Submits.vSubmits[idx];
            if( m_pCtx->_GetDDI().IsReady( pSubmit->m_hDDIFence ) )
            {
                m_pCtx->_GetDDI().Reset( &pSubmit->m_hDDIFence );
                _FreeCommandBuffers( pSubmit );
                return pSubmit;
            }
            return nullptr;
        }

        CSubmit* CSubmitManager::_GetNextSubmit()
        {
            return _GetNextSubmitFreeSubmitFirst();
        }

        CSubmit* CSubmitManager::_GetNextSubmitReadySubmitFirst()
        {
            // Get first submit
            CSubmit* pSubmit = nullptr;
            // If there are any submitts
            if( !m_Submits.qpSubmitted.IsEmpty() )
            {
                pSubmit = m_Submits.qpSubmitted.Front();
                // Check if oldest submit is ready
                if( m_pCtx->_GetDDI().IsReady( pSubmit->m_hDDIFence ) )
                {
                    m_Submits.qpSubmitted.PopFrontFast( &pSubmit );
                    m_pCtx->_GetDDI().Reset( &pSubmit->m_hDDIFence );
                    if( !pSubmit->m_vDynamicCmdBuffers.IsEmpty() )
                    {
                        _FreeCommandBuffers(pSubmit);
                    }
                    return pSubmit;
                }
            }
            // If the oldest submit is not ready get next one
            {
                // If there are no submitted cmd buffers
                auto& idx = m_Submits.currSubmitIdx;
                if( idx < m_Submits.vSubmits.GetCount() )
                {
                    pSubmit = &m_Submits.vSubmits[idx++];
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

        CSubmit* CSubmitManager::_GetNextSubmitFreeSubmitFirst()
        {
            // Get first submit
            CSubmit* pSubmit = nullptr;
            // Get next submit from the pool
            auto& idx = m_Submits.currSubmitIdx;
            if( idx < m_Submits.vSubmits.GetCount() )
            {
                pSubmit = &m_Submits.vSubmits[ idx++ ];
                assert(pSubmit);
                return pSubmit;
            }

            // If there are any submitts in the pool try to get the oldest submitted if ready
            if( !m_Submits.qpSubmitted.IsEmpty() )
            {
                pSubmit = m_Submits.qpSubmitted.Front();
                // Check if oldest submit is ready
                if( m_pCtx->_GetDDI().IsReady( pSubmit->m_hDDIFence ) )
                {
                    m_Submits.qpSubmitted.PopFrontFast( &pSubmit );
                    m_pCtx->_GetDDI().Reset( &pSubmit->m_hDDIFence );
                    if( !pSubmit->m_vDynamicCmdBuffers.IsEmpty() )
                    {
                        _FreeCommandBuffers(pSubmit);
                    }
                    return pSubmit;
                }
            }
            // If the oldest submit is not ready create a new one
            _CreateSubmits( SUBMIT_COUNT );
            pSubmit = _GetNextSubmitFreeSubmitFirst();
            return pSubmit;
        }

        CSubmit* CSubmitManager::GetNextSubmit(uint8_t cmdBufferCount, const DDISemaphore& vkWaitSemaphore)
        {
            //assert(m_pCurrSubmit->m_submitted && "Current submit batch should be submitted before acquire a next one");
            CSubmit* pSubmit = nullptr;
            if (!m_pCurrSubmit->m_submitted)
            {
                pSubmit = m_pCurrSubmit;
            }
            else
            {
                pSubmit = _GetNextSubmitFreeSubmitFirst();
                assert(pSubmit);
            }
            assert(pSubmit && "No free submit batch left");
            pSubmit->_Clear();
            pSubmit->m_hDDIWaitSemaphore = vkWaitSemaphore;
            pSubmit->m_submitCount = cmdBufferCount;
            pSubmit->m_submitted = false;
            // $TID GetNextSubmit: pCurr={(void*)m_pCurrSubmit}, pNext={(void*)pSubmit}
            m_pCurrSubmit = pSubmit;
            return m_pCurrSubmit;
        }

        void CSubmitManager::_FreeCommandBuffers(CSubmit* pSubmit)
        {
            auto& vCmdBuffers = pSubmit->m_vDynamicCmdBuffers;
            m_pCtx->_FreeCommandBuffers( vCmdBuffers.GetCount(), &vCmdBuffers[0] );
            vCmdBuffers.Clear();
        }

        void CSubmitManager::_CreateCommandBuffers(CSubmit* pSubmit, uint32_t count)
        {
            pSubmit->m_vDynamicCmdBuffers.Resize( count );
            m_pCtx->_CreateCommandBuffers( count, &pSubmit->m_vCommandBuffers[0] );
            //auto pCb = pSubmit->m_vCommandBuffers[ 0 ];
            // $TID CreateCommandBuffers: pSubmit={(void*)this}, cb={pCb}
        }

        void CSubmitManager::_Submit(CSubmit* pSubmit)
        {
            const auto& DDI = m_pCtx->_GetDDI();
            static const VkPipelineStageFlags vkWaitMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            VkSubmitInfo si;
            Vulkan::InitInfo(&si, VK_STRUCTURE_TYPE_SUBMIT_INFO);
            si.pSignalSemaphores = &pSubmit->m_hDDISignalSemaphore;
            si.signalSemaphoreCount = 1;
            si.pWaitSemaphores = &pSubmit->m_hDDIWaitSemaphore;
            si.waitSemaphoreCount = ( pSubmit->m_hDDIWaitSemaphore != VK_NULL_HANDLE ) ? 1 : 0;
            si.pWaitDstStageMask = &vkWaitMask;
            si.commandBufferCount = pSubmit->m_vCommandBuffers.GetCount();
            si.pCommandBuffers = &pSubmit->m_vDDICommandBuffers[ 0 ];
            //VK_ERR( m_pQueue->Submit( ICD, si, pSubmit->m_hDDIFence ) );
            
            pSubmit->m_submitted = true;
            // $TID _Submit: cb={si.pCommandBuffers[0]} ss={si.pSignalSemaphores[0]}, ws={si.pWaitSemaphores[0]}
            //auto c = m_Submits.qpSubmitted.GetCount();
            m_Submits.qpSubmitted.PushBack( pSubmit );
        }
   
    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER