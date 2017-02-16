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

        Result CResourceManager::Create(const SResourceManagerDesc& /*Desc*/)
        {
            // Set first element as null
            m_vImages.PushBack(VK_NULL_HANDLE);
            m_vImageViews.PushBack(VK_NULL_HANDLE);
            m_vRenderpasses.PushBack(VK_NULL_HANDLE);
            m_vFramebuffers.PushBack(VK_NULL_HANDLE);
            m_vImageDescs.PushBack({});
            m_vImageViewDescs.PushBack({});
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

        void CResourceManager::DestroyTexture(const TextureHandle& hTex)
        {
            VkImage vkImg = _DestroyResource< VkImage >(hTex.handle, m_vImages, ResourceTypes::TEXTURE);
            m_pCtx->_GetDevice().DestroyObject(nullptr, &vkImg);
        }

        void CResourceManager::DestroyTextureView(const TextureViewHandle& hTexView)
        {
            VkImageView vkView = _DestroyResource< VkImageView >(hTexView.handle, m_vImageViews,
                                                                 ResourceTypes::TEXTURE_VIEW);
            m_pCtx->_GetDevice().DestroyObject(nullptr, &vkView);
        }

        void CResourceManager::DestroyFramebuffer(const FramebufferHandle& hFramebuffer)
        {
            VkFramebuffer vkFb = _DestroyResource< VkFramebuffer >(hFramebuffer.handle, m_vFramebuffers,
                                                                   ResourceTypes::FRAMEBUFFER);
            m_pCtx->_GetDevice().DestroyObject(nullptr, &vkFb);
        }

        void CResourceManager::DestroyRenderPass(const RenderPassHandle& hPass)
        {
            VkRenderPass vkRp = _DestroyResource< VkRenderPass >(hPass.handle, m_vRenderpasses,
                                                                 ResourceTypes::RENDERPASS);
            m_pCtx->_GetDevice().DestroyObject(nullptr, &vkRp);
        }

        uint32_t CalcMipLevelCount(const ExtentU32& Size)
        {
            auto count = 1 + floor(log2(max(Size.width, Size.height)));
            return static_cast<uint32_t>(count);
        }

        TextureHandle CResourceManager::CreateTexture(const STextureDesc& Desc, VkImage* pOut)
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
            ci.format = Vulkan::Map::Format(Desc.format);
            ci.imageType = Vulkan::Map::ImageType(Desc.type);
            ci.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
            ci.mipLevels = mipLevels;
            ci.pQueueFamilyIndices = &queue;
            ci.queueFamilyIndexCount = 1;
            ci.samples = Vulkan::Map::SampleCount(Desc.multisampling);
            ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            ci.tiling = VK_IMAGE_TILING_OPTIMAL;
            ci.usage = Vulkan::Map::ImageUsage(Desc.usage);
            VkImage vkImg;
            VK_ERR(m_pCtx->_GetDevice().CreateObject(ci, nullptr, &vkImg));
            if( pOut )
            {
                *pOut = vkImg;
            }
            return TextureHandle{ _AddResource(vkImg, ci, ResourceTypes::TEXTURE, m_vImages, m_vImageDescs) };
            
        }
        
        static const VkComponentMapping g_DefaultMapping =
        {
            VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A
        };

        TextureViewHandle CResourceManager::CreateTextureView(const STextureViewDesc& Desc, VkImageView* pOut)
        {
            VkImage vkImg = VK_NULL_HANDLE;
            {
                // Lock should not be required here as request image should be present
                //Threads::ScopedLock l(m_aSyncObjects[ ResourceTypes::TEXTURE ]);
                if( Desc.hTexture.IsNativeHandle() )
                    vkImg = reinterpret_cast< VkImage >( Desc.hTexture.handle );
                else
                    vkImg = m_vImages[ static_cast<uint32_t>(Desc.hTexture.handle) ];
            }
            uint32_t endMipmapLevel = Desc.endMipmapLevel;
            if( endMipmapLevel == 0 )
            {
                if( !Desc.hTexture.IsNativeHandle() )
                {
                    const auto& ImgDesc = m_vImageDescs[static_cast<uint32_t>(Desc.hTexture.handle)];
                    endMipmapLevel = ImgDesc.mipLevels;
                }
            }

            assert(vkImg != VK_NULL_HANDLE);
            VkImageViewCreateInfo ci;
            Vulkan::InitInfo(&ci, VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO);
            ci.components = g_DefaultMapping;
            ci.flags = 0;
            ci.format = Vulkan::Map::Format(Desc.format);
            ci.image = vkImg;
            ci.subresourceRange.aspectMask = Vulkan::Map::ImageAspect(Desc.aspect);
            ci.subresourceRange.baseArrayLayer = 0;
            ci.subresourceRange.baseMipLevel = Desc.beginMipmapLevel;
            ci.subresourceRange.layerCount = 1;
            ci.subresourceRange.levelCount = Desc.endMipmapLevel;
            ci.viewType = Vulkan::Map::ImageViewType(Desc.type);
            VkImageView vkView;
            VK_ERR(m_pCtx->_GetDevice().CreateObject(ci, nullptr, &vkView));
            if( pOut )
            {
                *pOut = vkView;
            }
            return TextureViewHandle{ _AddResource(vkView, ci, ResourceTypes::TEXTURE_VIEW, m_vImageViews,
                                                   m_vImageViewDescs) };
        }

        TextureViewHandle CResourceManager::CreateTextureView(const TextureHandle& hTexture, VkImageView* pOut)
        {
            assert(hTexture != NULL_HANDLE);
            
            const auto& TexDesc = GetTextureDesc(hTexture);
            VkImageViewCreateInfo ci;
            Vulkan::InitInfo(&ci, VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO);
            ci.components = g_DefaultMapping;
            ci.flags = 0;
            ci.format = TexDesc.format;
            ci.image = m_vImages[static_cast<uint32_t>(hTexture.handle)];
            ci.subresourceRange.aspectMask = Vulkan::Convert::UsageToAspectMask(TexDesc.usage);
            ci.subresourceRange.baseArrayLayer = 0;
            ci.subresourceRange.baseMipLevel = 0;
            ci.subresourceRange.levelCount = TexDesc.mipLevels;
            ci.subresourceRange.layerCount = 1;
            ci.viewType = Vulkan::Convert::ImageTypeToViewType(TexDesc.imageType);
            VkImageView vkView = VK_NULL_HANDLE;
            VK_ERR(m_pCtx->_GetDevice().CreateObject(ci, nullptr, &vkView));
            if( pOut )
            {
                *pOut = vkView;
            }
            return TextureViewHandle{ _AddResource(vkView, ci, ResourceTypes::TEXTURE_VIEW, m_vImageViews,
                                                   m_vImageViewDescs) };
        }

        FramebufferHandle CResourceManager::CreateFramebuffer(const SFramebufferDesc& Desc)
        {
            ImageViewArray vImgViews;
            for( uint32_t i = 0; i < Desc.vAttachments.GetCount(); ++i )
            {
                handle_t hView = Desc.vAttachments[ i ];
                VkImageView vkView = m_vImageViews[static_cast<uint32_t>(hView)];
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
            
            VkRenderPass vkRenderPass = m_vRenderpasses[static_cast<uint32_t>(Desc.hRenderPass)];
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
            return FramebufferHandle{ _AddResource(vkFramebuffer, ci, ResourceTypes::FRAMEBUFFER, m_vFramebuffers) };
        }
        
        RenderPassHandle CResourceManager::CreateRenderPass(const SRenderPassDesc& /*Desc*/)
        {
            return NULL_HANDLE;
        }

        TextureHandle CResourceManager::_FindTexture(const VkImage& vkImg) const
        {
            for( uint32_t i = 0; i < m_vImages.GetCount(); ++i )
            {
                if( m_vImages[ i ] == vkImg )
                {
                    return TextureHandle{ i };
                }
            }
            return NULL_HANDLE;
        }

        const VkImageCreateInfo& CResourceManager::FindTextureDesc(const TextureViewHandle& hView) const
        {
            const auto& ViewDesc = GetTextureViewDesc(hView);
            auto hTex = _FindTexture(ViewDesc.image);
            return GetTextureDesc(hTex);
        }

        Result CResourceManager::AddTexture(const VkImage& vkImg, const VkImageCreateInfo& Info)
        {
            m_mCustomTextures.insert(TextureMap::value_type( vkImg, Info ) );
            return VKE_OK;
        }

        const VkImageCreateInfo& CResourceManager::GetTextureDesc(const VkImage& vkImg) const
        {
            auto Itr = m_mCustomTextures.find(vkImg);
            return Itr->second;
        }

        void CResourceManager::RemoveTexture(const VkImage& vkImg)
        {
            m_mCustomTextures.erase(vkImg);
        }
    }
}
#endif // VKE_VULKAN_RENDERER