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
                    , m_DevFunctions(Functions)
                {}

                CDeviceWrapper& operator=(const CDeviceWrapper&) = delete;

                VkDevice GetDeviceHandle() const { return m_vkDevice; }

                template<typename _FUNC_, typename _CREATE_INFO_TYPE_, typename _RES_TYPE_>
                vke_force_inline 
                VkResult CreateObject(_FUNC_& func, const _CREATE_INFO_TYPE_& Info,
                                        const VkAllocationCallbacks* pCallbacks, _RES_TYPE_* pOut);

                template<typename _FUNC_, typename _RES_TYPE_>
                vke_force_inline 
                void DestroyObject(_FUNC_& func, const VkAllocationCallbacks* pCallbacks, _RES_TYPE_* pOut);

                template<typename ObjType, typename CreateInfoType>
                vke_force_inline
                VkResult CreateObject(const CreateInfoType& Info, const VkAllocationCallbacks* pAllocator,
                    ObjType* pOut);

                template<typename ObjType>
                vke_force_inline
                void DestroyObject(const VkAllocationCallbacks* pAllocator, ObjType* pInOut);

                vke_force_inline
                VkResult AcquireNextImageKHR(const VkSwapchainKHR& swapchain, const uint64_t& timeout,
                                             const VkSemaphore& semaphore, const VkFence& fence,
                                             uint32_t* pImageIndex)
                {
                    return m_DevFunctions.vkAcquireNextImageKHR(m_vkDevice, swapchain, timeout, semaphore,
                                                                fence, pImageIndex);
                }

                vke_force_inline
                VkResult QueuePresentKHR(const VkQueue& queue, const VkPresentInfoKHR& pi)
                {
                    return m_DevFunctions.vkQueuePresentKHR(queue, &pi);
                }

            protected:

                VkDevice                m_vkDevice;
                const VkICD::Device&    m_DevFunctions;
        };

        template<typename _FUNC_, typename _CREATE_INFO_TYPE_, typename _RES_TYPE_>
        vke_force_inline
        VkResult CDeviceWrapper::CreateObject(_FUNC_& func, const _CREATE_INFO_TYPE_& Info,
                                                const VkAllocationCallbacks* pCallbacks, _RES_TYPE_* pOut)
        {
            return func(m_vkDevice, &Info, pCallbacks, pOut);
        }

        template<typename _FUNC_, typename _RES_TYPE_>
        vke_force_inline
        void CDeviceWrapper::DestroyObject(_FUNC_& func, const VkAllocationCallbacks* pCallbacks, _RES_TYPE_* pOut)
        {
            if (*pOut != VK_NULL_HANDLE)
                func(m_vkDevice, *pOut, pCallbacks);
            *pOut = VK_NULL_HANDLE;
        }

#define VK_DEFINE_ICD_CREATE_OBJ(_type) \
    template<> vke_force_inline VkResult \
    CDeviceWrapper::CreateObject< Vk##_type, Vk##_type##CreateInfo >(const Vk##_type##CreateInfo& Info, \
        const VkAllocationCallbacks* pAllocator, Vk##_type* pOut) \
                            { return m_DevFunctions.vkCreate##_type( m_vkDevice, &Info, pAllocator, pOut ); }

#define VK_DEFINE_ICD_DESTROY_OBJ(_type) \
    template<> vke_force_inline void \
    CDeviceWrapper::DestroyObject< Vk##_type >(const VkAllocationCallbacks* pAllocator, Vk##_type* pOut) \
    { if(*pOut != VK_NULL_HANDLE) m_DevFunctions.vkDestroy##_type( m_vkDevice, *pOut, pAllocator ); *pOut = VK_NULL_HANDLE; }

        VK_DEFINE_ICD_CREATE_OBJ(RenderPass);
        VK_DEFINE_ICD_CREATE_OBJ(Image);
        VK_DEFINE_ICD_CREATE_OBJ(ImageView);
        VK_DEFINE_ICD_CREATE_OBJ(Sampler);
        VK_DEFINE_ICD_CREATE_OBJ(ShaderModule);
        VK_DEFINE_ICD_CREATE_OBJ(Framebuffer);
        VK_DEFINE_ICD_CREATE_OBJ(Fence);
        VK_DEFINE_ICD_CREATE_OBJ(Event);
        VK_DEFINE_ICD_CREATE_OBJ(Semaphore);
        //VK_DEFINE_ICD_CREATE_OBJ();

        VK_DEFINE_ICD_DESTROY_OBJ(RenderPass);
        VK_DEFINE_ICD_DESTROY_OBJ(Image);
        VK_DEFINE_ICD_DESTROY_OBJ(ImageView);
        VK_DEFINE_ICD_DESTROY_OBJ(Sampler);
        VK_DEFINE_ICD_DESTROY_OBJ(ShaderModule);
        VK_DEFINE_ICD_DESTROY_OBJ(Framebuffer);
        VK_DEFINE_ICD_DESTROY_OBJ(Fence);
        VK_DEFINE_ICD_DESTROY_OBJ(Event);
        VK_DEFINE_ICD_DESTROY_OBJ(Semaphore);
        //VK_DEFINE_ICD_DESTROY_OBJ();

    } // RenderSystem
} // VKE