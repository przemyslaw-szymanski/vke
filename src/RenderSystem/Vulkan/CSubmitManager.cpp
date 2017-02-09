#if VKE_VULKAN_RENDERER
#include "RenderSystem/Vulkan/CSubmitManager.h"
#include "RenderSystem/CGraphicsContext.h"

namespace VKE
{
    namespace RenderSystem
    {
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
            auto& VkDevice = m_pCtx->_GetDevice();

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
                Tmp.m_vkFence = m_pCtx->_CreateFence();
                Tmp.m_vkSignalSemaphore = m_pCtx->_CreateSemaphore();
                m_Submits.vSubmits.PushBack(Tmp);
            }
            m_pCurrSubmit = &m_Submits.vSubmits[ 0 ];
        }

        Result CSubmitManager::Create(const SSubmitManagerDesc& Desc)
        {
            m_pQueue = m_pCtx->_GetQueue();
            _CreateSubmits(SUBMIT_COUNT);
            return VKE_OK;
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
            if( m_Submits.currSubmitIdx < m_Submits.vSubmits.GetCount() )
            {
                pSubmit = &m_Submits.vSubmits[ m_Submits.currSubmitIdx++ ];
            }
            else
            {
                // Check if first submit is ready for reuse
                CSubmit& FirstSubmit = m_Submits.vSubmits[ 0 ];
                if( m_pCtx->_GetDevice().IsFenceReady(FirstSubmit.m_vkFence) )
                {
                    m_pCtx->_GetDevice().ResetFences(1, &FirstSubmit.m_vkFence);
                    // Return all command buffers to the pool
                    _FreeCommandBuffers(&FirstSubmit);
                    m_Submits.currSubmitIdx = 0;
                    pSubmit = &FirstSubmit;
                }
                else
                {
                    _CreateSubmits(1);
                    pSubmit = &m_Submits.vSubmits[ m_Submits.currSubmitIdx++ ];
                }
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
            static const VkPipelineStageFlags vkWaitMask = VK_PIPELINE_BIND_POINT_GRAPHICS;
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
        }
   
    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER