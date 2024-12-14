#pragma once
#if VKE_VULKAN_RENDER_SYSTEM
#include "Core/VKECommon.h"
#include "Core/Utils/TCSmartPtr.h"
#include "Core/Resources/TCManager.h"
#include "Core/Memory/CFreeList.h"
#include "RenderSystem/Vulkan/CCommandBuffer.h"
#include "RenderSystem/CDDI.h"

#define VKE_DUMP_CB 1

namespace VKE
{
    namespace Vulkan
    {
        class CDeviceWrapper;
    }

    namespace RenderSystem
    {
        class CCommandBuffer;
        class CDeviceContext;

        struct SCommandBuffer
        {
            DDICommandBuffer    handle = DDI_NULL_HANDLE;
            uint32_t            refCount = 0;
        };

        struct SCommandPoolDesc
        {
            uint32_t commandBufferCount = 0;
        };

        struct SCommandBufferManagerDesc
        {
        };

        

        class VKE_API CCommandBufferManager// : public Core::TCManager< CCommandBuffer >
        {            
            friend class CDeviceContext;
            friend class CGraphicsContext;
            friend class CRenderQueue;
            friend class CCommandBuffer;

            static const uint32_t DEFAULT_COMMAND_BUFFER_COUNT = 64;
            static const uint32_t MAX_THREAD_COUNT = 32;

            using DDICommandBufferVec = Utils::TCDynamicArray< DDICommandBuffer, DEFAULT_COMMAND_BUFFER_COUNT >;
            using CommandBufferVec = Utils::TCDynamicArray< CCommandBuffer, DEFAULT_COMMAND_BUFFER_COUNT >;
            using CommandBufferPtrVec = Utils::TCDynamicArray< CCommandBuffer*, DEFAULT_COMMAND_BUFFER_COUNT >;
            using UintVec = Utils::TCDynamicArray< uint32_t, DEFAULT_COMMAND_BUFFER_COUNT >;

            struct SCommandPool
            {
                CommandBufferVec        vCommandBuffers;
                CommandBufferPtrVec     vpFreeCommandBuffers;
                DDICommandBufferVec     vDDICommandBuffers;
                Threads::SyncObject     SyncObj;
                DDICommandBufferPool    hDDIPool = DDI_NULL_HANDLE;
                uint32_t                handle = INVALID_HANDLE;
            };

            using CommandPoolArray = Utils::TCDynamicArray< SCommandPool* >;

            public:

                CCommandBufferManager(CContextBase*);
                ~CCommandBufferManager();

                Result Create(const SCommandBufferManagerDesc& Desc);
                void Destroy();

                handle_t CreatePool(const SCommandBufferPoolDesc& Desc);
                handle_t GetPool() { return _GetPool< true >()->handle; }
                void DestroyPool(const handle_t& hPool);
                void FreeCommandBuffers(const handle_t& hPool);
                
                template<bool ThreadSafe>
                void FreeCommandBuffers( uint32_t count, CCommandBuffer** ppArray );
                
                template<bool ThreadSafe>
                Result CreateCommandBuffers( uint32_t count, CCommandBuffer** ppArray );

                template<bool ThreadSafe>
                Result CreateCommandBuffers( uint32_t count, uint8_t threadIndex, CCommandBuffer** ppArray );

                template<bool ThreadSafe> VkCommandBuffer GetNextCommandBuffer();

                bool GetCommandBuffer( CCommandBuffer** );

                //Result EndCommandBuffer( EXECUTE_COMMAND_BUFFER_FLAGS, DDISemaphore* );
                Result EndCommandBuffer( EXECUTE_COMMAND_BUFFER_FLAGS, DDISemaphore*, CCommandBuffer** );

            protected:

                vke_force_inline
                SCommandPool* _GetPool(const handle_t hPool)
                {
                    SCommandBufferPoolHandleDecoder Decoder = { (uint32_t)hPool };
                    return m_avpPools[Decoder.Decode.threadId][ Decoder.Decode.index ];
                }

                template<bool Create>
                SCommandPool* _GetPool()
                {
                    return _GetPool< Create >( _GetThreadId() );
                }

                template<bool Create> SCommandPool* _GetPool( uint8_t threadIndex );

                uint8_t _GetThreadId() const
                {
                    auto tid = Platform::ThisThread::GetID() % MAX_THREAD_COUNT;
                    return (uint8_t)tid;
                }

                template<typename... ArgsT>
                void _LogCommand(CCommandBuffer*, cstr_t pFmt, ArgsT&&...);

                void _DestroyPool( SCommandPool** ppPool );
                CCommandBuffer*     _GetNextCommandBuffer(SCommandPool* pPool);
                void                _FreeCommandBuffers(uint32_t count, CCommandBuffer** pArray);
                Result              _CreateCommandBuffers( uint32_t count, SCommandPool* pPool, CCommandBuffer** ppOut );
                Result              _FindFirstFreeCommandBuffer( SCommandPool* pPool, CCommandBuffer** ppOut );

            protected:

                SCommandBufferManagerDesc       m_Desc;
                CContextBase*                   m_pCtx = nullptr;
                CommandPoolArray                m_avpPools[MAX_THREAD_COUNT];
                CCommandBuffer*                 m_apCurrentCommandBuffers[ MAX_THREAD_COUNT ];
        #if VKE_DUMP_CB
                FILE* m_pFile = nullptr;
        #endif
        };

        template<bool Create>
        CCommandBufferManager::SCommandPool* CCommandBufferManager::_GetPool( uint8_t threadIndex )
        {
            SCommandPool* pPool = nullptr;
            if( !m_avpPools[ threadIndex ].IsEmpty() )
            {
                pPool = m_avpPools[ threadIndex ].Back();
            }
            else if constexpr( Create )
            {
                SCommandBufferPoolDesc Desc;
                Desc.commandBufferCount = DEFAULT_COMMAND_BUFFER_COUNT;
                Desc.pContext = m_pCtx;
                Desc.threadIndex = threadIndex;
                CreatePool( Desc );
                return _GetPool<false>( threadIndex );
            }
            VKE_ASSERT2( pPool, "" );
            return pPool;
        }

        template<bool ThreadSafe>
        VkCommandBuffer CCommandBufferManager::GetNextCommandBuffer()
        {
            SCommandPool* pPool = _GetPool< true >();
            if( ThreadSafe )
            {
                pPool->SyncObj.Lock();
            }
            VkCommandBuffer vkCb = _GetNextCommandBuffer(pPool);
            if( ThreadSafe )
            {
                pPool->SyncObj.Unlock();
            }
            return vkCb;
        }

        template<bool ThreadSafe>
        void CCommandBufferManager::FreeCommandBuffers( uint32_t count, CCommandBuffer** pArray )
        {
            _FreeCommandBuffers(count, pArray);
        }

        template<bool ThreadSafe>
        Result CCommandBufferManager::CreateCommandBuffers( uint32_t count, CCommandBuffer** ppArray )
        {
            auto pPool = _GetPool<true>();
            if( ThreadSafe )
            {
                pPool->SyncObj.Lock();
            }
            Result ret = _CreateCommandBuffers( count, pPool, ppArray );
            if( ThreadSafe )
            {
                pPool->SyncObj.Unlock();
            }
            return ret;
        }

        template<bool ThreadSafe>
        Result CCommandBufferManager::CreateCommandBuffers( uint32_t count, uint8_t threadIndex,
            CCommandBuffer** ppArray )
        {
            auto pPool = _GetPool<true>( threadIndex );
            if( ThreadSafe )
            {
                pPool->SyncObj.Lock();
            }
            Result ret = _CreateCommandBuffers( count, pPool, ppArray );
            if( ThreadSafe )
            {
                pPool->SyncObj.Unlock();
            }
            return ret;
        }

        template<typename... _ArgsT>
        void CCommandBufferManager::_LogCommand( CCommandBuffer* pCmdBuffer,
            cstr_t pFmt, _ArgsT&&... args )
        {
#if VKE_DUMP_CB
            if( m_pFile != nullptr )
            {
                fprintf_s( m_pFile, "[%d][%p][%s]: ",
                    Platform::ThisThread::GetID(), (void*)pCmdBuffer->GetDDIObject(),
                    pCmdBuffer->GetDebugName() );
                fprintf_s( m_pFile, pFmt, std::forward<_ArgsT>( args )... );
                fprintf_s( m_pFile, "\n" );
                fflush( m_pFile );
            }
#endif
        }
    } // RenderSystem
} // vke
#endif // VKE_VULKAN_RENDER_SYSTEM