#include "RenderSystem/Vulkan/CResourceManager.h"
#if VKE_VULKAN_RENDERER
#include "RenderSystem/CDeviceContext.h"
#include "Core/Utils/CLogger.h"
#include "Core/VKEConfig.h"
#include "RenderSystem/Vulkan/CDeviceMemoryManager.h"

namespace VKE
{
    namespace RenderSystem
    {
        CResourceManager::CResourceManager(CDeviceContext* pCtx) :
            m_pCtx(pCtx)
        {}

        CResourceManager::~CResourceManager()
        {
            Destroy();
        }

        Result CResourceManager::Create(const SResourceManagerDesc& Desc)
        {
            // Set first element as null
            Result res = VKE_FAIL;
            m_Desc = Desc;
            if( VKE_FAILED( Memory::CreateObject( &HeapAllocator, &m_pDeviceMemMgr, m_pCtx ) ) )
            {
                VKE_LOG_ERR("No memory to allocate CDeviceMemoryManager. Memory error.");
                res = VKE_ENOMEMORY;
                return res;
            }
            res = m_pDeviceMemMgr->Create( Desc.DeviceMemoryDesc );
            if( VKE_FAILED( res ) )
            {
                return res;
            }
            // Add null handle
            {
                VkImage vkImg = VK_NULL_HANDLE;
                VkImageCreateInfo vkCreateInfo;
                _AddResource( vkImg, vkCreateInfo, ResourceTypes::TEXTURE, m_vImages, m_vImageDescs, 0 );
            }
            // Add null handle
            {
                VkImageView vkView = VK_NULL_HANDLE;
                SImageViewDesc Desc;
                _AddResource( vkView, Desc, ResourceTypes::TEXTURE_VIEW, m_vImageViews, m_vImageViewDescs, 0 );
            }
            return res;
        }

        void CResourceManager::Destroy()
        {
            if( m_pDeviceMemMgr )
            {
                auto& Device = m_pCtx->_GetDevice();
                for( uint32_t i = 1; i < m_vImageViews.GetCount(); ++i )
                {
                    auto& vkImgView = m_vImageViews[ i ];
                    Device.DestroyObject( nullptr, &vkImgView );
                }
                m_vImageViews.Clear<false>();

                for( uint32_t i = 1; i < m_vImages.GetCount(); ++i )
                {
                    Device.DestroyObject( nullptr, &m_vImages[ i ] );
                }
                m_vImages.Clear<false>();

                for( uint32_t i = 1; i < m_vFramebuffers.GetCount(); ++i )
                {
                    Device.DestroyObject( nullptr, &m_vFramebuffers[ i ] );
                }
                m_vFramebuffers.Clear<false>();

                for( uint32_t i = 1; i < m_vRenderpasses.GetCount(); ++i )
                {
                    Device.DestroyObject( nullptr, &m_vRenderpasses[ i ] );
                }
                m_vRenderpasses.Clear<false>();

                for( uint32_t i = 0; i < ResourceTypes::_MAX_COUNT; ++i )
                {
                    m_avFreeHandles[ i ].FastClear();
                }

                Memory::DestroyObject( &HeapAllocator, &m_pDeviceMemMgr );
            }
        }

        

        static const VkMemoryPropertyFlags g_aRequiredMemoryFlags[] =
        {
            0, // unknown
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, // gpu
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, // cpu access
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT // cpu access optimal
        };

        VkMemoryPropertyFlags MemoryUsagesToVkMemoryPropertyFlags(MEMORY_USAGES usages)
        {
            VkMemoryPropertyFlags flags = 0;
            if( usages & MemoryUsages::GPU_ACCESS )
            {
                flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            }
            else
            {
                if( usages & MemoryUsages::CPU_ACCESS )
                {
                    flags |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
                }
                if( usages & MemoryUsages::CPU_NO_FLUSH )
                {
                    flags |= VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
                }
                if( usages & MemoryUsages::CPU_CACHED )
                {
                    flags |= VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
                }
            }
            return flags;
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
            VkFormat vkFormat;
            VkImageType vkType;
            VkSampleCountFlagBits vkSampleCount;
            VkImageUsageFlags vkUsages;
            {
                auto& TexCache = m_Cache.Texture;
                Threads::ScopedLock l( TexCache.SyncObj );
                if( TexCache.format != Desc.format )
                {
                    TexCache.format = Desc.format;
                    TexCache.vkFormat = Vulkan::Map::Format( Desc.format );
                }
                if( TexCache.type != Desc.type )
                {
                    TexCache.type = Desc.type;
                    TexCache.vkType = Vulkan::Map::ImageType( Desc.type );
                }
                if( TexCache.multisampling != Desc.multisampling )
                {
                    TexCache.multisampling = Desc.multisampling;
                    TexCache.vkMultisampling = Vulkan::Map::SampleCount( Desc.multisampling );
                }
                if( TexCache.usages != Desc.usage )
                {
                    TexCache.usages = Desc.usage;
                    TexCache.vkUsages = Vulkan::Map::ImageUsage( Desc.usage );
                }

                vkUsages = TexCache.vkUsages;
                vkSampleCount = TexCache.vkMultisampling;
                vkFormat = TexCache.vkFormat;
                vkType = TexCache.vkType;
            }
            ci.arrayLayers = 1;
            ci.extent.width = Desc.Size.width;
            ci.extent.height = Desc.Size.height;
            ci.extent.depth = 1;
            ci.flags = 0;
            ci.format = vkFormat;
            ci.imageType = vkType;
            ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            ci.mipLevels = mipLevels;
            ci.pQueueFamilyIndices = &queue;
            ci.queueFamilyIndexCount = 1;
            ci.samples = vkSampleCount;
            ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            ci.tiling = VK_IMAGE_TILING_OPTIMAL;
            ci.usage = vkUsages;
            VkImage vkImg;
            TextureHandle hTex = NULL_HANDLE;

            auto& Device = m_pCtx->_GetDevice();
            {
                VK_ERR( Device.CreateObject( ci, nullptr, &vkImg ) );
                VkMemoryPropertyFlags vkMemFlags = MemoryUsagesToVkMemoryPropertyFlags( Desc.memoryUsage );
                uint64_t mem = m_pDeviceMemMgr->Allocate( vkImg, vkMemFlags );
                if( mem > 0 )
                {
                    hTex.handle = _AddResource( vkImg, ci, ResourceTypes::TEXTURE, m_vImages, m_vImageDescs, mem );
                    if( pOut )
                    {
                        *pOut = vkImg;
                    }
                }
                else
                {
                    Device.DestroyObject( nullptr, &vkImg );
                }
            }
           
            return hTex;
        }
        
        static const VkComponentMapping g_DefaultMapping =
        {
            VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A
        };

        TextureViewHandle CResourceManager::CreateTextureView(const STextureViewDesc& Desc, VkImageView* pOut)
        {
            VkImage vkImg = VK_NULL_HANDLE;
            SImageViewDesc ImgViewDesc;
            {
                // Lock should not be required here as request image should be present
                //Threads::ScopedLock l(m_aSyncObjects[ ResourceTypes::TEXTURE ]);
                if( Desc.hTexture.IsNativeHandle() )
                {
                    vkImg = reinterpret_cast< VkImage >( Desc.hTexture.handle );
                }
                else
                {
                    vkImg = m_vImages[ static_cast< uint32_t >( Desc.hTexture.handle ) ];
                    ImgViewDesc.hTexture = Desc.hTexture;
                }
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
            VkImageViewCreateInfo& ci = ImgViewDesc.Info;
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
            
            TextureViewHandle hView =  TextureViewHandle{ _AddResource( vkView, ImgViewDesc, ResourceTypes::TEXTURE_VIEW,
                                                                        m_vImageViews, m_vImageViewDescs, 0 ) };
            if( hView != NULL_HANDLE )
            {
                if( pOut )
                {
                    *pOut = vkView;
                }
            }
            return hView;
        }

        TextureViewHandle CResourceManager::CreateTextureView(const TextureHandle& hTexture, VkImageView* pOut)
        {
            assert(hTexture != NULL_HANDLE);
            
            const auto& TexDesc = GetTextureDesc(hTexture);
            SImageViewDesc ImgViewDesc;
            ImgViewDesc.hTexture = hTexture;
            VkImageViewCreateInfo& ci = ImgViewDesc.Info;
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
            return TextureViewHandle{ _AddResource( vkView, ImgViewDesc, ResourceTypes::TEXTURE_VIEW,
                                                    m_vImageViews, m_vImageViewDescs, 0 ) };
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

        const VkImageCreateInfo& CResourceManager::GetTextureDesc(const TextureViewHandle& hView) const
        {
            const SImageViewDesc& Desc = m_vImageViewDescs[ hView.handle ];
            if( Desc.hTexture == NULL_HANDLE )
            {
                return GetTextureDesc( Desc.Info.image );
            }
            return m_vImageDescs[ Desc.hTexture.handle ];
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