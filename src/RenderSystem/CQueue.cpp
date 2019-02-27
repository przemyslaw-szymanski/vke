#include "RenderSystem/CQueue.h"
#include "RenderSystem/CDeviceContext.h"

namespace VKE
{
    namespace RenderSystem
    {
        void CQueue::Wait()
        {
            VKE_ASSERT( m_pCtx != nullptr, "Device context must be initialized." );
            m_pCtx->DDI().WaitForQueue( m_PresentData.hQueue );
        }

        Result CQueue::Submit(const SSubmitInfo& Info )
        {
            VKE_ASSERT( m_pCtx != nullptr, "Device context must be initialized." );
            m_submitCount++;
            return m_pCtx->DDI().Submit( Info );
        }

        Result CQueue::Present( const SPresentInfo& Info )
        {
            VKE_ASSERT( m_pCtx != nullptr, "Device context must be initialized." );
            Result res = VKE_ENOTREADY;
            Lock();
            m_PresentData.vImageIndices.PushBack( Info.imageIndex );
            m_PresentData.vSwapchains.PushBack( Info.hDDISwapChain );
            if( Info.hDDIWaitSemaphore != DDI_NULL_HANDLE )
            {
                m_PresentData.vWaitSemaphores.PushBack( Info.hDDIWaitSemaphore );
            }
            m_presentCount++;
            m_isPresentDone = false;
            if( m_contextRefCount == m_PresentData.vSwapchains.GetCount() )
            {
                /*m_PresentInfo.pImageIndices = &m_PresentData.vImageIndices[0];
                m_PresentInfo.pSwapchains = &m_PresentData.vSwapChains[0];
                m_PresentInfo.pWaitSemaphores = &m_PresentData.vWaitSemaphores[0];
                m_PresentInfo.swapchainCount = m_PresentData.vSwapChains.GetCount();
                m_PresentInfo.waitSemaphoreCount = m_PresentData.vWaitSemaphores.GetCount();
                VK_ERR( ICD.vkQueuePresentKHR( vkQueue, &m_PresentInfo ) );*/
                m_pCtx->DDI().Present( m_PresentData );
                // $TID Present: q={vkQueue}, sc={m_PresentInfo.pSwapchains[0]}, imgIdx={m_PresentInfo.pImageIndices[0]}, ws={m_PresentInfo.pWaitSemaphores[0]}
                m_isPresentDone = true;
                Reset();
                res = VKE_OK;
            }
            Unlock();
            return res;
        }

        void CQueue::Reset()
        {
            m_PresentData.vImageIndices.Clear();
            m_PresentData.vSwapchains.Clear();
            m_PresentData.vWaitSemaphores.Clear();
            m_submitCount = 0;
        }

    } // RenderSystem
} // VKE