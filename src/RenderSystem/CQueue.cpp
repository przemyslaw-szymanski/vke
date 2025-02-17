#include "RenderSystem/CQueue.h"
#include "RenderSystem/CDeviceContext.h"
#include "RenderSystem/CSwapChain.h"
#include "RenderSystem/Vulkan/Managers/CSubmitManager.h"

#define VKE_EXECUTE_DEBUG_ENABLE 0

namespace VKE
{
    namespace RenderSystem
    {
        CQueue::CQueue()
        {}

        CQueue::~CQueue()
        {
            //Memory::DestroyObject( &HeapAllocator, &m_pSubmitMgr );
            //m_pSubmitMgr = nullptr;
        }

        Result CQueue::Init( const SQueueInitInfo& Info )
        {
            Result ret = VKE_OK;
            VKE_ASSERT2( Info.pContext != nullptr, "Device context must be initialized." );
            m_Desc = Info;
            m_PresentData.hQueue = Info.hDDIQueue;
            m_familyIndex = Info.familyIndex;
            m_type = Info.type;
            m_pCtx = Info.pContext;
            //VKE_ASSERT2( m_pSubmitMgr == nullptr, "" );
            //m_pSubmitMgr = nullptr;
            return ret;
        }

        void CQueue::Wait()
        {
            VKE_ASSERT2( m_pCtx != nullptr, "Device context must be initialized." );
            m_pCtx->NativeAPI().WaitForQueue( m_PresentData.hQueue );
        }

        Result CQueue::Wait( NativeAPI::CPUFence hFence )
        {
            return m_pCtx->NativeAPI().WaitForFences( hFence, UINT64_MAX );
        }

        Result CQueue::Execute( const SSubmitInfo& Info )
        {
            VKE_ASSERT2( m_pCtx != nullptr, "Device context must be initialized." );
            {
#if VKE_EXECUTE_DEBUG_ENABLE
                VKE_LOGGER_LOG_BEGIN;
                VKE_LOGGER << m_Desc.GetDebugName() << "\n\tsignal gpu fences [" << Info.signalSemaphoreCount << "]:";
                for( uint32_t i = 0; i < Info.signalSemaphoreCount; ++i )
                {
                    VKE_LOGGER << ( void* )Info.pDDISignalSemaphores[ i ] << ",";
                }
                VKE_LOGGER << "\n\twait for gpu fences [" << Info.waitSemaphoreCount << "]:";
                for( uint32_t i = 0; i < Info.waitSemaphoreCount; ++i )
                {
                    VKE_LOGGER << ( void* )Info.pDDIWaitSemaphores[ i ] << ",";
                }
                VKE_LOGGER_END;
#endif
            }

            Result ret;
            Lock();
            m_submitCount++;
            m_isBusy = true;
            ret = m_pCtx->NativeAPI().Submit( Info );
            m_isBusy = false;
            Unlock();
            return ret;
        }

        Result CQueue::Present( const SPresentInfo& Info )
        {
            VKE_ASSERT2( m_pCtx != nullptr, "Device context must be initialized." );
            Result ret = VKE_ENOTREADY;
            Lock();
            {
                m_PresentData.vImageIndices.PushBack( Info.imageIndex );
                m_PresentData.vSwapchains.PushBack( Info.pSwapChain->GetDDIObject() );
                m_vpSwapChains.PushBack( Info.pSwapChain );
                if( Info.hDDIWaitSemaphore != DDI_NULL_HANDLE )
                {
                    m_PresentData.vWaitSemaphores.PushBack( Info.hDDIWaitSemaphore );
                }
                m_presentCount++;
                m_isPresentDone = false;
#if VKE_EXECUTE_DEBUG_ENABLE
                VKE_LOG( "\n\tWait gpu fence: " << (void*)Info.hDDIWaitSemaphore << "\n\timage index: " << Info.imageIndex );
#endif
                /*VKE_LOG( "m_presentCount = " << m_presentCount << " swapchainRefCount = " <<
                   (uint32_t)GetSwapChainRefCount()
                    << " Present swapchain count = " << m_PresentData.vSwapchains.GetCount() );*/
                //if( static_cast<uint32_t>( GetSwapChainRefCount() ) == m_PresentData.vSwapchains.GetCount() )
                {
                    m_isBusy = true;
                    // const auto pIndices = m_PresentData.vImageIndices.GetData();
                    ret = m_pCtx->NativeAPI().Present( m_PresentData );
                    VKE_ASSERT2( ret != VKE_FAIL, "" );
                    // VKE_LOG( "Present status: " << ret );
                    if( ret != VKE_FAIL )
                    {
                        for( uint32_t i = 0; i < m_vpSwapChains.GetCount(); ++i )
                        {
                            m_vpSwapChains[ i ]->NotifyPresent();
                        }
                    }
                    Reset();
                    m_isPresentDone = true;
                    m_isBusy = false;
                }
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

        void CQueue::SetDebugName(cstr_t pName)
        {
            m_Desc.SetDebugName( pName );
            m_pCtx->NativeAPI().SetQueueDebugName( ( uint64_t )GetDDIObject(), pName );
        }

    } // RenderSystem
} // VKE