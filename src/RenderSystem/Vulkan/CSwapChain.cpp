#if VKE_VULKAN_RENDERER
#include "RenderSystem/Vulkan/CSwapChain.h"
#include "RenderSystem/Vulkan/Vulkan.h"
#include "Core/Utils/CLogger.h"

#include "RenderSystem/CDeviceContext.h"
#include "CVkEngine.h"
#include "RenderSystem/Vulkan/CRenderSystem.h"
#include "Core/Platform/CWindow.h"
#include "Core/Memory/Memory.h"
#include "RenderSystem/Vulkan/PrivateDescs.h"

#include "RenderSystem/CGraphicsContext.h"
#include "RenderSystem/CDeviceContext.h"

#include "RenderSystem/Vulkan/Wrappers/CCommandBuffer.h"
#include "RenderSystem/Managers/CAPIResourceManager.h"
#include "RenderSystem/Vulkan/CRenderPass.h"
#include "RenderSystem/Vulkan/CRenderingPipeline.h"
#include "RenderSystem/Managers/CBackBufferManager.h"

#include <iostream>

namespace VKE
{
    namespace RenderSystem
    {

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
            Memory::DestroyObject( &HeapAllocator, &m_pBackBufferMgr );
            m_pCtx->GetDeviceContext()->_GetDDI().DestroySwapChain( &m_SwapChain, nullptr );
        }

        Result CSwapChain::Create(const SSwapChainDesc& Desc)
        {
            Result ret = VKE_OK;
            m_Desc = Desc;
            m_Desc.pPrivate = &m_DDIDesc;

            if( m_Desc.pWindow.IsNull() )
            {
                auto pEngine = m_pCtx->GetDeviceContext()->GetRenderSystem()->GetEngine();
                m_Desc.pWindow = pEngine->GetWindow();
            }
            const SWindowDesc& WndDesc = m_Desc.pWindow->GetDesc();

            ret = m_pCtx->GetDeviceContext()->_GetDDI().CreateSwapChain( m_Desc, nullptr, &m_SwapChain );
            if( VKE_FAILED( ret ) )
            {
                goto ERR;
            }

            if( Constants::OPTIMAL.IsOptimal( m_Desc.elementCount ) )
            {
                m_Desc.elementCount = Min( static_cast<uint16_t>(m_PresentSurfaceCaps.maxImageCount), 2 );
            }
            else
            {
                m_Desc.elementCount = Min( m_Desc.elementCount, static_cast<uint16_t>(m_PresentSurfaceCaps.maxImageCount) );
            }


            /// @todo check for fullscreen if format is 32bit

            ret = Memory::CreateObject( &HeapAllocator, &m_pBackBufferMgr, m_pCtx );
            if( VKE_SUCCEEDED( ret ) )
            {
                Managers::SBackBufferManagerDesc Desc;
                Desc.backBufferCount = m_Desc.elementCount;
                if( VKE_SUCCEEDED( m_pBackBufferMgr->Create( Desc ) ) )
                {
                    SBackBuffer aData[Config::MAX_BACK_BUFFER_COUNT];
                    m_backBufferIdx = m_pBackBufferMgr->AddCustomData( reinterpret_cast<uint8_t*>(aData), sizeof( SBackBuffer ) );
                }
                else
                {
                    goto ERR;
                }
            }
            else
            {
                VKE_LOG_ERR( "Unable to allocate memory for BackBufferManager object." );
                goto ERR;
            }

            // Render pass
            {
                /// @TODO: Use DDI
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
                AtDesc.format = Vulkan::Map::Format( m_Desc.format );
                AtDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                AtDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                AtDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                AtDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                AtDesc.samples = VK_SAMPLE_COUNT_1_BIT;

                m_pCtx->GetDeviceContext()->_GetDDI().DestroyObject( &m_vkRenderPass, nullptr );
                VkRenderPassCreateInfo ci;
                Vulkan::InitInfo( &ci, VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO );
                ci.flags = 0;
                ci.attachmentCount = 1;
                ci.pAttachments = &AtDesc;
                ci.pDependencies = nullptr;
                ci.pSubpasses = &SubPassDesc;
                ci.subpassCount = 1;
                ci.dependencyCount = 0;
                m_pCtx->GetDeviceContext()->_GetDDI().GetICD().vkCreateRenderPass( m_pCtx->GetDeviceContext()->_GetDDI().GetDevice(), &ci, nullptr, &m_vkRenderPass );
            }

            ret = _CreateBackBuffers( m_Desc.elementCount );
            if( VKE_SUCCEEDED( ret ) )
            {
                ret = Resize( m_Desc.Size.width, m_Desc.Size.height );
                if( VKE_SUCCEEDED( ret ) )
                {
                    // Set the current back buffer
                    //res = GetNextBackBuffer();
                    ret = SwapBuffers();
                }
                else
                {
                    goto ERR;
                }
            }
            
            return ret;

        ERR:
            Destroy();
            return ret;
        }

        Result CSwapChain::_CreateBackBuffers(uint32_t count)
        {
            Result ret = VKE_OK;
            if( m_vBackBuffers.IsEmpty() )
            {
                if( !m_vBackBuffers.Resize( count ) )
                {
                    ret = VKE_ENOMEMORY;
                }
                else
                {
                    /*CommandBufferPtr pCmdBuffer = m_pCtx->CreateCommandBuffer();
                    pCmdBuffer->Begin();*/
                    VkImageSubresourceRange SubresRange;
                    SubresRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    SubresRange.baseArrayLayer = 0;
                    SubresRange.baseMipLevel = 0;
                    SubresRange.layerCount = 1;
                    SubresRange.levelCount = 1;

                    const uint32_t imgCount = m_SwapChain.vImages.GetCount();
                    const auto& vImages = m_SwapChain.vImages;
                    const auto& vImageViews = m_SwapChain.vImageViews;

                    for( uint32_t i = 0; i < imgCount; ++i )
                    {
                        auto& BackBuffer = m_vBackBuffers[i];
                        SAcquireElement& Element = m_vBackBuffers[i].AcquiredElement;
                        Element.vkImage = vImages[i];
                        Element.vkImageView = vImageViews[i];

                        {
                            SSemaphoreDesc Desc;
                            BackBuffer.vkAcquireSemaphore = m_pCtx->GetDeviceContext()->_GetDDI().CreateObject( Desc, nullptr );
                            BackBuffer.vkCmdBufferSemaphore = m_pCtx->GetDeviceContext()->_GetDDI().CreateObject( Desc, nullptr );
                            if( BackBuffer.vkAcquireSemaphore == DDI_NULL_HANDLE ||
                                BackBuffer.vkCmdBufferSemaphore == DDI_NULL_HANDLE )
                            {
                                ret = VKE_FAIL;
                                break;
                            }
                        }
                        {
                            if( Element.hRenderPass == NULL_HANDLE )
                            {
                                SRenderPassAttachmentDesc Attachment;
                                Attachment.usage = RenderPassAttachmentUsages::COLOR_CLEAR_STORE;
                                Attachment.beginLayout = TextureLayouts::COLOR_RENDER_TARGET;
                                Attachment.endLayout = TextureLayouts::COLOR_RENDER_TARGET;
                                Attachment.hTextureView.handle = reinterpret_cast<handle_t>(Element.vkImageView);
                                SRenderPassDesc RpDesc;
                                RpDesc.Size = m_Desc.Size;
                                RpDesc.vAttachments.PushBack( Attachment );
                                SRenderPassDesc::SSubpassDesc SpDesc;
                                {
                                    SRenderPassDesc::SSubpassDesc::SAttachmentDesc AtDesc;
                                    AtDesc.hTextureView.handle = reinterpret_cast<handle_t>(Element.vkImageView);
                                    AtDesc.layout = TextureLayouts::COLOR_RENDER_TARGET;
                                    SpDesc.vRenderTargets.PushBack( AtDesc );
                                }
                                RpDesc.vSubpasses.PushBack( SpDesc );
                                //m_pCtx->GetDeviceContext()->_GetDDI().CreateObject( RpDesc, nullptr );
                                RenderPassHandle hPass = m_pCtx->GetDeviceContext()->_CreateRenderPass( RpDesc, true );
                                Element.hRenderPass = hPass;
                                    //Element.hRenderPass = m_pCtx->GetDeviceContext()->CreateRenderPass(RpDesc);
                                    //Element.pRenderPass = m_pCtx->GetDeviceContext()->GetRenderPass(Element.hRenderPass);
                                    // $TID scCreateRenderPass: sc={(void*)this}, hrp={Element.hRenderPass}, rp={(void*)Element.pRenderPass}, img={Element.vkImage
                            }
                        }
                        {
                            VkImageMemoryBarrier& ImgBarrier = Element.vkBarrierAttachmentToPresent;
                            Vulkan::InitInfo( &ImgBarrier, VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER );
                            ImgBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                            ImgBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
                            ImgBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                            ImgBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
                            ImgBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                            ImgBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                            ImgBarrier.image = Element.vkImage;
                            ImgBarrier.subresourceRange = SubresRange;
                        }
                        {
                            VkImageMemoryBarrier& ImgBarrier = Element.vkBarrierPresentToAttachment;
                            Vulkan::InitInfo( &ImgBarrier, VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER );
                            ImgBarrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
                            ImgBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                            ImgBarrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
                            ImgBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                            ImgBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                            ImgBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                            ImgBarrier.image = Element.vkImage;
                            ImgBarrier.subresourceRange = SubresRange;
                        }
                    }
                }
            }
            if( ret == VKE_OK )
            {
                m_pBackBufferMgr->UpdateCustomData( m_backBufferIdx, &m_vBackBuffers[0] );
            }
            return ret;
        }

        Result CSwapChain::Resize(uint32_t width, uint32_t height)
        {
            Result ret = VKE_FAIL;
            // Do nothing if size is not changed
            if( m_SwapChain.Size.width == width && m_SwapChain.Size.height == height )
            {
                return VKE_OK;
            }
            /*m_ICD.Instance.vkGetPhysicalDeviceSurfaceCapabilitiesKHR( m_vkPhysicalDevice, m_vkSurface, &m_vkSurfaceCaps );
            auto hasColorAttachment = m_vkSurfaceCaps.supportedUsageFlags | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            if( !hasColorAttachment )
            {
                VKE_LOG_ERR( "Device surface has no color attachment." );
                return VKE_FAIL;
            }*/
            /*if( !m_PresentSurfaceCaps.canBeUsedAsRenderTarget )
            {
                VKE_LOG_ERR( "Device present surface has no color attachment." );
                goto ERR;
            }

            width = m_PresentSurfaceCaps.CurrentSize.width;
            height = m_PresentSurfaceCaps.CurrentSize.height;*/

            /*VkSwapchainKHR vkCurrSwapChain = m_vkSwapChain;
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

            m_VkDevice.DestroyObject(nullptr, &vkCurrSwapChain);*/

            /*uint32_t imgCount = 0;
            m_ICD.Device.vkGetSwapchainImagesKHR(vkDevice, m_vkSwapChain, &imgCount, nullptr);
            if( imgCount != m_Desc.elementCount )
            {
                VKE_LOG_ERR("Swap chain element count is different than requested.");
                return VKE_FAIL;
            }
            Utils::TCDynamicArray< VkImage > vImages(imgCount);
            VK_ERR(m_ICD.Device.vkGetSwapchainImagesKHR(vkDevice, m_vkSwapChain, &imgCount,
                   &vImages[ 0 ]));*/

            m_Desc.Size = ExtentU16( width, height );
            ret = m_pCtx->GetDeviceContext()->_GetDDI().CreateSwapChain( m_Desc, nullptr, &m_SwapChain );
            if( VKE_SUCCEEDED( ret ) )
            {
                const uint32_t imgCount = m_SwapChain.vImages.GetCount();
                const auto& vImages = m_SwapChain.vImages;
                const auto& vImageViews = m_SwapChain.vImageViews;

                //auto pImg1 = vImages[ 0 ];
                //auto pImg2 = vImages[ 1 ];
                // $TID: CreateSwapChain: sc={(void*)this}, img0={pImg1}, img1={pImg2}
                //m_vAcquireElements.Resize(imgCount);
                _CreateBackBuffers( imgCount );

                /*CommandBufferPtr pCmdBuffer = m_pCtx->CreateCommandBuffer();
                pCmdBuffer->Begin();
                VkImageSubresourceRange SubresRange;
                SubresRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                SubresRange.baseArrayLayer = 0;
                SubresRange.baseMipLevel = 0;
                SubresRange.layerCount = 1;
                SubresRange.levelCount = 1;*/

                auto& ResMgr = m_pCtx->GetDeviceContext()->Resource();

                for( uint32_t i = 0; i < imgCount; ++i )
                {
                    //auto& Element = m_vAcquireElements[ i ];
                    SAcquireElement& Element = m_vBackBuffers[i].AcquiredElement;
                    //m_VkDevice.DestroyObject(nullptr, &Element.vkImage);
                    //m_VkDevice.DestroyObject(nullptr, &Element.vkFramebuffer);
                    if( Element.vkCbAttachmentToPresent == VK_NULL_HANDLE )
                    {
                        //Element.vkCbAttachmentToPresent = m_pCtx->_CreateCommandBuffer(RenderQueueUsages::STATIC);
                        //Element.vkCbPresentToAttachment = m_pCtx->_CreateCommandBuffer(RenderQueueUsages::STATIC);
                    }

                    Element.vkImage = vImages[i];
                    Element.vkImageView = vImageViews[i];
                    {
                        TextureViewHandle hView;
                        {
                            // Add image
                            /*VkImageCreateInfo ci;
                            Vulkan::InitInfo(&ci, VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO);
                            ci.arrayLayers = 1;
                            ci.extent.width = width;
                            ci.extent.height = height;
                            ci.extent.depth = 1;
                            ci.flags = 0;
                            ci.format = Vulkan::Map::Format( m_Desc.format );
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
                            assert(res == VKE_OK);*/

                            /*STextureViewDesc Desc;
                            Desc.aspect = TextureAspects::COLOR;
                            Desc.beginMipmapLevel = 0;
                            Desc.endMipmapLevel = 1;
                            Desc.format = Vulkan::Convert::ImageFormat(m_vkSurfaceFormat.format);
                            Desc.hTexture.SetNative( Element.vkImage );
                            Desc.type = TextureViewTypes::VIEW_2D;
                            hView = ResMgr.CreateTextureView(Desc, Element.vkImage, &Element.vkImageView);*/
                        }
                        {
                            //if( Element.hRenderPass == NULL_HANDLE )
                            //{
                            //    SRenderPassAttachmentDesc Attachment;
                            //    Attachment.usage = RenderPassAttachmentUsages::COLOR_CLEAR_STORE;
                            //    Attachment.beginLayout = TextureLayouts::COLOR_RENDER_TARGET;
                            //    Attachment.endLayout = TextureLayouts::COLOR_RENDER_TARGET;
                            //    SRenderPassDesc RpDesc;
                            //    RpDesc.Size.width = static_cast<uint16_t>(width);
                            //    RpDesc.Size.height = static_cast<uint16_t>(height);
                            //    RpDesc.vAttachments.PushBack( Attachment );
                            //    SRenderPassDesc::SSubpassDesc SpDesc;
                            //    {
                            //        SRenderPassDesc::SSubpassDesc::SAttachmentDesc AtDesc;
                            //        AtDesc.hTextureView = hView;
                            //        AtDesc.layout = TextureLayouts::COLOR_RENDER_TARGET;
                            //        SpDesc.vRenderTargets.PushBack( AtDesc );
                            //    }
                            //    RpDesc.vSubpasses.PushBack( SpDesc );
                            //    m_pCtx->GetDeviceContext()->_GetDDI().CreateObject( RpDesc, nullptr );
                            //    //Element.hRenderPass = m_pCtx->GetDeviceContext()->CreateRenderPass(RpDesc);
                            //    //Element.pRenderPass = m_pCtx->GetDeviceContext()->GetRenderPass(Element.hRenderPass);
                            //    // $TID scCreateRenderPass: sc={(void*)this}, hrp={Element.hRenderPass}, rp={(void*)Element.pRenderPass}, img={Element.vkImage
                            //}
                            //else
                            //{
                            //    //m_pCtx->GetDeviceContext()->Update
                            //}

                            {
                                SRenderingPipelineDesc Desc;
                                SRenderingPipelineDesc::SPassDesc PassDesc;
                                PassDesc.hPass = Element.hRenderPass;
                                Desc.vRenderPassHandles.PushBack( PassDesc );
                                //Element.pRenderingPipeline = m_pCtx->CreateRenderingPipeline( Desc );
                            }
                        }


                        // Set memory barrier
                        /*VkImageMemoryBarrier ImgBarrier;
                        Vulkan::InitInfo(&ImgBarrier, VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER);
                        ImgBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                        ImgBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
                        ImgBarrier.srcAccessMask = 0;
                        ImgBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
                        ImgBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                        ImgBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                        ImgBarrier.image = Element.vkImage;
                        ImgBarrier.subresourceRange = SubresRange;

                        pCmdBuffer->PipelineBarrier( VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                                     VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                                     0,
                                                     0, nullptr,
                                                     0, nullptr,
                                                     1, &ImgBarrier );*/
                        /*CResourceBarrierManager::SImageBarrierInfo Barrier;
                        Barrier.vkImg = Element.vkImage;
                        Barrier.vkCurrentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                        Barrier.vkNewLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
                        pCmdBuffer->Barrier( Barrier );
                        pCmdBuffer->ExecuteBarriers();
                        Element.vkCurrLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;*/
                    }
                }

                //pCmdBuffer->End();
                //CGraphicsContext::CommandBufferArray vCmdBuffers = { pCmdBuffer };
                ////vCmdBuffers.PushBack(pCmdBuffer->GetNative());
                //m_pCtx->_SubmitCommandBuffers( vCmdBuffers, VK_NULL_HANDLE );

                // Prepare static command buffers with layout transitions
                for( uint32_t i = 0; i < imgCount; ++i )
                {
                    //auto& Element = m_vAcquireElements[ i ];
                    SAcquireElement& Element = m_vBackBuffers[i].AcquiredElement;

                    {
                        //Element.vkCbAttachmentToPresent = m_pCtx->_CreateCommandBuffer();
                        //Vulkan::Wrapper::CCommandBuffer Cb(m_VkDevice.GetICD(), Element.vkCbAttachmentToPresent);
                        //Cb.Begin(VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT);
                        // Set memory barrier
                        /*VkImageMemoryBarrier& ImgBarrier = Element.vkBarrierAttachmentToPresent;
                        Vulkan::InitInfo( &ImgBarrier, VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER );
                        ImgBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                        ImgBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
                        ImgBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                        ImgBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
                        ImgBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                        ImgBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                        ImgBarrier.image = Element.vkImage;
                        ImgBarrier.subresourceRange = SubresRange;*/

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
                        /*VkImageMemoryBarrier& ImgBarrier = Element.vkBarrierPresentToAttachment;
                        Vulkan::InitInfo( &ImgBarrier, VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER );
                        ImgBarrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
                        ImgBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                        ImgBarrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
                        ImgBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                        ImgBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                        ImgBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                        ImgBarrier.image = Element.vkImage;
                        ImgBarrier.subresourceRange = SubresRange;*/

                        /*Cb.PipelineBarrier(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                           VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                           0,
                                           0, nullptr,
                                           0, nullptr,
                                           1, &ImgBarrier);
                        Cb.End();*/
                    }
                }

                /*m_pCtx->Wait();
                m_pCtx->_FreeCommandBuffer( pCmdBuffer );*/
                ret = VKE_OK;
            }
            else
            {
                Destroy();
            }

            return ret;
        }

        Result CSwapChain::GetNextBackBuffer()
        {
            /*m_currBackBufferIdx++;
            m_currBackBufferIdx %= m_Desc.elementCount;
            m_pCurrBackBuffer = &m_vBackBuffers[ m_currBackBufferIdx ];*/
            m_pBackBufferMgr->AcquireNextBuffer();
            m_pCurrBackBuffer = reinterpret_cast< SBackBuffer* >( m_pBackBufferMgr->GetCustomData( m_backBufferIdx ) );
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
            SDDIGetBackBufferInfo Info;
            Info.hAcquireSemaphore = m_pCurrBackBuffer->vkAcquireSemaphore;
            m_pCtx->GetDeviceContext()->_GetDDI().GetCurrentBackBufferIndex( m_SwapChain, Info );
            /*VkSemaphore& vkSemaphore = m_pCurrBackBuffer->vkAcquireSemaphore;
            VK_ERR(m_VkDevice.AcquireNextImageKHR(m_vkSwapChain, UINT64_MAX, vkSemaphore,
                   VK_NULL_HANDLE, &m_pCurrBackBuffer->currImageIdx));*/
            //const auto& e1 = m_vAcquireElements[ 0 ];
            //const auto& e2 = m_vAcquireElements[ 1 ];
            //m_pCurrAcquireElement = &m_vAcquireElements[ m_pCurrBackBuffer->currImageIdx ];
            m_pCurrAcquireElement = &m_vBackBuffers[ m_pCurrBackBuffer->currImageIdx ].AcquiredElement;
            // $TID SwapBuffers: sc={(void*)this}, imgIdx={m_pCurrBackBuffer->currImageIdx}, img={m_pCurrAcquireElement->vkImage}, {m_pCurrAcquireElement->vkOldLayout}, {m_pCurrAcquireElement->vkCurrLayout}, as={vkSemaphore}
            return VKE_OK;
        }

        ExtentU32 CSwapChain::GetSize() const
        {
            //ExtentU32 Size = { m_vkSurfaceCaps.currentExtent.width, m_vkSurfaceCaps.currentExtent.height };
            //return Size;
            return m_SwapChain.Size;
        }

        void CSwapChain::BeginFrame(VkCommandBuffer vkCb)
        {
            Vulkan::Wrapper::CCommandBuffer Cb(m_pCtx->GetDeviceContext()->_GetDDI().GetDeviceICD(), vkCb);
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
            Vulkan::Wrapper::CCommandBuffer Cb( m_pCtx->GetDeviceContext()->_GetDDI().GetDeviceICD(), vkCb);
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
            bi.framebuffer = m_pCurrAcquireElement->pRenderPass->m_hFramebuffer;
            bi.renderPass = m_pCurrAcquireElement->pRenderPass->GetDDIObject();
            bi.pClearValues = &ClearValue;
            bi.clearValueCount = 1;
            bi.renderArea.extent.width = Size.width;
            bi.renderArea.extent.height = Size.height;
            bi.renderArea.offset.x = 0;
            bi.renderArea.offset.y = 0;
            
            m_pCtx->GetDeviceContext()->_GetDDI().GetICD().vkCmdBeginRenderPass(vkCb, &bi, VK_SUBPASS_CONTENTS_INLINE);
        }

        void CSwapChain::EndPass(VkCommandBuffer vkCb)
        {
            //m_VkDevice.GetICD().vkCmdEndRenderPass(vkCb);
            m_pCtx->GetDeviceContext()->_GetDDI().EndRenderPass( vkCb );
        }

    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER