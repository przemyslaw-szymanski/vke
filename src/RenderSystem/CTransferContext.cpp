#include "RenderSystem/CTransferContext.h"

namespace VKE
{
    namespace RenderSystem
    {
        CTransferContext::CTransferContext( CDeviceContext* pCtx ) :
            m_pCtx( pCtx )
        {

        }

        CTransferContext::~CTransferContext()
        {
            Destroy();
        }

        void CTransferContext::Destroy()
        {
            if( m_pCtx != nullptr )
            {
                m_pCtx = nullptr;
            }
        }

        Result CTransferContext::Create( const STransferContextDesc& Desc )
        {
            Result ret = VKE_OK;
            m_Desc = Desc;
            ret = CContextBase::Create( Desc.BaseDesc );
            if( VKE_SUCCEEDED( ret ) )
            {
                m_pCurrentCommandBuffer = this->_CreateCommandBuffer();
            }
            return ret;
        }
    }
}