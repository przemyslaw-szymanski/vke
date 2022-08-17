#include "RenderSystem/CRenderPass.h"
#if VKE_VULKAN_RENDER_SYSTEM
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
                const auto& Curr = Desc.vRenderTargets[ i ];
                Hash.Combine( Curr.beginState, Curr.endState, Curr.format, Curr.hTextureView.handle,
                              Curr.sampleCount );
                Hash.Combine( Curr.usage, Curr.ClearValue.Color.r, Curr.ClearValue.Color.g, Curr.ClearValue.Color.b );
                Hash.Combine( Curr.ClearValue.Color.a );
            }
            for( uint32_t i = 0; i < Desc.vSubpasses.GetCount(); ++i )
            {
                const auto& Curr = Desc.vSubpasses[ i ];
                Hash.Combine( Curr.DepthBuffer.hTextureView.handle, Curr.DepthBuffer.state );
                for( uint32_t t = 0; t < Curr.vTextures.GetCount(); ++t )
                {
                    const auto& Curr2 = Curr.vTextures[ t ];
                    Hash.Combine( Curr2.hTextureView.handle, Curr2.state );
                }
                for( uint32_t r = 0; r < Curr.vRenderTargets.GetCount(); ++r )
                {
                    const auto& Curr2 = Curr.vRenderTargets[ r ];
                    Hash.Combine( Curr2.hTextureView.handle, Curr2.state );
                }
            }
            return Hash.value;
        }

        hash_t CRenderPass::CalcHash(const SSimpleRenderPassDesc& Desc)
        {
            Utils::SHash Hash;
            Hash.Combine( Desc.GetDebugName() );
            for(uint32_t i =0; i < Desc.vRenderTargets.GetCount(); ++i)
            {
                const auto& Curr = Desc.vRenderTargets[ i ];
                Hash.Combine( Curr.ClearColor.Color.r, Curr.ClearColor.Color.g, Curr.ClearColor.Color.b, Curr.ClearColor.Color.a );
                Hash.Combine( Curr.renderPassOp, *(handle_t*)&Curr.RenderTarget, Curr.state );
            }
            return Hash.value;
        }

        CRenderPass::CRenderPass( CDeviceContext* pCtx )
            : m_pCtx( pCtx )
        {
        }
        CRenderPass::~CRenderPass() {}
        void CRenderPass::_Destroy( bool destroyRenderPass )
        {
            if( destroyRenderPass )
            {
                m_pCtx->_GetDDI().DestroyRenderPass( &m_hDDIObject, nullptr );
            }
        }
        int32_t FindTextureHandle( const SRenderPassDesc::AttachmentDescArray& vAttachments,
                                   const TextureViewHandle& hTexView )
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
        bool MakeAttachmentRef( const SRenderPassDesc::AttachmentDescArray& vAttachments,
                                const SSubpassAttachmentDesc& SubpassAttachmentDesc, VkAttachmentReference* pRefOut )
        {
            int32_t idx = FindTextureHandle( vAttachments, SubpassAttachmentDesc.hTextureView );
            bool res = false;
            if( idx >= 0 )
            {
                pRefOut->attachment = idx;
                pRefOut->layout = Vulkan::Map::ImageLayout( SubpassAttachmentDesc.state );
                res = true;
            }
            return res;
        }
        // DDI api handles only
        Result CRenderPass::Create( const SRenderPassDesc& Desc )
        {
            Result ret = VKE_OK;
            m_Desc = Desc;
            if( !Desc.vRenderTargetDescs.IsEmpty() )
            {
                for( auto& RtDesc: Desc.vRenderTargetDescs )
                {
                    auto hRT = m_pCtx->CreateRenderTarget( RtDesc );
                    if( hRT == INVALID_HANDLE )
                    {
                        ret = VKE_FAIL;
                        break;
                    }
                    auto pRT = m_pCtx->GetRenderTarget( hRT );
                    SRenderPassDesc::SRenderTargetDesc RtPassDesc;
                    RtPassDesc.beginState = RtDesc.beginState;
                    RtPassDesc.endState = RtDesc.endState;
                    RtPassDesc.hTextureView = pRT->GetTextureView();
                    RtPassDesc.ClearValue = RtDesc.ClearValue;
                    RtPassDesc.format = RtDesc.format;
                    RtPassDesc.sampleCount = RtDesc.multisampling;
                    RtPassDesc.usage = RtDesc.renderPassUsage;
                    RtPassDesc.SetDebugName( RtDesc.GetDebugName() );
                    m_Desc.vRenderTargets.PushBack( RtPassDesc );
                }
            }
            if( VKE_SUCCEEDED( ret ) )
            {
                m_hDDIObject = m_pCtx->_GetDDI().CreateRenderPass( m_Desc, nullptr );
            }
            if( m_hDDIObject != DDI_NULL_HANDLE )
            {
                SFramebufferDesc FbDesc;
                FbDesc.hRenderPass.handle = ( handle_t )( m_hDDIObject );
                FbDesc.Size = m_Desc.Size;
                for( uint32_t i = 0; i < m_Desc.vRenderTargets.GetCount(); ++i )
                {
                    TextureViewHandle hView = m_Desc.vRenderTargets[ i ].hTextureView;
                    VKE_ASSERT( hView != INVALID_HANDLE, "A proper texture view handle must be set in Attachment" );
                    if( hView != INVALID_HANDLE )
                    {
                        // DDITextureView hDDIView = reinterpret_cast<DDITextureView>(hView.handle);
                        TextureViewPtr pView = m_pCtx->GetTextureView( hView );
                        FbDesc.vDDIAttachments.PushBack( pView->GetDDIObject() );
                        DDIClearValue DDIValue;
                        m_pCtx->DDI().Convert( m_Desc.vRenderTargets[ i ].ClearValue, &DDIValue );
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
                    m_BeginInfo.RenderArea.Position.x = 0;
                    m_BeginInfo.RenderArea.Position.y = 0;
                    m_BeginInfo.RenderArea.Size = m_Desc.Size;
                }
                ret = VKE_OK;
            }
            return ret;
        }

        RenderTargetPtr CRenderPass::_GetRenderTarget( const RenderTargetID& ID )
        {
            RenderTargetPtr pRT;
            switch( ID.type )
            {
                case RES_ID_HANDLE:
                {
                    pRT = m_pCtx->GetRenderTarget( ID.handle );
                }
                break;
                case RES_ID_NAME:
                {
                    pRT = m_pCtx->GetRenderTarget( ID.name );
                }
                break;
                case RES_ID_POINTER:
                {
                    pRT = *(RenderTargetPtr*)ID.ptr;
                }
                break;
            }
            return pRT;
        }

        Result CRenderPass::Create( const SSimpleRenderPassDesc& Desc )
        {
            Result ret = VKE_OK;
            m_BeginInfo2.RenderArea = Desc.RenderArea;

            for( uint32_t i =0;i < Desc.vRenderTargets.GetCount(); ++i )
            {
                const auto& Info = Desc.vRenderTargets[ i ];
                ret = SetRenderTarget( i, Info );
                if( VKE_FAILED(ret) )
                {
                    break;
                }
            }
            return ret;
        }

        CRenderPass::SRenderTargetDesc& CRenderPass::AddRenderTarget( TextureViewHandle hView )
        {
            SRenderTargetDesc Desc;
            Desc.hTextureView = hView;
            //VKE_RENDER_SYSTEM_SET_DEBUG_NAME( Desc, m_pCtx->GetTextureView( hView )->GetDesc().GetDebugName() );
            Desc.SetDebugName( m_pCtx->GetTextureView( hView )->GetDesc().GetDebugName() );
            uint32_t idx = m_Desc.vRenderTargets.PushBack( Desc );
            m_isDirty = true;
            return m_Desc.vRenderTargets[ idx ];
        }
        uint32_t CRenderPass::AddRenderTarget( const SRenderTargetDesc& Desc )
        {
            m_isDirty = true;
            return m_Desc.vRenderTargets.PushBack( Desc );
        }
        Result CRenderPass::SetRenderTarget( uint32_t idx, const SSetRenderTargetInfo& Info )
        {
            Result ret = VKE_OK;
            RenderTargetPtr pRT = _GetRenderTarget( Info.RenderTarget );
            SRenderTargetInfo RTInfo;
            VKE_ASSERT( pRT.IsValid(), "" );
            if( idx < MAX_RT_COUNT )
            {
                TexturePtr pTex = m_pCtx->GetTexture( pRT->GetTexture() );
                RTInfo.hDDIView = pTex->GetView()->GetDDIObject();
                RTInfo.ClearColor = Info.ClearColor;
                RTInfo.state = Info.state;
                RTInfo.renderPassOp = Info.renderPassOp;
                if( pTex->IsColor() )
                {
                    if( idx >= m_vColorRenderTargetInfos.GetCount() )
                    {
                        m_vColorRenderTargetInfos.Resize( idx + 1 );
                        m_BeginInfo2.vColorRenderTargetInfos.Resize( idx + 1 );
                    }
                    m_vColorRenderTargetInfos[ idx ] = RTInfo;
                    m_BeginInfo2.vColorRenderTargetInfos[ idx ] = RTInfo;
                }
                else if( pTex->IsDepth() )
                {
                    m_DepthRenderTargetInfo = RTInfo;
                    m_BeginInfo2.DepthRenderTargetInfo = RTInfo;
                }
                else if( pTex->IsStencil() )
                {
                    m_StencilRenderTargetInfo = RTInfo;
                    m_BeginInfo2.StencilRenderTargetInfo = RTInfo;
                }
            }
            else
            {
                VKE_LOG_ERR( "Render target index out of max supported render target count in RenderPass: "
                             << m_SimpleDesc.GetDebugName() );
                ret = VKE_FAIL;
            }
            return ret;
        }
        uint32_t CRenderPass::AddPass( const SPassDesc& Desc )
        {
            m_isDirty = true;
            return m_Desc.vSubpasses.PushBack( Desc );
        }
        CRenderPass::SPassDesc& CRenderPass::AddPass( cstr_t pName )
        {
            SPassDesc Desc;
            //VKE_RENDER_SYSTEM_SET_DEBUG_NAME( Desc, pName );
            Desc.SetDebugName( pName );
            uint32_t idx = m_Desc.vSubpasses.PushBack( Desc );
            m_isDirty = true;
            return m_Desc.vSubpasses[ idx ];
        }
    } // namespace RenderSystem
} // namespace VKE
#endif // VKE_VULKAN_RENDER_SYSTEM