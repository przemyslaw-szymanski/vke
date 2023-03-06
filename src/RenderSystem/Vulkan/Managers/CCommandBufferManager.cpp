#include "RenderSystem/Vulkan/Managers/CCommandBufferManager.h"
#if VKE_VULKAN_RENDER_SYSTEM
#include "Core/Utils/CLogger.h"
#include "Core/Memory/Memory.h"

#include "RenderSystem/Vulkan/CVkDeviceWrapper.h"
#include "RenderSystem/CDeviceContext.h"
#include "RenderSystem/CContextBase.h"

namespace VKE
{
    namespace RenderSystem
    {
        CCommandBufferManager::CCommandBufferManager(CContextBase* pCtx) :
            m_pCtx(pCtx)
        {}

        CCommandBufferManager::~CCommandBufferManager()
        {
            Destroy();
        }

        void CCommandBufferManager::Destroy()
        {
            for( uint32_t t = 0; t < MAX_THREAD_COUNT; ++t )
            {
                auto vpPools = m_avpPools[ t ];
                for( uint32_t i = 1; i < vpPools.GetCount(); ++i )
                {
                    _DestroyPool( &vpPools[i] );
                }
                vpPools.Clear();
            }
        }

        Result CCommandBufferManager::Create(const SCommandBufferManagerDesc& Desc)
        {
            Result ret = VKE_FAIL;
            m_Desc = Desc;
            memset( m_apCurrentCommandBuffers, 0, sizeof( m_apCurrentCommandBuffers ) );
            ret = VKE_OK;
            return ret;
        }

        handle_t CCommandBufferManager::CreatePool(const SCommandBufferPoolDesc& Desc)
        {
            handle_t ret = INVALID_HANDLE;
            SCommandPool* pPool;
            VKE_ASSERT2( m_pCtx == Desc.pContext, "" );
            if( VKE_FAILED(Memory::CreateObject(&HeapAllocator, &pPool)) )
            {
                VKE_LOG_ERR("Unable to create command pool object. No memory.");
                return INVALID_HANDLE;
            }

            if( !pPool->vCommandBuffers.Reserve( Desc.commandBufferCount ) )
            {
                VKE_LOG_ERR("Unable to resize vCommandBuffers. No memory.");
                return INVALID_HANDLE;
            }
            if( !pPool->vDDICommandBuffers.Reserve( Desc.commandBufferCount ) )
            {
                VKE_LOG_ERR( "Unable to resize vCommandBuffers. No memory." );
                return INVALID_HANDLE;
            }

            auto tid = _GetThreadId();
            pPool->hDDIPool = m_pCtx->_GetDDI().CreateCommandBufferPool( Desc, nullptr );
            auto idx = m_avpPools[ tid ].PushBack( pPool );
            SCommandBufferPoolHandleDecoder Decoder;
            Decoder.Decode.threadId = tid;
            Decoder.Decode.index = idx;
            pPool->handle = Decoder.value;

            // Create command buffer only in the pool and add them into a free pool
            // vpFreeCommandBuffer will be resized due to PushBacks
            Result res = _CreateCommandBuffers( Desc.commandBufferCount, pPool, nullptr );
            
            if( VKE_SUCCEEDED( res ) )
            {
            }
            else
            {
                m_pCtx->_GetDDI().DestroyCommandBufferPool( &pPool->hDDIPool, nullptr );
            }
            //auto pCbs = &pPool->vCommandBuffers[ 0 ];
            // $TID AllocCmdBuffers: mgr={(void*)this}, pool={(void*)pPool}, cbs={pCbs, 64}
            return ret;
        }

        Result CCommandBufferManager::EndCommandBuffer( EXECUTE_COMMAND_BUFFER_FLAGS flags,
            DDISemaphore* phDDISemaphore, CCommandBuffer** ppInOut )
        {
            auto pCb = *ppInOut;
            //SCommandBufferPoolHandleDecoder Decoder{ (uint32_t)pCb->m_hPool };
            auto Decoder = pCb->m_hPool;
            if( m_apCurrentCommandBuffers[ Decoder.Decode.threadId ] == pCb )
            {
                m_apCurrentCommandBuffers[ Decoder.Decode.threadId ] = nullptr;
            }
            return VKE_OK;
        }

        /*Result CCommandBufferManager::EndCommandBuffer( EXECUTE_COMMAND_BUFFER_FLAGS flags,
            DDISemaphore* phDDISemaphore)
        {
            auto tid = _GetThreadId();
            auto pCb = m_apCurrentCommandBuffers[ tid ];
            VKE_ASSERT2( pCb != nullptr, "" );
            return EndCommandBuffer( flags, phDDISemaphore, &pCb );
        }*/

        bool CCommandBufferManager::GetCommandBuffer( CCommandBuffer** ppOut)
        {
            bool ret = false;
            auto tid = _GetThreadId();
            auto pCurr = m_apCurrentCommandBuffers[ tid ];
            *ppOut = pCurr;
            if( pCurr == nullptr )
            {
                if(VKE_SUCCEEDED( CreateCommandBuffers< false >( 1u, ppOut )))
                {
                    pCurr = *ppOut;
                    pCurr->Begin();
                    m_apCurrentCommandBuffers[ tid ] = pCurr;
                    
                    ret = true;
                }
            }
            return ret;
        }

        void CCommandBufferManager::_DestroyPool( SCommandPool** ppPool )
        {
            auto pPool = *ppPool;
            pPool->vCommandBuffers.ClearFull();
            pPool->vpFreeCommandBuffers.ClearFull();
            m_pCtx->_GetDDI().DestroyCommandBufferPool( &pPool->hDDIPool, nullptr );
            Memory::DestroyObject( &HeapAllocator, &pPool );
        }

        void CCommandBufferManager::DestroyPool(const handle_t& hPool)
        {
            auto pPool = _GetPool(hPool);
            //m_VkDevice.DestroyObject(nullptr, &pPool->m_hPool);
            _DestroyPool( &pPool );
        }

        void CCommandBufferManager::FreeCommandBuffers(const handle_t& hPool)
        {
            auto pPool = _GetPool(hPool);
            // All command buffers must be freed
            VKE_ASSERT2(pPool->vDDICommandBuffers.GetCount() == pPool->vpFreeCommandBuffers.GetCount(),
                "All command buffers must be freed" );
            //const auto& ICD = m_VkDevice.GetICD();
            const auto count = pPool->vCommandBuffers.GetCount();
            SFreeCommandBufferInfo Info;
            Info.hDDIPool = pPool->hDDIPool;
            Info.pDDICommandBuffers = &pPool->vDDICommandBuffers[0];
            Info.count = count;
            m_pCtx->_GetDDI().FreeObjects( Info );
            pPool->vDDICommandBuffers.Clear();
            pPool->vpFreeCommandBuffers.Clear();
            pPool->vCommandBuffers.Clear();
        }

        CCommandBuffer* CCommandBufferManager::_GetNextCommandBuffer(SCommandPool* pPool)
        {
            auto& vpFrees = pPool->vpFreeCommandBuffers;
            CCommandBuffer* pCb;
            if( vpFrees.PopBack( &pCb ) )
            {
                return pCb;
            }
            else
            {
                VKE_LOG_ERR("Max command buffer for pool:" << pPool->hDDIPool << " reached.");
                assert(0 && "Command buffer resize is not supported now.");
                return nullptr;
            }
        }

        void CCommandBufferManager::_FreeCommandBuffers(uint32_t count, CCommandBuffer** ppArray)
        {
            for( uint32_t i = 0; i < count; ++i )
            {
                auto pCb = ppArray[ i ];
                auto pPool = _GetPool( pCb->m_hPool.value );
                auto& vFreeCbs = pPool->vpFreeCommandBuffers;
                ppArray[ i ]->_NotifyExecuted();
                vFreeCbs.PushBack( ppArray[ i ] );
            }
        }

        Result CCommandBufferManager::_CreateCommandBuffers(uint32_t count,
            SCommandPool* pPool, CCommandBuffer** ppOut)
        {
            assert(pPool);
            auto& vFreeCbs = pPool->vpFreeCommandBuffers;
            Result ret = VKE_OK;
            if( vFreeCbs.GetCount() < count )
            {
                Utils::TCDynamicArray< DDICommandBuffer, DEFAULT_COMMAND_BUFFER_COUNT > vTmps( count );

                auto& DDI = m_pCtx->_GetDDI();

                SAllocateCommandBufferInfo Info;
                Info.count = count;
                Info.hDDIPool = pPool->hDDIPool;
                Info.level = CommandBufferLevels::PRIMARY;
                ret = DDI.AllocateObjects( Info, &vTmps[0] );
                if( VKE_SUCCEEDED( ret ) )
                {
                    //SSemaphoreDesc SemaphoreDesc;
                    // $TID CreateCommandBuffers: cbmgr={(void*)this}, pool={pPool->m_hPool}, cbs={vTmps}
                    pPool->vDDICommandBuffers.Append( vTmps.GetCount(), &vTmps[0] );
                    for( uint32_t i = 0; i < count; ++i )
                    {
                        CCommandBuffer Cb;
                        Cb.m_hDDIObject = vTmps[i];
                        Cb.m_hPool.value = pPool->handle;
                        Cb.m_hDDICmdBufferPool = pPool->hDDIPool;
                        Cb.m_pBaseCtx = m_pCtx;
                        pPool->vCommandBuffers.PushBack( Cb );
                        pPool->vpFreeCommandBuffers.PushBack( ( &pPool->vCommandBuffers.Back() ) );
                    }
                    ret = _CreateCommandBuffers( count, pPool, ppOut );
                }
            }

            if( ppOut != nullptr )
            {
                for( uint32_t i = 0; i < count; ++i )
                {
                    vFreeCbs.PopBack( &ppOut[i] );
                    //( *ppOut )->m_pBaseCtx = m_pCtx;
                    VKE_ASSERT2( ( *ppOut )->m_pBaseCtx == m_pCtx, "Command buffer must be created for the same context" );
                }
            }
            return ret;
        }

        Result CCommandBufferManager::_FindFirstFreeCommandBuffer( SCommandPool* pPool, CCommandBuffer** ppOut )
        {
            Result ret = VKE_FAIL;
            auto& vCmdBuffers = pPool->vCommandBuffers;
            for( uint32_t i = 0; i < vCmdBuffers.GetCount(); ++i )
            {
                if( vCmdBuffers[ i ].IsExecuted() )
                {
                    *ppOut = &vCmdBuffers[ i ];
                    ret = VKE_OK;
                    break;
                }
            }
            return ret;
        }

    } // RenderSystem
} // vke
#endif // VKE_VULKAN_RENDER_SYSTEM