#include "RenderSystem/CContextBase.h"
#include "RenderSystem/CCommandBuffer.h"
#include "RenderSystem/CDeviceContext.h"
#include "RenderSystem/Managers/CBufferManager.h"
#include "RenderSystem/Managers/CTextureManager.h"
#include "RenderSystem/Managers/CDescriptorSetManager.h"

namespace VKE
{
    namespace RenderSystem
    {
        static CCommandBufferBatch g_sDummyBatch;

        static BINDING_TYPE BufferUsageToBindingType( const BUFFER_USAGE usage )
        {
            BINDING_TYPE ret = BindingTypes::_MAX_COUNT;
            if( ( usage & BufferUsages::CONSTANT_BUFFER ) == BufferUsages::CONSTANT_BUFFER )
            {
                ret = BindingTypes::CONSTANT_BUFFER;
                if( ( usage & BufferUsages::TEXEL_BUFFER ) == BufferUsages::TEXEL_BUFFER )
                {
                    ret = BindingTypes::READ_ONLY_TEXEL_BUFFER;
                }
            }
            else if( ( usage & BufferUsages::BUFFER ) == BufferUsages::BUFFER )
            {
                ret = BindingTypes::BUFFER;
                if( ( usage & BufferUsages::TEXEL_BUFFER ) == BufferUsages::TEXEL_BUFFER )
                {
                    ret = BindingTypes::READ_WRITE_TEXEL_BUFFER;
                }
            }
            VKE_ASSERT2( ret != BindingTypes::_MAX_COUNT, "Invalid buffer usage." );
            return ret;
        }

        void SCreateBindingDesc::AddBinding( const SResourceBinding& Binding, const BufferPtr& pBuffer )
        {
            SDescriptorSetLayoutDesc::SBinding BindInfo;
            BindInfo.count  = Binding.count;
            BindInfo.idx    = Binding.index;
            BindInfo.stages = Binding.stages;
            BindInfo.type   = BufferUsageToBindingType( pBuffer->GetDesc().usage );
            BindInfo.space  = Binding.space;
            LayoutDesc.vBindings.PushBack( BindInfo );
        }

        void SCreateBindingDesc::AddBinding( const STextureBinding& Binding )
        {
            SDescriptorSetLayoutDesc::SBinding BindInfo;
            BindInfo.count  = Binding.count;
            BindInfo.idx    = Binding.index;
            BindInfo.stages = Binding.stages;
            BindInfo.type   = BindingTypes::TEXTURE;
            BindInfo.space  = Binding.space;
            LayoutDesc.vBindings.PushBack( BindInfo );
        }

        void SCreateBindingDesc::AddBinding( const SSamplerBinding& Binding )
        {
            SDescriptorSetLayoutDesc::SBinding BindInfo;
            BindInfo.count  = Binding.count;
            BindInfo.idx    = Binding.index;
            BindInfo.stages = Binding.stages;
            BindInfo.type   = BindingTypes::SAMPLER;
            BindInfo.space  = Binding.space;
            LayoutDesc.vBindings.PushBack( BindInfo );
        }

        void SCreateBindingDesc::AddConstantBuffer( uint8_t index, PIPELINE_STAGES stages, uint16_t space )
        {
            SDescriptorSetLayoutDesc::SBinding BindInfo;
            BindInfo.count  = 1;
            BindInfo.idx    = index;
            BindInfo.stages = stages;
            BindInfo.type   = BindingTypes::CONSTANT_BUFFER;
            BindInfo.space  = space;
            LayoutDesc.vBindings.PushBack( BindInfo );
        }

        void SCreateBindingDesc::AddBuffer( uint8_t index, PIPELINE_STAGES stages, const uint16_t& arrayElementCount, uint16_t space )
        {
            SDescriptorSetLayoutDesc::SBinding BindInfo;
            BindInfo.count  = arrayElementCount;
            BindInfo.idx    = index;
            BindInfo.stages = stages;
            BindInfo.type   = BindingTypes::BUFFER;
            BindInfo.space  = space;
            LayoutDesc.vBindings.PushBack( BindInfo );
        }

        void SCreateBindingDesc::AddDynamicConstantBuffer( uint8_t index, PIPELINE_STAGES stages, uint16_t space )
        {
            SDescriptorSetLayoutDesc::SBinding BindInfo;
            BindInfo.count  = 1;
            BindInfo.idx    = index;
            BindInfo.stages = stages;
            BindInfo.type   = BindingTypes::DYNAMIC_CONSTANT_BUFFER;
            BindInfo.space  = space;
            LayoutDesc.vBindings.PushBack( BindInfo );
        }

        void SCreateBindingDesc::AddDynamicBuffer( uint8_t index, PIPELINE_STAGES stages,
                                                   const uint16_t& arrayElementCount, uint16_t space )
        {
            SDescriptorSetLayoutDesc::SBinding BindInfo;
            BindInfo.count  = arrayElementCount;
            BindInfo.idx    = index;
            BindInfo.stages = stages;
            BindInfo.type   = BindingTypes::DYNAMIC_BUFFER;
            BindInfo.space  = space;
            LayoutDesc.vBindings.PushBack( BindInfo );
        }

        void SCreateBindingDesc::AddTextures( uint8_t index, PIPELINE_STAGES stages, uint16_t count,
                                              uint16_t space )
        {
            SDescriptorSetLayoutDesc::SBinding BindInfo;
            BindInfo.count  = count;
            BindInfo.idx    = index;
            BindInfo.stages = stages;
            BindInfo.type   = BindingTypes::TEXTURE;
            BindInfo.space  = space;
            LayoutDesc.vBindings.PushBack( BindInfo );
        }

        void SCreateBindingDesc::AddSamplers( uint8_t index, PIPELINE_STAGES stages, uint16_t count,
                                              uint16_t space )
        {
            SDescriptorSetLayoutDesc::SBinding BindInfo;
            BindInfo.count  = count;
            BindInfo.idx    = index;
            BindInfo.stages = stages;
            BindInfo.type   = BindingTypes::SAMPLER;
            BindInfo.space  = space;
            LayoutDesc.vBindings.PushBack( BindInfo );
        }

        CContextBase::CContextBase( CDeviceContext* pCtx, cstr_t pName ) :
            m_DDI( pCtx->RHI() ), m_pDeviceCtx( pCtx ), m_pName( pName ), m_pLastExecutedBatch( &g_sDummyBatch ),
            m_CmdBuffMgr( this )
        {
        }

        Result CContextBase::Create( const SContextBaseDesc& Desc )
        {
            Result ret = VKE_OK;
            m_pQueue   = Desc.pQueue;

            SCommandBufferManagerDesc MgrDesc;
            ret = m_CmdBuffMgr.Create( MgrDesc );

            if( VKE_SUCCEEDED( ret ) )
            {
                m_pQueue->SetDebugName( m_pName );
            }
            return ret;
        }

        void CContextBase::Destroy()
        {
            if( m_pDeviceCtx != nullptr )
            {
                m_pDeviceCtx = nullptr;
            }
        }

        Result CContextBase::_CreateCommandBuffers( const SCreateCommandBufferInfo& Info, CCommandBuffer** ppArray )
        {
            Result ret = _GetCommandBufferManager().CreateCommandBuffers< VKE_NOT_THREAD_SAFE >(
                Info.count, Info.threadIndex, ppArray );
            if( VKE_SUCCEEDED( ret ) )
            {
                for( uint32_t i = 0; i < Info.count; ++i )
                {
                    SCommandBufferInitInfo Init = { .pBaseCtx = this };
                    ( ppArray )[ i ]->Init( Init );
                }
            }
            return ret;
        }

        CCommandBuffer* CContextBase::_CreateCommandBuffer()
        {
            CCommandBuffer* pCb;
            Result res = _GetCommandBufferManager().CreateCommandBuffers< VKE_NOT_THREAD_SAFE >( 1, &pCb );
            if( VKE_SUCCEEDED( res ) )
            {
                SCommandBufferInitInfo Info;
                Info.pBaseCtx            = this;
                Info.backBufferIdx       = m_backBufferIdx;
                Info.initComputeShader   = m_initComputeShader;
                Info.initGraphicsShaders = m_initGraphicsShaders;
                pCb->Init( Info );
                // pCb->m_hDDIFence = m_pCurrentExecuteBatch->hSignalCPUFence;
                // pCb->_SetCPUSyncObject( m_pCurrentExecuteBatch->hSignalCPUFence );
                // pCb->_SetGPUSyncObject( m_pCurrentExecuteBatch->hSignalGPUFence );
                pCb->Begin();
            }
            return pCb;
        }

        CCommandBuffer* CContextBase::_GetCurrentCommandBuffer()
        {
            CCommandBuffer* pCb;
            if( _GetCommandBufferManager().GetCommandBuffer( &pCb ) )
            {
                
            }
            VKE_ASSERT2( pCb->GetState() != CCommandBuffer::States::END, "" );
            return pCb;
        }

        Result CContextBase::Execute( const SSubmitInfo& Info )
        {
            return m_pQueue->Execute( Info );
        }

        void CContextBase::_Reset( CCommandBuffer* pCmdBuffer )
        {
            VKE_ASSERT( pCmdBuffer->GetState() != CommandBufferStates::BEGIN );

            // Instead of assert we'll skip reset if it's already in reset state.
            // VKE_ASSERT2( pCmdBuffer->GetState() != CommandBufferStates::RESET,
            //              "Command buffer is in RESET state, are you attempting to reset command buffer twice?" );

            if( pCmdBuffer->m_state == CCommandBuffer::States::RESET )
            {
                // VKE_LOG_WARN( "Command buffer is in RESET state, are you attempting to reset command buffer twice?" );
            }
            else
            {
                m_pDeviceCtx->RHI().Reset( pCmdBuffer->GetDDIObject(), pCmdBuffer->m_hDDICmdBufferPool );
                pCmdBuffer->m_state = CCommandBuffer::States::RESET;
            }
        }

        Result CContextBase::_BeginCommandBuffer( CCommandBuffer** ppInOut )
        {
            Result          ret = VKE_OK;
            CCommandBuffer* pCb = *ppInOut;
            VKE_ASSERT2( pCb && pCb->m_pBaseCtx, "pCb and context cannot be null" );

            _Reset( pCb );
            pCb->m_hPool;

            m_pDeviceCtx->RHI().BeginCommandBuffer( pCb->GetDDIObject(), pCb->getNativeCmdBufferPool() );
            pCb->m_currBackBufferIdx = m_backBufferIdx;
            pCb->m_state             = CCommandBuffer::States::BEGIN;
            return ret;
        }

        Result CContextBase::_EndCurrentCommandBuffer()
        {
            CCommandBuffer* pCb;
            bool            isNew = _GetCommandBufferManager().GetCommandBuffer( &pCb );
            VKE_ASSERT2( isNew == false, "" );
            (void)isNew;
            return _EndCommandBuffer( &pCb );
        }

        Result CContextBase::_EndCommandBuffer( CCommandBuffer** ppInOut )
        {
            // VKE_ASSERT2( m_pCurrentExecuteBatch != nullptr, "" );
            Result          ret = VKE_OK;
            CCommandBuffer* pCb = *ppInOut;
            VKE_ASSERT2( pCb->m_state != CCommandBuffer::States::END, "" );
            // VKE_ASSERT2( m_pCurrentExecuteBatch->vpCommandBuffers.Find( pCb ) >= 0, "CommandBuffer was not added to
            // the execution!" );

            pCb->_ExecutePendingOperations();
            // pCb->DumpDebugMarkerTexts();
            m_DDI.EndCommandBuffer( pCb->GetDDIObject() );

            // if( flags & ExecuteCommandBufferFlags::END )
            {
                _GetCommandBufferManager().EndCommandBuffer( 0, nullptr, ppInOut );
                pCb->m_state = CCommandBuffer::States::END;
            }

            return ret;
        }

        CTransferContext* CContextBase::GetTransferContext() const
        {
            return GetDeviceContext()->GetTransferContext();
        }

        Result CContextBase::UpdateBuffer( CommandBufferPtr pCb, const SUpdateMemoryInfo& Info, BufferPtr* ppInOut )
        {
            VKE_ASSERT2( ppInOut != nullptr && ( *ppInOut )!= nullptr, "Buffer must be a valid pointer." );
            Result   ret     = VKE_FAIL;
            CBuffer* pBuffer = ( *ppInOut ).Get();
            ret              = m_pDeviceCtx->m_pBufferMgr->UpdateBuffer( pCb, Info, &pBuffer );
            return ret;
        }

        Result CContextBase::UpdateBuffer( CommandBufferPtr pCb, const SUpdateMemoryInfo& Info, BufferHandle* phInOut )
        {
            Result   ret     = VKE_FAIL;
            CBuffer* pBuffer = m_pDeviceCtx->m_pBufferMgr->GetBuffer( *phInOut ).Get();
            ret              = m_pDeviceCtx->m_pBufferMgr->UpdateBuffer( pCb, Info, &pBuffer );
            return ret;
        }

        Result CContextBase::UpdateTexture( const SUpdateMemoryInfo& Info, TextureHandle* phInOut )
        {
            Result ret = VKE_FAIL;
            // CTexture* pTexture = m_pDeviceCtx->m_pTextureMgr->GetTexture(*phInOut).Get();
            // ret = m_pDeviceCtx->m_pTextureMgr->UpdateTexture( Info, this, &pTexture );
            return ret;
        }

        PipelinePtr CContextBase::BuildCurrentPipeline()
        {
            PipelinePtr pRet;
            /*PipelineLayoutPtr pLayout = m_pDeviceCtx->CreatePipelineLayout(
            this->m_pCurrentCommandBuffer->m_CurrentPipelineLayoutDesc );
            this->m_pCurrentCommandBuffer->m_CurrentPipelineDesc.Pipeline.hDDILayout = pLayout->GetDDIObject();
            pRet = m_pDeviceCtx->CreatePipeline( this->m_pCurrentCommandBuffer->m_CurrentPipelineDesc );*/
            // this->m_pCurrentCommandBuffer->_UpdateCurrentPipeline();
            // pRet = this->m_pCurrentCommandBuffer->m_pCurrentPipeline;
            CCommandBuffer* pCb;
            _GetCommandBufferManager().GetCommandBuffer( &pCb );
            pCb->_UpdateCurrentPipeline();
            pRet = pCb->GetCurrentState().pPipeline;
            return pRet;
        }

        void CContextBase::_SetTextureState( CCommandBuffer* pCb, TEXTURE_STATE state, TextureHandle* phInOut )
        {
            TextureHandle       hTex = *phInOut;
            TexturePtr          pTex = m_pDeviceCtx->GetTexture( hTex );
            STextureBarrierInfo Info;
            if( pTex->SetState( state, &Info ) )
            {
                pCb->Barrier( Info );
            }
        }

        void CContextBase::SetTextureState( CommandBufferPtr pCmdbuffer, TEXTURE_STATE state,
                                            RenderTargetHandle* phInOut )
        {
            RenderTargetHandle hRt  = *phInOut;
            RenderTargetPtr    pRT  = m_pDeviceCtx->GetRenderTarget( hRt );
            TextureHandle      hTex = pRT->GetTexture();
            SetTextureState( pCmdbuffer, state, &hTex );
        }

        CContextBase::SExecuteData* CContextBase::_GetFreeExecuteData()
        {
            SExecuteData* pRet = nullptr;
            uint32_t      handle;
            if( m_ExecuteDataPool.GetFreeHandle( &handle ) )
            {
                pRet = &m_ExecuteDataPool[ handle ];
                pRet->vWaitSemaphores.Clear();
            }
            else
            {
                SExecuteData Data;
                handle = m_ExecuteDataPool.Add( Data );
                m_ExecuteDataPool.Free( handle );
                m_ExecuteDataPool[ handle ].handle = handle;
                pRet                               = _GetFreeExecuteData();
            }
            return pRet;
        }

        CContextBase::SExecuteData* CContextBase::_PopExecuteData()
        {
            SExecuteData* pRet = nullptr;
            if( m_qExecuteData.empty() == false )
            {
                // dataReady = m_qExecuteData.PopFront(&Data);
                pRet = m_qExecuteData.front();
                m_qExecuteData.pop_front();
                m_ExecuteDataPool.Free( pRet->handle );
            }
            return pRet;
        }

        void CContextBase::_FreeExecutedBatches()
        {
        }

        void CContextBase::SignalGPUFence()
        {
        }

    } // namespace RenderSystem
} // namespace VKE