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
            VKE_ASSERT2( ret != BindingTypes::_MAX_COUNT, "Invalid buffer usage." );
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
                m_pQueue->SetDebugName( m_pName );
                for(uint32_t i = 0; i < 4; ++i)
                {
                    _CreateNewExecuteBatch();
                }
                _AcquireExecuteBatch();
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

        Result CContextBase::_CreateNewExecuteBatch()
        {
            auto& DDI = m_pDeviceCtx->DDI();
            Result ret = VKE_FAIL;
            SExecuteBatch Batch;
            SFenceDesc FenceDesc;
            SSemaphoreDesc SemaphoreDesc;
#if VKE_RENDER_SYSTEM_DEBUG
            char name1[ 128 ], name2[ 128 ];
            vke_sprintf( name1, 128, "%s_batch%d_CPUSignalFence", m_pName, m_vExecuteBatches.GetCount() );
            FenceDesc.SetDebugName( name1 );
            vke_sprintf( name2, 128, "%s_batch%d_GPUSignalFence", m_pName, m_vExecuteBatches.GetCount() );
            SemaphoreDesc.SetDebugName( name2 );
#endif
            FenceDesc.isSignaled = true; // signaled means ready to use
            Batch.hSignalCPUFence = DDI.CreateFence( FenceDesc, nullptr );
            Batch.hSignalGPUFence = DDI.CreateSemaphore( SemaphoreDesc, nullptr );
            Batch.executionResult = Results::NOT_READY;
            
            if( Batch.hSignalCPUFence != DDI_NULL_HANDLE && Batch.hSignalGPUFence != DDI_NULL_HANDLE )
            {
                //DDI.Reset( &Batch.hSignalCPUFence );
                m_vExecuteBatches.PushBack( Batch );
                ret = VKE_OK;
            }
            VKE_ASSERT2( VKE_SUCCEEDED( ret ), "" );
            return ret;
        }

        CContextBase::SExecuteBatch* CContextBase::_ResetExecuteBatch(uint32_t idx)
        {
            SExecuteBatch* pRet = nullptr;
            auto& Batch = m_vExecuteBatches[ idx ];
            auto& DDI = m_pDeviceCtx->DDI();
            bool signaled = DDI.IsSignaled( Batch.hSignalCPUFence );
            bool executed = Batch.executionResult == Results::OK;
            bool hasCmdBuffers = !Batch.vpCommandBuffers.IsEmpty();
            if(!signaled && !executed && !hasCmdBuffers)
            {
                pRet = &Batch; // not used batch
            }
            else
            if( executed )
            {
                if( signaled )
                {
                    //DDI.Reset( &Batch.hSignalCPUFence );
                    if( !Batch.vpCommandBuffers.IsEmpty() )
                    {
                        _FreeCommandBuffers( Batch.vpCommandBuffers.GetCount(), Batch.vpCommandBuffers.GetData() );
                        Batch.vDDIWaitGPUFences.Clear();
                        Batch.vpCommandBuffers.Clear();
                    }
                    Batch.executionResult = Results::NOT_READY;
                    pRet = &Batch;
                }
            }
            return pRet;
        }

        CContextBase::SExecuteBatch* CContextBase::_AcquireExecuteBatch()
        {
            // Get first free
            VKE_ASSERT2( m_pCurrentExecuteBatch == nullptr ||
                            ( m_pCurrentExecuteBatch != nullptr && m_pCurrentExecuteBatch->executionResult == VKE_OK ),
                        "" );
            // If current batch was acquired by GetCurrentCommandBuffer already
            if(m_pCurrentExecuteBatch != nullptr && m_pCurrentExecuteBatch->executionResult == Results::NOT_READY)
            {
                return m_pCurrentExecuteBatch;
            }
            // Get next index
            m_currExeBatchIdx = ( m_currExeBatchIdx + 1 ) % 4;
            m_pCurrentExecuteBatch = _ResetExecuteBatch(m_currExeBatchIdx);
            // If next batch is not ready try to find any ready batch that was used before
            if( m_pCurrentExecuteBatch == nullptr )
            {
                for( uint32_t i = 0; i < m_vExecuteBatches.GetCount(); ++i )
                {
                    m_pCurrentExecuteBatch = _ResetExecuteBatch( i );
                    if( m_pCurrentExecuteBatch != nullptr )
                    {
                        break;
                    }
                }
                if( m_pCurrentExecuteBatch == nullptr )
                {
                    if( VKE_SUCCEEDED(_CreateNewExecuteBatch()) )
                    {
                        m_pCurrentExecuteBatch = &m_vExecuteBatches.Back();
                    }
                }
            }
            VKE_LOG( "acquire batch: " << m_pCurrentExecuteBatch );
            return m_pCurrentExecuteBatch;
        }

        CContextBase::SExecuteBatch* CContextBase::_PushCurrentBatchToExecuteQueue()
        {
            Threads::ScopedLock l( m_ExecuteBatchSyncObj );
            auto pBatch = m_pCurrentExecuteBatch;
            m_qExecuteBatches.push_back( m_pCurrentExecuteBatch );
            m_pCurrentExecuteBatch = nullptr;
            return pBatch;
        }

        CContextBase::SExecuteBatch* CContextBase::_PopExecuteBatch()
        {
            SExecuteBatch* pRet = nullptr;
            Threads::ScopedLock l( m_ExecuteBatchSyncObj );
            if (!m_qExecuteBatches.empty())
            {
                pRet = m_qExecuteBatches.front();
                m_qExecuteBatches.pop_front();
            }
            return pRet;
        }

        Result CContextBase::_ExecuteAllBatches( EXECUTE_COMMAND_BUFFER_FLAGS flags )
        {
            Result ret = VKE_OK;

            for(uint32_t i = 0; i < m_vExecuteBatches.GetCount();++i)
            {
                auto& Batch = m_vExecuteBatches[ i ];
                if(Batch.executionResult != VKE_OK && !Batch.vpCommandBuffers.IsEmpty())
                {
                    ret = _ExecuteBatch( &Batch, flags );
                }
            }

            return ret;
        }

        Result CContextBase::_ExecuteBatch(CContextBase::SExecuteBatch* pBatch,
            EXECUTE_COMMAND_BUFFER_FLAGS flags)
        {
            Result ret;
            VKE_LOG( "execute batch: " << pBatch );
            SSubmitInfo Info;
            const auto& vpBuffers = pBatch->vpCommandBuffers;
            VKE_ASSERT2( !vpBuffers.IsEmpty(), "" );
            Utils::TCDynamicArray<DDICommandBuffer, DEFAULT_CMD_BUFFER_COUNT> vDDIBuffers(vpBuffers.GetCount());
            const auto count = pBatch->vpCommandBuffers.GetCount();
            EXECUTE_COMMAND_BUFFER_FLAGS userFlags = 0;
            for( uint32_t i = 0; i < count; ++i )
            {
                vpBuffers[ i ]->Flush();
                userFlags |= vpBuffers[ i ]->GetExecuteFlags();
                vDDIBuffers[ i ] = vpBuffers[ i ]->GetDDIObject();
            }

            auto type = m_pQueue->GetType();
            flags |= userFlags;
            Utils::TCBitset<EXECUTE_COMMAND_BUFFER_FLAGS> ExecuteFlags( flags );

            Info.commandBufferCount = (uint16_t)vDDIBuffers.GetCount();
            Info.pDDICommandBuffers = vDDIBuffers.GetData();
            Info.hDDIFence = pBatch->hSignalCPUFence;
            Info.hDDIQueue = m_pQueue->GetDDIObject();
            if( ExecuteFlags == ExecuteCommandBufferFlags::WAIT_FOR_GPU_FENCE )
            {
                m_pDeviceCtx->_GetSignaledSemaphores( type, &pBatch->vDDIWaitGPUFences );
                Info.pDDIWaitSemaphores = pBatch->vDDIWaitGPUFences.GetData();
                Info.waitSemaphoreCount = ( uint16_t )pBatch->vDDIWaitGPUFences.GetCount();
            }
            else
            if( (flags & ExecuteCommandBufferFlags::DONT_WAIT_FOR_SEMAPHORE) == ExecuteCommandBufferFlags::DONT_WAIT_FOR_SEMAPHORE )
            {
                Info.pDDIWaitSemaphores = nullptr;
                Info.waitSemaphoreCount = 0;
            }
            else
            {

                m_pDeviceCtx->_GetSignaledSemaphores( type, &pBatch->vDDIWaitGPUFences );

                Info.waitSemaphoreCount = ( uint16_t )pBatch->vDDIWaitGPUFences.GetCount();
                Info.pDDIWaitSemaphores = pBatch->vDDIWaitGPUFences.GetData();
            }
            if( ExecuteFlags == ExecuteCommandBufferFlags::SIGNAL_GPU_FENCE )
            {
                Info.pDDISignalSemaphores = &pBatch->hSignalGPUFence;
                Info.signalSemaphoreCount = 1;
                if( flags != ExecuteCommandBufferFlags::DONT_PUSH_SIGNAL_SEMAPHORE )
                {
                    if( type == QueueTypes::TRANSFER )
                    {
                        m_pDeviceCtx->_PushSignaledSemaphore( QueueTypes::GENERAL, pBatch->hSignalGPUFence );
                    }
                    else if( type == QueueTypes::COMPUTE )
                    {
                        m_pDeviceCtx->_PushSignaledSemaphore( QueueTypes::GENERAL, pBatch->hSignalGPUFence );
                    }
                }
            }
            else
            if( ( flags & ExecuteCommandBufferFlags::DONT_SIGNAL_SEMAPHORE ) == ExecuteCommandBufferFlags::DONT_SIGNAL_SEMAPHORE )
            {
                Info.pDDISignalSemaphores = nullptr;
                Info.signalSemaphoreCount = 0;
            }
            else
            {
                Info.pDDISignalSemaphores = &pBatch->hSignalGPUFence;
                Info.signalSemaphoreCount = 1;

                if( !( ( flags & ExecuteCommandBufferFlags::DONT_PUSH_SIGNAL_SEMAPHORE ) ==
                       ExecuteCommandBufferFlags::DONT_PUSH_SIGNAL_SEMAPHORE ) )
                {
                    if( type == QueueTypes::TRANSFER )
                    {
                        m_pDeviceCtx->_PushSignaledSemaphore( QueueTypes::GENERAL, pBatch->hSignalGPUFence );
                    }
                    else if( type == QueueTypes::COMPUTE )
                    {
                        m_pDeviceCtx->_PushSignaledSemaphore( QueueTypes::GENERAL, pBatch->hSignalGPUFence );
                    }
                }
            }

#if VKE_RENDER_SYSTEM_DEBUG && 0
            std::stringstream ss;
            ss << "Context: " << m_pName;
            if( Info.signalSemaphoreCount )
            {
                ss << "\n signal: " << Info.pDDISignalSemaphores[ 0 ];
            }
            for(uint32_t i = 0; i < Info.waitSemaphoreCount; ++i)
            {
                ss << "\n wait: " << Info.pDDIWaitSemaphores[ i ];
            }
            VKE_LOG( ss.str() );
#endif
            VKE_ASSERT2( m_DDI.IsSignaled( pBatch->hSignalCPUFence ), "CPU Fence must be signaled before execute!" );
            m_DDI.Reset( &pBatch->hSignalCPUFence );
            ret = m_pQueue->Execute( Info );
            if (VKE_SUCCEEDED(ret) && (flags & ExecuteCommandBufferFlags::WAIT) == ExecuteCommandBufferFlags::WAIT)
            {
                m_pQueue->Wait();
            }
            pBatch->executionResult = ret;
            return ret;
        }

        CCommandBuffer* CContextBase::_CreateCommandBuffer()
        {
            CCommandBuffer* pCb;
            VKE_ASSERT2( m_pCurrentExecuteBatch != nullptr, "" );
            Result res = _GetCommandBufferManager().CreateCommandBuffers< VKE_NOT_THREAD_SAFE >( 1, &pCb );
            if( VKE_SUCCEEDED( res ) )
            {
                SCommandBufferInitInfo Info;
                Info.pBaseCtx = this;
                Info.backBufferIdx = m_backBufferIdx;
                Info.initComputeShader = m_initComputeShader;
                Info.initGraphicsShaders = m_initGraphicsShaders;
                pCb->Init( Info );
                //pCb->m_hDDIFence = m_pCurrentExecuteBatch->hSignalCPUFence;
                pCb->_SetCPUSyncObject( m_pCurrentExecuteBatch->hSignalCPUFence );
                pCb->_SetGPUSyncObject( m_pCurrentExecuteBatch->hSignalGPUFence );
                pCb->Begin();
            }
            return pCb;
        }

        CCommandBuffer* CContextBase::_GetCurrentCommandBuffer()
        {
            CCommandBuffer* pCb;
            if( _GetCommandBufferManager().GetCommandBuffer( &pCb ) )
            {
                Threads::ScopedLock l( m_ExecuteBatchSyncObj );
                if(m_pCurrentExecuteBatch == nullptr)
                {
                    m_pCurrentExecuteBatch = _AcquireExecuteBatch();
                }
                VKE_ASSERT2( m_pCurrentExecuteBatch != nullptr, "" );
                //pCb->m_hDDIFence = m_pCurrentExecuteBatch->hSignalCPUFence;
                pCb->_SetCPUSyncObject( m_pCurrentExecuteBatch->hSignalCPUFence );
                pCb->_SetGPUSyncObject( m_pCurrentExecuteBatch->hSignalGPUFence );
                VKE_ASSERT2( m_pCurrentExecuteBatch->vpCommandBuffers.Find( pCb ) == INVALID_POSITION, "" );
                m_pCurrentExecuteBatch->vpCommandBuffers.PushBack( pCb );
            }
            VKE_ASSERT2( pCb->GetState() != CCommandBuffer::States::END, "" );
            return pCb;
        }

        Result CContextBase::Execute(EXECUTE_COMMAND_BUFFER_FLAGS flags)
        {
            //flags |= ExecuteCommandBufferFlags::END;
            //return _GetCommandBufferManager().EndCommandBuffer( flags, nullptr );
            Result ret = VKE_OK;
            Threads::ScopedLock l( m_ExecuteBatchSyncObj );
            if( !m_pCurrentExecuteBatch->vpCommandBuffers.IsEmpty() )
            {
                for( uint32_t i = 0; i < m_pCurrentExecuteBatch->vpCommandBuffers.GetCount(); ++i )
                {
                    m_pCurrentExecuteBatch->vpCommandBuffers[ i ]->End();
                }
                // ret = _ExecuteCurrentBatch( flags );
                _PushCurrentBatchToExecuteQueue();
                auto* pBatch = _PopExecuteBatch();
                ret = _ExecuteBatch( pBatch, flags );
                _AcquireExecuteBatch();
            }
            return ret;
        }

        Result CContextBase::_BeginCommandBuffer( CCommandBuffer** ppInOut )
        {
            Result ret = VKE_OK;
            CCommandBuffer* pCb = *ppInOut;
            VKE_ASSERT2( pCb && pCb->m_pBaseCtx, "" );

            m_pDeviceCtx->DDI().Reset( pCb->GetDDIObject() );
            m_pDeviceCtx->_GetDDI().BeginCommandBuffer( pCb->GetDDIObject() );
            pCb->m_currBackBufferIdx = m_backBufferIdx;
            pCb->m_state = CCommandBuffer::States::BEGIN;
            return ret;
        }

        Result CContextBase::_EndCurrentCommandBuffer()
        {
            CCommandBuffer* pCb;
            bool isNew = _GetCommandBufferManager().GetCommandBuffer( &pCb );
            VKE_ASSERT2( isNew == false, "" );
            ( void )isNew;
            return _EndCommandBuffer( &pCb );
        }

        Result CContextBase::_EndCommandBuffer( CCommandBuffer** ppInOut )
        {
            //VKE_ASSERT2( m_pCurrentExecuteBatch != nullptr, "" );
            Result ret = VKE_OK;
            CCommandBuffer* pCb = *ppInOut;
            VKE_ASSERT2( pCb->m_state != CCommandBuffer::States::END, "" );
            //VKE_ASSERT2( m_pCurrentExecuteBatch->vpCommandBuffers.Find( pCb ) >= 0, "CommandBuffer was not added to the execution!" );
            
            pCb->_ExecutePendingOperations();
            //pCb->DumpDebugMarkerTexts();
            m_DDI.EndCommandBuffer( pCb->GetDDIObject() );
            

            //if( flags & ExecuteCommandBufferFlags::END )
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
            VKE_ASSERT2( ppInOut != nullptr && (*ppInOut).IsValid(), "Buffer must be a valid pointer." );
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

        void CContextBase::_FreeExecutedBatches()
        {
        
        }

        void CContextBase::SyncExecute( DDISemaphore hApi )
        {
            m_pCurrentExecuteBatch->vDDIWaitGPUFences.PushBack( hApi );
        }

        void CContextBase::SignalGPUFence()
        {
      
        }

    } // RenderSystem
} // VKE