#include "RenderSystem/CTransferContext.h"
#include "RenderSystem/CDeviceContext.h"

namespace VKE
{
    namespace RenderSystem
    {
        CTransferContext::CTransferContext( CDeviceContext* pCtx ) :
            CContextBase( pCtx )
        {
            // Transfer queue should not wait for anything
            this->m_additionalEndFlags = ExecuteCommandBufferFlags::DONT_WAIT_FOR_SEMAPHORE;
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
            Threads::ScopedLock l(m_CmdBuffSyncObj);
            const auto threadId = std::this_thread::get_id();
            auto pCmdBuff = m_mCommandBuffers[ threadId ];
            if( pCmdBuff == nullptr )
            {
                // Transfer context should be used as a singleton
                // For each thread a different command buffer should be created
                pCmdBuff = this->_CreateCommandBuffer();
                pCmdBuff->Begin();
                m_mCommandBuffers[ threadId ] = pCmdBuff;
            }
            VKE_ASSERT( pCmdBuff->GetState() == CCommandBuffer::States::BEGIN, "" );
            return pCmdBuff;
        }

        Result CTransferContext::_Execute( bool pushSemaphore, EXECUTE_COMMAND_BUFFER_FLAGS flags )
        {
            Result res = VKE_OK;

            if( !m_mCommandBuffers.empty() )
            {
                {
                    Threads::ScopedLock l( m_CmdBuffSyncObj );
                    for( auto& Pair : m_mCommandBuffers )
                    {
                        auto& pCmdBuff = Pair.second;
                        if( pCmdBuff != nullptr )
                        {
                            res = pCmdBuff->End( ExecuteCommandBufferFlags::END, nullptr );
                            pCmdBuff = nullptr;
                        }
                    }
                }
                CCommandBufferBatch* pBatch;
                res = this->m_pQueue->_GetSubmitManager()->ExecuteCurrentBatch( this->m_pDeviceCtx, this->m_pQueue,
                                                                                &pBatch );
                if (VKE_SUCCEEDED(res) && (flags & ExecuteCommandBufferFlags::WAIT) )
                {
                    this->m_pQueue->Wait();
                    this->m_pQueue->_GetSubmitManager()->_FreeBatch(this->m_pDeviceCtx, this->m_hCommandPool, &pBatch);
                }

                if( pushSemaphore )
                {
                    this->m_pDeviceCtx->_PushSignaledSemaphore( pBatch->GetSignaledSemaphore() );
                }
                {
                    Threads::ScopedLock l( m_CmdBuffSyncObj );
                    m_mCommandBuffers.clear();
                }
            }
            return res;
        }

    } //
}