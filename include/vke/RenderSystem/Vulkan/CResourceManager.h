#pragma once
#if VKE_VULKAN_RENDERER

#include "RenderSystem/Vulkan/Vulkan.h"

namespace VKE
{
    namespace RenderSystem
    {
        class CGraphicsContext;
        class CDeviceContext;

        struct SResourceManagerDesc
        {

        };

        class VKE_API CResourceManager final
        {
            friend class CGraphicsContext;
            friend class CDeviceContext;

            static const uint32_t DEFAULT_RESOURCE_COUNT = 256;

            using ImageArray = Utils::TCDynamicArray< VkImage, DEFAULT_RESOURCE_COUNT >;
            using ImageViewArray = Utils::TCDynamicArray< VkImageView, DEFAULT_RESOURCE_COUNT >;
            using FramebufferArray = Utils::TCDynamicArray< VkFramebuffer, DEFAULT_RESOURCE_COUNT >;
            using RenderpassArray = Utils::TCDynamicArray< VkRenderPass, DEFAULT_RESOURCE_COUNT >;
            using SamplerArray = Utils::TCDynamicArray< VkSampler, DEFAULT_RESOURCE_COUNT >;
            using UintArray = Utils::TCDynamicArray< uint16_t >;
            using ImageDescArray = Utils::TCDynamicArray< VkImageCreateInfo, DEFAULT_RESOURCE_COUNT >;
            using ImageViewDescArray = Utils::TCDynamicArray< VkImageViewCreateInfo, DEFAULT_RESOURCE_COUNT >;
            using TextureMap = vke_hash_map< VkImage, VkImageCreateInfo >;

            public:

                CResourceManager(CDeviceContext*);
                ~CResourceManager();

                Result Create(const SResourceManagerDesc& Desc);
                void Destroy();

                TextureHandle CreateTexture(const STextureDesc& Desc, VkImage* pOut = nullptr);
                TextureViewHandle CreateTextureView(const STextureViewDesc& Desc, VkImageView* pOut = nullptr);
                TextureViewHandle CreateTextureView(const TextureHandle& hTexture, VkImageView* pOut = nullptr);
                RenderPassHandle CreateRenderPass(const SRenderPassDesc& Desc);
                FramebufferHandle CreateFramebuffer(const SFramebufferDesc& Desc);

                Result AddTexture(const VkImage& vkImg, const VkImageCreateInfo& Info);
                const VkImageCreateInfo& GetTextureDesc(const VkImage& vkImg) const;
                void RemoveTexture(const VkImage& vkImg);
                //Result AddTextureView(const VkImageView& vkView, const VkImageViewCreateInfo& Info);
                //const VkImageViewCreateInfo& GetTextureViewDesc(const VkImageView& vkView) const;

                void DestroyTexture(const TextureHandle& hTex);
                void DestroyTextureView(const TextureViewHandle& hTexView);
                void DestroyRenderPass(const RenderPassHandle& hPass);
                void DestroyFramebuffer(const FramebufferHandle& hFramebuffer);

                const VkImage& GetTexture(const TextureHandle& hTex) const { return m_vImages[hTex.handle]; }
                const VkImageCreateInfo& GetTextureDesc(const TextureHandle& hTex) const { return m_vImageDescs[hTex.handle]; }
                const VkImageView& GetTextureView(const TextureViewHandle& hView) const { return m_vImageViews[hView.handle]; }
                const VkImageViewCreateInfo& GetTextureViewDesc(const TextureViewHandle& hView) const
                {
                    return m_vImageViewDescs[ hView.handle ];
                }
                const VkImageCreateInfo& FindTextureDesc(const TextureViewHandle& hView) const;

                TextureHandle _FindTexture(const VkImage& vkImg) const;

            protected:

                template<typename VkResourceType, typename ArrayType>
                VkResourceType _DestroyResource(const handle_t& hRes, ArrayType& vResources, RESOURCE_TYPE type)
                {
                    const auto& vkRes = vResources[ hRes ];
                    {
                        Threads::ScopedLock l(m_aSyncObjects[ type ]);
                        m_avFreeHandles[ type ].PushBack(hRes);
                    }
                    return vkRes;
                }

                template<typename VkResType, typename VkInfo, typename ArrayType, typename ArrayDescType>
                handle_t _AddResource(VkResType vkRes, const VkInfo& Info, RESOURCE_TYPE type, ArrayType& vResources,
                                      ArrayDescType& Descs)
                {
                    Threads::ScopedLock l(m_aSyncObjects[ type ]);
                    uint16_t freeId;
                    handle_t idx = NULL_HANDLE;
                    if( m_avFreeHandles[ type ].PopBack(&freeId) )
                    {
                        idx = freeId;
                        vResources[ freeId ] = vkRes;
                        Descs[ freeId ] = Info;
                    }
                    else
                    {
                        idx = vResources.PushBack(vkRes);
                        Descs.PushBack(Info);
                    }
                    return idx;
                }

                template<typename VkResType, typename VkInfo, typename ArrayType>
                handle_t _AddResource(VkResType vkRes, const VkInfo& Info, RESOURCE_TYPE type, ArrayType& vResources)
                {
                    Threads::ScopedLock l(m_aSyncObjects[ type ]);
                    uint16_t freeId;
                    handle_t idx = NULL_HANDLE;
                    if( m_avFreeHandles[ type ].PopBack(&freeId) )
                    {
                        idx = freeId;
                        vResources[ freeId ] = vkRes;
                    }
                    else
                    {
                        idx = vResources.PushBack(vkRes);
                    }
                    return idx;
                }

                

            protected:

                ImageArray          m_vImages;
                ImageViewArray      m_vImageViews;
                FramebufferArray    m_vFramebuffers;
                RenderpassArray     m_vRenderpasses;

                ImageDescArray      m_vImageDescs;
                ImageViewDescArray  m_vImageViewDescs;

                UintArray           m_avFreeHandles[ResourceTypes::_MAX_COUNT];
                Threads::SyncObject m_aSyncObjects[ ResourceTypes::_MAX_COUNT ];

                TextureMap          m_mCustomTextures;

                CDeviceContext*     m_pCtx;
        };
    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER