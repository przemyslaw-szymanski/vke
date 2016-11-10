#pragma once

#include "RenderSystem/Vulkan/Common.h"
#include "Vulkan.h"
#include "Core/Utils/TCDynamicArray.h"

namespace VKE
{
    namespace Vulkan
    {
        class CDeviceWrapper;
    }

    namespace RenderSystem
    {
        class CDevice;
        class CGraphicsContext;
        struct SFrameData;

        struct SBackBuffer
        {
            VkSemaphore     vkAcquireSemaphore = VK_NULL_HANDLE;
            VkSemaphore     vkCmdBufferSemaphore = VK_NULL_HANDLE;
            void*           pFrameData = nullptr;
        };

        class CSwapChain
        {
            friend class CGraphicsContext;
            struct SPrivate;

            struct SAcquireElement
            {
                VkImage         vkImage = VK_NULL_HANDLE;
                VkImageView     vkImageView = VK_NULL_HANDLE;
                VkCommandBuffer vkCbAttachmentToPresent = VK_NULL_HANDLE;
                VkCommandBuffer vkCbPresentToAttachment = VK_NULL_HANDLE;
            };

            using BackBufferArray = Utils::TCDynamicArray< SBackBuffer >;
            using AcquireElementArray = Utils::TCDynamicArray< SAcquireElement >;
           
            public:

                CSwapChain(CGraphicsContext* pCtx);
                ~CSwapChain();

                Result Create(const SSwapChainDesc& Desc);
                void Destroy();

                Result Resize(uint32_t width, uint32_t height);

                Result    GetNextElement();

                void    BeginPresent();
                void    EndPresent();

            protected:


            protected:
              
                SSwapChainDesc              m_Desc;            
                CGraphicsContext*           m_pCtx = nullptr;
                Vulkan::ICD::Device&        m_ICD;
                Vulkan::CDeviceWrapper      m_Device;
                VkPhysicalDevice            m_vkPhysicalDevice = VK_NULL_HANDLE;
                VkInstance                  m_vkInstance = VK_NULL_HANDLE;
                VkSurfaceCapabilitiesKHR    m_vkSurfaceCaps;
                VkSurfaceKHR                m_vkSurface = VK_NULL_HANDLE;
                VkSurfaceFormatKHR          m_vkSurfaceFormat;
                VkPresentModeKHR            m_vkPresentMode;
                VkSwapchainKHR              m_vkSwapChain = VK_NULL_HANDLE;
                VkQueue                     m_vkQueue = VK_NULL_HANDLE;
                uint32_t                    m_queueFamilyIndex = 0;
                VkPresentInfoKHR            m_PresentInfo;
                uint32_t                    m_currElementId = 0;
                uint32_t                    m_currImageId = 0;
                
                
                VKE_DEBUG_CODE(VkSwapchainCreateInfoKHR m_vkCreateInfo);
        };
    } // RenderSystem
} // VKE
