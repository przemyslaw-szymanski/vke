#pragma once

#include "RenderSystem/CContextBase.h"
#include "RenderSystem/CCommandBuffer.h"

namespace VKE
{
    namespace RenderSystem
    {
        class CTransferContext : public CContextBase
        {
            friend class CDeviceContext;
            friend class CContextBase;

            public:

                CTransferContext( CDeviceContext* pCtx );
                ~CTransferContext();

                Result Create( const STransferContextDesc& Desc );
                void Destroy();

                void Begin();
                void End();

                void Copy( const SCopyBufferInfo& Info );

            protected:

                void    _Destroy();

            protected:

                STransferContextDesc    m_Desc;
        };
    }
}