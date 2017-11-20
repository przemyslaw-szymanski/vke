#pragma once

#include "Core/Memory/CMemoryPoolManager.h"
#if VKE_VULKAN_RENDERER
#include "RenderSystem/Common.h"
#include "RenderSystem/Vulkan/Vulkan.h"

namespace VKE
{
    namespace RenderSystem
    {
        class CDeviceMemoryManager final : public Memory::CMemoryPoolManager
        {
            friend class CDeviceContext;
            
            struct SVkCache
            {
                VkMemoryPropertyFlags   vkPropertyFlags;
                uint32_t                memoryBits;
                int32_t                 memoryTypeIndex;
            };
            using VkCacheVec = Utils::TCDynamicArray< SVkCache >;

            public:

                            CDeviceMemoryManager(CDeviceContext* pCtx);
                            ~CDeviceMemoryManager();

                Result      Create(const SDeviceMemoryManagerDesc& Desc);
                void        Destroy() override;

                uint64_t    Allocate(const VkImage& vkImg, const VkMemoryPropertyFlags& vkProperties);
                uint64_t    Allocate(const SAllocateInfo& Info, SAllocatedData* pOut) override;
                //Result      Free(uint64_t memory) override;
                Result      AllocatePool(const SPoolAllocateInfo& Info) override;

            protected:

                SDeviceMemoryManagerDesc                    m_Desc;
                CDeviceContext*                             m_pCtx;
                const VkPhysicalDeviceMemoryProperties*     m_pVkMemProperties;
                VkCacheVec                                  m_vVkCaches;
        };
    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER