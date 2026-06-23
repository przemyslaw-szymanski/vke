#pragma once

#include "RenderSystem/RHI.h"
#include "RenderSystem/Common.h"
#include "Core/VKEForwardDeclarations.h"

namespace VKE
{
    namespace RenderSystem
    {
        class CGraphicsContext;
        class CDeviceContext;

        class VKE_API CRenderPass
        {
            friend class CGraphicsContext;
            friend class CDeviceContext;
            friend class CRenderingPipeline;
            friend class CSwapChain;
            friend class CCommandBuffer;

        public:
            static const uint32_t MAX_RT_COUNT = 8;

        protected:
            using ImageArray       = Utils::TCDynamicArray< RHI::Texture, 8 >;
            using ImageViewArray   = Utils::TCDynamicArray< RHI::TextureView, 8 >;
            using ClearValueArray  = Utils::TCDynamicArray< SClearValue, 8 >;
            using FramebufferArray = Utils::TCDynamicArray< RHI::Framebuffer, 8 >;

            VKE_ADD_DDI_OBJECT( RHI::RenderPass );
            VKE_DECL_BASE_OBJECT( RenderPassHandle );

            using SRenderTargetDesc = SRenderPassDesc::SRenderTargetDesc;
            using SPassDesc         = SRenderPassDesc::SSubpassDesc;

            /*struct SRenderTargetInfo
            {
                RHI::TextureView hView;
                TEXTURE_STATE state;
                RENDER_TARGET_RENDER_PASS_OP renderPassOp;
                SClearValue ClearValue;
            };*/

        public:
            using RenderTargetInfoArray = Utils::TCDynamicArray< SRenderTargetInfo, MAX_RT_COUNT >;

        public:
            CRenderPass( CDeviceContext* );
            ~CRenderPass();

            static hash_t CalcHash( const SRenderPassDesc& Desc );
            static hash_t CalcHash( const SSimpleRenderPassDesc& Desc );

            Result Create( const SRenderPassDesc& Desc );
            Result Create( const SSimpleRenderPassDesc& Desc );
            Result Update( const SRenderPassDesc& Desc );
            void   Clear( const SColor& ClearColor, float clearDepth, float clearStencil );

            RHI::Texture GetColorRenderTarget( uint32_t idx ) const
            {
                return m_vImages[ idx ];
            }

            RHI::TextureView GetColorRenderTargetView( uint32_t idx ) const
            {
                return m_vImageViews[ idx ];
            }

            bool IsActive() const
            {
                return m_isActive;
            }

            const SBeginRenderPassInfo& GetBeginInfo() const
            {
                return m_BeginInfo;
            }

            const RenderTargetInfoArray& GetColorRenderTargetInfos() const
            {
                return m_vColorRenderTargetInfos;
            }

            const SRenderTargetInfo& GetDepthRenderTargetInfo() const
            {
                return m_DepthRenderTargetInfo;
            }

            const SRenderTargetInfo& GetStencilRenderTargetInfo() const
            {
                return m_StencilRenderTargetInfo;
            }

            SRenderTargetDesc& AddRenderTarget( TextureViewHandle hView );
            uint32_t           AddRenderTarget( const SRenderTargetDesc& Desc );

            Result SetRenderTarget( uint32_t idx, const SSetRenderTargetInfo& Info );

            SPassDesc& AddPass( cstr_t pName );
            uint32_t   AddPass( const SPassDesc& Desc );

        protected:
            /*const VkImageCreateInfo* _AddTextureView(TextureViewHandle hView);
            Result _CreateTextureView(const STextureDesc& Desc, TextureHandle* phTextureOut,
                TextureViewHandle* phTextureViewOut, const VkImageCreateInfo** ppCreateInfoOut);*/

            void _IsActive( bool is )
            {
                m_isActive = is;
            }

            void _Destroy( bool destroyRenderPass = true );

            RenderTargetPtr _GetRenderTarget( const RenderTargetID& ID );

        protected:
            SRenderPassDesc       m_Desc;
            SSimpleRenderPassDesc m_SimpleDesc;
            // VkRenderPassBeginInfo   m_vkBeginInfo;
            // Vulkan::CDeviceWrapper& m_VkDevice;
            SBeginRenderPassInfo   m_BeginInfo;
            SBeginRenderPassInfo2  m_BeginInfo2;
            RenderTargetInfoArray  m_vColorRenderTargetInfos;
            SRenderTargetInfo      m_DepthRenderTargetInfo;
            SRenderTargetInfo      m_StencilRenderTargetInfo;
            CDeviceContext*        m_pCtx;
            ImageArray             m_vImages;
            ImageViewArray         m_vImageViews; // color only
            RHI::Framebuffer m_hDDIFramebuffer = RHI::Null;
            bool                   m_isActive        = false;
            bool                   m_isDirty         = false;
        };

    } // namespace RenderSystem
} // namespace VKE
