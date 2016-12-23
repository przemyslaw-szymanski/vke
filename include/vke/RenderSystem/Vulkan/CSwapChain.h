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
            uint32_t        currImageIdx = 0;
        };

        class CSwapChain
        {
            friend class CGraphicsContext;
            struct SPrivate;

            struct SAcquireElement
            {
                VkImage         vkImage = VK_NULL_HANDLE;
                VkImageView     vkImageView = VK_NULL_HANDLE;
                //VkFramebuffer   vkFramebuffer = VK_NULL_HANDLE;
                VkCommandBuffer vkCbAttachmentToPresent = VK_NULL_HANDLE;
                VkCommandBuffer vkCbPresentToAttachment = VK_NULL_HANDLE;
                RenderTargetHandle hRenderTarget = NULL_HANDLE;
                CRenderTarget*  pRenderTarget = nullptr;
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

                void BeginPass(VkCommandBuffer vkCb);
                void EndPass(VkCommandBuffer vkCb);

                CGraphicsContext* GetGraphicsContext() const { return m_pCtx; }
                RenderTargetHandle GetRenderTarget() const { return m_pCurrAcquireElement->hRenderTarget; }

                ExtentU32 GetSize() const;

            protected:

                uint32_t            _GetCurrentImageIndex() const { return m_pCurrBackBuffer->currImageIdx; }
                const SBackBuffer&  _GetCurrentBackBuffer() const { return *m_pCurrBackBuffer; }
                VkSwapchainKHR      _GetSwapChain() const { return m_vkSwapChain; }

            protected:
              
                SSwapChainDesc              m_Desc;
                AcquireElementArray         m_vAcquireElements;
                BackBufferArray             m_vBackBuffers;
                SBackBuffer*                m_pCurrBackBuffer = nullptr;
                SAcquireElement*            m_pCurrAcquireElement = nullptr;
                CGraphicsContext*           m_pCtx = nullptr;
                const Vulkan::ICD::Device&  m_ICD;
                Vulkan::CDeviceWrapper&     m_VkDevice;
                VkPhysicalDevice            m_vkPhysicalDevice = VK_NULL_HANDLE;
                VkInstance                  m_vkInstance = VK_NULL_HANDLE;
                VkSurfaceCapabilitiesKHR    m_vkSurfaceCaps;
                VkSurfaceKHR                m_vkSurface = VK_NULL_HANDLE;
                VkSurfaceFormatKHR          m_vkSurfaceFormat;
                VkPresentModeKHR            m_vkPresentMode;
                VkSwapchainKHR              m_vkSwapChain = VK_NULL_HANDLE;
                VkRenderPass                m_vkRenderPass = VK_NULL_HANDLE;
                Vulkan::Queue               m_pQueue = nullptr;
                VkPresentInfoKHR            m_PresentInfo;
                uint32_t                    m_currBackBufferIdx = 0;
                bool                        m_needPresent = false;
                
                
                VKE_DEBUG_CODE(VkSwapchainCreateInfoKHR m_vkCreateInfo);
        };
    } // RenderSystem
} // VKE
