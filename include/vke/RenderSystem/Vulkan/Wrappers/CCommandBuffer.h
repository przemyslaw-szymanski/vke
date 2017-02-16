#pragma once
#if VKE_VULKAN_RENDERER
#include "RenderSystem/Vulkan/Vulkan.h"

namespace VKE
{
    namespace Vulkan
    {
        namespace Wrappers
        {
            using DevICD = const VkICD::Device&;

            template<typename VkHandle>
            class TCBase
            {
                public:                  

                    TCBase(DevICD ICD, VkHandle vkHandle) :
                        m_ICD(ICD),
                        m_vkHandle(vkHandle)
                    {}

                    void operator=(const TCBase&) = delete;

                protected:

                    DevICD      m_ICD;
                    VkHandle    m_vkHandle;
            };

            class CCommandBuffer final : TCBase< VkCommandBuffer >
            {
                public:

                    CCommandBuffer(DevICD ICD, VkCommandBuffer vkCb) :
                        TCBase(ICD, vkCb)
                    {}

                    VkCommandBuffer GetHandle() const { return m_vkHandle; }

                    vke_force_inline VkResult Begin(
                        VkCommandBufferUsageFlags vkCbUsageFlags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT)
                    {
                        VkCommandBufferInheritanceInfo ii = {};
                        Vulkan::InitInfo(&ii, VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO);
                        ii.framebuffer = VK_NULL_HANDLE;
                        ii.occlusionQueryEnable = false;
                        ii.pipelineStatistics = 0;
                        ii.queryFlags = 0;
                        ii.renderPass = VK_NULL_HANDLE;
                        ii.subpass = 0;
                        
                        VkCommandBufferBeginInfo bi;
                        Vulkan::InitInfo(&bi, VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO);
                        bi.flags = vkCbUsageFlags;
                        bi.pInheritanceInfo = &ii;
                        return m_ICD.vkBeginCommandBuffer(m_vkHandle, &bi);
                    }

                    vke_force_inline VkResult End()
                    {
                        return m_ICD.vkEndCommandBuffer(m_vkHandle);
                    }

                    vke_force_inline VkResult Reset(VkCommandBufferResetFlags flags)
                    {
                        return m_ICD.vkResetCommandBuffer(m_vkHandle, flags);
                    }

                    vke_force_inline 
                    void PipelineBarrier(VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,
                                         VkDependencyFlags dependencyFlags,
                                         uint32_t memoryBarrierCount, const VkMemoryBarrier* pMemoryBarriers,
                                         uint32_t bufferBarrierCount, const VkBufferMemoryBarrier* pBufferBarriers,
                                         uint32_t imageBarrierCount, const VkImageMemoryBarrier* pImageBarriers)
                    {
                        m_ICD.vkCmdPipelineBarrier(m_vkHandle, srcStageMask, dstStageMask, dependencyFlags,
                                                   memoryBarrierCount, pMemoryBarriers,
                                                   bufferBarrierCount, pBufferBarriers,
                                                   imageBarrierCount, pImageBarriers);
                    }
            };
        } // Wrappers
    } // Vulkan
} // VKE

#endif // VKE_VULKAN_RENDERER