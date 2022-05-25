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
            using CommandBufferArray = Utils::TCDynamicArray< CCommandBuffer* >;

            public:

                CTransferContext( CDeviceContext* pCtx );
                ~CTransferContext();

                Result Create( const STransferContextDesc& Desc );
                void Destroy();

                CCommandBuffer* GetCommandBuffer();

                template<EXECUTE_COMMAND_BUFFER_FLAGS Flags = ExecuteCommandBufferFlags::END>
                Result                      Execute(bool pushSemaphore);

            protected:

                void    _Destroy();
                Result  _Execute( bool pushSemaphore, EXECUTE_COMMAND_BUFFER_FLAGS flags = 0 );

            protected:

                STransferContextDesc    m_Desc;
                //CommandBufferMap        m_mCommandBuffers;
                //CommandBufferArray      m_vCommandBuffers;
                Threads::SyncObject     m_CmdBuffSyncObj;
        };
    } // RenderSystem

    namespace RenderSystem
    {
        template<EXECUTE_COMMAND_BUFFER_FLAGS Flags>
        Result CTransferContext::Execute(bool pushSemaphore)
        {
            return _Execute(pushSemaphore, Flags);
        }
    }
} // VKE