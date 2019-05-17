#pragma once
#if VKE_VULKAN_RENDERER

#include "RenderSystem/CDDI.h"
#include "RenderSystem/Common.h"
#include "Core/VKEForwardDeclarations.h"

namespace VKE
{
    namespace RenderSystem
    {
        class CGraphicsContext;
        class CDeviceContext;

        class VKE_API CRenderPass final : public Core::CObject
        {
            friend class CGraphicsContext;
            friend class CDeviceContext;
            friend class CRenderingPipeline;
            friend class CSwapChain;
            friend class CCommandBuffer;

            using ImageArray = Utils::TCDynamicArray< DDITexture, 8 >;
            using ImageViewArray = Utils::TCDynamicArray< DDITextureView, 8 >;
            using ClearValueArray = Utils::TCDynamicArray< SClearValue, 8 >;
            using FramebufferArray = Utils::TCDynamicArray< DDIFramebuffer, 8 >;

            VKE_ADD_DDI_OBJECT( DDIRenderPass );

            public:

                using SRenderTargetDesc = SRenderPassDesc::SRenderTargetDesc;
                using SPassDesc = SRenderPassDesc::SSubpassDesc;

                class CPass
                {
                    public:

                        

                    protected:
                };

            public:

                CRenderPass(CDeviceContext*);
                ~CRenderPass();

                static hash_t  CalcHash( const SRenderPassDesc& Desc );

                Result  Create(const SRenderPassDesc& Desc);
                Result  Update(const SRenderPassDesc& Desc);
                void    Clear(const SColor& ClearColor, float clearDepth, float clearStencil);
                

                DDITexture GetColorRenderTarget(uint32_t idx) const { return m_vImages[ idx ]; }
                DDITextureView GetColorRenderTargetView(uint32_t idx) const { return m_vImageViews[idx]; }

                bool    IsActive() const { return m_isActive; }

                const SBeginRenderPassInfo& GetBeginInfo() const { return m_BeginInfo; }

                SRenderTargetDesc&  AddRenderTarget( TextureViewHandle hView );
                uint32_t            AddRenderTarget( const SRenderTargetDesc& Desc );

                SPassDesc&  AddPass( cstr_t pName );
                uint32_t    AddPass( const SPassDesc& Desc );

            protected:

                const VkImageCreateInfo* _AddTextureView(TextureViewHandle hView);
                Result _CreateTextureView(const STextureDesc& Desc, TextureHandle* phTextureOut,
                    TextureViewHandle* phTextureViewOut, const VkImageCreateInfo** ppCreateInfoOut);

                void    _IsActive(bool is) { m_isActive = is; }

                void    _Destroy( bool destroyRenderPass = true );

            protected:

                SRenderPassDesc         m_Desc;

                //VkRenderPassBeginInfo   m_vkBeginInfo;
                //Vulkan::CDeviceWrapper& m_VkDevice;
                SBeginRenderPassInfo    m_BeginInfo;
                CDeviceContext*         m_pCtx;
                ImageArray              m_vImages;
                ImageViewArray          m_vImageViews;
                DDIFramebuffer          m_hDDIFramebuffer = DDI_NULL_HANDLE;
                bool                    m_isActive = false;
                bool                    m_isDirty = false;
        };
        using RenderPassPtr = Utils::TCWeakPtr< CRenderPass >;
        using RenderPassRefPtr = Utils::TCObjectSmartPtr< CRenderPass >;

    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER