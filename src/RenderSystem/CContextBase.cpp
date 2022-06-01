#include "RenderSystem/CContextBase.h"
#include "RenderSystem/CCommandBuffer.h"
#include "RenderSystem/CDeviceContext.h"
#include "RenderSystem/Managers/CBufferManager.h"
#include "RenderSystem/Managers/CTextureManager.h"
#include "RenderSystem/Vulkan/Managers/CDescriptorSetManager.h"

namespace VKE
{
    namespace RenderSystem
    {
        static CCommandBufferBatch g_sDummyBatch;

        static BINDING_TYPE BufferUsageToBindingType( const BUFFER_USAGE usage )
        {
            BINDING_TYPE ret = BindingTypes::_MAX_COUNT;
            if( (usage & BufferUsages::CONSTANT_BUFFER) == BufferUsages::CONSTANT_BUFFER )
            {
                ret = BindingTypes::CONSTANT_BUFFER;
                if( (usage & BufferUsages::TEXEL_BUFFER) == BufferUsages::TEXEL_BUFFER )
                {
                    ret = BindingTypes::UNIFORM_TEXEL_BUFFER;
                }
            }
            else if( ( usage & BufferUsages::STORAGE_BUFFER ) == BufferUsages::STORAGE_BUFFER )
            {
                ret = BindingTypes::STORAGE_BUFFER;
                if( ( usage & BufferUsages::TEXEL_BUFFER ) == BufferUsages::TEXEL_BUFFER )
                {
                    ret = BindingTypes::STORAGE_TEXEL_BUFFER;
                }
            }
            VKE_ASSERT( ret != BindingTypes::_MAX_COUNT, "Invalid buffer usage." );
            return ret;
        }

        void SCreateBindingDesc::AddBinding( const SResourceBinding& Binding, const BufferPtr& pBuffer )
        {
            SDescriptorSetLayoutDesc::SBinding BindInfo;
            BindInfo.count = Binding.count;
            BindInfo.idx = Binding.index;
            BindInfo.stages = Binding.stages;
            BindInfo.type = BufferUsageToBindingType( pBuffer->GetDesc().usage );
            LayoutDesc.vBindings.PushBack( BindInfo );
        }

        void SCreateBindingDesc::AddBinding( const STextureBinding& Binding )
        {
            SDescriptorSetLayoutDesc::SBinding BindInfo;
            BindInfo.count = Binding.count;
            BindInfo.idx = Binding.index;
            BindInfo.stages = Binding.stages;
            BindInfo.type = BindingTypes::TEXTURE;
            LayoutDesc.vBindings.PushBack( BindInfo );
        }

        void SCreateBindingDesc::AddBinding( const SSamplerBinding& Binding )
        {
            SDescriptorSetLayoutDesc::SBinding BindInfo;
            BindInfo.count = Binding.count;
            BindInfo.idx = Binding.index;
            BindInfo.stages = Binding.stages;
            BindInfo.type = BindingTypes::SAMPLER;
            LayoutDesc.vBindings.PushBack( BindInfo );
        }

        void SCreateBindingDesc::AddBinding( const SSamplerTextureBinding& Binding )
        {
            SDescriptorSetLayoutDesc::SBinding BindInfo;
            BindInfo.count = Binding.count;
            BindInfo.idx = Binding.index;
            BindInfo.stages = Binding.stages;
            BindInfo.type = BindingTypes::SAMPLER_AND_TEXTURE;
            LayoutDesc.vBindings.PushBack( BindInfo );
        }

        void SCreateBindingDesc::AddConstantBuffer( uint8_t index, PIPELINE_STAGES stages )
        {
            SDescriptorSetLayoutDesc::SBinding BindInfo;
            BindInfo.count = 1;
            BindInfo.idx = index;
            BindInfo.stages = stages;
            BindInfo.type = BindingTypes::DYNAMIC_CONSTANT_BUFFER;
            LayoutDesc.vBindings.PushBack( BindInfo );
        }

        void SCreateBindingDesc::AddStorageBuffer( uint8_t index, PIPELINE_STAGES stages,
                                                   const uint16_t& arrayElementCount )
        {
            SDescriptorSetLayoutDesc::SBinding BindInfo;
            BindInfo.count = arrayElementCount;
            BindInfo.idx = index;
            BindInfo.stages = stages;
            BindInfo.type = BindingTypes::DYNAMIC_STORAGE_BUFFER;
            LayoutDesc.vBindings.PushBack( BindInfo );
        }

        void SCreateBindingDesc::AddTextures( uint8_t index, PIPELINE_STAGES stages, uint16_t count )
        {
            SDescriptorSetLayoutDesc::SBinding BindInfo;
            BindInfo.count = count;
            BindInfo.idx = index;
            BindInfo.stages = stages;
            BindInfo.type = BindingTypes::TEXTURE;
            LayoutDesc.vBindings.PushBack( BindInfo );
        }

        void SCreateBindingDesc::AddSamplers( uint8_t index, PIPELINE_STAGES stages, uint16_t count )
        {
            SDescriptorSetLayoutDesc::SBinding BindInfo;
            BindInfo.count = count;
            BindInfo.idx = index;
            BindInfo.stages = stages;
            BindInfo.type = BindingTypes::SAMPLER;
            LayoutDesc.vBindings.PushBack( BindInfo );
        }

        void SCreateBindingDesc::AddSamplerAndTexture( uint8_t index, PIPELINE_STAGES stages )
        {
            SDescriptorSetLayoutDesc::SBinding BindInfo;
            BindInfo.count = 1;
            BindInfo.idx = index;
            BindInfo.stages = stages;
            BindInfo.type = BindingTypes::SAMPLER_AND_TEXTURE;
            LayoutDesc.vBindings.PushBack( BindInfo );
        }

        CContextBase::CContextBase( CDeviceContext* pCtx, cstr_t pName ) :
            m_DDI( pCtx->DDI() )
            , m_pDeviceCtx( pCtx )
            , m_pName( pName )
            , m_pLastExecutedBatch( &g_sDummyBatch )
            , m_CmdBuffMgr( this )
        {

        }

        Result CContextBase::Create( const SContextBaseDesc& Desc )
        {
            Result ret = VKE_OK;
            m_pQueue = Desc.pQueue;

            SCommandBufferManagerDesc MgrDesc;
            ret = m_CmdBuffMgr.Create( MgrDesc );
            
            if( VKE_SUCCEEDED( ret ) )
            {
                if( m_PreparationData.pCmdBuffer == nullptr )
                {
                    m_PreparationData.pCmdBuffer = _CreateCommandBuffer();
                    SFenceDesc FenceDesc;
                    m_PreparationData.hDDIFence = m_DDI.CreateFence( FenceDesc, nullptr );
                }
                _GetCurrentCommandBuffer();
                
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

        CCommandBuffer* CContextBase::_CreateCommandBuffer()
        {
            CCommandBuffer* pCb;
            //m_CmdBuffMgr.CreateCommandBuffers< VKE_THREAD_SAFE >( 1, &pCb );
            Result res = _GetCommandBufferManager().CreateCommandBuffers< VKE_NOT_THREAD_SAFE >( 1, &pCb );
            if( VKE_SUCCEEDED( res ) )
            {
                SCommandBufferInitInfo Info;
                Info.pBaseCtx = this;
                Info.backBufferIdx = m_backBufferIdx;
                Info.initComputeShader = m_initComputeShader;
                Info.initGraphicsShaders = m_initGraphicsShaders;
                pCb->Init( Info );
                pCb->Begin();
            }
            return pCb;
        }

        CCommandBuffer* CContextBase::_GetCurrentCommandBuffer()
        {
            //if( m_pCurrentCommandBuffer == nullptr )
            //{
            //    m_pCurrentCommandBuffer = _CreateCommandBuffer();
            //    //m_pCurrentCommandBuffer->Begin();
            //}
            //return m_pCurrentCommandBuffer;
            CCommandBuffer* pCb;
            if( _GetCommandBufferManager().GetCommandBuffer( &pCb ) )
            {
                Threads::ScopedLock l( m_CommandBufferSyncObj );
                m_vCommandBuffers.PushBack( CommandBufferPtr{ pCb } );
            }
            VKE_ASSERT( pCb->GetState() != CCommandBuffer::States::END, "" );
            return pCb;
        }

        Result CContextBase::Execute(EXECUTE_COMMAND_BUFFER_FLAGS flags)
        {
            flags |= ExecuteCommandBufferFlags::END;
            //return _GetCommandBufferManager().EndCommandBuffer( flags, nullptr );
            Result ret = VKE_OK;
            Threads::ScopedLock l( m_CommandBufferSyncObj );
            for(uint32_t i = 0; i < m_vCommandBuffers.GetCount(); ++i)
            {
                auto pCb = m_vCommandBuffers[ i ].Get();
                pCb->End( flags, nullptr );
            }
            m_vCommandBuffers.Clear();
            return ret;
        }

        Result CContextBase::_BeginCommandBuffer( CCommandBuffer** ppInOut )
        {
            Result ret = VKE_OK;
            CCommandBuffer* pCb = *ppInOut;
            VKE_ASSERT( pCb && pCb->m_pBaseCtx, "" );

            m_pDeviceCtx->DDI().Reset( pCb->GetDDIObject() );
            m_pDeviceCtx->_GetDDI().BeginCommandBuffer( pCb->GetDDIObject() );
            pCb->m_currBackBufferIdx = m_backBufferIdx;

            return ret;
        }

        Result CContextBase::_EndCurrentCommandBuffer( EXECUTE_COMMAND_BUFFER_FLAGS flags, DDISemaphore* phDDIOut )
        {
            CCommandBuffer* pCb;
            bool isNew = _GetCommandBufferManager().GetCommandBuffer( &pCb );
            VKE_ASSERT( isNew == false, "" );
            ( void )isNew;
            return _EndCommandBuffer( flags, &pCb, phDDIOut );
        }

        Result CContextBase::_EndCommandBuffer( EXECUTE_COMMAND_BUFFER_FLAGS flags, CCommandBuffer** ppInOut,
            DDISemaphore* phDDIOut )
        {
            Result ret = VKE_OK;
            CCommandBuffer* pCb = *ppInOut;
            m_DDI.EndCommandBuffer( pCb->GetDDIObject() );
            flags |= m_additionalEndFlags;

            auto pSubmitMgr = m_pQueue->_GetSubmitManager();
            pSubmitMgr->m_signalSemaphore = (flags & ExecuteCommandBufferFlags::DONT_SIGNAL_SEMAPHORE) == 0;
            pSubmitMgr->m_waitForSemaphores = (flags & ExecuteCommandBufferFlags::DONT_WAIT_FOR_SEMAPHORE) == 0;
            const bool pushSignaledSemaphore = (flags & ExecuteCommandBufferFlags::PUSH_SIGNAL_SEMAPHORE) != 0 && pSubmitMgr->m_signalSemaphore;

            if( flags & ExecuteCommandBufferFlags::END )
            {
                _GetCommandBufferManager().EndCommandBuffer( flags, phDDIOut, ppInOut );
                pCb->m_state = CCommandBuffer::States::END;
                auto hPool = _GetCommandBufferManager().GetPool();
                pSubmitMgr->Submit( this, hPool, pCb );
                if( phDDIOut )
                {
                    VKE_ASSERT( pSubmitMgr->m_pCurrBatch != nullptr, "" );
                    *phDDIOut = pSubmitMgr->m_pCurrBatch->GetSignaledSemaphore();
                }

            }
            if( flags & ExecuteCommandBufferFlags::EXECUTE )
            {
                pCb->m_state = CCommandBuffer::States::FLUSH;
                auto hPool = _GetCommandBufferManager().GetPool();
                auto pBatch = pSubmitMgr->_GetNextBatch< NextSubmitBatchAlgorithms::FIRST_FREE >( this, hPool );

                pBatch->_Submit( pCb );

                ret = pSubmitMgr->ExecuteBatch( this, m_pQueue, &pBatch );
                if( VKE_SUCCEEDED( ret ) )
                {
                    if( phDDIOut )
                    {
                        *phDDIOut = pBatch->GetSignaledSemaphore();
                    }
                    if( pushSignaledSemaphore )
                    {
                        m_pDeviceCtx->_PushSignaledSemaphore( pBatch->GetSignaledSemaphore() );
                    }
                    if( flags & ExecuteCommandBufferFlags::WAIT )
                    {
                        {
                            ret = m_DDI.WaitForFences( pBatch->m_hDDIFence, UINT64_MAX );
                            pSubmitMgr->_FreeBatch(this, hPool, &pBatch);
                        }
                    }
                }
            }

            return ret;
        }

        CTransferContext* CContextBase::GetTransferContext() const
        {
            return GetDeviceContext()->GetTransferContext();
        }

        Result CContextBase::UpdateBuffer( CommandBufferPtr pCb, const SUpdateMemoryInfo& Info, BufferPtr* ppInOut )
        {
            VKE_ASSERT( ppInOut != nullptr && (*ppInOut).IsValid(), "Buffer must be a valid pointer." );
            Result ret = VKE_FAIL;
            CBuffer* pBuffer = ( *ppInOut ).Get();
            ret = m_pDeviceCtx->m_pBufferMgr->UpdateBuffer( pCb, Info, &pBuffer );
            return ret;
        }

        Result CContextBase::UpdateBuffer( CommandBufferPtr pCb, const SUpdateMemoryInfo& Info, BufferHandle* phInOut )
        {
            Result ret = VKE_FAIL;
            CBuffer* pBuffer = m_pDeviceCtx->m_pBufferMgr->GetBuffer( *phInOut ).Get();
            ret = m_pDeviceCtx->m_pBufferMgr->UpdateBuffer( pCb, Info, &pBuffer );
            return ret;
        }

        Result CContextBase::UpdateTexture( const SUpdateMemoryInfo& Info, TextureHandle* phInOut )
        {
            Result ret = VKE_FAIL;
            //CTexture* pTexture = m_pDeviceCtx->m_pTextureMgr->GetTexture(*phInOut).Get();
            //ret = m_pDeviceCtx->m_pTextureMgr->UpdateTexture( Info, this, &pTexture );
            return ret;
        }

        
        PipelinePtr CContextBase::BuildCurrentPipeline()
        {
            PipelinePtr pRet;
            /*PipelineLayoutPtr pLayout = m_pDeviceCtx->CreatePipelineLayout( this->m_pCurrentCommandBuffer->m_CurrentPipelineLayoutDesc );
            this->m_pCurrentCommandBuffer->m_CurrentPipelineDesc.Pipeline.hDDILayout = pLayout->GetDDIObject();
            pRet = m_pDeviceCtx->CreatePipeline( this->m_pCurrentCommandBuffer->m_CurrentPipelineDesc );*/
            //this->m_pCurrentCommandBuffer->_UpdateCurrentPipeline();
            //pRet = this->m_pCurrentCommandBuffer->m_pCurrentPipeline;
            CCommandBuffer* pCb;
            _GetCommandBufferManager().GetCommandBuffer( &pCb );
            pCb->_UpdateCurrentPipeline();
            pRet = pCb->m_pCurrentPipeline;
            return pRet;
        }

        void CContextBase::_SetTextureState( CCommandBuffer* pCb, TEXTURE_STATE state, TextureHandle* phInOut )
        {
            TextureHandle hTex = *phInOut;
            TexturePtr pTex = m_pDeviceCtx->GetTexture( hTex );
            STextureBarrierInfo Info;
            if( pTex->SetState( state, &Info ) )
            {
                pCb->Barrier( Info );
            }
        }

        void CContextBase::SetTextureState( CommandBufferPtr pCmdbuffer, TEXTURE_STATE state,
                                            RenderTargetHandle* phInOut )
        {
            RenderTargetHandle hRt = *phInOut;
            RenderTargetPtr pRT = m_pDeviceCtx->GetRenderTarget( hRt );
            TextureHandle hTex = pRT->GetTexture();
            SetTextureState( pCmdbuffer, state, &hTex );
        }

        CCommandBuffer* CContextBase::GetPreparationCommandBuffer()
        {
            if( m_PreparationData.pCmdBuffer->m_state != CCommandBuffer::States::BEGIN )
            {
                if( VKE_FAILED( BeginPreparation() ) )
                {
                    return nullptr;
                }
            }
            return m_PreparationData.pCmdBuffer;
        }

        bool CContextBase::IsPreparationDone()
        {
            return m_DDI.IsReady( m_PreparationData.hDDIFence );
        }

        Result CContextBase::BeginPreparation()
        {
            Result ret = VKE_ENOTREADY;
            if( IsPreparationDone() )
            {
                if( m_PreparationData.pCmdBuffer->m_state != CCommandBuffer::States::BEGIN )
                {
                    ret = _BeginCommandBuffer( &m_PreparationData.pCmdBuffer );
                    if( VKE_SUCCEEDED( ret ) )
                    {
                        m_PreparationData.pCmdBuffer->m_state = CCommandBuffer::States::BEGIN;
                    }
                }
                else
                {
                    ret = VKE_OK;
                }
            }
            return ret;
        }

        Result CContextBase::EndPreparation()
        {
            VKE_ASSERT( m_PreparationData.pCmdBuffer->m_state == CCommandBuffer::States::BEGIN, "" );
            Result ret = VKE_FAIL;
            DDICommandBuffer hDDICmdBuffer = m_PreparationData.pCmdBuffer->GetDDIObject();
            m_DDI.EndCommandBuffer( hDDICmdBuffer );
            m_PreparationData.pCmdBuffer->m_state = CCommandBuffer::States::END;

            SSubmitInfo Info;
            Info.commandBufferCount = 1;
            Info.hDDIFence = m_PreparationData.hDDIFence;
            Info.hDDIQueue = m_pQueue->GetDDIObject();
            Info.pDDICommandBuffers = &hDDICmdBuffer;
            Info.pDDISignalSemaphores = nullptr;
            Info.pDDIWaitSemaphores = nullptr;
            Info.signalSemaphoreCount = 0;
            Info.waitSemaphoreCount = 0;

            ret = m_pQueue->Execute( Info );
            m_PreparationData.pCmdBuffer->m_state = CCommandBuffer::States::FLUSH;
            return ret;
        }

        Result CContextBase::WaitForPreparation()
        {
            Result ret = m_DDI.WaitForFences( m_PreparationData.hDDIFence, UINT64_MAX );
            m_DDI.Reset( &m_PreparationData.hDDIFence );
            return ret;
        }

        

        CContextBase::SExecuteData* CContextBase::_GetFreeExecuteData()
        {
            SExecuteData* pRet = nullptr;
            uint32_t handle;
            if(m_ExecuteDataPool.GetFreeHandle(&handle))
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
                pRet = _GetFreeExecuteData();
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

    } // RenderSystem
} // VKE