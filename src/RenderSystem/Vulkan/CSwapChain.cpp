#include "CSwapChain.h"
#include "Vulkan.h"
#include "Core/Utils/CLogger.h"

#include "RenderSystem/CGraphicsContext.h"
#include "CDevice.h"
#include "CVkEngine.h"
#include "RenderSystem/Vulkan/CRenderSystem.h"
#include "Core/Platform/CWindow.h"
#include "Core/Memory/Memory.h"
#include "PrivateDescs.h"

#include "RenderSystem/CGraphicsContext.h"
#include "RenderSystem/CDeviceContext.h"

namespace VKE
{
    namespace RenderSystem
    {

        CSwapChain::CSwapChain(CGraphicsContext* pCtx) :
            m_pCtx(pCtx),
            m_ICD(m_pCtx->_GetICD()),
            m_Device(pCtx->GetDeviceContext()->_GetDevice(), m_pCtx->_GetICD().Device)
        {
        }

        CSwapChain::~CSwapChain()
        {
            Destroy();
        }

        void CSwapChain::Destroy()
        {
            for (uint32_t i = 0; i < m_Desc.elementCount; ++i)
            {
                //m_Device.DestroyObject(nullptr, &aSemaphores[i]);
                //m_Device.DestroyObject(nullptr, &aElements[i].vkImageView);
            }

            if ( m_vkSwapChain != VK_NULL_HANDLE )
            {
                m_ICD.Device.vkDestroySwapchainKHR(m_Device.GetDeviceHandle(), m_vkSwapChain, nullptr);
                m_vkSwapChain = VK_NULL_HANDLE;
            }
        }

        Result CSwapChain::Create(const SSwapChainDesc& Desc)
        {
            auto pPrivate = reinterpret_cast< SSwapChainPrivateDesc* >( Desc.pPrivate );
            m_Desc = Desc;
            m_vkPhysicalDevice = pPrivate->vkPhysicalDevice;
            m_vkInstance = pPrivate->vkInstance;
            m_vkQueue = pPrivate->Queue.vkQueue;
            m_queueFamilyIndex = pPrivate->Queue.familyIndex;

            if (m_Desc.hWnd == NULL_HANDLE)
            {
                auto pEngine = m_pCtx->GetDeviceContext()->GetRenderSystem()->GetEngine();
                auto pWnd = pEngine->GetWindow();
                m_Desc.hWnd = pWnd->GetInfo().wndHandle;
                m_Desc.hPlatform = pWnd->GetInfo().platformHandle;
            }
            
#if VKE_USE_VULKAN_WINDOWS
            if (!m_ICD.Instance.vkGetPhysicalDeviceWin32PresentationSupportKHR(m_vkPhysicalDevice, m_queueFamilyIndex))
            {
                VKE_LOG_ERR("Queue index: %d" << m_queueFamilyIndex << " does not support presentation");
                return VKE_FAIL;
            }
            HINSTANCE hInst = reinterpret_cast<HINSTANCE>(m_Desc.hPlatform);
            HWND hWnd = reinterpret_cast<HWND>(m_Desc.hWnd);
            VkWin32SurfaceCreateInfoKHR SurfaceCI;
            Vulkan::InitInfo(&SurfaceCI, VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR);
            SurfaceCI.flags = 0;
            SurfaceCI.hinstance = hInst;
            SurfaceCI.hwnd = hWnd;
            VK_ERR(m_ICD.Instance.vkCreateWin32SurfaceKHR(m_vkInstance, &SurfaceCI, nullptr, &m_vkSurface));
#elif VKE_USE_VULKAN_LINUX
            if (!Vk.vkGetPhysicalDeviceXcbPresentationSupportKHR(s_physical_device, s_queue_family_index, s_window.xcb_connection, s_window.visual_id))
            {
                DPF("vkGetPhysicalDeviceXcbPresentationSupportKHR returned FALSE, %d queue index does not support presentation on visual_id %x\n", s_queue_family_index, s_window.visual_id);
                return 0;
            }
            VkXcbSurfaceCreateInfoKHR SurfaceCI;
            Vulkan::InitInfo(&SurfaceCI, VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR);
            SurfaceCI.flags = 0;
            SurfaceCI.connection = reinterpret_cast<xcb_connection_t*>(m_Desc.hPlatform);
            SurfaceCI.window = m_Desc.hWnd;
            EXPECT_SUCCESS(Vk.vkCreateXcbSurfaceKHR(s_instance, &SurfaceCI, NO_ALLOC_CALLBACK, &s_surface))
#elif VKE_USE_VULKAN_ANDROID
            VkAndroidSurfaceCreateInfoKHR SurfaceCI;
            Vulkan::InitInfo(&SurfaceCI, VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR);
            SurfaceCI.flags = 0;
            SurfaceCI.window = m_Desc.hWnd;
            EXPECT_SUCCESS(Vk.vkCreateAndroidSurfaceKHR(s_instance, s_window.window->getNativeHandle(), NO_ALLOC_CALLBACK, &s_surface));
#endif

            VkBool32 isSurfaceSupported = VK_FALSE;
            VK_ERR(m_ICD.Instance.vkGetPhysicalDeviceSurfaceSupportKHR(m_vkPhysicalDevice, m_queueFamilyIndex,
                   m_vkSurface, &isSurfaceSupported));
            if(!isSurfaceSupported)
            {
                VKE_LOG_ERR("Queue index: " << m_queueFamilyIndex << " does not support the surface.");
                return VKE_FAIL;
            }

            m_ICD.Instance.vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_vkPhysicalDevice, m_vkSurface, &m_vkSurfaceCaps);
            auto hasColorAttachment = m_vkSurfaceCaps.supportedUsageFlags | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            if (!hasColorAttachment)
            {
                VKE_LOG_ERR("Device surface has no color attachment.");
                return VKE_FAIL;
            }

            if ( m_vkSurfaceCaps.maxImageCount == 0 )
            {
                m_vkSurfaceCaps.maxImageCount = Constants::RenderSystem::MAX_SWAP_CHAIN_ELEMENTS;
            }

            if(Constants::OPTIMAL.IsOptimal(m_Desc.elementCount))
            {
                m_Desc.elementCount = Min(static_cast<uint16_t>( m_vkSurfaceCaps.maxImageCount), 3);
            }
            else
            {
                m_Desc.elementCount = Min(m_Desc.elementCount, static_cast<uint16_t>( m_vkSurfaceCaps.maxImageCount));
            }

            // Select surface format
            vke_vector<VkSurfaceFormatKHR> vSurfaceFormats;
            uint32_t formatCount = 0;
            VK_ERR(m_ICD.Instance.vkGetPhysicalDeviceSurfaceFormatsKHR(m_vkPhysicalDevice, m_vkSurface, &formatCount,
                   nullptr));
            if (formatCount == 0)
            {
                VKE_LOG_ERR("No supported device surface formats.");
                return VKE_FAIL;
            }
            vSurfaceFormats.resize(formatCount);
            VK_ERR(m_ICD.Instance.vkGetPhysicalDeviceSurfaceFormatsKHR(m_vkPhysicalDevice, m_vkSurface, &formatCount,
                   &vSurfaceFormats[0]));

            bool formatFound = false;
            for(auto& Format : vSurfaceFormats)
            {
                //const auto& format = g_aFormats[m_Desc.format];
                if (Format.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR)
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
            VK_ERR(m_ICD.Instance.vkGetPhysicalDeviceSurfacePresentModesKHR(m_vkPhysicalDevice, m_vkSurface,
                   &presentCount, nullptr));
            if(presentCount == 0)
            {
                VKE_LOG_ERR("No present modes supported.");
                return VKE_FAIL;
            }
            vPresents.resize(presentCount);
            VK_ERR(m_ICD.Instance.vkGetPhysicalDeviceSurfacePresentModesKHR(m_vkPhysicalDevice, m_vkSurface,
                   &presentCount, &vPresents[0]));

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
            ci.imageExtent.width = m_Desc.Size.width;
            ci.imageExtent.height = m_Desc.Size.height;
            ci.imageFormat = m_vkSurfaceFormat.format;
            ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            ci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            ci.minImageCount = m_Desc.elementCount;
            ci.oldSwapchain = VK_NULL_HANDLE;
            ci.pQueueFamilyIndices = &m_queueFamilyIndex;
            ci.queueFamilyIndexCount = 1;
            ci.presentMode = m_vkPresentMode;
            ci.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
            ci.surface = m_vkSurface;

            VkDevice vkDevice = m_Device.GetDeviceHandle();

            VK_ERR(m_ICD.Device.vkCreateSwapchainKHR(vkDevice, &ci, nullptr, &m_vkSwapChain));
            VKE_DEBUG_CODE(m_vkCreateInfo = ci);
            m_PresentInfo.pSwapchains = &m_vkSwapChain;

            uint32_t imgCount = 0;
            m_ICD.Device.vkGetSwapchainImagesKHR(vkDevice, m_vkSwapChain, &imgCount, nullptr);
            if(imgCount != m_Desc.elementCount)
            {
                VKE_LOG_ERR("Swap chain element count is different than requested.");
                return VKE_FAIL;
            }
            VkImage aImages[Constants::RenderSystem::MAX_SWAP_CHAIN_ELEMENTS];
            VK_ERR(m_ICD.Device.vkGetSwapchainImagesKHR(vkDevice, m_vkSwapChain, &imgCount,
                   aImages));

            for(uint32_t i = 0; i < imgCount; ++i)
            {
                /*m_aElements[i].vkImage = aImages[i];
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
                VK_ERR(m_Device.CreateObject(ci, nullptr, &m_aElements[i].vkImageView));*/

                /*VkSemaphoreCreateInfo SemaphoreCI;
                Vulkan::InitInfo(&SemaphoreCI, VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO);
                SemaphoreCI.flags = 0;
                VK_ERR(m_Device.CreateObject(SemaphoreCI, nullptr, &m_aSemaphores[i]));
                m_aElements[ i ].vkSemaphore = m_aSemaphores[ i ];*/
            }

            return VKE_OK;
        }

        Result CSwapChain::Resize(uint32_t width, uint32_t height)
        {
            return VKE_OK;
        }

        Result CSwapChain::GetNextElement()
        {
            /*m_pCurrElement = &m_aElements[ m_currElementId ];
            auto vkSemaphore = m_pCurrElement->vkSemaphore;*/
            /*VK_ERR(m_pDeviceCtx->AcquireNextImageKHR(m_vkSwapChain, UINT64_MAX, vkSemaphore,
                   VK_NULL_HANDLE, &m_currImageId));*/

            return VKE_OK;
        }

        void CSwapChain::BeginPresent()
        {
            GetNextElement();
        }

        void CSwapChain::EndPresent()
        {
            /*auto& VkInternal = m_pPrivate->Vulkan;
            assert(m_vkCurrQueue != VK_NULL_HANDLE);

            auto& PresentInfo = m_PresentInfo;
            PresentInfo.pImageIndices = &m_currImageId;
            PresentInfo.pWaitSemaphores = &m_pCurrElement->vkSemaphore;

            VK_ERR(m_pDeviceCtx->QueuePresentKHR(m_vkCurrQueue, PresentInfo));

            m_currElementId++;
            m_currElementId %= m_Desc.elementCount;*/
        }

    } // RenderSystem
} // VKE