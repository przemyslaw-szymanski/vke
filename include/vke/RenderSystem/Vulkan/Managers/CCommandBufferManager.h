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

        class VKE_API CCommandBufferManager : public Resources::TCManager< CCommandBuffer >
        {            
            friend class CDeviceContext;
            friend class CGraphicsContext;
            friend class CRenderQueue;

            static const uint32_t DEFAULT_COMMAND_BUFFER_COUNT = 64;
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
            };

            using CommandPoolArray = Utils::TCDynamicArray< SCommandPool* >;

            public:

                CCommandBufferManager(CDeviceContext*);
                ~CCommandBufferManager();

                Result Create(const SCommandBufferManagerDesc& Desc);
                void Destroy();

                handle_t CreatePool(const SCommandBufferPoolDesc& Desc);
                void DestroyPool(const handle_t& hPool);
                void FreeCommandBuffers(const handle_t& hPool);
                
                template<bool ThreadSafe>
                void FreeCommandBuffers( const handle_t& hPool, uint32_t count, CCommandBuffer** ppArray );
                
                template<bool ThreadSafe>
                Result CreateCommandBuffers( const handle_t& hPool, uint32_t count, CCommandBuffer** ppArray );

                template<bool ThreadSafe>
                VkCommandBuffer GetNextCommandBuffer(const handle_t& hPool);

            protected:

                vke_force_inline
                SCommandPool* _GetPool(const handle_t hPool)
                {
                    return m_vpPools[ hPool ];
                }

                CCommandBuffer*     _GetNextCommandBuffer(SCommandPool* pPool);
                void                _FreeCommandBuffers(uint32_t count, CCommandBuffer** pArray, SCommandPool* pPool);
                Result              _CreateCommandBuffers( uint32_t count, SCommandPool* pPool, CCommandBuffer** ppOut );

            protected:

                SCommandBufferManagerDesc       m_Desc;
                CDeviceContext*                 m_pCtx = nullptr;
                CommandPoolArray                m_vpPools;
        };

        template<bool ThreadSafe>
        VkCommandBuffer CCommandBufferManager::GetNextCommandBuffer(const handle_t& hPool)
        {
            SCommandPool* pPool = _GetPool(hPool);
            assert(pPool);
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
        void CCommandBufferManager::FreeCommandBuffers( const handle_t& hPool, uint32_t count, CCommandBuffer** pArray )
        {
            auto pPool = _GetPool(hPool);
            if( ThreadSafe )
            {
                pPool->SyncObj.Lock();
            }
            _FreeCommandBuffers(count, pArray, pPool);
            if( ThreadSafe )
            {
                pPool->SyncObj.Unlock();
            }
        }

        template<bool ThreadSafe>
        Result CCommandBufferManager::CreateCommandBuffers( const handle_t& hPool, uint32_t count, CCommandBuffer** ppArray )
        {
            auto pPool = _GetPool( hPool );
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