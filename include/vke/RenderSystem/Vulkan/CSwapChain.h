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
        class CRenderPass;
        struct SFrameData;
        class CRenderingPipeline;

        namespace Managers
        {
            class CBackBufferManager;
        }

        class CSwapChain
        {
            friend class CGraphicsContext;
            struct SPrivate;

            struct SAcquireElement
            {
                VkImageMemoryBarrier    vkBarrierAttachmentToPresent;
                VkImageMemoryBarrier    vkBarrierPresentToAttachment;
                VkImage                 vkImage = VK_NULL_HANDLE;
                VkImageView             vkImageView = VK_NULL_HANDLE;
                VkImageLayout           vkOldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                VkImageLayout           vkCurrLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                //VkFramebuffer   vkFramebuffer = VK_NULL_HANDLE;
                VkCommandBuffer         vkCbAttachmentToPresent = VK_NULL_HANDLE;
                VkCommandBuffer         vkCbPresentToAttachment = VK_NULL_HANDLE;
                RenderPassHandle        hRenderPass = NULL_HANDLE;
                CRenderPass*            pRenderPass = nullptr;
            };

            struct SBackBuffer
            {
                SAcquireElement     AcquiredElement;
                VkSemaphore         vkAcquireSemaphore = VK_NULL_HANDLE;
                VkSemaphore         vkCmdBufferSemaphore = VK_NULL_HANDLE;
                uint32_t            currImageIdx = 0;
            };

            using BackBufferVec = Utils::TCDynamicRingArray< SBackBuffer >;
            using AcquireElementVec = Utils::TCDynamicArray< SAcquireElement >;
            using CBackBufferManager = Managers::CBackBufferManager;
           
            public:

                CSwapChain(CGraphicsContext* pCtx);
                ~CSwapChain();

                void operator=(const CSwapChain&) = delete;

                Result Create(const SSwapChainDesc& Desc);
                void Destroy();

                uint32_t GetPresentationElementCount() const { return m_vAcquireElements.GetCount(); }
                uint32_t GetBackBufferCount() const { return m_vBackBuffers.GetCount(); }

                Result Resize(uint32_t width, uint32_t height);

                Result    GetNextBackBuffer();

                Result    SwapBuffers();

                void BeginPass(VkCommandBuffer vkCb);
                void EndPass(VkCommandBuffer vkCb);
                void BeginFrame(VkCommandBuffer vkCb);
                void EndFrame(VkCommandBuffer vkCb);

                CGraphicsContext* GetGraphicsContext() const { return m_pCtx; }
                //RenderTargetHandle GetRenderTarget() const { return m_pCurrAcquireElement->hRenderTarget; }
                CRenderPass* GetRenderPass() const { return m_pCurrAcquireElement->pRenderPass; }

                ExtentU32 GetSize() const;

                const SBackBuffer&  GetCurrentBackBuffer() const { return *m_pCurrBackBuffer; }

            protected:

                uint32_t            _GetCurrentImageIndex() const { return m_pCurrBackBuffer->currImageIdx; }
                const SBackBuffer&  _GetCurrentBackBuffer() const { return *m_pCurrBackBuffer; }
                VkSwapchainKHR      _GetSwapChain() const { return m_vkSwapChain; }

                BackBufferVec&      _GetBackBuffers() { return m_vBackBuffers; }
                AcquireElementVec&  _GetAcquireElements() { return m_vAcquireElements; }

            protected:
              
                SSwapChainDesc              m_Desc;
                AcquireElementVec           m_vAcquireElements;
                BackBufferVec               m_vBackBuffers;
                uint32_t                    m_backBufferIdx = 0;
                CBackBufferManager*         m_pBackBufferMgr = nullptr;
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
