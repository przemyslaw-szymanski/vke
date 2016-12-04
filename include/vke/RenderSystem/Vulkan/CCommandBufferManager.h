#pragma once
#if VKE_VULKAN_RENDERER
#include "Core/VKECommon.h"
#include "Core/Utils/TCSmartPtr.h"
#include "Core/Resources/TCManager.h"
#include "Core/Memory/CFreeList.h"
#include "RenderSystem/Vulkan/CCommandBuffer.h"
#include "RenderSystem/Vulkan/Vulkan.h"

namespace VKE
{
    namespace Vulkan
    {
        class CDeviceWrapper;
    }

    namespace RenderSystem
    {
        class CCommandBuffer;
        class CGraphicsContext;

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

            using CommandBufferArray = Utils::TCDynamicArray< VkCommandBuffer, 64 >;

            struct SCommandPool
            {
                CommandBufferArray  vCommandBuffers;
                CommandBufferArray  vFreeCommandBuffers;
                Threads::SyncObject m_SyncObj;
                VkCommandPool       vkPool = VK_NULL_HANDLE;
            };

            using CommandPoolArray = Utils::TCDynamicArray< SCommandPool* >;

            public:

                CCommandBufferManager(CGraphicsContext*);
                ~CCommandBufferManager();

                Result Create(const SCommandBufferManagerDesc& Desc);
                void Destroy();

                handle_t CreatePool(const SCommandPoolDesc& Desc);
                void DestroyPool(const handle_t& hPool = NULL_HANDLE);
                void FreeCommandBuffers(const handle_t& hPool = NULL_HANDLE);
                template<bool ThreadSafe>
                void FreeCommandBuffer(const VkCommandBuffer& vkCb, const handle_t& hPool = NULL_HANDLE);

                template<bool ThreadSafe>
                const VkCommandBuffer& GetNextCommandBuffer(const handle_t& hPool = NULL_HANDLE);

            protected:

                vke_force_inline
                SCommandPool* _GetPool(const handle_t hPool)
                {
                    return ( hPool ) ? m_vpPools[ hPool ] : m_vpPools[ 1 ];
                }

                const VkCommandBuffer& _GetNextCommandBuffer(SCommandPool* pPool);
                void _FreeCommandBuffer(const VkCommandBuffer& vkCb, SCommandPool* pPool);

            protected:

                SCommandBufferManagerDesc       m_Desc;
                CGraphicsContext*               m_pCtx = nullptr;
                const Vulkan::CDeviceWrapper&   m_VkDevice;
                CommandPoolArray                m_vpPools;
        };

        template<bool ThreadSafe>
        const VkCommandBuffer& CCommandBufferManager::GetNextCommandBuffer(const handle_t& hPool)
        {
            SCommandPool* pPool = _GetPool(hPool);
            assert(pPool);
            if( ThreadSafe )
            {
                pPool->m_SyncObj.Lock();
            }
            const VkCommandBuffer& vkCb = _GetNextCommandBuffer(pPool);
            if( ThreadSafe )
            {
                pPool->m_SyncObj.Unlock();
            }
            return vkCb;
        }

        template<bool ThreadSafe>
        void CCommandBufferManager::FreeCommandBuffer(const VkCommandBuffer& vkCb, const handle_t& hPool)
        {
            auto pPool = _GetPool(hPool);
            if( ThreadSafe )
            {
                pPool->m_SyncObj.Lock();
            }
            _FreeCommandBuffer(vkCb, pPool);
            if( ThreadSafe )
            {
                pPool->m_SyncObj.Unlock();
            }
        }

    } // RenderSystem
} // vke
#endif // VKE_VULKAN_RENDERER