#include "RenderSystem/Vulkan/CResourceManager.h"
#if VKE_VULKAN_RENDERER
#include "RenderSystem/CDeviceContext.h"
#include "Core/Utils/CLogger.h"
#include "Core/VKEConfig.h"

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
            Result res = VKE_FAIL;
            m_Desc = Desc;
            m_vImages.PushBack(VK_NULL_HANDLE);
            m_vImageViews.PushBack(VK_NULL_HANDLE);
            m_vRenderpasses.PushBack(VK_NULL_HANDLE);
            m_vFramebuffers.PushBack(VK_NULL_HANDLE);
            m_vImageDescs.PushBack({});
            m_vImageViewDescs.PushBack({});
            res = _SetBestMemoryFlags();
            if( VKE_SUCCEEDED( res ) )
            {
                res = _AllocateMemoryPools();
            }
            return res;
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

        Result CResourceManager::_AllocateMemoryPools()
        {
            Result res = VKE_OK;

            for( uint32_t memType = 0; memType < MemoryAccessTypes::_MAX_COUNT; ++memType )
            {
                for( uint32_t resType = 0; resType < ResourceTypes::_MAX_COUNT; ++resType )
                {
                    auto& memSize = m_Desc.aMemorySizes[ memType ];
                    if( memSize == 0 )
                    {
                        memSize = Config::DEFAULT_GPU_ACCESS_MEMORY_POOL_SIZE;
                    }
                    /*res = _AllocateMemoryPool( static_cast<MEMORY_ACCESS_TYPE>(memType), static_cast<RESOURCE_TYPE>(resType), memSize );
                    if( VKE_FAILED( res ) )
                    {
                        return res;
                    }*/
                }
            }
            return res;
        }

        int32_t FindMemoryTypeIndex(const VkPhysicalDeviceMemoryProperties& MemProps,
                                    uint32_t requiredMemBits,
                                    VkMemoryPropertyFlags requiredProperties)
        {
            const uint32_t memCount = MemProps.memoryTypeCount;
            for( uint32_t memIdx = 0; memIdx < memCount; ++memIdx )
            {
                const uint32_t memTypeBits = ( 1 << memIdx );
                const bool isRequiredMemType = requiredMemBits & memTypeBits;
                const VkMemoryPropertyFlags props = MemProps.memoryTypes[ memIdx ].propertyFlags;
                const bool hasRequiredProps = ( props & requiredProperties ) == requiredProperties;
                if( isRequiredMemType && hasRequiredProps )
                    return static_cast< int32_t >( memIdx );
            }
            return -1;
        }

        int32_t FindMemoryTypeIndex2(const VkPhysicalDeviceMemoryProperties& MemProps, VkMemoryPropertyFlags flags)
        {
            const uint32_t memCount = MemProps.memoryTypeCount;
            for( uint32_t memIdx = 0; memIdx < memCount; ++memIdx )
            {
                if( MemProps.memoryTypes[ memIdx ].propertyFlags & flags )
                {
                    return static_cast< int32_t >( memIdx );
                }
            }
            return -1;
        }

        Result CResourceManager::_SetBestMemoryFlags()
        {
            return VKE_OK;
        }

        static const VkMemoryPropertyFlags g_aRequiredMemoryFlags[] =
        {
            0, // unknown
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, // gpu
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, // cpu access
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT // cpu access optimal
        };

        
        Result CResourceManager::_AllocateMemoryPool(SAllocateMemoryPoolInfo* pInOut)
        {
            Result res = VKE_FAIL;
            auto& Device = m_pCtx->_GetDevice();
            const auto& MemProps = m_pCtx->GetDeviceInfo().MemoryProperties;
            SAllocateMemoryInfo* pAllocInfo = pInOut->pAllocateInfoInOut;
            VkMemoryPropertyFlags requiredFlags = g_aRequiredMemoryFlags[ pAllocInfo->accessType ];
            const VkMemoryRequirements& vkMemReq = pAllocInfo->MemChunk.vkMemoryRequirements;
            int32_t idx = FindMemoryTypeIndex( MemProps, vkMemReq.memoryTypeBits, requiredFlags );
            const VkDeviceSize vkAlignedSize = pInOut->size + ( pInOut->size % vkMemReq.alignment );
            VkMemoryAllocateInfo MemInfo;
            Vulkan::InitInfo( &MemInfo, VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO );
            MemInfo.allocationSize = vkAlignedSize;
            MemInfo.memoryTypeIndex = static_cast< uint32_t >( idx );
            VkDeviceMemory vkMemory;
            VkResult vkRes = m_pCtx->_GetDevice().AllocateMemory( MemInfo, nullptr, &vkMemory );
            if( vkRes == VK_SUCCESS )
            {
                res = VKE_OK;
                SFreeMemoryInfo FreeInfo;
                FreeInfo.vkMemory = vkMemory;
                FreeInfo.currentOffset = 0;
                FreeInfo.freeMemory = vkAlignedSize;
                auto& MemoryType = pAllocInfo->pMemoryInOut->mMemoryTypes[ vkMemReq.memoryTypeBits ];
                SMemoryInfo MemInfo;
                MemInfo.typeIndex = idx;
                MemInfo.vkMemory = vkMemory;
                MemoryType.vBuffers.PushBack( MemInfo );
                MemoryType.vFreeInfos.PushBack( FreeInfo );
            }
            else
            {
                VKE_LOG_ERR( "Unable to create memory pool for memory type bits: " << vkMemReq.memoryTypeBits << " and size: " << vkAlignedSize );
            }
            return res;
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
                if( pOut )
                {
                    *pOut = vkImg;
                }
            }
            {
                VkMemoryRequirements vkMemReq;
                Device.GetMemoryRequirements( vkImg, &vkMemReq );
                SAllocateMemoryInfo AllocInfo;
                AllocInfo.accessType = MemoryAccessTypes::GPU;
                AllocInfo.pMemoryInOut = &m_Memory;
                AllocInfo.MemChunk.vkMemoryRequirements = vkMemReq;
                if( VKE_SUCCEEDED( _AllocateMemory( &AllocInfo ) ) )
                {
                    VK_ERR( Device.BindMemory( vkImg, AllocInfo.MemChunk.vkMemory, AllocInfo.MemChunk.vkOffset ) );
                    hTex = TextureHandle{ _AddResource( vkImg, ci, ResourceTypes::TEXTURE, 
                                                        m_vImages, m_vImageDescs, &AllocInfo ) };
                }
                else
                {
                    Device.DestroyObject( nullptr, &vkImg );
                }

                {
                    _AllocateMemory( &AllocInfo );
                    _FreeMemory( &m_Memory, AllocInfo.MemChunk );
                    _AllocateMemory( &AllocInfo );
                    _FreeMemory( &m_Memory, AllocInfo.MemChunk );
                    _AllocateMemory( &AllocInfo );
                    _FreeMemory( &m_Memory, AllocInfo.MemChunk );
                    _AllocateMemory( &AllocInfo );
                    _FreeMemory( &m_Memory, AllocInfo.MemChunk );
                }
            }
            return hTex;
        }

        Result CResourceManager::_FreeMemory(SMemory* pMem, const SAllocatedMemoryChunk& MemChunk)
        {
            Result res = VKE_FAIL;
            SFreeMemoryInfo FreeInfo;
            FreeInfo.currentOffset = MemChunk.vkOffset;
            FreeInfo.vkMemory = MemChunk.vkMemory;
            FreeInfo.freeMemory = MemChunk.vkMemoryRequirements.size;
            FreeInfoVec* pvFreeInfos;

            Threads::ScopedLock l( pMem->SyncObj );
            if( pMem->lastUsedTypeBits == MemChunk.vkMemoryRequirements.memoryTypeBits )
            { 
                pvFreeInfos = &pMem->pLastUsedMemory->vFreeInfos;
                res = VKE_OK;
            }
            else
            {
                auto& MemType = pMem->mMemoryTypes[ MemChunk.vkMemoryRequirements.memoryTypeBits ];
                pvFreeInfos = &MemType.vFreeInfos;
                res = VKE_OK;
            }
            pvFreeInfos->PushBack( FreeInfo );
            // Merge free chunks
            // Sort
            auto pBegin = &( *pvFreeInfos )[ 0 ];
            auto pEnd = pBegin + pvFreeInfos->GetCount();
            std::sort( pBegin, pEnd, [](const SFreeMemoryInfo& A, const SFreeMemoryInfo& B)
            {
                return A.currentOffset < B.currentOffset;
            } );
            FreeInfoVec vFreeInfos;
            SFreeMemoryInfo CurrFreeInfo;
            const uint32_t count = pvFreeInfos->GetCount();
            for( uint32_t i = 0; i < count; ++i )
            {
                const SFreeMemoryInfo& FirstInfo = pvFreeInfos->At( i );
                CurrFreeInfo = FirstInfo;
                for( uint32_t j = i + 1; j < count; ++j )
                {
                    const SFreeMemoryInfo& NextInfo = pvFreeInfos->At( j );
                    if( CurrFreeInfo.vkMemory == NextInfo.vkMemory )
                    {
                        if( CurrFreeInfo.currentOffset + CurrFreeInfo.freeMemory == NextInfo.currentOffset )
                        {
                            CurrFreeInfo.freeMemory += NextInfo.freeMemory;
                            CurrFreeInfo.vkMemory = FirstInfo.vkMemory;
                            i = j;
                        }
                        else
                        {
                            break;
                        }
                    }
                }
                vFreeInfos.PushBack( CurrFreeInfo );
            }
            pvFreeInfos->FastClear();
            pvFreeInfos->Append( vFreeInfos );
            return res;
        }

        Result CResourceManager::_FindBestFitFreeMemory(const FreeInfoVec& vFreeInfos, uint32_t size, SFreeMemoryInfo** ppOut)
        {
            uint32_t minIdx = UINT32_MAX;
            uint32_t minDiff = UINT32_MAX;
            Result res = VKE_FAIL;
            for( uint32_t i = 0; i < vFreeInfos.GetCount(); ++i )
            {
                if( vFreeInfos[ i ].freeMemory >= size )
                {
                    uint32_t diff = vFreeInfos[ i ].freeMemory - size;
                    if( diff < minDiff )
                    {
                        minDiff = diff;
                        minIdx = i;
                    }
                }
            }
            if( minIdx != UINT32_MAX )
            {
                res = VKE_OK;
                *ppOut = &vFreeInfos[ minIdx ];
            }
            return res;
        }

        Result CResourceManager::_GetMemory(SGetMemoryInfo* pInOut)
        {
            SMemory* pMemory = pInOut->pAllocateInfo->pMemoryInOut;
            SAllocateMemoryInfo* pAllocInfo = pInOut->pAllocateInfo;

            Threads::ScopedLock l( pMemory->SyncObj );
            Result res = VKE_FAIL;
            const auto bits = pAllocInfo->MemChunk.vkMemoryRequirements.memoryTypeBits;
            const auto size = pAllocInfo->MemChunk.vkMemoryRequirements.size;
            const auto alignment = pAllocInfo->MemChunk.vkMemoryRequirements.alignment;
            SFreeMemoryInfo* pMemInfo = nullptr;
            
            if( bits == pMemory->lastUsedTypeBits )
            {
                res = _FindBestFitFreeMemory( pMemory->pLastUsedMemory->vFreeInfos, size, &pMemInfo );
            }
            else
            {
                auto& MemType = pMemory->mMemoryTypes[ bits ];
                if( !MemType.vBuffers.IsEmpty() || !MemType.vFreeInfos.IsEmpty() )
                {
                    res = _FindBestFitFreeMemory( MemType.vFreeInfos, size, &pMemInfo );
                    if( VKE_SUCCEEDED( res ) )
                    {
                        pMemory->lastUsedTypeBits = bits;
                        pMemory->pLastUsedMemory = &MemType;
                    }
                }
            }
            if( pMemInfo )
            {
                SFreeMemoryInfo& Info = *pMemInfo;
                const uint32_t sizeLeft = Info.freeMemory - Info.currentOffset;
                const uint32_t alignedSize = size + ( size % alignment );
                if( sizeLeft > alignedSize )
                {
                    pAllocInfo->MemChunk.vkOffset = Info.currentOffset;
                    pAllocInfo->MemChunk.vkMemory = Info.vkMemory;
                    Info.freeMemory -= alignedSize;
                    Info.currentOffset += alignedSize;
                }
                else
                {
                    res = VKE_FAIL;
                }
            }
            return res;
        }

        VkResult memory_type_from_properties(const VkPhysicalDeviceMemoryProperties& Props, uint32_t typeBits, VkFlags properties, uint32_t *typeIndex )
        {
            // Search memtypes to find first index with those properties
            for( uint32_t i = 0; i < 32; i++ )
            {
                if( ( typeBits & 1 ) == 1 )
                {
                    // Type is available, does it match user properties?
                    if( ( Props.memoryTypes[ i ].propertyFlags & properties ) == properties )
                    {
                        *typeIndex = i;
                        return VK_SUCCESS;
                    }
                }
                typeBits >>= 1;
            }
            // No memory types matched, return failure
            return VK_ERROR_FORMAT_NOT_SUPPORTED;
        }

        Result CResourceManager::_AllocateMemory(SAllocateMemoryInfo* pInOut)
        {
            Result res = VKE_FAIL;
            SGetMemoryInfo GetInfo;
            GetInfo.pAllocateInfo = pInOut;
            if( VKE_SUCCEEDED( _GetMemory( &GetInfo ) ) )
            {
                res = VKE_OK;
            }
            else
            {
                SAllocateMemoryPoolInfo Info;
                Info.pAllocateInfoInOut = pInOut;
                Info.size = m_Desc.aMemorySizes[ pInOut->accessType ];
                res = _AllocateMemoryPool( &Info );
                if( VKE_SUCCEEDED( res ) )
                {
                    res = _GetMemory( &GetInfo );
                }
            }
            return res;
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
            return TextureViewHandle{ _AddResource( vkView, ci, ResourceTypes::TEXTURE_VIEW,
                                                    m_vImageViews, m_vImageViewDescs, nullptr ) };
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
            return TextureViewHandle{ _AddResource( vkView, ci, ResourceTypes::TEXTURE_VIEW,
                                                    m_vImageViews, m_vImageViewDescs, nullptr ) };
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