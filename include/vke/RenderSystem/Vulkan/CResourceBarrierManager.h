#pragma once

#include "Common.h"
#if VKE_VULKAN_RENDER_SYSTEM
namespace VKE
{
    namespace RenderSystem
    {
        class CResourceBarrierManager final
        {
            friend class CDeviceContext;
            public:
                
                static const uint32_t DEFAULT_BARRIER_COUNT = 16;

                struct SImageBarrierInfo
                {
                    VkImage         vkImg;
                    VkImageLayout   vkCurrentLayout;
                    VkImageLayout   vkNewLayout;
                };
                using ImageBarrierVec = Utils::TCDynamicArray< SImageBarrierInfo, DEFAULT_BARRIER_COUNT >;
                using VkImageBarrierVec = Utils::TCDynamicArray< VkImageMemoryBarrier, DEFAULT_BARRIER_COUNT >;

                struct SMemoryBarrierInfo
                {

                };
                using MemoryBarrierVec = Utils::TCDynamicArray< SMemoryBarrierInfo, DEFAULT_BARRIER_COUNT >;
                using VkMemoryBarrierVec = Utils::TCDynamicArray< VkMemoryBarrier, DEFAULT_BARRIER_COUNT >;

                struct SBufferBarrierInfo
                {

                };
                using BufferBarrierVec = Utils::TCDynamicArray< SBufferBarrierInfo, DEFAULT_BARRIER_COUNT >;
                using VkBufferBarrierVec = Utils::TCDynamicArray< VkBufferMemoryBarrier, DEFAULT_BARRIER_COUNT >;

                struct SBarrierData
                {
                    VkMemoryBarrierVec  vMemoryBarriers;
                    VkBufferBarrierVec  vBufferBarriers;
                    VkImageBarrierVec   vImageBarriers;
                };

            public:

                CResourceBarrierManager();
                ~CResourceBarrierManager();

                void AddBarrier(const SImageBarrierInfo& Info);
                void ExecuteBarriers(SBarrierData* pOut);

            protected:

                MemoryBarrierVec    m_vMemoryBarriers;
                BufferBarrierVec    m_vBufferBarriers;
                ImageBarrierVec     m_vImageBarriers;
        };
    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDER_SYSTEM