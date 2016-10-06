#include "CSwapChain.h"
#include "Vulkan.h"
#include "Core/Utils/CLogger.h"

#include "RenderSystem/Vulkan/CContext.h"
#include "CDevice.h"
#include "Internals.h"
#include "CVkEngine.h"
#include "RenderSystem/Vulkan/CRenderSystem.h"
#include "Core/Platform/CWindow.h"
#include "Core/Memory/Memory.h"

namespace VKE
{
    namespace RenderSystem
    {
        

        struct CSwapChain::SInternal
        {
            uint32_t                    aElementQueue[ Constants::RenderSystem::MAX_SWAP_CHAIN_ELEMENTS ] = { 0 };

            struct
            {
                VkSurfaceCapabilitiesKHR    vkSurfaceCaps;
                VkSurfaceKHR                vkSurface = VK_NULL_HANDLE;
                VkSurfaceFormatKHR          vkSurfaceFormat;
                VkPresentModeKHR            vkPresentMode;
                VkSwapchainKHR              vkSwapChain = VK_NULL_HANDLE;
                VkQueue                     vkCurrQueue = VK_NULL_HANDLE;
                VkPresentInfoKHR            PresentInfo;
                uint32_t                    currElementId = 0;
                uint32_t                    currImageId = 0;
                SSwapChainElement*          pCurrElement = nullptr;
                SSwapChainElement           aElements[ Constants::RenderSystem::MAX_SWAP_CHAIN_ELEMENTS ];
                VkSemaphore                 aSemaphores[ Constants::RenderSystem::MAX_SWAP_CHAIN_ELEMENTS ] = { VK_NULL_HANDLE };

                struct
                {

                } Present;
            } Vulkan;

            SInternal()
            {
                auto& PresentInfo = Vulkan.PresentInfo;
                Vulkan::InitInfo(&PresentInfo, VK_STRUCTURE_TYPE_PRESENT_INFO_KHR);
                PresentInfo.pResults = nullptr;
                PresentInfo.swapchainCount = 1;
                PresentInfo.waitSemaphoreCount = 1;
            }
        };

        CSwapChain::CSwapChain(CContext* pCtx) :
            m_pCtx(pCtx)
        {
            m_pDevice = m_pCtx->GetDevice();
            m_pDeviceCtx = m_pCtx->GetDeviceContext();
        }

        CSwapChain::~CSwapChain()
        {
            Destroy();
        }

        void CSwapChain::Destroy()
        {
            auto& Device = GetDevice()->GetDeviceFunctions();
            auto& VulkanData = m_pInternal->Vulkan;

            for (uint32_t i = 0; i < m_Info.elementCount; ++i)
            {
                m_pDeviceCtx->DestroySemaphore(nullptr, &VulkanData.aSemaphores[i]);
                m_pDeviceCtx->DestroyImageView(nullptr, &VulkanData.aElements[i].vkImageView);
            }

            if (VulkanData.vkSwapChain != VK_NULL_HANDLE)
            {
                Device.vkDestroySwapchainKHR(m_pDeviceCtx->GetDeviceHandle(), VulkanData.vkSwapChain, nullptr);
                VulkanData.vkSwapChain = VK_NULL_HANDLE;
            }

            Memory::DestroyObject(&HeapAllocator, &m_pInternal);
        }

        void CSwapChain::SetQueue(VkQueue vkQueue)
        {
            m_pInternal->Vulkan.vkCurrQueue = vkQueue;
        }

        Result CSwapChain::Create(const SSwapChainInfo& Info)
        {
            m_Info = Info;
            VKE_RETURN_IF_FAILED(Memory::CreateObject(&HeapAllocator, &m_pInternal));

            auto& VkInternal = m_pInternal->Vulkan;

            auto* pDevice = m_pCtx->GetDevice();
            auto& Device = pDevice->GetDeviceFunctions();
            auto& Instance = pDevice->GetInstanceFunctions();
            const auto& queueIndex = pDevice->GetQueueIndex(CDevice::QueueTypes::GRAPHICS);
            VkInternal.vkCurrQueue = pDevice->GetQueue(queueIndex);

            VkPhysicalDevice vkPhysicalDevice = pDevice->GetPhysicalDevice();
            VkInstance vkInstance = pDevice->GetAPIInstance();

            if (m_Info.hWnd == NULL_HANDLE)
            {
                auto pEngine = pDevice->GetRenderSystem()->GetEngine();
                auto pWnd = pEngine->GetWindow();
                m_Info.hWnd = pWnd->GetInfo().wndHandle;
                m_Info.hPlatform = pWnd->GetInfo().platformHandle;
            }
            
#if VKE_USE_VULKAN_WINDOWS
            if (!Instance.vkGetPhysicalDeviceWin32PresentationSupportKHR(vkPhysicalDevice, queueIndex))
            {
                VKE_LOG_ERR("Queue index: %d" << queueIndex << " does not support presentation");
                return VKE_FAIL;
            }
            HINSTANCE hInst = reinterpret_cast<HINSTANCE>(m_Info.hPlatform);
            HWND hWnd = reinterpret_cast<HWND>(m_Info.hWnd);
            VkWin32SurfaceCreateInfoKHR SurfaceCI;
            Vulkan::InitInfo(&SurfaceCI, VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR);
            SurfaceCI.flags = 0;
            SurfaceCI.hinstance = hInst;
            SurfaceCI.hwnd = hWnd;
            VK_ERR(Instance.vkCreateWin32SurfaceKHR(vkInstance, &SurfaceCI, nullptr, &VkInternal.vkSurface));
#elif VKE_USE_VULKAN_LINUX
            if (!Vk.vkGetPhysicalDeviceXcbPresentationSupportKHR(s_physical_device, s_queue_family_index, s_window.xcb_connection, s_window.visual_id))
            {
                DPF("vkGetPhysicalDeviceXcbPresentationSupportKHR returned FALSE, %d queue index does not support presentation on visual_id %x\n", s_queue_family_index, s_window.visual_id);
                return 0;
            }
            VkXcbSurfaceCreateInfoKHR SurfaceCI;
            Vulkan::InitInfo(&SurfaceCI, VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR);
            SurfaceCI.flags = 0;
            SurfaceCI.connection = reinterpret_cast<xcb_connection_t*>(m_Info.hPlatform);
            SurfaceCI.window = m_Info.hWnd;
            EXPECT_SUCCESS(Vk.vkCreateXcbSurfaceKHR(s_instance, &SurfaceCI, NO_ALLOC_CALLBACK, &s_surface))
#elif VKE_USE_VULKAN_ANDROID
            VkAndroidSurfaceCreateInfoKHR SurfaceCI;
            Vulkan::InitInfo(&SurfaceCI, VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR);
            SurfaceCI.flags = 0;
            SurfaceCI.window = m_Info.hWnd;
            EXPECT_SUCCESS(Vk.vkCreateAndroidSurfaceKHR(s_instance, s_window.window->getNativeHandle(), NO_ALLOC_CALLBACK, &s_surface));
#endif

            VkBool32 isSurfaceSupported = VK_FALSE;
            VK_ERR(Instance.vkGetPhysicalDeviceSurfaceSupportKHR(vkPhysicalDevice, queueIndex,
                   VkInternal.vkSurface, &isSurfaceSupported));
            if(!isSurfaceSupported)
            {
                VKE_LOG_ERR("Queue index: " << queueIndex << " does not support the surface.");
                return VKE_FAIL;
            }

            Instance.vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vkPhysicalDevice, VkInternal.vkSurface,
                                                               &VkInternal.vkSurfaceCaps);
            auto hasColorAttachment = VkInternal.vkSurfaceCaps.supportedUsageFlags | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            if (!hasColorAttachment)
            {
                VKE_LOG_ERR("Device surface has no color attachment.");
                return VKE_FAIL;
            }

            if ( VkInternal.vkSurfaceCaps.maxImageCount == 0 )
            {
                VkInternal.vkSurfaceCaps.maxImageCount = Constants::RenderSystem::MAX_SWAP_CHAIN_ELEMENTS;
            }

            if(Constants::OPTIMAL.IsOptimal(m_Info.elementCount))
            {
                m_Info.elementCount = Min(static_cast<uint16_t>( VkInternal.vkSurfaceCaps.maxImageCount), 3);
            }
            else
            {
                m_Info.elementCount = Min(m_Info.elementCount, static_cast<uint16_t>( VkInternal.vkSurfaceCaps.maxImageCount));
            }

            // Select surface format
            vke_vector<VkSurfaceFormatKHR> vSurfaceFormats;
            uint32_t formatCount = 0;
            VK_ERR(Instance.vkGetPhysicalDeviceSurfaceFormatsKHR(vkPhysicalDevice, VkInternal.vkSurface,
                   &formatCount, nullptr));
            if (formatCount == 0)
            {
                VKE_LOG_ERR("No supported device surface formats.");
                return VKE_FAIL;
            }
            vSurfaceFormats.resize(formatCount);
            VK_ERR(Instance.vkGetPhysicalDeviceSurfaceFormatsKHR(vkPhysicalDevice, VkInternal.vkSurface,
                   &formatCount, &vSurfaceFormats[0]));

            bool formatFound = false;
            for(auto& Format : vSurfaceFormats)
            {
                const auto& format = g_aFormats[m_Info.format];
                if (Format.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR)
                {
                    VkInternal.vkSurfaceFormat = Format;
                    formatFound = true;
                    break;
                }
                
            }

            if(!formatFound)
            {
                VKE_LOG_ERR("No proper device surface format.");
                return VKE_FAIL;
            }

            /// @todo check for fullscreen if format is 32bit


            // Select present mode
            uint32_t presentCount = 0;
            vke_vector<VkPresentModeKHR> vPresents;
            VK_ERR(Instance.vkGetPhysicalDeviceSurfacePresentModesKHR(vkPhysicalDevice, VkInternal.vkSurface,
                   &presentCount, nullptr));
            if(presentCount == 0)
            {
                VKE_LOG_ERR("No present modes supported.");
                return VKE_FAIL;
            }
            vPresents.resize(presentCount);
            VK_ERR(Instance.vkGetPhysicalDeviceSurfacePresentModesKHR(vkPhysicalDevice, VkInternal.vkSurface,
                   &presentCount, &vPresents[0]));

            bool presentFound = false;
            for (auto& Present : vPresents)
            {
                if (Present == VK_PRESENT_MODE_IMMEDIATE_KHR || Present == VK_PRESENT_MODE_FIFO_KHR)
                {
                    VkInternal.vkPresentMode = Present;
                    presentFound = true;
                    break;
                }
            }

            if(!presentFound)
            {
                VKE_LOG_ERR("No proper present mode found.");
                return VKE_FAIL;
            }

            VkSwapchainCreateInfoKHR ci;
            Vulkan::InitInfo(&ci, VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR);
            ci.clipped = false;
            ci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
            ci.flags = 0;
            ci.imageArrayLayers = 1;
            ci.imageColorSpace = VkInternal.vkSurfaceFormat.colorSpace;
            ci.imageExtent.width = m_Info.Size.width;
            ci.imageExtent.height = m_Info.Size.height;
            ci.imageFormat = VkInternal.vkSurfaceFormat.format;
            ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            ci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            ci.minImageCount = m_Info.elementCount;
            ci.oldSwapchain = VK_NULL_HANDLE;
            ci.pQueueFamilyIndices = &queueIndex;
            ci.queueFamilyIndexCount = 1;
            ci.presentMode = VkInternal.vkPresentMode;
            ci.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
            ci.surface = VkInternal.vkSurface;

            VK_ERR(Device.vkCreateSwapchainKHR(m_pDeviceCtx->GetDeviceHandle(), &ci, nullptr, &VkInternal.vkSwapChain));
            VKE_DEBUG_CODE(m_vkCreateInfo = ci);
            VkInternal.PresentInfo.pSwapchains = &VkInternal.vkSwapChain;

            uint32_t imgCount = 0;
            Device.vkGetSwapchainImagesKHR(m_pDeviceCtx->GetDeviceHandle(), VkInternal.vkSwapChain, &imgCount, nullptr);
            if(imgCount != m_Info.elementCount)
            {
                VKE_LOG_ERR("Swap chain element count is different than requested.");
                return VKE_FAIL;
            }
            VkImage aImages[Constants::RenderSystem::MAX_SWAP_CHAIN_ELEMENTS];
            VK_ERR(Device.vkGetSwapchainImagesKHR(m_pDeviceCtx->GetDeviceHandle(), VkInternal.vkSwapChain, &imgCount,
                   aImages));

            for(uint32_t i = 0; i < imgCount; ++i)
            {
                VkInternal.aElements[i].vkImage = aImages[i];
                VkImageViewCreateInfo ci;
                Vulkan::InitInfo(&ci, VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO);
                ci.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
                ci.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
                ci.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
                ci.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
                ci.flags = 0;
                ci.format = VkInternal.vkSurfaceFormat.format;
                ci.image = VkInternal.aElements[i].vkImage;
                ci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                ci.subresourceRange.baseArrayLayer = 0;
                ci.subresourceRange.baseMipLevel = 0;
                ci.subresourceRange.layerCount = 1;
                ci.subresourceRange.levelCount = 1;
                ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
                //VK_ERR(Device.vkCreateImageView(m_vkDevice, &ci, nullptr, &m_aElements[i].vkImageView));
                VK_ERR(m_pDeviceCtx->CreateImageView(ci, nullptr, &VkInternal.aElements[i].vkImageView));

                VkSemaphoreCreateInfo SemaphoreCI;
                Vulkan::InitInfo(&SemaphoreCI, VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO);
                SemaphoreCI.flags = 0;
                VK_ERR(m_pDeviceCtx->CreateSemaphore(SemaphoreCI, nullptr, &VkInternal.aSemaphores[i]));
                VkInternal.aElements[ i ].vkSemaphore = VkInternal.aSemaphores[ i ];
            }

            return VKE_OK;
        }

        Result CSwapChain::Resize(uint32_t width, uint32_t height)
        {
            return VKE_OK;
        }

        Result CSwapChain::GetNextElement()
        {
            auto& VkInternal = m_pInternal->Vulkan;

            VkInternal.pCurrElement = &VkInternal.aElements[ VkInternal.currElementId ];
            auto vkSemaphore = VkInternal.pCurrElement->vkSemaphore;
            VK_ERR(m_pDeviceCtx->AcquireNextImageKHR(VkInternal.vkSwapChain, UINT64_MAX, vkSemaphore,
                   VK_NULL_HANDLE, &VkInternal.currImageId));

            return VKE_OK;
        }

        void CSwapChain::BeginPresent()
        {
            GetNextElement();
        }

        void CSwapChain::EndPresent()
        {
            auto& VkInternal = m_pInternal->Vulkan;
            assert(VkInternal.vkCurrQueue != VK_NULL_HANDLE);

            auto& PresentInfo = VkInternal.PresentInfo;
            PresentInfo.pImageIndices = &VkInternal.currImageId;
            PresentInfo.pWaitSemaphores = &VkInternal.pCurrElement->vkSemaphore;

            VK_ERR(m_pDeviceCtx->QueuePresentKHR(VkInternal.vkCurrQueue, PresentInfo));

            VkInternal.currElementId++;
            VkInternal.currElementId %= m_Info.elementCount;
        }

        const SSwapChainElement const* CSwapChain::GetCurrentElement() const
        {
            return m_pInternal->Vulkan.pCurrElement;
        }

    } // RenderSystem
} // VKE