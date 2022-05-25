#pragma once
#if VKE_VULKAN_RENDERER
#include "Core/VKECommon.h"
#include "Core/Utils/TCSmartPtr.h"
#include "Core/Resources/TCManager.h"
#include "Core/Memory/CFreeList.h"
#include "RenderSystem/Vulkan/CCommandBuffer.h"
#include "RenderSystem/CDDI.h"

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

        struct SCommandBufferPoolHandleDecoder
        {
            union
            {
                struct
                {
                    uint32_t threadId : 8;
                    uint32_t index : 24; 
                } Decode;
                uint32_t value;
            };
        };

        class VKE_API CCommandBufferManager// : public Core::TCManager< CCommandBuffer >
        {            
            friend class CDeviceContext;
            friend class CGraphicsContext;
            friend class CRenderQueue;

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

                template<bool ThreadSafe> VkCommandBuffer GetNextCommandBuffer();

                bool GetCommandBuffer( CCommandBuffer** );

                Result EndCommandBuffer( EXECUTE_COMMAND_BUFFER_FLAGS, DDISemaphore* );
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
                    SCommandPool* pPool = nullptr;
                    auto tid = _GetThreadId();
                    if( !m_avpPools[ tid ].IsEmpty() )
                    {
                        pPool = m_avpPools[ tid ].Back();
                    }
                    else if constexpr(Create)
                    {
                        SCommandBufferPoolDesc Desc;
                        Desc.commandBufferCount = DEFAULT_COMMAND_BUFFER_COUNT;
                        Desc.pContext = m_pCtx;
                        CreatePool( Desc );
                        return _GetPool< false >();
                    }
                    VKE_ASSERT( pPool, "" );
                    return pPool;
                }

                uint32_t _GetThreadId() const
                {
                    auto tid = Platform::ThisThread::GetID() % MAX_THREAD_COUNT;
                    return tid;
                }

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
        };

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
            auto pPool = _GetPool< true >();
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

    } // RenderSystem
} // vke
#endif // VKE_VULKAN_RENDERER