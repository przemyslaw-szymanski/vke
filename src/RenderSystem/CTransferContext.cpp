#include "RenderSystem/CTransferContext.h"

namespace VKE
{
    namespace RenderSystem
    {
        CTransferContext::CTransferContext( CDeviceContext* pCtx ) :
            CContextBase( pCtx )
        {
            // Transfer queue should not wait for anything
            this->m_additionalEndFlags = CommandBufferEndFlags::DONT_WAIT_FOR_SEMAPHORE;
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
                
            }

            return ret;
        }

        CCommandBuffer* CTransferContext::GetCommandBuffer()
        {
            // Transfer context should be used as a singleton
            // For each thread a different command buffer should be created
            auto pCmdBuff = this->_CreateCommandBuffer();
            pCmdBuff->Begin();
            return pCmdBuff;
        }

    }
}