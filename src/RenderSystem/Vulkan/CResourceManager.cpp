#include "RenderSystem/Vulkan/CResourceManager.h"
#if VKE_VULKAN_RENDERER
#include "RenderSystem/CDeviceContext.h"
#include "Core/Utils/CLogger.h"

namespace VKE
{
    namespace RenderSystem
    {
        CResourceManager::CResourceManager(CDeviceContext* pCtx) :
            m_pCtx(pCtx)
        {}

        CResourceManager::~CResourceManager()
        {

        }

        Result CResourceManager::Create(const SResourceManagerDesc& Desc)
        {
            // Set first element as null
            m_vImages.PushBack(VK_NULL_HANDLE);
            m_vImageViews.PushBack(VK_NULL_HANDLE);
            m_vRenderpasses.PushBack(VK_NULL_HANDLE);
            m_vFramebuffers.PushBack(VK_NULL_HANDLE);
            return VKE_OK;
        }

        void CResourceManager::Destroy()
        {
            auto& Device = m_pCtx->_GetDevice();
            for( uint32_t i = 0; i < m_vImageViews.GetCount(); ++i )
            {
                Device.DestroyObject(nullptr, &m_vImageViews[ i ]);
            }
            m_vImageViews.Clear<false>();

            for( uint32_t i = 0; i < m_vImages.GetCount(); ++i )
            {
                Device.DestroyObject(nullptr, &m_vImages[ i ]);
            }
            m_vImages.Clear<false>();

            for( uint32_t i = 0; i < m_vFramebuffers.GetCount(); ++i )
            {
                Device.DestroyObject(nullptr, &m_vFramebuffers[ i ]);
            }
            m_vFramebuffers.Clear<false>();

            for( uint32_t i = 0; i < m_vRenderpasses.GetCount(); ++i )
            {
                Device.DestroyObject(nullptr, &m_vRenderpasses[ i ]);
            }
            m_vRenderpasses.Clear<false>();

            for( uint32_t i = 0; i < ResourceTypes::_MAX_COUNT; ++i )
            {
                m_avFreeHandles[ i ].FastClear();
            }
        }

        void CResourceManager::DestroyTexture(const handle_t& hTex)
        {
            VkImage vkImg = _DestroyResource< VkImage >(hTex, m_vImages, ResourceTypes::TEXTURE);
            m_pCtx->_GetDevice().DestroyObject(nullptr, &vkImg);
        }

        void CResourceManager::DestroyTextureView(const handle_t& hTexView)
        {
            VkImageView vkView = _DestroyResource< VkImageView >(hTexView, m_vImageViews, ResourceTypes::TEXTURE_VIEW);
            m_pCtx->_GetDevice().DestroyObject(nullptr, &vkView);
        }

        void CResourceManager::DestroyFramebuffer(const handle_t& hFramebuffer)
        {
            VkFramebuffer vkFb = _DestroyResource< VkFramebuffer >(hFramebuffer, m_vFramebuffers,
                                                                   ResourceTypes::FRAMEBUFFER);
            m_pCtx->_GetDevice().DestroyObject(nullptr, &vkFb);
        }

        void CResourceManager::DestroyRenderPass(const handle_t& hPass)
        {
            VkRenderPass vkRp = _DestroyResource< VkRenderPass >(hPass, m_vRenderpasses, ResourceTypes::RENDERPASS);
            m_pCtx->_GetDevice().DestroyObject(nullptr, &vkRp);
        }

        uint32_t CalcMipLevelCount(const ExtentU32& Size)
        {
            auto count = 1 + floor(log2(max(Size.width, Size.height)));
            return count;
        }

        handle_t CResourceManager::CreateTexture(const STextureDesc& Desc)
        {
            uint32_t mipLevels = Desc.mipLevelCount;
            if( mipLevels == 0 )
                mipLevels = CalcMipLevelCount(Desc.Size);

            uint32_t queue = VK_QUEUE_FAMILY_IGNORED;

            VkImageCreateInfo ci;
            Vulkan::InitInfo(&ci, VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO);
            ci.arrayLayers = 1;
            ci.extent.width = Desc.Size.width;
            ci.extent.height = Desc.Size.height;
            ci.extent.depth = 1;
            ci.flags = 0;
            ci.format = Vulkan::GetFormat(Desc.format);
            ci.imageType = Vulkan::GetImageType(Desc.type);
            ci.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
            ci.mipLevels = mipLevels;
            ci.pQueueFamilyIndices = &queue;
            ci.queueFamilyIndexCount = 1;
            ci.samples = Vulkan::GetSampleCount(Desc.multisampling);
            ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            ci.tiling = VK_IMAGE_TILING_OPTIMAL;
            ci.usage = Vulkan::GetImageUsage(Desc.usage);
            VkImage vkImg;
            VK_ERR(m_pCtx->_GetDevice().CreateObject(ci, nullptr, &vkImg));
                    
            return _AddResource(vkImg, ci, ResourceTypes::TEXTURE, m_vImages, m_vImageDescs);
        }

        handle_t CResourceManager::CreateTextureView(const STextureViewDesc& Desc)
        {
            static const VkComponentMapping DefaultMapping =
            {
                VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A
            };

            VkImage vkImg = VK_NULL_HANDLE;
            {
                // Lock should not be required here as request image should be present
                //Threads::ScopedLock l(m_aSyncObjects[ ResourceTypes::TEXTURE ]);
                vkImg = m_vImages[ Desc.hTexture ];
            }
            uint32_t endMipmapLevel = Desc.endMipmapLevel;
            if( endMipmapLevel == 0 )
            {
                const auto& ImgDesc = m_vImageDescs[ Desc.hTexture ];
                endMipmapLevel = ImgDesc.mipLevels;
            }

            assert(vkImg != VK_NULL_HANDLE);
            VkImageViewCreateInfo ci;
            Vulkan::InitInfo(&ci, VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO);
            ci.components = DefaultMapping;
            ci.flags = 0;
            ci.format = Vulkan::GetFormat(Desc.format);
            ci.image = vkImg;
            ci.subresourceRange.aspectMask = Vulkan::GetImageAspect(Desc.aspect);
            ci.subresourceRange.baseArrayLayer = 0;
            ci.subresourceRange.baseMipLevel = Desc.beginMipmapLevel;
            ci.subresourceRange.layerCount = 1;
            ci.subresourceRange.levelCount = Desc.endMipmapLevel;
            ci.viewType = Vulkan::GetImageViewType(Desc.type);
            VkImageView vkView;
            VK_ERR(m_pCtx->_GetDevice().CreateObject(ci, nullptr, &vkView));
           
            return _AddResource(vkView, ci, ResourceTypes::TEXTURE_VIEW, m_vImageViews, m_vImageViewDescs);
        }

        handle_t CResourceManager::CreateFramebuffer(const SFramebufferDesc& Desc)
        {
            ImageViewArray vImgViews;
            for( uint32_t i = 0; i < Desc.vAttachments.GetCount(); ++i )
            {
                handle_t hView = Desc.vAttachments[ i ];
                VkImageView vkView = m_vImageViews[ hView ];
                if( vkView != VK_NULL_HANDLE )
                {
                    vImgViews.PushBack(vkView);
                }
                else
                {
                    VKE_LOG_ERR("Handle: " << Desc.vAttachments[i] << " does not exists.");
                    return NULL_HANDLE;
                }
            }
            
            VkRenderPass vkRenderPass = m_vRenderpasses[ Desc.hRenderPass ];
            if( vkRenderPass == VK_NULL_HANDLE )
            {
                VKE_LOG_ERR("Handle: " << Desc.hRenderPass << " does not exists.");
                return NULL_HANDLE;
            }

            VkFramebufferCreateInfo ci;
            Vulkan::InitInfo(&ci, VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO);
            ci.attachmentCount = vImgViews.GetCount();
            ci.flags = 0;
            ci.height = Desc.Size.height;
            ci.width = Desc.Size.width;
            ci.layers = 1;
            ci.pAttachments = &vImgViews[ 0 ];
            ci.renderPass = vkRenderPass;
            
            VkFramebuffer vkFramebuffer;
            VK_ERR(m_pCtx->_GetDevice().CreateObject(ci, nullptr, &vkFramebuffer));
            return _AddResource(vkFramebuffer, ci, ResourceTypes::FRAMEBUFFER, m_vFramebuffers);
        }
        
        handle_t CResourceManager::CreateRenderPass(const SRenderPassDesc& Desc)
        {
            Utils::TCDynamicArray< VkAttachmentDescription > vAttachments;
            Utils::TCDynamicArray< VkSubpassDescription > vSubpasses;

            for( uint32_t i = 0; i < Desc.vAttachmentDescs.GetCount(); ++i )
            {
                const SAttachmentDesc& AttachmentDesc = Desc.vAttachmentDescs[ i ];
                VkAttachmentDescription vkDesc;
                vkDesc.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                vkDesc.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                vkDesc.flags = 0;
                vkDesc.format = Vulkan::GetFormat(AttachmentDesc.format);
                vkDesc.samples = Vulkan::GetSampleCount(AttachmentDesc.multisampling);
                vkDesc.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                vkDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                vkDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                vkDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
                vAttachments.PushBack(vkDesc);
            }

            VkAttachmentReference ColorAttachmentRef;
            ColorAttachmentRef.attachment = 0;
            ColorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            for( uint32_t i = 0; i < Desc.vSubpassDescs.GetCount(); ++i )
            {
                const SSubpassDesc& SubpassDesc = Desc.vSubpassDescs[ i ];
                VkSubpassDescription vkDesc;
                vkDesc.flags = 0;
                vkDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
                vkDesc.inputAttachmentCount = 0;
                vkDesc.pInputAttachments = nullptr;
                vkDesc.colorAttachmentCount = 1;
                vkDesc.pColorAttachments = &ColorAttachmentRef;
                vkDesc.pResolveAttachments = nullptr;
                vkDesc.pDepthStencilAttachment = nullptr;
                vkDesc.preserveAttachmentCount = 0;
                vkDesc.pPreserveAttachments = nullptr;
                vSubpasses.PushBack(vkDesc);
            }

            VkRenderPass vkRp;
            VkRenderPassCreateInfo ci;
            Vulkan::InitInfo(&ci, VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO);
            ci.attachmentCount = vAttachments.GetCount();
            ci.pAttachments = &vAttachments[ 0 ];
            ci.dependencyCount = 0;
            ci.pDependencies = nullptr;
            ci.subpassCount = vSubpasses.GetCount();
            ci.pSubpasses = &vSubpasses[ 0 ];

            VK_ERR(m_pCtx->_GetDevice().CreateObject(ci, nullptr, &vkRp));
            
            return _AddResource(vkRp, ci, ResourceTypes::RENDERPASS, m_vRenderpasses);
        }
    }
}
#endif // VKE_VULKAN_RENDERER