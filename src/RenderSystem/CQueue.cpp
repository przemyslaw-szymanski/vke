#include "RenderSystem/CQueue.h"
#include "RenderSystem/CDeviceContext.h"
#include "RenderSystem/CSwapChain.h"

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
            Result ret;
            Lock();
            m_submitCount++;
            m_isBusy = true;
            ret = m_pCtx->DDI().Submit( Info );
            m_isBusy = false;
            Unlock();
            return ret;
        }

        Result CQueue::Present( const SPresentInfo& Info )
        {
            VKE_ASSERT( m_pCtx != nullptr, "Device context must be initialized." );
            Result res = VKE_ENOTREADY;
            Lock();
            m_PresentData.vImageIndices.PushBack( Info.imageIndex );
            m_PresentData.vSwapchains.PushBack( Info.pSwapChain->GetDDIObject() );
            m_vpSwapChains.PushBack( Info.pSwapChain );
            if( Info.hDDIWaitSemaphore != DDI_NULL_HANDLE )
            {
                m_PresentData.vWaitSemaphores.PushBack( Info.hDDIWaitSemaphore );
            }
            m_presentCount++;
            m_isPresentDone = false;
            if( GetContextRefCount() == m_PresentData.vSwapchains.GetCount() )
            {
                m_isBusy = true;
                const auto pIndices = m_PresentData.vImageIndices.GetData();
                m_pCtx->DDI().Present( m_PresentData );
                for( uint32_t i = 0; i < m_vpSwapChains.GetCount(); ++i )
                {
                    m_vpSwapChains[i]->NotifyPresent();
                }
                Reset();
                m_isPresentDone = true;
                m_isBusy = false;
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
            m_vpSwapChains.Clear();
            m_submitCount = 0;
        }

    } // RenderSystem
} // VKE