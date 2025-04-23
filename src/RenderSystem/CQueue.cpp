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
            m_pDevice->DestroyGPUFence( &m_hFence );
        }

        Result CQueue::Init( const SQueueInitInfo& Info )
        {
            const char* pTypeNames[ QueueTypes::_MAX_COUNT ] = { "General", "Compute", "Transfer", "Sparse", "Present" };
            Result ret = VKE_OK;
            VKE_ASSERT2( Info.pContext != nullptr, "Device context must be initialized." );
            m_Desc = Info;
            m_PresentData.hQueue = Info.hNativeAPIQueue;
            m_familyIndex = Info.familyIndex;
            m_type = Info.type;
            m_pDevice = Info.pContext;
            SetDebugName( std::format( "{}_{}", pTypeNames[ m_type ], m_familyIndex ).c_str() );
            SGPUFenceDesc Desc;
            Desc.SetDebugName( GetDebugName() );
            m_hFence = m_pDevice->CreateGPUFence( Desc );
            return ret;
        }

        void CQueue::Wait()
        {
            VKE_ASSERT2( m_pDevice != nullptr, "Device context must be initialized." );
            m_pDevice->NativeAPI().WaitForQueue( m_PresentData.hQueue );
        }

        Result CQueue::Wait( NativeAPI::CPUFence hFence )
        {
            return m_pDevice->NativeAPI().WaitForFences( hFence, UINT64_MAX );
        }

        Result CQueue::Execute( const SSubmitInfo& Info )
        {
            VKE_ASSERT2( m_pDevice != nullptr, "Device context must be initialized." );
            {
#if VKE_EXECUTE_DEBUG_ENABLE
                VKE_LOGGER_LOG_BEGIN;
                VKE_LOGGER << m_Desc.GetDebugName() << "\n\tsignal gpu fences [" << Info.signalSemaphoreCount << "]:";
                for( uint32_t i = 0; i < Info.signalSemaphoreCount; ++i )
                {
                    VKE_LOGGER << ( void* )Info.pNativeAPISignalSemaphores[ i ] << ",";
                }
                VKE_LOGGER << "\n\twait for gpu fences [" << Info.waitSemaphoreCount << "]:";
                for( uint32_t i = 0; i < Info.waitSemaphoreCount; ++i )
                {
                    VKE_LOGGER << ( void* )Info.pNativeAPIWaitSemaphores[ i ] << ",";
                }
                VKE_LOGGER_END;
#endif
            }

            Result ret;
            Lock();
            m_submitCount++;
            m_isBusy = true;
            ret = m_pDevice->NativeAPI().Submit( Info );
            m_isBusy = false;
            Unlock();
            return ret;
        }

        Result CQueue::Present( const SPresentInfo& Info )
        {
            VKE_ASSERT2( m_pDevice != nullptr, "Device context must be initialized." );
            Result ret = VKE_ENOTREADY;
            Lock();
            {
                m_PresentData.vImageIndices.PushBack( Info.imageIndex );
                m_PresentData.vSwapchains.PushBack( Info.pSwapChain->GetNativeAPIObject() );
                m_vpSwapChains.PushBack( Info.pSwapChain );
                if( Info.hNativeAPIWaitSemaphore != NativeAPI::Null )
                {
                    m_PresentData.vWaitSemaphores.PushBack( Info.hNativeAPIWaitSemaphore );
                }
                m_presentCount++;
                m_isPresentDone = false;
#if VKE_EXECUTE_DEBUG_ENABLE
                VKE_LOG( "\n\tWait gpu fence: " << (void*)Info.hNativeAPIWaitSemaphore << "\n\timage index: " << Info.imageIndex );
#endif
                /*VKE_LOG( "m_presentCount = " << m_presentCount << " swapchainRefCount = " <<
                   (uint32_t)GetSwapChainRefCount()
                    << " Present swapchain count = " << m_PresentData.vSwapchains.GetCount() );*/
                //if( static_cast<uint32_t>( GetSwapChainRefCount() ) == m_PresentData.vSwapchains.GetCount() )
                {
                    m_isBusy = true;
                    // const auto pIndices = m_PresentData.vImageIndices.GetData();
                    ret = m_pDevice->NativeAPI().Present( m_PresentData );
                    VKE_ASSERT2( ret != VKE_FAIL, "" );
                    // VKE_LOG( "Present status: " << ret );
                    if( ret != VKE_FAIL )
                    {
                        for( uint32_t i = 0; i < m_vpSwapChains.GetCount(); ++i )
                        {
                            //m_vpSwapChains[ i ]->NotifyPresent();
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
            m_pDevice->NativeAPI().SetQueueDebugName( ( uint64_t )GetNativeAPIObject(), pName );
        }

    } // RenderSystem
} // VKE