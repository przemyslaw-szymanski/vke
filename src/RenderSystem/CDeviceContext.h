#pragma once

#include "Vulkan.h"

namespace VKE
{
    namespace RenderSystem
    {
        class CDeviceContext
        {
            public:

                CDeviceContext(VkDevice vkDevice, const ICD::Device& Functions) : 
                    m_vkDevice(vkDevice)
                    , m_DevFunctions(Functions)
                {}

                CDeviceContext& operator=(const CDeviceContext&) = delete;

                template<typename _FUNC_, typename _CREATE_INFO_TYPE_, typename _RES_TYPE_>
                vke_force_inline 
                VkResult CreateObject(_FUNC_ func, const _CREATE_INFO_TYPE_& Info,
                                        const VkAllocationCallbacks* pCallbacks, _RES_TYPE_* pOut);

                template<typename _FUNC_, typename _RES_TYPE_>
                vke_force_inline 
                void DestroyObject(_FUNC_ func, const VkAllocationCallbacks* pCallbacks, _RES_TYPE_* pOut);

                vke_force_inline 
                VkResult CreateImage(const VkImageCreateInfo& Info,
                                     const VkAllocationCallbacks* pCallbacks, VkImage* pOut)
                {
                    return CreateObject(m_DevFunctions.vkCreateImage, Info, pCallbacks, pOut);
                }

                vke_force_inline 
                void DestroyImage(const VkAllocationCallbacks* pCallbacks, VkImage* pInOut)
                {
                    DestroyObject(m_DevFunctions.vkDestroyImage, pCallbacks, pInOut);
                }

                vke_force_inline
                VkResult CreateImageView(const VkImageViewCreateInfo& Info, const VkAllocationCallbacks* pCallbacks,
                                         VkImageView* pOut)
                {
                    return CreateObject(m_DevFunctions.vkCreateImageView, Info, pCallbacks, pOut);
                }

                vke_force_inline
                void DestroyImageView(const VkAllocationCallbacks* pCallbacks, VkImageView* pInOut)
                {
                    DestroyObject(m_DevFunctions.vkDestroyImageView, pCallbacks, pInOut);
                }

                vke_force_inline
                VkResult CreateSemaphore(const VkSemaphoreCreateInfo& Info, const VkAllocationCallbacks* pCallbacks,
                                         VkSemaphore* pOut)
                {
                    return CreateObject(m_DevFunctions.vkCreateSemaphore, Info, pCallbacks, pOut);
                }

                vke_force_inline
                void DestroySemaphore(const VkAllocationCallbacks* pCallbacks, VkSemaphore* pInOut)
                {
                    DestroyObject(m_DevFunctions.vkDestroySemaphore, pCallbacks, pInOut);
                }

            protected:

                VkDevice            m_vkDevice;
                const ICD::Device&  m_DevFunctions;
        };

        template<typename _FUNC_, typename _CREATE_INFO_TYPE_, typename _RES_TYPE_>
        VkResult CDeviceContext::CreateObject(_FUNC_ func, const _CREATE_INFO_TYPE_& Info,
                                                const VkAllocationCallbacks* pCallbacks, _RES_TYPE_* pOut)
        {
            return func(m_vkDevice, &Info, pCallbacks, pOut);
        }

        template<typename _FUNC_, typename _RES_TYPE_>
        void CDeviceContext::DestroyObject(_FUNC_ func, const VkAllocationCallbacks* pCallbacks, _RES_TYPE_* pOut)
        {
#if VKE_DEBUG
            if (*pOut != VK_NULL_HANDLE)
#endif
            func(m_vkDevice, *pOut, pCallbacks);
            *pOut = VK_NULL_HANDLE;
        }

    } // RenderSystem
} // VKE