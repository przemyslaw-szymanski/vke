#include "RenderSystem/CQueue.h"
#include "RenderSystem/CDeviceContext.h"
#include "RenderSystem/CSwapChain.h"
#include "RenderSystem/Vulkan/Managers/CSubmitManager.h"

namespace VKE
{
    namespace RenderSystem
    {
        CQueue::CQueue()
        {}

        CQueue::~CQueue()
        {
            Memory::DestroyObject( &HeapAllocator, &m_pSubmitMgr );
            m_pSubmitMgr = nullptr;
        }

        Result CQueue::Init( const SQueueInitInfo& Info )
        {
            Result ret = VKE_OK;
            VKE_ASSERT( Info.pContext != nullptr, "Device context must be initialized." );
            m_PresentData.hQueue = Info.hDDIQueue;
            m_familyIndex = Info.familyIndex;
            m_type = Info.type;
            m_pCtx = Info.pContext;
            VKE_ASSERT( m_pSubmitMgr == nullptr, "" );
            m_pSubmitMgr = nullptr;
            return ret;
        }

        void CQueue::Wait()
        {
            VKE_ASSERT( m_pCtx != nullptr, "Device context must be initialized." );
            m_pCtx->DDI().WaitForQueue( m_PresentData.hQueue );
        }

        Result CQueue::Execute( const SSubmitInfo& Info )
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
            Result ret = VKE_ENOTREADY;
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
                ret = m_pCtx->DDI().Present( m_PresentData );
                if( ret == VKE_OK )
                {
                    for( uint32_t i = 0; i < m_vpSwapChains.GetCount(); ++i )
                    {
                        m_vpSwapChains[i]->NotifyPresent();
                    }
                }
                Reset();
                m_isPresentDone = true;
                m_isBusy = false;
            }
            Unlock();
            return ret;
        }

        void CQueue::Reset()
        {
            m_PresentData.vImageIndices.Clear();
            m_PresentData.vSwapchains.Clear();
            m_PresentData.vWaitSemaphores.Clear();
            m_vpSwapChains.Clear();
            m_submitCount = 0;
        }

        Result CQueue::_CreateSubmitManager( const struct SSubmitManagerDesc* pDesc )
        {
            Result ret = VKE_OK;
            Threads::ScopedLock l( m_SyncObj );
            if( m_pSubmitMgr == nullptr )
            {
                Memory::CreateObject( &HeapAllocator, &m_pSubmitMgr );
                ret = m_pSubmitMgr->Create( *pDesc );
            }
            return ret;
        }

    } // RenderSystem
} // VKE