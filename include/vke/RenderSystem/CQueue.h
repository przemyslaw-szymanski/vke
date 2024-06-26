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
            void*           pSubmitManagerDesc = nullptr;
            DDIQueue        hDDIQueue;
            uint32_t        familyIndex;
            QUEUE_TYPE      type;
            VKE_RENDER_SYSTEM_DEBUG_NAME;
        };

        class CQueue final : public Core::CObject
        {
            friend class CDeviceContext;
            friend class CSubmitManager;
            friend class CContextBase;
            friend class CGraphicsContext;
            friend class CComputeContext;
            friend class CTransferContext;
            
            using SwapChainArray = Utils::TCDynamicArray< CSwapChain*, 8 >;

            public: 

                friend class RenderSystem::CDeviceContext;

                CQueue();
                ~CQueue();

                Result Init( const SQueueInitInfo& Info );

                void Lock()
                {
                    if( GetContextRefCount() > 1 )
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

                /// <summary>
                /// TODO: remove this
                /// </summary>
                /// <returns></returns>
                bool WillNextSwapchainDoPresent() const
                {
                    return m_vpSwapChains.GetCount() == m_PresentData.vSwapchains.GetCount() + 1;
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
                Result Wait( NativeAPI::CPUFence );

                Result Execute( const SSubmitInfo& Info );

                //uint32_t GetSubmitCount() const { return m_submitCount; }

                uint32_t GetFamilyIndex() const { return m_familyIndex; }

                Result Present( const SPresentInfo& Info );

                void Reset();

                bool IsBusy() const { return m_isBusy; }

                int8_t GetContextRefCount() const { return m_contextRefCount; }
                //int8_t GetSwapChainRefCount() const { return m_swapChainRefCount; }

                cstr_t GetDebugName() const
                {
                    return m_Desc.GetDebugName();
                }

                void SetDebugName( cstr_t pName );

            protected:

                //CSubmitManager * _GetSubmitManager() { return m_pSubmitMgr; }

                //Result          _CreateSubmitManager( const struct SSubmitManagerDesc* pDesc );

                void _AddContextRef() { m_contextRefCount++; }
                void _RemoveContextRef() { m_contextRefCount--; VKE_ASSERT2( m_contextRefCount >= 0, "Too many ref removes." ); }

                //void _AddSwapChainRef() { m_swapChainRefCount++; }
                //void _RemoveSwapChainRef() { m_swapChainRefCount--; VKE_ASSERT2( m_swapChainRefCount >= 0, "" ); }

            private:

                SQueueInitInfo      m_Desc;
                CDeviceContext*     m_pCtx = nullptr;
                //CSubmitManager*     m_pSubmitMgr = nullptr;
                SPresentData        m_PresentData;
                SwapChainArray      m_vpSwapChains;
                //uint32_t            m_swapChainCount = 0;
                int32_t             m_presentCount = 0;
                uint32_t            m_submitCount = 0;
                uint32_t            m_familyIndex = 0;
                int8_t              m_contextRefCount = 0; // number of contexts associated with this queue
                //int8_t              m_swapChainRefCount = 0; // number of swapchains to be presented by this queue
                bool                m_isPresentDone = false;
                bool                m_isBusy = false;
                QUEUE_TYPE          m_type;
        };
        using QueuePtr = Utils::TCWeakPtr< CQueue >;
        using QueueRefPtr = Utils::TCObjectSmartPtr< CQueue >;
        using QueueArray = Utils::TCDynamicArray< CQueue, DEFAULT_QUEUE_FAMILY_PROPERTY_COUNT >;
    } // RenderSystem
} // VKE