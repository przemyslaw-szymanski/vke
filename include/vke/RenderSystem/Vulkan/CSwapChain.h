#pragma once

#include "RenderSystem/Vulkan/Common.h"
#include "Vulkan.h"
#include "Core/Utils/TCDynamicRingArray.h"

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

            using BackBufferArray = Utils::TCDynamicRingArray< SBackBuffer >;
            using AcquireElementArray = Utils::TCDynamicArray< SAcquireElement >;
           
            public:

                CSwapChain(CGraphicsContext* pCtx);
                ~CSwapChain();

                Result Create(const SSwapChainDesc& Desc);
                void Destroy();

                Result Resize(uint32_t width, uint32_t height);

                Result    GetNextBackBuffer();

                Result    SwapBuffers();

                CGraphicsContext* GetGraphicsContext() const { return m_pCtx; }

            protected:

                uint32_t            _GetCurrentImageIndex() const { return m_currImageId; }
                const SBackBuffer&  _GetCurrentBackBuffer() const { return *m_pCurrBackBuffer; }
                VkSwapchainKHR      _GetSwapChain() const { return m_vkSwapChain; }

            protected:
              
                SSwapChainDesc              m_Desc;
                AcquireElementArray         m_vAcquireElements;
                BackBufferArray             m_vBackBuffers;
                SBackBuffer*                m_pCurrBackBuffer = nullptr;
                CGraphicsContext*           m_pCtx = nullptr;
                Vulkan::ICD::Device&        m_ICD;
                Vulkan::CDeviceWrapper&     m_VkDevice;
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
                uint32_t                    m_currBackBufferIdx = 0;
                uint32_t                    m_currImageId = 0;
                bool                        m_needPresent = false;
                
                
                VKE_DEBUG_CODE(VkSwapchainCreateInfoKHR m_vkCreateInfo);
        };
    } // RenderSystem
} // VKE
