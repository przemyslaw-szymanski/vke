#if VKE_VULKAN_RENDERER
#include "RenderSystem/Vulkan/CSwapChain.h"
#include "RenderSystem/Vulkan/Vulkan.h"
#include "Core/Utils/CLogger.h"

#include "RenderSystem/CGraphicsContext.h"
#include "CVkEngine.h"
#include "RenderSystem/Vulkan/CRenderSystem.h"
#include "Core/Platform/CWindow.h"
#include "Core/Memory/Memory.h"
#include "RenderSystem/Vulkan/PrivateDescs.h"

#include "RenderSystem/CGraphicsContext.h"
#include "RenderSystem/CDeviceContext.h"

#include "RenderSystem/Vulkan/Wrappers/CCommandBuffer.h"
#include "RenderSystem/Vulkan/CResourceManager.h"
#include "RenderSystem/Vulkan/CRenderPass.h"

#include <iostream>

namespace VKE
{
    namespace RenderSystem
    {

        CSwapChain::CSwapChain(CGraphicsContext* pCtx) :
            m_pCtx(pCtx),
            m_ICD(m_pCtx->_GetICD()),
            m_VkDevice(pCtx->_GetDevice())
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
                //m_VkDevice.DestroyObject(nullptr, &aSemaphores[i]);
                //m_VkDevice.DestroyObject(nullptr, &aElements[i].vkImageView);
                //m_VkDevice.DestroyObject(nullptr, )
            }

            if ( m_vkSwapChain != VK_NULL_HANDLE )
            {
                m_ICD.Device.vkDestroySwapchainKHR(m_VkDevice.GetHandle(), m_vkSwapChain, nullptr);
                m_vkSwapChain = VK_NULL_HANDLE;
            }
        }

        Result CSwapChain::Create(const SSwapChainDesc& Desc)
        {
            auto pPrivate = reinterpret_cast< SSwapChainPrivateDesc* >( Desc.pPrivate );
            m_Desc = Desc;
            m_vkPhysicalDevice = pPrivate->vkPhysicalDevice;
            m_vkInstance = pPrivate->vkInstance;
            m_pQueue = pPrivate->pQueue;

            if( m_Desc.pWindow.IsNull() )
            {
                auto pEngine = m_pCtx->GetDeviceContext()->GetRenderSystem()->GetEngine();
                m_Desc.pWindow = pEngine->GetWindow();
            }
            const SWindowDesc& WndDesc = m_Desc.pWindow->GetDesc();

#if VKE_USE_VULKAN_WINDOWS
            HINSTANCE hInst = reinterpret_cast<HINSTANCE>( WndDesc.hProcess );
            HWND hWnd = reinterpret_cast<HWND>( WndDesc.hWnd );
            VkWin32SurfaceCreateInfoKHR SurfaceCI;
            Vulkan::InitInfo(&SurfaceCI, VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR);
            SurfaceCI.flags = 0;
            SurfaceCI.hinstance = hInst;
            SurfaceCI.hwnd = hWnd;
            VK_ERR(m_ICD.Instance.vkCreateWin32SurfaceKHR(m_vkInstance, &SurfaceCI, nullptr, &m_vkSurface));
#elif VKE_USE_VULKAN_LINUX
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
            const auto queueIndex = m_pQueue->familyIndex;
            VK_ERR(m_ICD.Instance.vkGetPhysicalDeviceSurfaceSupportKHR(m_vkPhysicalDevice, queueIndex,
                   m_vkSurface, &isSurfaceSupported));
            if(!isSurfaceSupported)
            {
                VKE_LOG_ERR("Queue index: " << queueIndex << " does not support the surface.");
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
                m_Desc.elementCount = Min(static_cast<uint16_t>( m_vkSurfaceCaps.maxImageCount), 2);
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

            // Render pass
            {

                VkAttachmentReference ColorAttachmentRef;
                ColorAttachmentRef.attachment = 0;
                ColorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

                VkSubpassDescription SubPassDesc = {};
                SubPassDesc.colorAttachmentCount = 1;
                SubPassDesc.pColorAttachments = &ColorAttachmentRef;
                SubPassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

                VkAttachmentDescription AtDesc = {};
                AtDesc.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                AtDesc.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                AtDesc.format = m_vkSurfaceFormat.format;
                AtDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                AtDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                AtDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                AtDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                AtDesc.samples = VK_SAMPLE_COUNT_1_BIT;

                m_VkDevice.DestroyObject(nullptr, &m_vkRenderPass);
                VkRenderPassCreateInfo ci;
                Vulkan::InitInfo(&ci, VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO);
                ci.flags = 0;
                ci.attachmentCount = 1;
                ci.pAttachments = &AtDesc;
                ci.pDependencies = nullptr;
                ci.pSubpasses = &SubPassDesc;
                ci.subpassCount = 1;
                ci.dependencyCount = 0;
                VK_ERR(m_VkDevice.CreateObject(ci, nullptr, &m_vkRenderPass));
            }
            
            Result res = Resize(m_Desc.Size.width, m_Desc.Size.height);
            if( VKE_SUCCEEDED( res ) )
            {
                // Set the current back buffer
                //res = GetNextBackBuffer();
                res = SwapBuffers();
            }
            return res;
        }

        Result CSwapChain::Resize(uint32_t width, uint32_t height)
        {
            m_ICD.Instance.vkGetPhysicalDeviceSurfaceCapabilitiesKHR( m_vkPhysicalDevice, m_vkSurface, &m_vkSurfaceCaps );
            auto hasColorAttachment = m_vkSurfaceCaps.supportedUsageFlags | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            if( !hasColorAttachment )
            {
                VKE_LOG_ERR( "Device surface has no color attachment." );
                return VKE_FAIL;
            }

            width = m_vkSurfaceCaps.currentExtent.width;
            height = m_vkSurfaceCaps.currentExtent.height;

            VkSwapchainKHR vkCurrSwapChain = m_vkSwapChain;
            VkSwapchainCreateInfoKHR ci;
            Vulkan::InitInfo(&ci, VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR);
            ci.clipped = false;
            ci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
            ci.flags = 0;
            ci.imageArrayLayers = 1;
            ci.imageColorSpace = m_vkSurfaceFormat.colorSpace;
            ci.imageExtent.width = width;
            ci.imageExtent.height = height;
            ci.imageFormat = m_vkSurfaceFormat.format;
            ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            ci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            ci.minImageCount = m_Desc.elementCount;
            ci.oldSwapchain = vkCurrSwapChain;
            ci.pQueueFamilyIndices = &m_pQueue->familyIndex;
            ci.queueFamilyIndexCount = 1;
            ci.presentMode = m_vkPresentMode;
            ci.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
            ci.surface = m_vkSurface;

            VkDevice vkDevice = m_VkDevice.GetHandle();

            VK_ERR(m_VkDevice.CreateObject(ci, nullptr, &m_vkSwapChain));
            VKE_DEBUG_CODE(m_vkCreateInfo = ci);
            m_PresentInfo.pSwapchains = &m_vkSwapChain;

            m_VkDevice.DestroyObject(nullptr, &vkCurrSwapChain);

            uint32_t imgCount = 0;
            m_ICD.Device.vkGetSwapchainImagesKHR(vkDevice, m_vkSwapChain, &imgCount, nullptr);
            if( imgCount != m_Desc.elementCount )
            {
                VKE_LOG_ERR("Swap chain element count is different than requested.");
                return VKE_FAIL;
            }
            Utils::TCDynamicArray< VkImage > vImages(imgCount);
            VK_ERR(m_ICD.Device.vkGetSwapchainImagesKHR(vkDevice, m_vkSwapChain, &imgCount,
                   &vImages[ 0 ]));

            //auto pImg1 = vImages[ 0 ];
            //auto pImg2 = vImages[ 1 ];
            // $TID: CreateSwapChain: sc={(void*)this}, img0={pImg1}, img1={pImg2}
            m_vAcquireElements.Resize(imgCount);
            if( m_vBackBuffers.IsEmpty() )
            {
                if( !m_vBackBuffers.Resize(imgCount) )
                {
                    return VKE_ENOMEMORY;
                }
                for( auto& BackBuffer : m_vBackBuffers )
                {
                    VkSemaphoreCreateInfo ci;
                    Vulkan::InitInfo(&ci, VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO);
                    ci.flags = 0;
                    VK_ERR(m_VkDevice.CreateObject(ci, nullptr, &BackBuffer.vkAcquireSemaphore));
                    VK_ERR(m_VkDevice.CreateObject(ci, nullptr, &BackBuffer.vkCmdBufferSemaphore));
                }
            }

            VkCommandBuffer vkTmpCb = m_pCtx->_CreateCommandBuffer();
            Vulkan::Wrapper::CCommandBuffer CmdBuffer(m_VkDevice.GetICD(), vkTmpCb);
            CmdBuffer.Begin();
            VkImageSubresourceRange SubresRange;
            SubresRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            SubresRange.baseArrayLayer = 0;
            SubresRange.baseMipLevel = 0;
            SubresRange.layerCount = 1;
            SubresRange.levelCount = 1;

            auto& ResMgr = m_pCtx->GetDeviceContext()->GetResourceManager();

            for( uint32_t i = 0; i < imgCount; ++i )
            {
                auto& Element = m_vAcquireElements[ i ];
                //m_VkDevice.DestroyObject(nullptr, &Element.vkImage);
                //m_VkDevice.DestroyObject(nullptr, &Element.vkFramebuffer);
                if( Element.vkCbAttachmentToPresent == VK_NULL_HANDLE )
                {
                    //Element.vkCbAttachmentToPresent = m_pCtx->_CreateCommandBuffer(RenderQueueUsages::STATIC);
                    //Element.vkCbPresentToAttachment = m_pCtx->_CreateCommandBuffer(RenderQueueUsages::STATIC);
                }

                Element.vkImage = vImages[ i ];
                {
                    TextureViewHandle hView;
                    {
                        /*VkImageViewCreateInfo ci;
                        Vulkan::InitInfo(&ci, VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO);
                        ci.components.r = VK_COMPONENT_SWIZZLE_R;
                        ci.components.g = VK_COMPONENT_SWIZZLE_G;
                        ci.components.b = VK_COMPONENT_SWIZZLE_B;
                        ci.components.a = VK_COMPONENT_SWIZZLE_A;
                        ci.flags = 0;
                        ci.format = m_vkSurfaceFormat.format;
                        ci.image = Element.vkImage;
                        ci.subresourceRange = SubresRange;
                        ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
                        VK_ERR(m_VkDevice.CreateObject(ci, nullptr, &Element.vkImageView));*/
                        
                        // Add image
                        VkImageCreateInfo ci;
                        Vulkan::InitInfo(&ci, VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO);
                        ci.arrayLayers = 1;
                        ci.extent.width = width;
                        ci.extent.height = height;
                        ci.extent.depth = 1;
                        ci.flags = 0;
                        ci.format = m_vkSurfaceFormat.format;
                        ci.imageType = VK_IMAGE_TYPE_2D;
                        ci.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
                        ci.mipLevels = 1;
                        ci.pQueueFamilyIndices = nullptr;
                        ci.queueFamilyIndexCount = 0;
                        ci.samples = VK_SAMPLE_COUNT_1_BIT;
                        ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                        ci.tiling = VK_IMAGE_TILING_OPTIMAL;
                        ci.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
                        auto res = ResMgr.AddTexture(Element.vkImage, ci);
                        assert(res == VKE_OK);

                        STextureViewDesc Desc;
                        Desc.aspect = TextureAspects::COLOR;
                        Desc.beginMipmapLevel = 0;
                        Desc.endMipmapLevel = 1;
                        Desc.format = Vulkan::Convert::ImageFormat(m_vkSurfaceFormat.format);
                        Desc.hTexture.SetNative( Element.vkImage );
                        Desc.type = TextureViewTypes::VIEW_2D;
                        hView = ResMgr.CreateTextureView(Desc, &Element.vkImageView);
                    }
                    {
                        /*VkFramebufferCreateInfo ci;
                        Vulkan::InitInfo(&ci, VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO);
                        ci.flags = 0;
                        ci.width = width;
                        ci.height = height;
                        ci.layers = 1;
                        ci.renderPass = m_vkRenderPass;
                        ci.attachmentCount = 1;
                        ci.pAttachments = &Element.vkImageView;
                        VK_ERR(m_VkDevice.CreateObject(ci, nullptr, &Element.vkFramebuffer));*/

                        {
                            /*SRenderTargetDesc::SWriteAttachmentDesc Attachment;
                            Attachment.hTextureView = hView;
                            Attachment.usage = RenderTargetAttachmentUsages::Write::COLOR_CLEAR_STORE;
                            SRenderTargetDesc RtDesc;
                            RtDesc.vWriteAttachments.PushBack(Attachment);
                            RtDesc.Size.width = width;
                            RtDesc.Size.height = height;
                            if( Element.hRenderTarget == NULL_HANDLE )
                            {
                                Element.hRenderTarget = m_pCtx->GetDeviceContext()->CreateRenderTarget(RtDesc);
                                Element.pRenderTarget = m_pCtx->GetDeviceContext()->GetRenderTarget(Element.hRenderTarget);
                            }
                            else
                            {
                                m_pCtx->GetDeviceContext()->UpdateRenderTarget(Element.hRenderTarget, RtDesc);
                            }*/
                            SRenderPassAttachmentDesc Attachment;
                            Attachment.hTextureView = hView;
                            Attachment.usage = RenderPassAttachmentUsages::COLOR_CLEAR_STORE;
                            Attachment.beginLayout = TextureLayouts::COLOR_RENDER_TARGET;
                            Attachment.endLayout = TextureLayouts::COLOR_RENDER_TARGET;
                            SRenderPassDesc RpDesc;
                            RpDesc.Size.width = static_cast< uint16_t >( width );
                            RpDesc.Size.height = static_cast< uint16_t >( height );
                            RpDesc.vAttachments.PushBack(Attachment);
                            SRenderPassDesc::SSubpassDesc SpDesc;
                            {
                                SRenderPassDesc::SSubpassDesc::SAttachmentDesc AtDesc;
                                AtDesc.hTextureView = hView;
                                AtDesc.layout = TextureLayouts::COLOR_RENDER_TARGET;
                                SpDesc.vRenderTargets.PushBack(AtDesc);
                            }
                            RpDesc.vSubpasses.PushBack(SpDesc);

                            if( Element.hRenderPass == NULL_HANDLE )
                            {
                                Element.hRenderPass = m_pCtx->GetDeviceContext()->CreateRenderPass(RpDesc);
                                Element.pRenderPass = m_pCtx->GetDeviceContext()->GetRenderPass(Element.hRenderPass);
                                // $TID scCreateRenderPass: sc={(void*)this}, hrp={Element.hRenderPass}, rp={(void*)Element.pRenderPass}, img={Element.vkImage}
                            }
                            else
                            {
                                //m_pCtx->GetDeviceContext()->Update
                            }
                        }
                    }

                    // Set memory barrier
                    VkImageMemoryBarrier ImgBarrier;
                    Vulkan::InitInfo(&ImgBarrier, VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER);
                    ImgBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                    ImgBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
                    ImgBarrier.srcAccessMask = 0;
                    ImgBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
                    ImgBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    ImgBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    ImgBarrier.image = Element.vkImage;
                    ImgBarrier.subresourceRange = SubresRange;

                    CmdBuffer.PipelineBarrier(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                              VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                              0,
                                              0, nullptr,
                                              0, nullptr,
                                              1, &ImgBarrier);
                    Element.vkCurrLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
                }
            }

            CmdBuffer.End();
            CGraphicsContext::CommandBufferArray vCmdBuffers;
            vCmdBuffers.PushBack(CmdBuffer.GetHandle());
            m_pCtx->_SubmitCommandBuffers(vCmdBuffers, VK_NULL_HANDLE);

            // Prepare static command buffers with layout transitions
            for( uint32_t i = 0; i < imgCount; ++i )
            {
                auto& Element = m_vAcquireElements[ i ];
                          
                {
                    //Element.vkCbAttachmentToPresent = m_pCtx->_CreateCommandBuffer();
                    //Vulkan::Wrapper::CCommandBuffer Cb(m_VkDevice.GetICD(), Element.vkCbAttachmentToPresent);
                    //Cb.Begin(VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT);
                    // Set memory barrier
                    VkImageMemoryBarrier& ImgBarrier = Element.vkBarrierAttachmentToPresent;
                    Vulkan::InitInfo(&ImgBarrier, VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER);
                    ImgBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                    ImgBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
                    ImgBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                    ImgBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
                    ImgBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    ImgBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    ImgBarrier.image = Element.vkImage;
                    ImgBarrier.subresourceRange = SubresRange;

                    /*Cb.PipelineBarrier(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                       VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                                       0,
                                       0, nullptr,
                                       0, nullptr,
                                       1, &ImgBarrier);

                    
                    Cb.End();*/
                }
                {
                    //Element.vkCbPresentToAttachment = m_pCtx->_CreateCommandBuffer();
                    //Vulkan::Wrapper::CCommandBuffer Cb(m_VkDevice.GetICD(), Element.vkCbPresentToAttachment);
                    //Cb.Begin(VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT);
                    // Set memory barrier
                    VkImageMemoryBarrier& ImgBarrier = Element.vkBarrierPresentToAttachment;
                    Vulkan::InitInfo(&ImgBarrier, VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER);
                    ImgBarrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
                    ImgBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                    ImgBarrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
                    ImgBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                    ImgBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    ImgBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    ImgBarrier.image = Element.vkImage;
                    ImgBarrier.subresourceRange = SubresRange;

                    /*Cb.PipelineBarrier(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                       VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                       0,
                                       0, nullptr,
                                       0, nullptr,
                                       1, &ImgBarrier);
                    Cb.End();*/
                }
            }

            m_pCtx->Wait();
            m_pCtx->_FreeCommandBuffer(CmdBuffer.GetHandle());

            return VKE_OK;
        }

        Result CSwapChain::GetNextBackBuffer()
        {
            m_currBackBufferIdx++;
            m_currBackBufferIdx %= m_Desc.elementCount;
            m_pCurrBackBuffer = &m_vBackBuffers[ m_currBackBufferIdx ];
            return VKE_OK;
        }

        /*void CSwapChain::EndPresent()
        {
            auto& VkInternal = m_pPrivate->Vulkan;
            assert(m_vkCurrQueue != VK_NULL_HANDLE);

            auto& PresentInfo = m_PresentInfo;
            PresentInfo.pImageIndices = &m_currImageId;
            PresentInfo.pWaitSemaphores = &m_pCurrElement->vkSemaphore;

            VK_ERR(m_pDeviceCtx->QueuePresentKHR(m_vkCurrQueue, PresentInfo));

            m_currElementId++;
            m_currElementId %= m_Desc.elementCount;
        }*/

        Result CSwapChain::SwapBuffers()
        {
            m_pCtx->_AddToPresent(this);
            GetNextBackBuffer();
            VkSemaphore& vkSemaphore = m_pCurrBackBuffer->vkAcquireSemaphore;
            VK_ERR(m_VkDevice.AcquireNextImageKHR(m_vkSwapChain, UINT64_MAX, vkSemaphore,
                   VK_NULL_HANDLE, &m_pCurrBackBuffer->currImageIdx));
            //const auto& e1 = m_vAcquireElements[ 0 ];
            //const auto& e2 = m_vAcquireElements[ 1 ];
            m_pCurrAcquireElement = &m_vAcquireElements[ m_pCurrBackBuffer->currImageIdx ];
            // $TID SwapBuffers: sc={(void*)this}, imgIdx={m_pCurrBackBuffer->currImageIdx}, img={m_pCurrAcquireElement->vkImage}, {m_pCurrAcquireElement->vkOldLayout}, {m_pCurrAcquireElement->vkCurrLayout}, as={vkSemaphore}
            return VKE_OK;
        }

        ExtentU32 CSwapChain::GetSize() const
        {
            ExtentU32 Size = { m_vkSurfaceCaps.currentExtent.width, m_vkSurfaceCaps.currentExtent.height };
            return Size;
        }

        void CSwapChain::BeginFrame(VkCommandBuffer vkCb)
        {
            Vulkan::Wrapper::CCommandBuffer Cb(m_VkDevice.GetICD(), vkCb);
            Cb.PipelineBarrier(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                               VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                               0,
                               0, nullptr,
                               0, nullptr,
                               1, &m_pCurrAcquireElement->vkBarrierPresentToAttachment);
            // $TID scBeginFrame: sc={(void*)this}, cb={(void*)vkCb}, img={m_pCurrAcquireElement->vkImage}, {m_pCurrAcquireElement->vkOldLayout}->{m_pCurrAcquireElement->vkCurrLayout}
            m_pCurrAcquireElement->vkOldLayout = m_pCurrAcquireElement->vkBarrierPresentToAttachment.oldLayout;
            m_pCurrAcquireElement->vkCurrLayout = m_pCurrAcquireElement->vkBarrierPresentToAttachment.newLayout;
        }

        void CSwapChain::EndFrame(VkCommandBuffer vkCb)
        {
            Vulkan::Wrapper::CCommandBuffer Cb(m_VkDevice.GetICD(), vkCb);
            Cb.PipelineBarrier(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                               VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                               0,
                               0, nullptr,
                               0, nullptr,
                               1, &m_pCurrAcquireElement->vkBarrierAttachmentToPresent);
            // $TID scEndFrame: sc={(void*)this}, cb={(void*)vkCb}, img={m_pCurrAcquireElement->vkImage}, {m_pCurrAcquireElement->vkOldLayout}->{m_pCurrAcquireElement->vkCurrLayout}
            m_pCurrAcquireElement->vkOldLayout = m_pCurrAcquireElement->vkBarrierAttachmentToPresent.oldLayout;
            m_pCurrAcquireElement->vkCurrLayout = m_pCurrAcquireElement->vkBarrierAttachmentToPresent.newLayout;
        }

        void CSwapChain::BeginPass(VkCommandBuffer vkCb)
        {

            /*VkImage vkImg = m_pCurrAcquireElement->vkImage;
            VkImageSubresourceRange Range;
            Range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            Range.baseArrayLayer = 0;
            Range.baseMipLevel = 0;
            Range.layerCount = 1;
            Range.levelCount = 1;
            VkClearColorValue Color;
            Color.float32[ 0 ] = 1.0f;
            Color.float32[ 1 ] = 0.5f;
            Color.float32[ 2 ] = 0.3f;
            Color.float32[ 3 ] = 1.0f;
            m_VkDevice.GetICD().vkCmdClearColorImage(vkCb, vkImg,
                   VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, &Color, 1, &Range);*/

            const auto Size = GetSize();


            VkClearValue ClearValue;
            ClearValue.color.float32[ 0 ] = ( float )( rand() % 100 ) / 100.0f;
            ClearValue.color.float32[ 1 ] = ( float )( rand() % 100 ) / 100.0f;
            ClearValue.color.float32[ 2 ] = ( float )(rand() % 100) / 100.0f;
            ClearValue.color.float32[ 3 ] = ( float )(rand() % 100) / 100.0f;
            ClearValue.depthStencil.depth = 0.0f;

            VkRenderPassBeginInfo bi;
            Vulkan::InitInfo(&bi, VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO);
            //bi.framebuffer = m_pCurrAcquireElement->vkFramebuffer;
            //bi.renderPass = m_vkRenderPass;
            bi.framebuffer = m_pCurrAcquireElement->pRenderPass->m_vkFramebuffer;
            bi.renderPass = m_pCurrAcquireElement->pRenderPass->m_vkRenderPass;
            bi.pClearValues = &ClearValue;
            bi.clearValueCount = 1;
            bi.renderArea.extent.width = Size.width;
            bi.renderArea.extent.height = Size.height;
            bi.renderArea.offset.x = 0;
            bi.renderArea.offset.y = 0;
            
            m_VkDevice.GetICD().vkCmdBeginRenderPass(vkCb, &bi, VK_SUBPASS_CONTENTS_INLINE);
        }

        void CSwapChain::EndPass(VkCommandBuffer vkCb)
        {
            m_VkDevice.GetICD().vkCmdEndRenderPass(vkCb);
        }

    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER