#pragma once

#include "Vulkan.h"

namespace VKE
{
    namespace Vulkan
    {
        class CDeviceWrapper
        {
            public:

                CDeviceWrapper(VkDevice vkDevice, const VkICD::Device& Functions) :
                    m_vkDevice(vkDevice)
                    , m_ICD(Functions)
                {}

                CDeviceWrapper(const CDeviceWrapper& Other) :
                    CDeviceWrapper(Other.m_vkDevice, Other.m_ICD)
                {}

                CDeviceWrapper& operator=(const CDeviceWrapper& Other) = delete;

                const VkICD::Device& GetICD() const { return m_ICD; }

                VkDevice GetHandle() const { return m_vkDevice; }

                template<typename ObjType, typename CreateInfoType>
                vke_force_inline
                VkResult CreateObject(const CreateInfoType& Info, const VkAllocationCallbacks* pAllocator,
                                      ObjType* pOut) const;

                template<typename ObjType, typename AllocateInfoType>
                vke_force_inline
                VkResult AllocateObjects(const AllocateInfoType& Info, ObjType* pObjects) const
                {
                    return VK_ERROR_OUT_OF_HOST_MEMORY;
                }

                vke_force_inline
                VkResult CreatePipelines(VkPipelineCache vkCache, uint32_t createInfoCount,
                    const VkGraphicsPipelineCreateInfo* pInfos, const VkAllocationCallbacks* pAllocator, VkPipeline* pPipelines)
                {
                    return m_ICD.vkCreateGraphicsPipelines( m_vkDevice, vkCache, createInfoCount, pInfos,
                        pAllocator, pPipelines );
                }

                vke_force_inline
                VkResult CreatePipelines(VkPipelineCache vkCache, uint32_t createInfoCount,
                    const VkComputePipelineCreateInfo* pInfos, const VkAllocationCallbacks* pAllocator, VkPipeline* pPipelines)
                {
                    return m_ICD.vkCreateComputePipelines( m_vkDevice, vkCache, createInfoCount, pInfos,
                        pAllocator, pPipelines );
                }

                vke_force_inline
                VkPipeline CreatePipeline(VkPipelineCache vkCache, const VkGraphicsPipelineCreateInfo& Info,
                    const VkAllocationCallbacks* pAllocator)
                {
                    VkPipeline vkPipeline;
                    VkResult res = CreatePipelines( vkCache, 1u, &Info, pAllocator, &vkPipeline );
                    if (res == VK_SUCCESS)
                    {
                        return vkPipeline;
                    }
                    return VK_NULL_HANDLE;
                }

                vke_force_inline
                VkPipeline CreatePipeline(VkPipelineCache vkCache, const VkComputePipelineCreateInfo& Info,
                    const VkAllocationCallbacks* pAllocator)
                {
                    VkPipeline vkPipeline;
                    VkResult res = CreatePipelines( vkCache, 1u, &Info, pAllocator, &vkPipeline );
                    if (res == VK_SUCCESS)
                    {
                        return vkPipeline;
                    }
                    return VK_NULL_HANDLE;
                }

                template<typename ObjType, typename PoolType>
                vke_force_inline
                void FreeObjects(PoolType vkPool, uint32_t count, const ObjType* pObjects) const
                {}

                template<typename ObjType>
                vke_force_inline
                void DestroyObject(const VkAllocationCallbacks* pAllocator, ObjType* pInOut) const;

                vke_force_inline
                VkResult AcquireNextImageKHR(const VkSwapchainKHR& swapchain, const uint64_t& timeout,
                                             const VkSemaphore& semaphore, const VkFence& fence,
                                             uint32_t* pImageIndex) const
                {
                    return m_ICD.vkAcquireNextImageKHR(m_vkDevice, swapchain, timeout, semaphore,
                                                                fence, pImageIndex);
                }

                vke_force_inline
                VkResult QueuePresentKHR(const VkQueue& queue, const VkPresentInfoKHR& pi) const
                {
                    return m_ICD.vkQueuePresentKHR(queue, &pi);
                }

                vke_force_inline
                VkResult WaitForFences(uint32_t fenceCount, const VkFence* pFences, bool waitAll, uint64_t timeout) const
                {
                    return m_ICD.vkWaitForFences(m_vkDevice, fenceCount, pFences, waitAll, timeout);
                }

                vke_force_inline
                VkResult WaitForFence(const VkFence& fence) const
                {
                    return m_ICD.vkWaitForFences(m_vkDevice, 1, &fence, VK_TRUE, UINT64_MAX);
                }

                vke_force_inline
                VkResult ResetFences(uint32_t fenceCount, const VkFence* pFences) const
                {
                    return m_ICD.vkResetFences(m_vkDevice, fenceCount, pFences);
                }

                vke_force_inline
                bool IsFenceReady(const VkFence& vkFence) const
                {
                    if( vkFence != VK_NULL_HANDLE )
                    {
                        //return WaitForFences(1, &vkFence, true, 0) == VK_SUCCESS;
                        return m_ICD.vkGetFenceStatus(m_vkDevice, vkFence) == VK_SUCCESS;
                    }
                    return false;
                }

                vke_force_inline
                void Wait() const
                {
                    m_ICD.vkDeviceWaitIdle(m_vkDevice);
                }

                vke_force_inline
                VkResult AllocateMemory(const VkMemoryAllocateInfo& Info, const VkAllocationCallbacks* pAllocator,
                                        VkDeviceMemory* pMemory)
                {
                    return m_ICD.vkAllocateMemory( m_vkDevice, &Info, pAllocator, pMemory );
                }

                vke_force_inline
                VkResult BindMemory(const VkImage& vkImg, const VkDeviceMemory& vkMemory,
                                    const VkDeviceSize& vkMemoryOffset)
                {
                    return m_ICD.vkBindImageMemory( m_vkDevice, vkImg, vkMemory, vkMemoryOffset );
                }

                vke_force_inline
                void GetMemoryRequirements(const VkImage& vkImg, VkMemoryRequirements* pOut)
                {
                    m_ICD.vkGetImageMemoryRequirements( m_vkDevice, vkImg, pOut );
                }

            protected:

                VkDevice                m_vkDevice;
                const VkICD::Device&    m_ICD;
        };

#define VK_DEFINE_ICD_CREATE_OBJ(_type) \
    template<> vke_force_inline VkResult \
    CDeviceWrapper::CreateObject< Vk##_type, Vk##_type##CreateInfo >(const Vk##_type##CreateInfo& Info, \
        const VkAllocationCallbacks* pAllocator, Vk##_type* pOut) const \
                            { return m_ICD.vkCreate##_type( m_vkDevice, &Info, pAllocator, pOut ); }

#define VK_DEFINE_ICD_DESTROY_OBJ(_type) \
    template<> vke_force_inline void \
    CDeviceWrapper::DestroyObject< Vk##_type >(const VkAllocationCallbacks* pAllocator, Vk##_type* pOut) const \
    { if(*pOut != VK_NULL_HANDLE) m_ICD.vkDestroy##_type( m_vkDevice, *pOut, pAllocator ); *pOut = VK_NULL_HANDLE; }

        VK_DEFINE_ICD_CREATE_OBJ(RenderPass);
        VK_DEFINE_ICD_CREATE_OBJ(Image);
        VK_DEFINE_ICD_CREATE_OBJ(ImageView);
        VK_DEFINE_ICD_CREATE_OBJ(Sampler);
        VK_DEFINE_ICD_CREATE_OBJ(ShaderModule);
        VK_DEFINE_ICD_CREATE_OBJ(Framebuffer);
        VK_DEFINE_ICD_CREATE_OBJ(Fence);
        VK_DEFINE_ICD_CREATE_OBJ(Event);
        VK_DEFINE_ICD_CREATE_OBJ(Semaphore);
        VK_DEFINE_ICD_CREATE_OBJ(CommandPool);
        VK_DEFINE_ICD_CREATE_OBJ(DescriptorPool);
        VK_DEFINE_ICD_CREATE_OBJ(DescriptorSetLayout);
        VK_DEFINE_ICD_CREATE_OBJ(PipelineLayout);
        //VK_DEFINE_ICD_CREATE_OBJ();

        template<> vke_force_inline VkResult
        CDeviceWrapper::CreateObject<VkSwapchainKHR, VkSwapchainCreateInfoKHR>(
            const VkSwapchainCreateInfoKHR& Info, const VkAllocationCallbacks* pAllocator, VkSwapchainKHR* pOut) const
        {
            return m_ICD.vkCreateSwapchainKHR(m_vkDevice, &Info, pAllocator, pOut);
        }

        VK_DEFINE_ICD_DESTROY_OBJ(RenderPass);
        VK_DEFINE_ICD_DESTROY_OBJ(Image);
        VK_DEFINE_ICD_DESTROY_OBJ(ImageView);
        VK_DEFINE_ICD_DESTROY_OBJ(Sampler);
        VK_DEFINE_ICD_DESTROY_OBJ(ShaderModule);
        VK_DEFINE_ICD_DESTROY_OBJ(Framebuffer);
        VK_DEFINE_ICD_DESTROY_OBJ(Fence);
        VK_DEFINE_ICD_DESTROY_OBJ(Event);
        VK_DEFINE_ICD_DESTROY_OBJ(Semaphore);
        VK_DEFINE_ICD_DESTROY_OBJ(CommandPool);
        VK_DEFINE_ICD_DESTROY_OBJ(SwapchainKHR);
        VK_DEFINE_ICD_DESTROY_OBJ(DescriptorPool);
        VK_DEFINE_ICD_DESTROY_OBJ(DescriptorSetLayout);
        VK_DEFINE_ICD_DESTROY_OBJ(PipelineLayout);
        //VK_DEFINE_ICD_DESTROY_OBJ();

        

        template<> vke_force_inline
        VkResult CDeviceWrapper::AllocateObjects< VkCommandBuffer, VkCommandBufferAllocateInfo >(
            const VkCommandBufferAllocateInfo& Info, VkCommandBuffer* pObjects) const
        {
            return m_ICD.vkAllocateCommandBuffers(m_vkDevice, &Info, pObjects);
        }

        template<> vke_force_inline
        void CDeviceWrapper::FreeObjects< VkCommandBuffer, VkCommandPool >(
            VkCommandPool vkPool, uint32_t count, const VkCommandBuffer* pObjects) const
        {
            m_ICD.vkFreeCommandBuffers(m_vkDevice, vkPool, count, pObjects);
        }

        template<> vke_force_inline
        VkResult CDeviceWrapper::AllocateObjects< VkDescriptorSet, VkDescriptorSetAllocateInfo >(
            const VkDescriptorSetAllocateInfo& Info, VkDescriptorSet* pObjects) const
        {
            return m_ICD.vkAllocateDescriptorSets(m_vkDevice, &Info, pObjects);
        }

        template<> vke_force_inline
        void CDeviceWrapper::FreeObjects< VkDescriptorSet, VkDescriptorPool >(
            VkDescriptorPool vkPool, uint32_t count, const VkDescriptorSet* pObjects) const
        {
            m_ICD.vkFreeDescriptorSets(m_vkDevice, vkPool, count, pObjects);
        }

    } // RenderSystem
} // VKE