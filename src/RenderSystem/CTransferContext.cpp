#include "RenderSystem/CTransferContext.h"
#include "RenderSystem/CDeviceContext.h"

namespace VKE
{
    namespace RenderSystem
    {
        CTransferContext::CTransferContext( CDeviceContext* pCtx ) : CContextBase( pCtx, "Transfer" )
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

        void CTransferContext::Copy( const SCopyBufferToTextureInfo& Info, TEXTURE_STATE finalState,
            CTexture** ppInOut)
        {
            CTexture* pTex = *ppInOut;
            Threads::ScopedLock l( m_CmdBuffSyncObj );
            auto pCmdBuffer = _GetCommandBuffer();
            STextureBarrierInfo BarrierInfo;
            if( pTex->SetState( TextureStates::TRANSFER_DST, &BarrierInfo ) )
            {
                pCmdBuffer->Barrier( BarrierInfo );
            }
            pCmdBuffer->Copy( Info );
            if( pTex->SetState( finalState, &BarrierInfo ) )
            {
                pCmdBuffer->Barrier( BarrierInfo );
            }
        }

        CCommandBuffer* CTransferContext::_GetCommandBuffer()
        {
            
            return this->_GetCurrentCommandBuffer();
        }

        Result CTransferContext::_Execute( bool pushSemaphore, EXECUTE_COMMAND_BUFFER_FLAGS flags )
        {
            Result res = VKE_OK;

            Threads::ScopedLock l( m_CmdBuffSyncObj );
            if(!this->m_vCommandBuffers.IsEmpty())
            {
                {
                    
                    //for( auto& Pair : m_mCommandBuffers )
                    for( uint32_t i = 0; i < this->m_vCommandBuffers.GetCount(); ++i )
                    {
                        auto& pCmdBuff = this->m_vCommandBuffers[ i ];
                        if( pCmdBuff != nullptr )
                        {
                            res = pCmdBuff->End( ExecuteCommandBufferFlags::END | flags, nullptr );
                            pCmdBuff = nullptr;
                        }
                    }
                    this->m_vCommandBuffers.Clear();
                }
                CCommandBufferBatch* pBatch;
                res = this->m_pQueue->_GetSubmitManager()->ExecuteCurrentBatch( this, this->m_pQueue,
                                                                                &pBatch );
                if (VKE_SUCCEEDED(res) )
                {
                    if( pushSemaphore )
                    {
                        this->m_pDeviceCtx->_PushSignaledSemaphore( pBatch->GetSignaledSemaphore() );
                    }
                    if( ( flags & ExecuteCommandBufferFlags::WAIT ) == ExecuteCommandBufferFlags::WAIT )
                    {
                        this->m_pQueue->Wait();
                        auto hPool = this->_GetCommandBufferManager().GetPool();
                        this->m_pQueue->_GetSubmitManager()->_FreeBatch( this, hPool, &pBatch );
                    }
                }
            }
            
            return res;
        }

    } //
}