#include "RenderSystem/CRenderPass.h"
#if VKE_VULKAN_RENDERER
#include "RenderSystem/CDeviceContext.h"
#include "RenderSystem/Managers/CAPIResourceManager.h"

namespace VKE
{
    namespace RenderSystem
    {
        hash_t CRenderPass::CalcHash( const SRenderPassDesc& Desc )
        {
            Utils::SHash Hash;
            Hash.Combine( Desc.Size.width, Desc.Size.height );
            for( uint32_t i = 0; i < Desc.vRenderTargets.GetCount(); ++i )
            {
                const auto& Curr = Desc.vRenderTargets[i];
                Hash.Combine( Curr.beginLayout, Curr.endLayout, Curr.format, Curr.hTextureView.handle, Curr.sampleCount );
                Hash.Combine( Curr.usage, Curr.ClearValue.Color.r, Curr.ClearValue.Color.g, Curr.ClearValue.Color.b );
                Hash.Combine( Curr.ClearValue.Color.a );
            }
            for( uint32_t i = 0; i < Desc.vSubpasses.GetCount(); ++i )
            {
                const auto& Curr = Desc.vSubpasses[i];
                Hash.Combine( Curr.DepthBuffer.hTextureView.handle, Curr.DepthBuffer.layout );
                for( uint32_t t = 0; t < Curr.vTextures.GetCount(); ++t )
                {
                    const auto& Curr2 = Curr.vTextures[t];
                    Hash.Combine( Curr2.hTextureView.handle, Curr2.layout );
                }
                for( uint32_t r = 0; r < Curr.vRenderTargets.GetCount(); ++r )
                {
                    const auto& Curr2 = Curr.vRenderTargets[r];
                    Hash.Combine( Curr2.hTextureView.handle, Curr2.layout );
                }
            }

            return Hash.value;
        }


        CRenderPass::CRenderPass(CDeviceContext* pCtx) :
            m_pCtx( pCtx )
        {}

        CRenderPass::~CRenderPass()
        {

        }

        void CRenderPass::_Destroy(bool destroyRenderPass)
        {
            if( destroyRenderPass )
            {
                m_pCtx->_GetDDI().DestroyRenderPass( &m_hDDIObject, nullptr );
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

        // DDI api handles only
        Result CRenderPass::Create( const SRenderPassDesc& Desc )
        {
            Result ret = VKE_FAIL;
            m_hDDIObject = m_pCtx->_GetDDI().CreateRenderPass( Desc, nullptr );
            if( m_hDDIObject != DDI_NULL_HANDLE )
            {
                m_Desc = Desc;
                SFramebufferDesc FbDesc;
                FbDesc.hRenderPass.handle = (handle_t)(m_hDDIObject);
                FbDesc.Size = Desc.Size;

                for( uint32_t i = 0; i < Desc.vRenderTargets.GetCount(); ++i )
                {
                    TextureViewHandle hView = Desc.vRenderTargets[i].hTextureView;
                    VKE_ASSERT( hView != INVALID_HANDLE, "A proper texture view handle must be set in Attachment" );
                    if( hView != INVALID_HANDLE )
                    {
                        //DDITextureView hDDIView = reinterpret_cast<DDITextureView>(hView.handle);
                        TextureViewPtr pView = m_pCtx->GetTextureView( hView );
                        FbDesc.vDDIAttachments.PushBack( pView->GetDDIObject() );

                        DDIClearValue DDIValue;
                        m_pCtx->DDI().Convert( Desc.vRenderTargets[i].ClearValue, &DDIValue );
                        m_BeginInfo.vDDIClearValues.PushBack( DDIValue );
                    }
                    else
                    {
                        VKE_LOG_ERR( "Null TextureViewHandle for RenderPass Attachment." );
                        break;
                    }
                }
                m_hDDIFramebuffer = m_pCtx->_GetDDI().CreateFramebuffer( FbDesc, nullptr );
                if( m_hDDIFramebuffer != DDI_NULL_HANDLE )
                {
                    ret = VKE_OK;
                    m_BeginInfo.hDDIFramebuffer = m_hDDIFramebuffer;
                    m_BeginInfo.hDDIRenderPass = m_hDDIObject;
                    m_BeginInfo.RenderArea.Offset.x = 0;
                    m_BeginInfo.RenderArea.Offset.y = 0;
                    m_BeginInfo.RenderArea.Size = Desc.Size;
                }
                ret = VKE_OK;
            }

            return ret;
        }

        CRenderPass::SRenderTargetDesc& CRenderPass::AddRenderTarget( TextureViewHandle hView )
        {
            SRenderTargetDesc Desc;
            Desc.hTextureView = hView;
            VKE_RENDER_SYSTEM_SET_DEBUG_NAME( Desc, m_pCtx->GetTextureView( hView )->GetDesc().pDebugName );
            uint32_t idx = m_Desc.vRenderTargets.PushBack( Desc );
            m_isDirty = true;
            return m_Desc.vRenderTargets[ idx ];
        }

        uint32_t CRenderPass::AddRenderTarget( const SRenderTargetDesc& Desc )
        {
            m_isDirty = true;
            return m_Desc.vRenderTargets.PushBack( Desc );
        }

        uint32_t CRenderPass::AddPass( const SPassDesc& Desc )
        {
            m_isDirty = true;
            return m_Desc.vSubpasses.PushBack( Desc );
        }

        CRenderPass::SPassDesc& CRenderPass::AddPass( cstr_t pName )
        {
            SPassDesc Desc;
            VKE_RENDER_SYSTEM_SET_DEBUG_NAME( Desc, pName );
            uint32_t idx = m_Desc.vSubpasses.PushBack( Desc );
            m_isDirty = true;
            return m_Desc.vSubpasses[ idx ];
        }

    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER