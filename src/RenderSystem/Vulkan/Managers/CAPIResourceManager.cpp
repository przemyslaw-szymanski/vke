//#include "RenderSystem/Vulkan/Managers/CAPIResourceManager.h"
//#if VKE_VULKAN_RENDER_SYSTEM
//#include "RenderSystem/CDeviceContext.h"
//#include "Core/Utils/CLogger.h"
//#include "Core/VKEConfig.h"
//#include "RenderSystem/Vulkan/Managers/CDeviceMemoryManager.h"
//
//namespace VKE
//{
//    namespace RenderSystem
//    {
//        CAPIResourceManager::CAPIResourceManager(CDeviceContext* pCtx) :
//            m_pCtx(pCtx)
//        {}
//
//        CAPIResourceManager::~CAPIResourceManager()
//        {
//            Destroy();
//        }
//
//        Result CAPIResourceManager::Create(const SResourceManagerDesc& Desc)
//        {
//            // Set first element as null
//            Result res = VKE_FAIL;
//            m_Desc = Desc;
//            if( VKE_FAILED( Memory::CreateObject( &HeapAllocator, &m_pDeviceMemMgr, m_pCtx ) ) )
//            {
//                VKE_LOG_ERR("No memory to allocate CDeviceMemoryManager. Memory error.");
//                res = VKE_ENOMEMORY;
//                return res;
//            }
//            res = m_pDeviceMemMgr->Create( Desc.DeviceMemoryDesc );
//            if( VKE_FAILED( res ) )
//            {
//                return res;
//            }
//            // Add null handle
//            {
//                DDITexture hImg = DDI_NULL_HANDLE;
//                STextureDesc Desc;
//                _AddResource( hImg, Desc, ResourceTypes::TEXTURE, m_vImages, m_vImageDescs, 0 );
//            }
//            // Add null handle
//            {
//                DDITextureView hView = DDI_NULL_HANDLE;
//                STextureViewDesc Desc;
//                _AddResource( hView, Desc, ResourceTypes::TEXTURE_VIEW, m_vImageViews, m_vImageViewDescs, 0 );
//            }
//            return res;
//        }
//
//        void CAPIResourceManager::Destroy()
//        {
//            if( m_pDeviceMemMgr )
//            {
//                auto& Device = m_pCtx->_GetDDI();
//                for( uint32_t i = 1; i < m_vImageViews.GetCount(); ++i )
//                {
//                    auto& vkImgView = m_vImageViews[ i ];
//                    Device.DestroyObject( &vkImgView, nullptr );
//                }
//                m_vImageViews.Clear();
//
//                for( uint32_t i = 1; i < m_vImages.GetCount(); ++i )
//                {
//                    Device.DestroyObject( &m_vImages[ i ], nullptr );
//                }
//                m_vImages.Clear();
//
//                for( uint32_t i = 1; i < m_vFramebuffers.GetCount(); ++i )
//                {
//                    Device.DestroyObject( &m_vFramebuffers[ i ], nullptr );
//                }
//                m_vFramebuffers.Clear();
//
//                for( uint32_t i = 1; i < m_vRenderpasses.GetCount(); ++i )
//                {
//                    Device.DestroyObject( &m_vRenderpasses[ i ], nullptr );
//                }
//                m_vRenderpasses.Clear();
//
//                for( uint32_t i = 0; i < ResourceTypes::_MAX_COUNT; ++i )
//                {
//                    m_avFreeHandles[ i ].Clear();
//                }
//
//                Memory::DestroyObject( &HeapAllocator, &m_pDeviceMemMgr );
//            }
//        }
//
//        
//
//        static const VkMemoryPropertyFlags g_aRequiredMemoryFlags[] =
//        {
//            0, // unknown
//            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, // gpu
//            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, // cpu access
//            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT // cpu access optimal
//        };
//
//        VkMemoryPropertyFlags MemoryUsagesToVkMemoryPropertyFlags(MEMORY_USAGES usages)
//        {
//            VkMemoryPropertyFlags flags = 0;
//            if( usages & MemoryUsages::GPU_ACCESS )
//            {
//                flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
//            }
//            else
//            {
//                if( usages & MemoryUsages::CPU_ACCESS )
//                {
//                    flags |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
//                }
//                if( usages & MemoryUsages::CPU_NO_FLUSH )
//                {
//                    flags |= VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
//                }
//                if( usages & MemoryUsages::CPU_CACHED )
//                {
//                    flags |= VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
//                }
//            }
//            return flags;
//        }
//
//        void CAPIResourceManager::DestroyTexture(const TextureHandle& hTex)
//        {
//            DDITexture hImg = _DestroyResource< DDITexture >(hTex.handle, m_vImages, ResourceTypes::TEXTURE);
//            m_pCtx->_GetDDI().DestroyObject( &hImg, nullptr );
//        }
//
//        void CAPIResourceManager::DestroyTextureView(const TextureViewHandle& hTexView)
//        {
//            VkImageView vkView = _DestroyResource< VkImageView >(hTexView.handle, m_vImageViews,
//                                                                 ResourceTypes::TEXTURE_VIEW);
//            m_pCtx->_GetDDI().DestroyObject( &vkView, nullptr );
//        }
//
//        void CAPIResourceManager::DestroyFramebuffer(const FramebufferHandle& hFramebuffer)
//        {
//            VkFramebuffer vkFb = _DestroyResource< VkFramebuffer >(hFramebuffer.handle, m_vFramebuffers,
//                                                                   ResourceTypes::FRAMEBUFFER);
//            m_pCtx->_GetDDI().DestroyObject( &vkFb, nullptr );
//        }
//
//        void CAPIResourceManager::DestroyRenderPass(const RenderPassHandle& hPass)
//        {
//            VkRenderPass vkRp = _DestroyResource< VkRenderPass >(hPass.handle, m_vRenderpasses,
//                                                                 ResourceTypes::RENDERPASS);
//            m_pCtx->_GetDDI().DestroyObject( &vkRp, nullptr );
//        }
//
//        uint32_t CalcMipLevelCount(const ExtentU32& Size)
//        {
//            auto count = 1 + floor(log2(Max(Size.width, Size.height)));
//            return static_cast<uint32_t>(count);
//        }
//        
//
//        TextureHandle CAPIResourceManager::CreateTexture(const STextureDesc& Desc, VkImage* pOut)
//        {
//            uint32_t mipLevels = Desc.mipLevelCount;
//            if( mipLevels == 0 )
//                mipLevels = CalcMipLevelCount(Desc.Size);
//
//            /*uint32_t queue = VK_QUEUE_FAMILY_IGNORED;
//
//            VkImageCreateInfo ci;
//            Vulkan::InitInfo(&ci, VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO);
//            VkFormat vkFormat;
//            VkImageType vkType;
//            VkSampleCountFlagBits vkSampleCount;
//            VkImageUsageFlags vkUsages;*/
//            {
//                auto& TexCache = m_Cache.Texture;
//                Threads::ScopedLock l( TexCache.SyncObj );
//                if( TexCache.format != Desc.format )
//                {
//                    TexCache.format = Desc.format;
//                    TexCache.DDIFormat = Vulkan::Map::Format( Desc.format );
//                }
//                if( TexCache.type != Desc.type )
//                {
//                    TexCache.type = Desc.type;
//                    TexCache.DDIFormat = Desc.format; //Vulkan::Map::ImageType( Desc.type );
//                }
//                if( TexCache.multisampling != Desc.multisampling )
//                {
//                    TexCache.multisampling = Desc.multisampling;
//                    TexCache.vkMultisampling = Vulkan::Map::SampleCount( Desc.multisampling );
//                }
//                if( TexCache.usages != Desc.usage )
//                {
//                    TexCache.usages = Desc.usage;
//                    TexCache.DDIUsages = Vulkan::Map::ImageUsage( Desc.usage );
//                }
//
//                /*vkUsages = TexCache.DDIUsages;
//                vkSampleCount = TexCache.vkMultisampling;
//                vkFormat = TexCache.DDIFormat;
//                vkType = TexCache.DDIType;*/
//            }
//            /*ci.arrayLayers = 1;
//            ci.extent.width = Desc.Size.width;
//            ci.extent.height = Desc.Size.height;
//            ci.extent.depth = 1;
//            ci.flags = 0;
//            ci.format = vkFormat;
//            ci.imageType = vkType;
//            ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
//            ci.mipLevels = mipLevels;
//            ci.pQueueFamilyIndices = &queue;
//            ci.queueFamilyIndexCount = 1;
//            ci.samples = vkSampleCount;
//            ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
//            ci.tiling = VK_IMAGE_TILING_OPTIMAL;
//            ci.usage = vkUsages;
//            VkImage vkImg;*/
//            TextureHandle hTex = INVALID_HANDLE;
//
//            auto& DDI = m_pCtx->_GetDDI();
//            {
//                /*VK_ERR( DDI.CreateObject( ci, nullptr, &vkImg ) );
//                VkMemoryPropertyFlags vkMemFlags = MemoryUsagesToVkMemoryPropertyFlags( Desc.memoryUsage );
//                uint64_t mem = m_pDeviceMemMgr->Allocate( vkImg, vkMemFlags );
//                if( mem > 0 )
//                {
//                    hTex.handle = _AddResource( vkImg, ci, ResourceTypes::TEXTURE, m_vImages, m_vImageDescs, mem );
//                    if( pOut )
//                    {
//                        *pOut = vkImg;
//                    }
//                }
//                else
//                {
//                    Device.DestroyObject( nullptr, &vkImg );
//                }*/
//                DDITexture hImg = DDI.CreateObject( Desc, nullptr );
//            }
//           
//            return hTex;
//        }
//        
//        static const VkComponentMapping g_DefaultMapping =
//        {
//            VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A
//        };
//
//        TextureViewHandle CAPIResourceManager::CreateTextureView(const STextureViewDesc& Desc, DDITextureView* pOut)
//        {
//            /*SImageViewDesc ImgViewDesc;
//
//            assert( vkImg != VK_NULL_HANDLE );
//            VkImageViewCreateInfo& ci = ImgViewDesc.Info;
//            Vulkan::InitInfo( &ci, VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO );
//            ci.components = g_DefaultMapping;
//            ci.flags = 0;
//            ci.format = Vulkan::Map::Format( Desc.format );
//            ci.image = vkImg;
//            ci.subresourceRange.aspectMask = Vulkan::Map::ImageAspect( Desc.aspect );
//            ci.subresourceRange.baseArrayLayer = 0;
//            ci.subresourceRange.baseMipLevel = Desc.beginMipmapLevel;
//            ci.subresourceRange.layerCount = 1;
//            ci.subresourceRange.levelCount = Desc.endMipmapLevel;
//            ci.viewType = Vulkan::Map::ImageViewType( Desc.type );
//            VkImageView vkView;
//            VK_ERR( m_pCtx->_GetDDI().CreateObject( ci, nullptr, &vkView ) );*/
//    
//            DDITextureView hDDIView = m_pCtx->_GetDDI().CreateObject( Desc, nullptr );
//
//            TextureViewHandle hView = TextureViewHandle{ _AddResource( hDDIView, Desc, ResourceTypes::TEXTURE_VIEW,
//                                                                       m_vImageViews, m_vImageViewDescs, 0 ) };
//            if( hView != INVALID_HANDLE )
//            {
//                if( pOut )
//                {
//                    *pOut = hDDIView;
//                }
//            }
//            return hView;
//        }
//
//        TextureViewHandle CAPIResourceManager::CreateTextureView(const STextureViewDesc& Desc, DDITextureView* pOut)
//        {
//            /*VkImage vkImg = VK_NULL_HANDLE;
//            SImageViewDesc ImgViewDesc;
//            {
//                {
//                    vkImg = m_vImages[ static_cast< uint32_t >( Desc.hTexture.handle ) ];
//                    ImgViewDesc.hTexture = Desc.hTexture;
//                }
//            }
//            uint32_t endMipmapLevel = Desc.endMipmapLevel;
//            if( endMipmapLevel == 0 )
//            {
//                {
//                    const auto& ImgDesc = m_vImageDescs[static_cast<uint32_t>(Desc.hTexture.handle)];
//                    endMipmapLevel = ImgDesc.mipLevelCount;
//                }
//            }
//
//            assert(vkImg != VK_NULL_HANDLE);*/
//            /*VkImageViewCreateInfo& ci = ImgViewDesc.Info;
//            Vulkan::InitInfo(&ci, VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO);
//            ci.components = g_DefaultMapping;
//            ci.flags = 0;
//            ci.format = Vulkan::Map::Format(Desc.format);
//            ci.image = vkImg;
//            ci.subresourceRange.aspectMask = Vulkan::Map::ImageAspect(Desc.aspect);
//            ci.subresourceRange.baseArrayLayer = 0;
//            ci.subresourceRange.baseMipLevel = Desc.beginMipmapLevel;
//            ci.subresourceRange.layerCount = 1;
//            ci.subresourceRange.levelCount = Desc.endMipmapLevel;
//            ci.viewType = Vulkan::Map::ImageViewType(Desc.type);
//            VkImageView vkView;
//            VK_ERR(m_pCtx->_GetDDI().CreateObject(ci, nullptr, &vkView));*/
//            STextureViewDesc TmpDesc = Desc;
//            TmpDesc.hTexture.handle = reinterpret_cast<handle_t>( m_vImages[ Desc.hTexture.handle ] );
//            DDITextureView hDDIView = m_pCtx->_GetDDI().CreateObject( Desc, nullptr );
//            
//            TextureViewHandle hView =  TextureViewHandle{ _AddResource( hDDIView, TmpDesc, ResourceTypes::TEXTURE_VIEW,
//                                                                        m_vImageViews, m_vImageViewDescs, 0 ) };
//            if( hView != INVALID_HANDLE )
//            {
//                if( pOut )
//                {
//                    *pOut = hDDIView;
//                }
//            }
//            return hView;
//        }
//
//        TextureViewHandle CAPIResourceManager::CreateTextureView(const TextureHandle& hTexture, DDITextureView* pOut)
//        {
//            assert(hTexture != INVALID_HANDLE);
//            
//            const auto& TexDesc = GetTextureDesc(hTexture);
//            /*SImageViewDesc ImgViewDesc;
//            ImgViewDesc.hTexture = hTexture;
//            VkImageViewCreateInfo& ci = ImgViewDesc.Info;
//            Vulkan::InitInfo(&ci, VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO);
//            ci.components = g_DefaultMapping;
//            ci.flags = 0;
//            ci.format = TexDesc.format;
//            ci.image = m_vImages[static_cast<uint32_t>(hTexture.handle)];
//            ci.subresourceRange.aspectMask = Vulkan::Convert::UsageToAspectMask(TexDesc.usage);
//            ci.subresourceRange.baseArrayLayer = 0;
//            ci.subresourceRange.baseMipLevel = 0;
//            ci.subresourceRange.levelCount = TexDesc.mipLevels;
//            ci.subresourceRange.layerCount = 1;
//            ci.viewType = Vulkan::Convert::ImageTypeToViewType(TexDesc.imageType);
//            VkImageView vkView = VK_NULL_HANDLE;
//            VK_ERR(m_pCtx->_GetDDI().CreateObject(ci, nullptr, &vkView));
//            if( pOut )
//            {
//                *pOut = vkView;
//            }*/
//            STextureViewDesc Desc;
//            Desc.format = TexDesc.format;
//            Desc.aspect = TextureAspects::COLOR;
//            Desc.beginMipmapLevel = 0;
//            Desc.endMipmapLevel = TexDesc.mipLevelCount;
//            Desc.hTexture = hTexture;
//            Desc.type = TexDesc.type;
//            DDITextureView hView = m_pCtx->_GetDDI().CreateObject( Desc, nullptr );
//            return TextureViewHandle{ _AddResource( hView, Desc, ResourceTypes::TEXTURE_VIEW,
//                                                    m_vImageViews, m_vImageViewDescs, 0 ) };
//        }
//
//        FramebufferHandle CAPIResourceManager::CreateFramebuffer(const SFramebufferDesc& Desc)
//        {
//            SFramebufferDesc FbDesc;
//            
//            for( uint32_t i = 0; i < Desc.vAttachments.GetCount(); ++i )
//            {
//                handle_t hView = Desc.vAttachments[ i ].handle;
//                DDITextureView hDDIView = m_vImageViews[static_cast<uint32_t>(hView)];
//                if( hDDIView != DDI_NULL_HANDLE )
//                {
//                    FbDesc.vAttachments.PushBack( TextureViewHandle{ reinterpret_cast<handle_t>(hDDIView) } );
//                }
//                else
//                {
//                    VKE_LOG_ERR("Handle: " << Desc.vAttachments[i] << " does not exists.");
//                    return INVALID_HANDLE;
//                }
//            }
//            
//            VkRenderPass hRenderPass = m_vRenderpasses[static_cast<uint32_t>(Desc.hRenderPass.handle)];
//            if( hRenderPass == VK_NULL_HANDLE )
//            {
//                VKE_LOG_ERR("Handle: " << Desc.hRenderPass << " does not exists.");
//                return INVALID_HANDLE;
//            }
//
//            /*VkFramebufferCreateInfo ci;
//            Vulkan::InitInfo(&ci, VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO);
//            ci.attachmentCount = vImgViews.GetCount();
//            ci.flags = 0;
//            ci.height = Desc.Size.height;
//            ci.width = Desc.Size.width;
//            ci.layers = 1;
//            ci.pAttachments = &vImgViews[ 0 ];
//            ci.renderPass = vkRenderPass;
//            
//            VkFramebuffer vkFramebuffer;
//            VK_ERR(m_pCtx->_GetDDI().GetICD().CreateObject(ci, nullptr, &vkFramebuffer));*/
//            
//            FbDesc.hRenderPass.handle = reinterpret_cast<handle_t>(hRenderPass);
//            FbDesc.Size = Desc.Size;
//    
//            DDIFramebuffer hFramebuffer = m_pCtx->_GetDDI().CreateObject( FbDesc, nullptr );
//            return FramebufferHandle{ _AddResource(hFramebuffer, FbDesc, ResourceTypes::FRAMEBUFFER, m_vFramebuffers) };
//        }
//
//        const STextureDesc& CAPIResourceManager::GetTextureDesc(const TextureViewHandle& hView) const
//        {
//            const STextureViewDesc& Desc = m_vImageViewDescs[ hView.handle ];
//            if( Desc.hTexture == INVALID_HANDLE )
//            {
//                return GetTextureDesc( Desc.hTexture );
//            }
//            return m_vImageDescs[ Desc.hTexture.handle ];
//        }
//        
//        RenderPassHandle CAPIResourceManager::CreateRenderPass(const SRenderPassDesc& /*Desc*/)
//        {
//            return INVALID_HANDLE;
//        }
//
//        TextureHandle CAPIResourceManager::_FindTexture(const VkImage& vkImg) const
//        {
//            for( uint32_t i = 0; i < m_vImages.GetCount(); ++i )
//            {
//                if( m_vImages[ i ] == vkImg )
//                {
//                    return TextureHandle{ i };
//                }
//            }
//            return INVALID_HANDLE;
//        }
//
//        const VkImageCreateInfo& CAPIResourceManager::FindTextureDesc(const TextureViewHandle& hView) const
//        {
//            const auto& ViewDesc = GetTextureViewDesc(hView);
//            auto hTex = _FindTexture(ViewDesc.image);
//            return GetTextureDesc(hTex);
//        }
//
//        Result CAPIResourceManager::AddTexture(const VkImage& vkImg, const VkImageCreateInfo& Info)
//        {
//            m_mCustomTextures.insert(TextureMap::value_type( vkImg, Info ) );
//            return VKE_OK;
//        }
//
//        const VkImageCreateInfo& CAPIResourceManager::GetTextureDesc(const VkImage& vkImg) const
//        {
//            auto Itr = m_mCustomTextures.find(vkImg);
//            return Itr->second;
//        }
//
//        void CAPIResourceManager::RemoveTexture(const VkImage& vkImg)
//        {
//            m_mCustomTextures.erase(vkImg);
//        }
//    }
//}
//#endif // VKE_VULKAN_RENDER_SYSTEM