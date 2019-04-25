#include "RenderSystem/CTransferContext.h"

namespace VKE
{
    namespace RenderSystem
    {
        CTransferContext::CTransferContext( CDeviceContext* pCtx ) :
            CContextBase( pCtx )
        {

        }

        CTransferContext::~CTransferContext()
        {
            _Destroy();
        }

        void CTransferContext::Destroy()
        {
            // auto pThis = this;
            // m_pCtx->DestroyTransferContext( &pThis );
        }

        void CTransferContext::_Destroy()
        {
            if( this->m_pDeviceCtx != nullptr )
            {
                CContextBase::Destroy();
            }
        }

        Result CTransferContext::Create( const STransferContextDesc& Desc )
        {
            Result ret = VKE_OK;
            m_Desc = Desc;
            SContextBaseDesc& BaseDesc = *reinterpret_cast<SContextBaseDesc*>(m_Desc.pPrivate);
            ret = CContextBase::Create( BaseDesc );
            if( VKE_SUCCEEDED( ret ) )
            {
                this->m_pCurrentCommandBuffer = this->_CreateCommandBuffer();
            }

            return ret;
        }

        void CTransferContext::Begin()
        {
            if( this->m_pCurrentCommandBuffer->GetState() == CCommandBuffer::States::BEGIN )
            {
                End();
            }

            this->m_pCurrentCommandBuffer->Begin();
        }

        void CTransferContext::End()
        {
            if( this->m_pCurrentCommandBuffer->GetState() == CCommandBuffer::States::BEGIN )
            {
                this->m_pCurrentCommandBuffer->End( CommandBufferEndFlags::EXECUTE );
                this->m_pCurrentCommandBuffer = this->_CreateCommandBuffer();
            }
        }

        void CTransferContext::Copy( const SCopyBufferInfo& Info )
        {
            if( this->m_pCurrentCommandBuffer->GetState() != CCommandBuffer::States::BEGIN )
            {
                this->m_pCurrentCommandBuffer->Begin();
            }
            this->m_pCurrentCommandBuffer->Copy( Info );
        }
    }
}