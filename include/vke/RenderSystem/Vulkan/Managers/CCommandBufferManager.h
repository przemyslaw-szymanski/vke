#pragma once
#if VKE_VULKAN_RENDERER
#include "Core/VKECommon.h"
#include "Core/Utils/TCSmartPtr.h"
#include "Core/Resources/TCManager.h"
#include "Core/Memory/CFreeList.h"
#include "RenderSystem/Vulkan/CCommandBuffer.h"

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
            VkCommandBuffer vkHandle = VK_NULL_HANDLE;
            uint32_t        refCount = 0;
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
            using VkCommandBufferVec = Utils::TCDynamicArray< VkCommandBuffer, DEFAULT_COMMAND_BUFFER_COUNT >;
            using CommandBufferVec = Utils::TCDynamicArray< CCommandBuffer, DEFAULT_COMMAND_BUFFER_COUNT >;
            using CommandBufferPtrVec = Utils::TCDynamicArray< CommandBufferPtr, DEFAULT_COMMAND_BUFFER_COUNT >;
            using UintVec = Utils::TCDynamicArray< uint32_t, DEFAULT_COMMAND_BUFFER_COUNT >;

            struct SCommandPool
            {
                CommandBufferVec    vCommandBuffers;
                CommandBufferPtrVec vpFreeCommandBuffers;
                VkCommandBufferVec  vVkCommandBuffers;
                Threads::SyncObject m_SyncObj;
                VkCommandPool       vkPool = VK_NULL_HANDLE;
            };

            using CommandPoolArray = Utils::TCDynamicArray< SCommandPool* >;

            public:

                CCommandBufferManager(CDeviceContext*);
                ~CCommandBufferManager();

                Result Create(const SCommandBufferManagerDesc& Desc);
                void Destroy();

                handle_t CreatePool(const SCommandPoolDesc& Desc);
                void DestroyPool(const handle_t& hPool = NULL_HANDLE);
                void FreeCommandBuffers(const handle_t& hPool = NULL_HANDLE);
                
                template<bool ThreadSafe>
                void FreeCommandBuffers(uint32_t count, CommandBufferPtr* ppArray, const handle_t& hPool = NULL_HANDLE);
                
                template<bool ThreadSafe>
                void CreateCommandBuffers(uint32_t count, CommandBufferPtr* ppArray, const handle_t& hPool = NULL_HANDLE);

                template<bool ThreadSafe>
                VkCommandBuffer GetNextCommandBuffer(const handle_t& hPool = NULL_HANDLE);

            protected:

                vke_force_inline
                SCommandPool* _GetPool(const handle_t hPool)
                {
                    return ( hPool ) ? m_vpPools[ hPool ] : m_vpPools[ 1 ];
                }

                CommandBufferPtr    _GetNextCommandBuffer(SCommandPool* pPool);
                Result              _FreeCommandBuffers(uint32_t count, CommandBufferPtr* pArray, SCommandPool* pPool);
                void                _CreateCommandBuffers(uint32_t count, CommandBufferPtr* pArray, SCommandPool* pPool);

            protected:

                SCommandBufferManagerDesc       m_Desc;
                CDeviceContext*                 m_pCtx = nullptr;
                const Vulkan::CDeviceWrapper&   m_VkDevice;
                CommandPoolArray                m_vpPools;
        };

        template<bool ThreadSafe>
        VkCommandBuffer CCommandBufferManager::GetNextCommandBuffer(const handle_t& hPool)
        {
            SCommandPool* pPool = _GetPool(hPool);
            assert(pPool);
            if( ThreadSafe )
            {
                pPool->m_SyncObj.Lock();
            }
            VkCommandBuffer vkCb = _GetNextCommandBuffer(pPool);
            if( ThreadSafe )
            {
                pPool->m_SyncObj.Unlock();
            }
            return vkCb;
        }

        template<bool ThreadSafe>
        void CCommandBufferManager::FreeCommandBuffers(uint32_t count, CommandBufferPtr* pArray, const handle_t& hPool)
        {
            auto pPool = _GetPool(hPool);
            if( ThreadSafe )
            {
                pPool->m_SyncObj.Lock();
            }
            _FreeCommandBuffers(count, pArray, pPool);
            if( ThreadSafe )
            {
                pPool->m_SyncObj.Unlock();
            }
        }

        template<bool ThreadSafe>
        void CCommandBufferManager::CreateCommandBuffers(uint32_t count, CommandBufferPtr* ppArray,
                                                         const handle_t& hPool /* = NULL_HANDLE */)
        {
            auto pPool = _GetPool(hPool);
            if( ThreadSafe )
            {
                pPool->m_SyncObj.Lock();
            }
            _CreateCommandBuffers(count, ppArray, pPool);
            if( ThreadSafe )
            {
                pPool->m_SyncObj.Unlock();
            }
        }

    } // RenderSystem
} // vke
#endif // VKE_VULKAN_RENDERER