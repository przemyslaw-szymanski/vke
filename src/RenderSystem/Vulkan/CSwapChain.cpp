#include "CSwapChain.h"
#include "Vulkan.h"
#include "Core/Utils/CLogger.h"

#include "RenderSystem/Vulkan/CContext.h"
#include "CDevice.h"
#include "Internals.h"
#include "CVkEngine.h"
#include "RenderSystem/Vulkan/CRenderSystem.h"
#include "Core/Platform/CWindow.h"

namespace VKE
{
    namespace RenderSystem
    {

        CSwapChain::CSwapChain(CDevice* pDevice, CDeviceContext* pCtx) :
            m_pDevice(pDevice)
            , m_pCtx(pCtx)
        {
            m_vkDevice = m_pDevice->GetDevice();
            assert(m_vkDevice != VK_NULL_HANDLE);
        }

        CSwapChain::~CSwapChain()
        {
            Destroy();
        }

        void CSwapChain::Destroy()
        {
            auto& Device = m_pDevice->GetDeviceFunctions();

            for (uint32_t i = 0; i < m_Info.elementCount; ++i)
            {
                m_pCtx->DestroySemaphore(nullptr, &m_aSemaphores[i]);
                m_pCtx->DestroyImageView(nullptr, &m_aElements[i].vkImageView);
            }

            if (m_vkSwapChain != VK_NULL_HANDLE)
            {
                Device.vkDestroySwapchainKHR(m_vkDevice, m_vkSwapChain, nullptr);
                m_vkSwapChain = VK_NULL_HANDLE;
            }
        }

        Result CSwapChain::Create(const SSwapChainInfo& Info)
        {
            m_Info = Info;
            auto& Device = m_pDevice->GetDeviceFunctions();
            auto& Instance = m_pDevice->GetInstanceFunctions();
            const auto& queueIndex = m_pDevice->GetQueueIndex(CDevice::QueueTypes::GRAPHICS);

            VkPhysicalDevice vkPhysicalDevice = m_pDevice->GetPhysicalDevice();
            VkInstance vkInstance = m_pDevice->GetInstance();

            if (m_Info.hWnd == NULL_HANDLE)
            {
                auto pEngine = m_pDevice->GetRenderSystem()->GetEngine();
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
            VK_ERR(Instance.vkCreateWin32SurfaceKHR(vkInstance, &SurfaceCI, nullptr, &m_vkSurface));
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
            VK_ERR(Instance.vkGetPhysicalDeviceSurfaceSupportKHR(vkPhysicalDevice, queueIndex, m_vkSurface,
                &isSurfaceSupported));
            if(!isSurfaceSupported)
            {
                VKE_LOG_ERR("Queue index: " << queueIndex << " does not support the surface.");
                return VKE_FAIL;
            }

            Instance.vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vkPhysicalDevice, m_vkSurface, &m_vkSurfaceCaps);
            auto hasColorAttachment = m_vkSurfaceCaps.supportedUsageFlags | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            if (!hasColorAttachment)
            {
                VKE_LOG_ERR("Device surface has no color attachment.");
                return VKE_FAIL;
            }

            if (m_vkSurfaceCaps.maxImageCount == 0)
            {
                m_vkSurfaceCaps.maxImageCount = Constants::RenderSystem::MAX_SWAP_CHAIN_ELEMENTS;
            }

            if(Constants::OPTIMAL.IsOptimal(m_Info.elementCount))
            {
                m_Info.elementCount = Min(static_cast<uint16_t>(m_vkSurfaceCaps.maxImageCount), 3);
            }
            else
            {
                m_Info.elementCount = Min(m_Info.elementCount, static_cast<uint16_t>(m_vkSurfaceCaps.maxImageCount));
            }

            // Select surface format
            vke_vector<VkSurfaceFormatKHR> vSurfaceFormats;
            uint32_t formatCount = 0;
            VK_ERR(Instance.vkGetPhysicalDeviceSurfaceFormatsKHR(vkPhysicalDevice, m_vkSurface, &formatCount,
                nullptr));
            if (formatCount == 0)
            {
                VKE_LOG_ERR("No supported device surface formats.");
                return VKE_FAIL;
            }
            vSurfaceFormats.resize(formatCount);
            VK_ERR(Instance.vkGetPhysicalDeviceSurfaceFormatsKHR(vkPhysicalDevice, m_vkSurface, &formatCount,
                &vSurfaceFormats[0]));

            bool formatFound = false;
            for(auto& Format : vSurfaceFormats)
            {
                const auto& format = g_aFormats[m_Info.format];
                if (Format.format == format && Format.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR)
                {
                    m_vkSurfaceFormat = Format;
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
            VK_ERR(Instance.vkGetPhysicalDeviceSurfacePresentModesKHR(vkPhysicalDevice, m_vkSurface, &presentCount,
                nullptr));
            if(presentCount == 0)
            {
                VKE_LOG_ERR("No present modes supported.");
                return VKE_FAIL;
            }
            vPresents.resize(presentCount);
            VK_ERR(Instance.vkGetPhysicalDeviceSurfacePresentModesKHR(vkPhysicalDevice, m_vkSurface, &presentCount,
                &vPresents[0]));

            bool presentFound = false;
            for (auto& Present : vPresents)
            {
                if (Present == VK_PRESENT_MODE_IMMEDIATE_KHR || Present == VK_PRESENT_MODE_FIFO_KHR)
                {
                    m_vkPresentMode = Present;
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
            ci.imageColorSpace = m_vkSurfaceFormat.colorSpace;
            ci.imageExtent.width = m_Info.Size.width;
            ci.imageExtent.height = m_Info.Size.height;
            ci.imageFormat = m_vkSurfaceFormat.format;
            ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            ci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            ci.minImageCount = m_Info.elementCount;
            ci.oldSwapchain = VK_NULL_HANDLE;
            ci.pQueueFamilyIndices = &queueIndex;
            ci.queueFamilyIndexCount = 1;
            ci.presentMode = m_vkPresentMode;
            ci.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
            ci.surface = m_vkSurface;

            VK_ERR(Device.vkCreateSwapchainKHR(m_vkDevice, &ci, nullptr, &m_vkSwapChain));
            VKE_DEBUG_CODE(m_vkCreateInfo = ci);

            uint32_t imgCount = 0;
            Device.vkGetSwapchainImagesKHR(m_vkDevice, m_vkSwapChain, &imgCount, nullptr);
            if(imgCount != m_Info.elementCount)
            {
                VKE_LOG_ERR("Swap chain element count is different than requested.");
                return VKE_FAIL;
            }
            VkImage aImages[Constants::RenderSystem::MAX_SWAP_CHAIN_ELEMENTS];
            VK_ERR(Device.vkGetSwapchainImagesKHR(m_vkDevice, m_vkSwapChain, &imgCount, aImages));

            for(uint32_t i = 0; i < imgCount; ++i)
            {
                m_aElements[i].vkImage = aImages[i];
                VkImageViewCreateInfo ci;
                Vulkan::InitInfo(&ci, VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO);
                ci.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
                ci.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
                ci.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
                ci.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
                ci.flags = 0;
                ci.format = m_vkSurfaceFormat.format;
                ci.image = m_aElements[i].vkImage;
                ci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                ci.subresourceRange.baseArrayLayer = 0;
                ci.subresourceRange.baseMipLevel = 0;
                ci.subresourceRange.layerCount = 1;
                ci.subresourceRange.levelCount = 1;
                ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
                VK_ERR(Device.vkCreateImageView(m_vkDevice, &ci, nullptr, &m_aElements[i].vkImageView));

                VkSemaphoreCreateInfo SemaphoreCI;
                Vulkan::InitInfo(&SemaphoreCI, VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO);
                SemaphoreCI.flags = 0;
                VK_ERR(m_pCtx->CreateSemaphore(SemaphoreCI, nullptr, &m_aSemaphores[i]));
            }

            uint32_t currId = 0;
            for(uint32_t i = 0; i < imgCount; ++i)
            {
                VkSemaphore vkSemaphore = m_aSemaphores[i];
                VK_ERR(Device.vkAcquireNextImageKHR(m_vkDevice, m_vkSwapChain, UINT64_MAX, vkSemaphore, VK_NULL_HANDLE,
                    &currId));
                m_aElements[currId].vkSemaphore = vkSemaphore;
                m_aElementQueue[i] = currId;

                VkResult vkResult;
                VkPresentInfoKHR pi;
                Vulkan::InitInfo(&pi, VK_STRUCTURE_TYPE_PRESENT_INFO_KHR);
                pi.pResults = &vkResult;
                pi.pSwapchains = &m_vkSwapChain;
                pi.pWaitSemaphores = &vkSemaphore;
                pi.pImageIndices = &currId;
                pi.swapchainCount = 1;
                pi.waitSemaphoreCount = 1;
                VkQueue vkQueue = m_pDevice->GetQueue(queueIndex); // get the first queue
                Device.vkQueuePresentKHR(vkQueue, &pi);
            }

            return VKE_OK;
        }

        Result CSwapChain::GetNextElement()
        {
            auto& Device = m_pDevice->GetDeviceFunctions();
            uint32_t currId = m_aElementQueue[m_currElementId];
            auto vkSemaphore = m_aElements[currId].vkSemaphore;
            VK_ERR(Device.vkAcquireNextImageKHR(m_vkDevice, m_vkSwapChain, UINT64_MAX, vkSemaphore, VK_NULL_HANDLE,
                &currId));

            assert(currId == m_currElementId);
            if(++m_currElementId >= m_Info.elementCount)
                m_currElementId = 0;

            return VKE_OK;
        }

        void CSwapChain::BeginPresent()
        {
            GetNextElement();
        }

        void CSwapChain::EndPresent()
        {
            assert(m_vkCurrQueue != VK_NULL_HANDLE);
            auto& Device = m_pDevice->GetDeviceFunctions();
            const auto& CurrElement = GetCurrentElement();
            VkPresentInfoKHR PresentInfo;
            Vulkan::InitInfo(&PresentInfo, VK_STRUCTURE_TYPE_PRESENT_INFO_KHR);
            PresentInfo.pImageIndices = &m_currElementId;
            PresentInfo.pResults = nullptr;
            PresentInfo.pSwapchains = &m_vkSwapChain;
            PresentInfo.pWaitSemaphores = &CurrElement.vkSemaphore;
            PresentInfo.swapchainCount = 1;
            PresentInfo.waitSemaphoreCount = 1;

            VK_ERR(Device.vkQueuePresentKHR(m_vkCurrQueue, &PresentInfo));
        }

    } // RenderSystem
} // VKE