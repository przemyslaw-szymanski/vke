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

            public:

                CResourceManager(CDeviceContext*);
                ~CResourceManager();

                Result Create(const SResourceManagerDesc& Desc);
                void Destroy();

                handle_t CreateTexture(const STextureDesc& Desc);
                handle_t CreateTextureView(const STextureViewDesc& Desc);
                handle_t CreateRenderPass(const SRenderPassDesc& Desc);
                handle_t CreateFramebuffer(const SFramebufferDesc& Desc);

                void DestroyTexture(const handle_t& hTex);
                void DestroyTextureView(const handle_t& hTexView);
                void DestroyRenderPass(const handle_t& hPass);
                void DestroyFramebuffer(const handle_t& hFramebuffer);

                const VkImage& GetTexture(const handle_t& hTex) const { return m_vImages[hTex]; }
                const VkImageCreateInfo& GetTextureDesc(const handle_t& hTex) { return m_vImageDescs[hTex]; }
                const VkImageView& GetTextureView(const handle_t& hView) const { return m_vImageViews[hView]; }
                const VkImageViewCreateInfo& GetTextureViewDesc(const handle_t& hView) const
                {
                    return m_vImageViewDescs[ hView ];
                }

                handle_t _FindTexture(const VkImage& vkImg);

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

                CDeviceContext*     m_pCtx;
        };
    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER