#pragma once

#include "RenderSystem/Common.h"
#include "RenderSystem/CDDI.h"

namespace VKE
{
    namespace RenderSystem
    {
        struct SQueueInitInfo
        {
            CDeviceContext* pContext = nullptr;
            DDIQueue        hDDIQueue;
            uint32_t        familyIndex;
            QUEUE_TYPE      type;
        };

        class CQueue final : public Core::CObject
        {
            friend class CDeviceContext;
            friend class CSubmitManager;
            public: 

                friend class RenderSystem::CDeviceContext;

                CQueue() {}

                void Init( const SQueueInitInfo& Info )
                {
                    VKE_ASSERT( Info.pContext != nullptr, "Device context must be initialized." );
                    m_PresentData.hQueue = Info.hDDIQueue;
                    m_familyIndex = Info.familyIndex;
                    m_type = Info.type;
                    m_pCtx = Info.pContext;
                }

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

                QUEUE_TYPE GetType() const { return m_type; }
                bool IsType( QUEUE_TYPE type ) { return (m_type & type) != 0; }

                const DDIQueue& GetDDIObject() const { return m_PresentData.hQueue; }

                void Wait();

                Result Submit(const SSubmitInfo& Info );

                uint32_t GetFamilyIndex() const { return m_familyIndex; }

                Result Present(uint32_t imgIdx, DDISwapChain vkSwpChain,
                    DDISemaphore vkWaitSemaphore );

            private:

                CDeviceContext*     m_pCtx = nullptr;
                SPresentInfo        m_PresentData;
                uint32_t            m_swapChainCount = 0;
                int32_t             m_presentCount = 0;
                uint32_t            m_familyIndex = 0;
                Threads::SyncObject m_SyncObj; // for synchronization if refCount > 1
                bool                m_isPresentDone = false;
                QUEUE_TYPE          m_type;
        };
        using QueuePtr = Utils::TCWeakPtr< CQueue >;
        using QueueRefPtr = Utils::TCObjectSmartPtr< CQueue >;
        using QueueArray = Utils::TCDynamicArray< CQueue, DEFAULT_QUEUE_FAMILY_PROPERTY_COUNT >;
    } // RenderSystem
} // VKE