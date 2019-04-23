#pragma once

#include "RenderSystem/CContextBase.h"
#include "RenderSystem/CCommandBuffer.h"

namespace VKE
{
    namespace RenderSystem
    {
        class CTransferContext : public CContextBase, public CCommandBufferTransferContext
        {
            public:

            CTransferContext( CDeviceContext* pCtx );
            ~CTransferContext();

            Result Create( const STransferContextDesc& Desc );
            void Destroy();

            protected:

                STransferContextDesc    m_Desc;
        };
    }
}