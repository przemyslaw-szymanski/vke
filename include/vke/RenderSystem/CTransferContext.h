#pragma once

#include "RenderSystem/CContextBase.h"
#include "RenderSystem/CCommandBuffer.h"

namespace VKE
{
    namespace RenderSystem
    {
        class VKE_API CTransferContext : public CContextBase
        {
            friend class CDeviceContext;
            friend class CContextBase;

            using CommandBufferMap = vke_hash_map< std::thread::id, CCommandBuffer* >;

            public:

                CTransferContext( CDeviceContext* pCtx );
                ~CTransferContext();

                Result Create( const STransferContextDesc& Desc );
                void Destroy();

                CCommandBuffer* GetCommandBuffer();

            protected:

                void    _Destroy();
                Result  _Execute( bool pushSemaphore );

            protected:

                STransferContextDesc    m_Desc;
                CommandBufferMap        m_mCommandBuffers;
                Threads::SyncObject     m_CmdBuffSyncObj;
        };
    }
}