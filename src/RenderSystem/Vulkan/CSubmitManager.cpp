#if VKE_VULKAN_RENDERER
#include "RenderSystem/Vulkan/CSubmitManager.h"
#include "RenderSystem/CGraphicsContext.h"

namespace VKE
{
    namespace RenderSystem
    {
        void CSubmit::operator=(const CSubmit& Other)
        {
            m_vCommandBuffers = Other.m_vCommandBuffers;
            m_vDynamicCmdBuffers = Other.m_vDynamicCmdBuffers;
            m_vStaticCmdBuffers = Other.m_vStaticCmdBuffers;
            m_vkFence = Other.m_vkFence;
            m_vkWaitSemaphore = Other.m_vkWaitSemaphore;
            m_vkSignalSemaphore = Other.m_vkSignalSemaphore;
            m_pMgr = Other.m_pMgr;
            m_currCmdBuffer = Other.m_currCmdBuffer;
            m_submitCount = Other.m_submitCount;
            m_submitted = Other.m_submitted;
        }

        void CSubmit::Submit(const VkCommandBuffer& vkCb)
        {
            m_vCommandBuffers.PushBack(vkCb);
            m_vDynamicCmdBuffers.PushBack(vkCb);
            if( m_vCommandBuffers.GetCount() == m_submitCount )
            {
                m_pMgr->_Submit(this);
            }
        }

        void CSubmit::SubmitStatic(const VkCommandBuffer& vkCb)
        {
            m_vCommandBuffers.PushBack(vkCb);
            m_vStaticCmdBuffers.PushBack(vkCb);
            if( m_vCommandBuffers.GetCount() == m_submitCount )
            {
                m_pMgr->_Submit(this);
            }
        }

        void CSubmit::_Clear()
        {
            m_vCommandBuffers.FastClear();
            m_vDynamicCmdBuffers.FastClear();
            m_vStaticCmdBuffers.FastClear();
            m_submitCount = 0;
            m_submitted = false;
            m_currCmdBuffer = 0;
            m_vkWaitSemaphore = VK_NULL_HANDLE;
        }

        CSubmitManager::CSubmitManager(CGraphicsContext* pCtx) :
            m_pCtx(pCtx)
        {}

        CSubmitManager::~CSubmitManager()
        {
            Destroy();
        }

        void CSubmitManager::Destroy()
        {
           // auto& VkDevice = m_pCtx->_GetDevice();

            for( uint32_t i = 0; i < m_Submits.vSubmits.GetCount(); ++i )
            {
                m_pCtx->_DestroyFence(&m_Submits.vSubmits[ i ].m_vkFence);
                m_pCtx->_DestroySemaphore(&m_Submits.vSubmits[ i ].m_vkSignalSemaphore);
            }
            m_Submits.vSubmits.FastClear();
        }

        void CSubmitManager::_CreateSubmits(uint32_t count)
        {
            for( uint32_t i = count; i-- > 0; )
            {
                CSubmit Tmp;
                Tmp.m_pMgr = this;
                Tmp.m_vkFence = m_pCtx->_CreateFence(VK_FENCE_CREATE_SIGNALED_BIT);
                Tmp.m_vkSignalSemaphore = m_pCtx->_CreateSemaphore();
                m_Submits.vSubmits.PushBack(Tmp);
            }
        }

        Result CSubmitManager::Create(const SSubmitManagerDesc& Desc)
        {
            m_pQueue = m_pCtx->_GetQueue();
            _CreateSubmits(SUBMIT_COUNT);
            m_pCurrSubmit = _GetNextSubmit();
            return VKE_OK;
        }

        CSubmit* CSubmitManager::_GetSubmit(uint32_t idx)
        {
            CSubmit* pSubmit = &m_Submits.vSubmits[idx];
            if( m_pCtx->_GetDevice().IsFenceReady(pSubmit->m_vkFence) )
            {
                m_pCtx->_GetDevice().ResetFences(1, &pSubmit->m_vkFence);
                _FreeCommandBuffers(pSubmit);
                return pSubmit;
            }
            return nullptr;
        }

        CSubmit* CSubmitManager::_GetNextSubmit()
        {
            // Get first submit
            CSubmit* pSubmit;
            // If there are any submitts
            if( !m_Submits.qpSubmitted.IsEmpty() )
            {
                pSubmit = m_Submits.qpSubmitted.Front();
                // Check if oldest submit is ready
                const auto& Device = m_pCtx->_GetDevice();
                if( Device.IsFenceReady( pSubmit->m_vkFence ) )
                {
                    m_Submits.qpSubmitted.PopFrontFast(&pSubmit);
                    Device.ResetFences(1, &pSubmit->m_vkFence);
                    _FreeCommandBuffers(pSubmit);
                    return pSubmit;
                }
            }
            else
            {
                // If there are no submitted cmd buffers
                auto& idx = m_Submits.currSubmitIdx;
                if( idx < m_Submits.vSubmits.GetCount() )
                {
                    pSubmit = &m_Submits.vSubmits[idx++];
                    return pSubmit;
                }
                else
                {
                    _CreateSubmits(SUBMIT_COUNT);
                    return _GetNextSubmit();
                }
            }
            return nullptr;
        }

        CSubmit* CSubmitManager::GetNextSubmit(uint32_t cmdBufferCount, const VkSemaphore& vkWaitSemaphore)
        {
            //assert(m_pCurrSubmit->m_submitted && "Current submit batch should be submitted before acquire a next one");
            CSubmit* pSubmit = nullptr;
            if (!m_pCurrSubmit->m_submitted)
            {
                pSubmit = m_pCurrSubmit;
            }
            else
            {
                pSubmit = _GetNextSubmit();
                assert(pSubmit);
            }
            assert(pSubmit && "No free submit batch left");
            pSubmit->_Clear();
            pSubmit->m_vkWaitSemaphore = vkWaitSemaphore;
            pSubmit->m_submitCount = cmdBufferCount;
            pSubmit->m_submitted = false;
            m_pCurrSubmit = pSubmit;
            return m_pCurrSubmit;
        }

        void CSubmitManager::_FreeCommandBuffers(CSubmit* pSubmit)
        {
            auto& vCmdBuffers = pSubmit->m_vDynamicCmdBuffers;
            m_pCtx->_FreeCommandBuffers(vCmdBuffers.GetCount(),
                                        &vCmdBuffers[ 0 ]);
            vCmdBuffers.FastClear();
        }

        void CSubmitManager::_CreateCommandBuffers(CSubmit* pSubmit, uint32_t count)
        {
            pSubmit->m_vDynamicCmdBuffers.Resize(count);
            m_pCtx->_CreateCommandBuffers(count, &pSubmit->m_vCommandBuffers[ 0 ]);
        }

        void CSubmitManager::_Submit(CSubmit* pSubmit)
        {
            const auto& ICD = m_pCtx->_GetDevice().GetICD();
            static const VkPipelineStageFlags vkWaitMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            VkSubmitInfo si;
            Vulkan::InitInfo(&si, VK_STRUCTURE_TYPE_SUBMIT_INFO);
            si.pSignalSemaphores = &pSubmit->m_vkSignalSemaphore;
            si.signalSemaphoreCount = 1;
            si.pWaitSemaphores = &pSubmit->m_vkWaitSemaphore;
            si.waitSemaphoreCount = ( pSubmit->m_vkWaitSemaphore != VK_NULL_HANDLE ) ? 1 : 0;
            si.pWaitDstStageMask = &vkWaitMask;
            si.commandBufferCount = pSubmit->m_vCommandBuffers.GetCount();
            si.pCommandBuffers = &pSubmit->m_vCommandBuffers[ 0 ];
            VK_ERR(m_pQueue->Submit(ICD, si, pSubmit->m_vkFence));
            pSubmit->m_submitted = true;

            m_Submits.qpSubmitted.PushBack(pSubmit);
        }
   
    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER