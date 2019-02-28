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

                        DDIClearValue DDIValue;
                        m_pCtx->DDI().Convert( Desc.vAttachments[i].ClearValue, &DDIValue );
                        DDIValue.color.uint32[0] = 1;
                        DDIValue.color.uint32[1] = 0;
                        DDIValue.color.uint32[2] = 0;
                        DDIValue.color.uint32[3] = 1;
                        m_BeginInfo.vDDIClearValues.PushBack( DDIValue );
                    }
                    else
                    {
                        VKE_LOG_ERR( "Null TextureViewHandle for RenderPass Attachment." );
                        break;
                    }
                }
                /*m_hDDIFramebuffer = m_pCtx->_GetDDI().CreateObject( FbDesc, nullptr );
                if( m_hDDIFramebuffer != DDI_NULL_HANDLE )
                {
                    ret = VKE_OK;
                    m_BeginInfo.hDDIFramebuffer = m_hDDIFramebuffer;
                    m_BeginInfo.hDDIRenderPass = m_hDDIObject;
                    m_BeginInfo.RenderArea.Offset.x = 0;
                    m_BeginInfo.RenderArea.Offset.y = 0;
                    m_BeginInfo.RenderArea.Size = Desc.Size;
                }*/
                ret = VKE_OK;
            }

            return ret;
        }

    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER