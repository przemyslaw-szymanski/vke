#pragma once
#if VKE_VULKAN_RENDERER
#include "RenderSystem/Vulkan/CDeviceDriverInterface.h"
#endif // VKE_VULKAN_RENDERER

namespace VKE
{
    namespace RenderSystem
    {
        class CQueue final : public Core::CObject
        {
            friend class CDeviceContext;
            friend class CSubmitManager;
            public:

                friend class RenderSystem::CDeviceContext;

                CQueue();

                void Lock()
                {
                    if( this->GetRefCount() > 1 )
                    {
                        m_SyncObj.Lock();
                    }
                }

                void Unlock()
                {
                    if( m_SyncObj.IsLocked() )
                    {
                        m_SyncObj.Unlock();
                    }
                }

                bool WillNextSwapchainDoPresent() const
                {
                    return m_swapChainCount == m_PresentData.vSwapchains.GetCount() + 1;
                }

                bool IsPresentDone() const { return m_isPresentDone; }
                
                void ReleasePresentNotify()
                {
                    Lock();
                    if( m_presentCount-- < 0 )
                        m_presentCount = 0;
                    m_isPresentDone = m_presentCount == 0;
                    Unlock();
                }

                void Wait( CDDI& DDI )
                {
                    DDI.WaitForQueue( m_PresentData.hQueue );
                }

                Result Submit( CDDI& DDI, const SSubmitInfo& Info )
                {
                    DDI.Submit( Info );
                }

                uint32_t GetFamilyIndex() const { return m_familyIndex; }

                Result Present( CDDI& DDI, uint32_t imgIdx, DDISwapChain vkSwpChain,
                    DDISemaphore vkWaitSemaphore )
                {
                    Result res = VKE_ENOTREADY;
                    Lock();
                    m_PresentData.vImageIndices.PushBack( imgIdx );
                    m_PresentData.vSwapchains.PushBack( vkSwpChain );
                    m_PresentData.vWaitSemaphores.PushBack( vkWaitSemaphore );
                    m_presentCount++;
                    m_isPresentDone = false;
                    if( this->GetRefCount() == m_PresentData.vSwapchains.GetCount() )
                    {
                        /*m_PresentInfo.pImageIndices = &m_PresentData.vImageIndices[0];
                        m_PresentInfo.pSwapchains = &m_PresentData.vSwapChains[0];
                        m_PresentInfo.pWaitSemaphores = &m_PresentData.vWaitSemaphores[0];
                        m_PresentInfo.swapchainCount = m_PresentData.vSwapChains.GetCount();
                        m_PresentInfo.waitSemaphoreCount = m_PresentData.vWaitSemaphores.GetCount();
                        VK_ERR( ICD.vkQueuePresentKHR( vkQueue, &m_PresentInfo ) );*/
                        DDI.Present( m_PresentData );
                        // $TID Present: q={vkQueue}, sc={m_PresentInfo.pSwapchains[0]}, imgIdx={m_PresentInfo.pImageIndices[0]}, ws={m_PresentInfo.pWaitSemaphores[0]}
                        m_isPresentDone = true;
                        m_PresentData.vImageIndices.Clear();
                        m_PresentData.vSwapchains.Clear();
                        m_PresentData.vWaitSemaphores.Clear();
                        res = VKE_OK;
                    }
                    Unlock();
                    return res;
                }

            private:

                SPresentInfo        m_PresentData;
                uint32_t            m_swapChainCount = 0;
                int32_t             m_presentCount = 0;
                uint32_t            m_familyIndex = 0;
                Threads::SyncObject m_SyncObj; // for synchronization if refCount > 1
                bool                m_isPresentDone = false;
        };
        using QueuePtr = Utils::TCWeakPtr< CQueue >;
        using QueueRefPtr = Utils::TCObjectSmartPtr< CQueue >;
    } // RenderSystem
} // VKE