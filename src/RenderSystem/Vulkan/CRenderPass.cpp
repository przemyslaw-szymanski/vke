#include "RenderSystem/CRenderPass.h"
#if VKE_VULKAN_RENDERER
#include "RenderSystem/CDeviceContext.h"
#include "RenderSystem/Managers/CAPIResourceManager.h"

namespace VKE
{
    namespace RenderSystem
    {
        CRenderPass::CRenderPass(CDeviceContext* pCtx) :
            m_pCtx( pCtx )
        {}

        CRenderPass::~CRenderPass()
        {
            Destroy();
        }

        void CRenderPass::Destroy(bool destroyRenderPass)
        {
            if (m_hFramebuffer == DDI_NULL_HANDLE)
            {
                return;
            }

            m_pCtx->_GetDDI().DestroyObject( &m_hFramebuffer, nullptr );
            if( destroyRenderPass )
            {
                m_pCtx->_GetDDI().DestroyObject( &m_hDDIObject, nullptr );
            }
        }

        int32_t FindTextureHandle(const SRenderPassDesc::AttachmentDescArray& vAttachments,
                                  const TextureViewHandle& hTexView)
        {
            int32_t res = -1;
            for( uint32_t a = 0; a < vAttachments.GetCount(); ++a )
            {
                if( hTexView == vAttachments[ a ].hTextureView )
                {
                    res = a;
                    break;
                }
            }
            return res;
        }

        bool MakeAttachmentRef(const SRenderPassDesc::AttachmentDescArray& vAttachments,
                               const SSubpassAttachmentDesc& SubpassAttachmentDesc,
                               VkAttachmentReference* pRefOut)
        {
            int32_t idx = FindTextureHandle(vAttachments, SubpassAttachmentDesc.hTextureView);
            bool res = false;
            if( idx >= 0 )
            {
                pRefOut->attachment = idx;
                pRefOut->layout = Vulkan::Map::ImageLayout(SubpassAttachmentDesc.layout);
                res = true;
            }
            return res;
        }

        Result CRenderPass::Create(const SRenderPassDesc& Desc)
        {
            Destroy();

            m_Desc = Desc;

            auto& vAttachments = m_Desc.vAttachments;
            for( uint32_t i = 0; i < vAttachments.GetCount(); ++i )
            {
                DDITextureView hDDIView = m_pCtx->GetTextureView( vAttachments[i].hTextureView )->GetDDIObject();
                vAttachments[i].hTextureView.handle = reinterpret_cast<handle_t>( hDDIView );
            }
            auto& vSubpasses = m_Desc.vSubpasses;
            for( uint32_t i = 0; i < vSubpasses.GetCount(); ++i )
            {
                auto& vTextures = vSubpasses[i].vTextures;
                for( uint32_t t = 0; t < vTextures.GetCount(); ++t )
                {
                    DDITextureView hDDIView = m_pCtx->GetTextureView( vTextures[t].hTextureView )->GetDDIObject();
                    vTextures[t].hTextureView.handle = reinterpret_cast<handle_t>(hDDIView);
                }
                auto& vRenderTargets = vSubpasses[i].vRenderTargets;
                for( uint32_t r = 0; r < vRenderTargets.GetCount(); ++r )
                {
                    DDITextureView hDDIView = m_pCtx->GetTextureView( vRenderTargets[r].hTextureView )->GetDDIObject();
                    vRenderTargets[r].hTextureView.handle = reinterpret_cast<handle_t>(hDDIView);
                }
            }

            return VKE_OK;
        }

        // DDI api handles only
        Result CRenderPass::Create2( const SRenderPassDesc& Desc )
        {
            Result ret = VKE_FAIL;
            m_hDDIObject = m_pCtx->_GetDDI().CreateObject( Desc, nullptr );
            if( m_hDDIObject != DDI_NULL_HANDLE )
            {
                m_Desc = Desc;
                SFramebufferDesc FbDesc;
                FbDesc.hRenderPass.handle = reinterpret_cast<handle_t>(m_hDDIObject);
                FbDesc.Size = Desc.Size;
                for( uint32_t i = 0; i < Desc.vAttachments.GetCount(); ++i )
                {
                    TextureViewHandle hView = Desc.vAttachments[i].hTextureView;
                    VKE_ASSERT( hView != NULL_HANDLE, "A proper texture view handle must be set in Attachment" );
                    if( hView != NULL_HANDLE )
                    {
                        DDITextureView hDDIView = reinterpret_cast<DDITextureView>(hView.handle);
                        FbDesc.vAttachments.PushBack( hView );
                    }
                    else
                    {
                        VKE_LOG_ERR( "Null TextureViewHandle for RenderPass Attachment." );
                        break;
                    }
                }
                m_hFramebuffer = m_pCtx->_GetDDI().CreateObject( FbDesc, nullptr );
                if( m_hFramebuffer != DDI_NULL_HANDLE )
                {
                    ret = VKE_OK;
                }
            }

            return ret;
        }

        void CRenderPass::Begin(const DDICommandBuffer& hCb)
        {
            //assert(m_state != State::BEGIN);
            auto& ICD = m_pCtx->_GetDDI().GetICD();

            VkRenderPassBeginInfo vkBeginInfo;
            Vulkan::InitInfo( &vkBeginInfo, VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO );
            vkBeginInfo.clearValueCount = m_vClearValues.GetCount();
            vkBeginInfo.pClearValues = &m_vClearValues[0];
            vkBeginInfo.framebuffer = m_hFramebuffer;
            vkBeginInfo.renderPass = m_hDDIObject;

            vkBeginInfo.renderArea.extent.width = m_Desc.Size.width;
            vkBeginInfo.renderArea.extent.height = m_Desc.Size.height;
            vkBeginInfo.renderArea.offset.x = 0;
            vkBeginInfo.renderArea.offset.y = 0;

            VkClearValue ClearValue;
            ClearValue.color.float32[ 0 ] = ( float )( rand() % 100 ) / 100.0f;
            ClearValue.color.float32[ 1 ] = ( float )( rand() % 100 ) / 100.0f;
            ClearValue.color.float32[ 2 ] = ( float )( rand() % 100 ) / 100.0f;
            ClearValue.color.float32[ 3 ] = ( float )( rand() % 100 ) / 100.0f;
            ClearValue.depthStencil.depth = 0.0f;
            vkBeginInfo.clearValueCount = 1;
            vkBeginInfo.pClearValues = &ClearValue;
            // $TID BeginRenderPass: rp={(void*)this}, cb={vkCb}, rp={m_vkBeginInfo.renderPass}, fb={m_vkBeginInfo.framebuffer}
            ICD.vkCmdBeginRenderPass( hCb, &vkBeginInfo, VK_SUBPASS_CONTENTS_INLINE );
            //m_state = State::BEGIN;
        }

        void CRenderPass::End(const DDICommandBuffer& hCb)
        {
            //assert(m_state == State::BEGIN);
            auto& ICD = m_pCtx->_GetDDI().GetICD();
            ICD.vkCmdEndRenderPass( hCb );
            //m_state = State::END;
        }

    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER