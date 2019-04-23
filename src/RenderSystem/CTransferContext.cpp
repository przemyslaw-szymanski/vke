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
            Destroy();
        }

        void CTransferContext::Destroy()
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
                m_pCurrentCommandBuffer = this->_CreateCommandBuffer();
            }
            return ret;
        }
    }
}