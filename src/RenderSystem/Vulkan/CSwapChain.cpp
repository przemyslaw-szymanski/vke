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

        struct CSwapChain::SPrivate
        {
            uint32_t                    aElementQueue[ Constants::RenderSystem::MAX_SWAP_CHAIN_ELEMENTS ] = { };

            Vulkan::ICD::Device*            pICD;
            Vulkan::CDeviceWrapper          DevWrapper;

            struct
            {
                VkSurfaceCapabilitiesKHR    vkSurfaceCaps;
                VkSurfaceKHR                vkSurface = VK_NULL_HANDLE;
                VkSurfaceFormatKHR          vkSurfaceFormat;
                VkPresentModeKHR            vkPresentMode;
                VkSwapchainKHR              vkSwapChain = VK_NULL_HANDLE;
                SQueue                      Queue;
                VkPresentInfoKHR            PresentInfo;
                uint32_t                    currElementId = 0;
                uint32_t                    currImageId = 0;
                SSwapChainElement*          pCurrElement = nullptr;
                SSwapChainElement           aElements[ Constants::RenderSystem::MAX_SWAP_CHAIN_ELEMENTS ];
                VkSemaphore                 aSemaphores[ Constants::RenderSystem::MAX_SWAP_CHAIN_ELEMENTS ] = { };

                struct
                {

                } Present;
            } Vulkan;

            SPrivate(VkDevice vkDevice, const VkICD::Device& ICD) :
                DevWrapper(vkDevice, ICD)
            {
                Memory::Zero(&Vulkan);
                auto& PresentInfo = Vulkan.PresentInfo;
                Vulkan::InitInfo(&PresentInfo, VK_STRUCTURE_TYPE_PRESENT_INFO_KHR);
                PresentInfo.pResults = nullptr;
                PresentInfo.swapchainCount = 1;
                PresentInfo.waitSemaphoreCount = 1;
            }
        };

        CSwapChain::CSwapChain(CGraphicsContext* pCtx) :
            m_pCtx(pCtx)
        {
        }

        CSwapChain::~CSwapChain()
        {
            Destroy();
        }

        void CSwapChain::Destroy()
        {
            auto& VulkanData = m_pPrivate->Vulkan;
            auto& DevWrapper = m_pPrivate->DevWrapper;
            auto& Device = m_pPrivate->pICD->Device;

            for (uint32_t i = 0; i < m_Desc.elementCount; ++i)
            {
                DevWrapper.DestroyObject(nullptr, &VulkanData.aSemaphores[i]);
                DevWrapper.DestroyObject(nullptr, &VulkanData.aElements[i].vkImageView);
            }

            if (VulkanData.vkSwapChain != VK_NULL_HANDLE)
            {
                Device.vkDestroySwapchainKHR(DevWrapper.GetDeviceHandle(), VulkanData.vkSwapChain, nullptr);
                VulkanData.vkSwapChain = VK_NULL_HANDLE;
            }

            Memory::DestroyObject(&HeapAllocator, &m_pPrivate);
        }

        Result CSwapChain::Create(const SSwapChainDesc& Desc)
        {
            auto pPrivate = reinterpret_cast< SSwapChainPrivateDesc* >( Desc.pPrivate );
            auto& Instance = pPrivate->pICD->Instance;
            auto& Device = pPrivate->pICD->Device;
            m_Desc = Desc;

            VKE_RETURN_IF_FAILED(Memory::CreateObject(&HeapAllocator, &m_pPrivate, pPrivate->vkDevice, Device));
            m_pPrivate->pICD = pPrivate->pICD;
            m_pPrivate->Vulkan.Queue = pPrivate->Queue;
            
            VkPhysicalDevice vkPhysicalDevice = pPrivate->vkPhysicalDevice;
            VkInstance vkInstance = pPrivate->vkInstance;
            const auto queueIndex = pPrivate->Queue.familyIndex;
            auto& VkInternal = m_pPrivate->Vulkan;

            if (m_Desc.hWnd == NULL_HANDLE)
            {
                auto pEngine = m_pCtx->GetDeviceContext()->GetRenderSystem()->GetEngine();
                auto pWnd = pEngine->GetWindow();
                m_Desc.hWnd = pWnd->GetInfo().wndHandle;
                m_Desc.hPlatform = pWnd->GetInfo().platformHandle;
            }
            
#if VKE_USE_VULKAN_WINDOWS
            if (!Instance.vkGetPhysicalDeviceWin32PresentationSupportKHR(vkPhysicalDevice, queueIndex))
            {
                VKE_LOG_ERR("Queue index: %d" << queueIndex << " does not support presentation");
                return VKE_FAIL;
            }
            HINSTANCE hInst = reinterpret_cast<HINSTANCE>(m_Desc.hPlatform);
            HWND hWnd = reinterpret_cast<HWND>(m_Desc.hWnd);
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

            if(Constants::OPTIMAL.IsOptimal(m_Desc.elementCount))
            {
                m_Desc.elementCount = Min(static_cast<uint16_t>( VkInternal.vkSurfaceCaps.maxImageCount), 3);
            }
            else
            {
                m_Desc.elementCount = Min(m_Desc.elementCount, static_cast<uint16_t>( VkInternal.vkSurfaceCaps.maxImageCount));
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
                //const auto& format = g_aFormats[m_Desc.format];
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
            ci.imageExtent.width = m_Desc.Size.width;
            ci.imageExtent.height = m_Desc.Size.height;
            ci.imageFormat = VkInternal.vkSurfaceFormat.format;
            ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            ci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            ci.minImageCount = m_Desc.elementCount;
            ci.oldSwapchain = VK_NULL_HANDLE;
            ci.pQueueFamilyIndices = &queueIndex;
            ci.queueFamilyIndexCount = 1;
            ci.presentMode = VkInternal.vkPresentMode;
            ci.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
            ci.surface = VkInternal.vkSurface;

            VkDevice vkDevice = m_pPrivate->DevWrapper.GetDeviceHandle();

            VK_ERR(Device.vkCreateSwapchainKHR(vkDevice, &ci, nullptr, &VkInternal.vkSwapChain));
            VKE_DEBUG_CODE(m_vkCreateInfo = ci);
            VkInternal.PresentInfo.pSwapchains = &VkInternal.vkSwapChain;

            uint32_t imgCount = 0;
            Device.vkGetSwapchainImagesKHR(vkDevice, VkInternal.vkSwapChain, &imgCount, nullptr);
            if(imgCount != m_Desc.elementCount)
            {
                VKE_LOG_ERR("Swap chain element count is different than requested.");
                return VKE_FAIL;
            }
            VkImage aImages[Constants::RenderSystem::MAX_SWAP_CHAIN_ELEMENTS];
            VK_ERR(Device.vkGetSwapchainImagesKHR(vkDevice, VkInternal.vkSwapChain, &imgCount,
                   aImages));

            auto& DevWrapper = m_pPrivate->DevWrapper;

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
                VK_ERR(DevWrapper.CreateObject(ci, nullptr, &VkInternal.aElements[i].vkImageView));

                VkSemaphoreCreateInfo SemaphoreCI;
                Vulkan::InitInfo(&SemaphoreCI, VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO);
                SemaphoreCI.flags = 0;
                VK_ERR(DevWrapper.CreateObject(SemaphoreCI, nullptr, &VkInternal.aSemaphores[i]));
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
            auto& VkInternal = m_pPrivate->Vulkan;

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
            auto& VkInternal = m_pPrivate->Vulkan;
            assert(VkInternal.vkCurrQueue != VK_NULL_HANDLE);

            auto& PresentInfo = VkInternal.PresentInfo;
            PresentInfo.pImageIndices = &VkInternal.currImageId;
            PresentInfo.pWaitSemaphores = &VkInternal.pCurrElement->vkSemaphore;

            VK_ERR(m_pDeviceCtx->QueuePresentKHR(VkInternal.vkCurrQueue, PresentInfo));

            VkInternal.currElementId++;
            VkInternal.currElementId %= m_Desc.elementCount;
        }

        const SSwapChainElement* CSwapChain::GetCurrentElement() const
        {
            return m_pPrivate->Vulkan.pCurrElement;
        }

    } // RenderSystem
} // VKE