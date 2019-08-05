#include "RenderSystem/CContextBase.h"
#include "RenderSystem/CCommandBuffer.h"
#include "RenderSystem/CDeviceContext.h"
#include "RenderSystem/Managers/CBufferManager.h"
#include "RenderSystem/Vulkan/Managers/CDescriptorSetManager.h"

namespace VKE
{
    namespace RenderSystem
    {
        static CCommandBufferBatch g_sDummyBatch;

        static BINDING_TYPE BufferUsageToBindingType( const BUFFER_USAGE& usage )
        {
            BINDING_TYPE ret = BindingTypes::_MAX_COUNT;
            if( usage & BufferUsages::CONSTANT_BUFFER )
            {
                ret = BindingTypes::CONSTANT_BUFFER;
            }
            if( usage & BufferUsages::UNIFORM_TEXEL_BUFFER )
            {
                ret = BindingTypes::UNIFORM_TEXEL_BUFFER;
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
            BindInfo.type = BindingTypes::SAMPLED_TEXTURE;
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
            BindInfo.type = BindingTypes::COMBINED_TEXTURE_SAMPLER;
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

        void SCreateBindingDesc::AddTexture( uint8_t index, PIPELINE_STAGES stages )
        {
            SDescriptorSetLayoutDesc::SBinding BindInfo;
            BindInfo.count = 1;
            BindInfo.idx = index;
            BindInfo.stages = stages;
            BindInfo.type = BindingTypes::SAMPLED_TEXTURE;
            LayoutDesc.vBindings.PushBack( BindInfo );
        }

        void SCreateBindingDesc::AddSampler( uint8_t index, PIPELINE_STAGES stages )
        {
            SDescriptorSetLayoutDesc::SBinding BindInfo;
            BindInfo.count = 1;
            BindInfo.idx = index;
            BindInfo.stages = stages;
            BindInfo.type = BindingTypes::SAMPLER;
            LayoutDesc.vBindings.PushBack( BindInfo );
        }

        void SCreateBindingDesc::AddSamplerTexture( uint8_t index, PIPELINE_STAGES stages )
        {
            SDescriptorSetLayoutDesc::SBinding BindInfo;
            BindInfo.count = 1;
            BindInfo.idx = index;
            BindInfo.stages = stages;
            BindInfo.type = BindingTypes::COMBINED_TEXTURE_SAMPLER;
            LayoutDesc.vBindings.PushBack( BindInfo );
        }

        CContextBase::CContextBase( CDeviceContext* pCtx ) :
            m_DDI( pCtx->DDI() )
            , m_pDeviceCtx( pCtx )
            , m_pLastExecutedBatch( &g_sDummyBatch )
        {

        }

        Result CContextBase::Create( const SContextBaseDesc& Desc )
        {
            Result ret = VKE_OK;
            m_pQueue = Desc.pQueue;
            m_hCommandPool = Desc.hCommandBufferPool;

            if( m_PreparationData.pCmdBuffer == nullptr )
            {
                m_PreparationData.pCmdBuffer = _CreateCommandBuffer();
                SFenceDesc FenceDesc;
                m_PreparationData.hDDIFence = m_DDI.CreateObject( FenceDesc, nullptr );
            }

            _GetCurrentCommandBuffer();

            m_vDescPools.PushBack( INVALID_HANDLE );


            {
                SDescriptorPoolDesc PoolDesc;
                PoolDesc.maxSetCount = Desc.descPoolSize;
                
                {
                    for( uint32_t i = 0; i < DescriptorSetTypes::_MAX_COUNT; ++i )
                    {
                        SDescriptorPoolDesc::SSize Size;
                        Size.count = 16;
                        Size.type = static_cast<DESCRIPTOR_SET_TYPE>(i);
                        PoolDesc.vPoolSizes.PushBack( Size );
                    }
                }
                if( Desc.descPoolSize )
                {
                    handle_t hPool = m_pDeviceCtx->m_pDescSetMgr->CreatePool( PoolDesc );
                    if( hPool != INVALID_HANDLE )
                    {
                        m_vDescPools.PushBack( hPool );
                    }
                    else
                    {
                        ret = VKE_FAIL;
                    }
                }
                m_DescPoolDesc = PoolDesc;
                m_DescPoolDesc.maxSetCount = std::max( PoolDesc.maxSetCount, Config::RenderSystem::Pipeline::MAX_DESCRIPTOR_SET_COUNT );
            }

            return ret;
        }

        void CContextBase::Destroy()
        {
            if( m_pDeviceCtx != nullptr )
            {
                for( uint32_t i = 1; i < m_vDescPools.GetCount(); ++i )
                {
                    m_pDeviceCtx->m_pDescSetMgr->DestroyPool( &m_vDescPools[i] );
                }
                m_pDeviceCtx = nullptr;
                m_hCommandPool = INVALID_HANDLE;
                m_vDescPools.Clear();
            }
        }

        DescriptorSetHandle CContextBase::CreateDescriptorSet( const SDescriptorSetDesc& Desc )
        {
            DescriptorSetHandle hRet = INVALID_HANDLE;
            handle_t hPool;
            if( m_vDescPools.GetCount() == 1 )
            {
                hPool = m_pDeviceCtx->m_pDescSetMgr->CreatePool( m_DescPoolDesc );
            }
            else
            {
                hPool = m_vDescPools.Back();
            }
            if( hPool )
            {
                VKE_ASSERT( hPool != INVALID_HANDLE, "" );
                hRet = m_pDeviceCtx->m_pDescSetMgr->CreateSet( hPool, Desc );
                if( hRet == INVALID_HANDLE )
                {
                    m_pDeviceCtx->m_pDescSetMgr->CreatePool( m_DescPoolDesc );
                    hRet = CreateDescriptorSet( Desc );
                }
            }
            return hRet;
        }

        const DDIDescriptorSet& CContextBase::GetDescriptorSet( const DescriptorSetHandle& hSet )
        {
            return m_pDeviceCtx->m_pDescSetMgr->GetSet( hSet );
        }

        DescriptorSetLayoutHandle CContextBase::GetDescriptorSetLayout( const DescriptorSetHandle& hSet )
        {
            return m_pDeviceCtx->m_pDescSetMgr->GetLayout( hSet );
        }

        void CContextBase::UpdateDescriptorSet( BufferPtr pBuffer, DescriptorSetHandle* phInOut )
        {
            DescriptorSetHandle& hSet = *phInOut;
            const DDIDescriptorSet& hDDISet = m_pDeviceCtx->m_pDescSetMgr->GetSet( hSet );
            SUpdateBufferDescriptorSetInfo Info;
            
            SUpdateBufferDescriptorSetInfo::SBufferInfo BuffInfo;
            const auto& BindInfo = pBuffer->GetBindingInfo();

            BuffInfo.hDDIBuffer = pBuffer->GetDDIObject();
            BuffInfo.offset = BindInfo.offset;
            BuffInfo.range = BindInfo.range;

            Info.count = BindInfo.count;
            Info.binding = BindInfo.index;
            Info.hDDISet = hDDISet;
            
            Info.vBufferInfos.PushBack( BuffInfo );
            m_DDI.Update( Info );
        }

        void CContextBase::UpdateDescriptorSet( const RenderTargetHandle& hRT, DescriptorSetHandle* phInOut )
        {
            //DescriptorSetHandle& hSet = *phInOut;
            //const DDIDescriptorSet& hDDISet = m_pDeviceCtx->m_pDescSetMgr->GetSet( hSet );
            
            //TexturePtr pTex = m_pDeviceCtx->GetTexture( hRT );

            
            
        }

        void CContextBase::UpdateDescriptorSet( const SamplerHandle& hSampler, const RenderTargetHandle& hRT,
            DescriptorSetHandle* phInOut )
        {
            DescriptorSetHandle& hSet = *phInOut;
            const DDIDescriptorSet& hDDISet = m_pDeviceCtx->m_pDescSetMgr->GetSet( hSet );
            RenderTargetPtr pRT = m_pDeviceCtx->GetRenderTarget( hRT );

            SSamplerTextureBinding Binding;
            Binding.hSampler = hSampler;
            Binding.hTextureView = pRT->GetTextureView();
            //Binding.textureState = TextureStates::SHADER_READ;
            SUpdateTextureDescriptorSetInfo UpdateInfo;
            UpdateInfo.binding = 0;
            UpdateInfo.count = 1;
            UpdateInfo.hDDISet = hDDISet;
            SUpdateTextureDescriptorSetInfo::STextureInfo TexInfo;
            TexInfo.hDDISampler = m_pDeviceCtx->GetSampler( hSampler )->GetDDIObject();
            TexInfo.hDDITextureView = m_pDeviceCtx->GetTextureView( pRT->GetTextureView() )->GetDDIObject();
            TexInfo.textureState = TextureStates::SHADER_READ;
            UpdateInfo.vTextureInfos.PushBack( TexInfo );
            m_DDI.Update( UpdateInfo );
        }

        void CContextBase::UpdateDescriptorSet( const SUpdateBindingsHelper& Info, DescriptorSetHandle* phInOut )
        {
            DescriptorSetHandle& hSet = *phInOut;
            const DDIDescriptorSet& hDDISet = m_pDeviceCtx->m_pDescSetMgr->GetSet( hSet );
            m_DDI.Update( hDDISet, Info );
        }

        CCommandBuffer* CContextBase::_CreateCommandBuffer()
        {
            CCommandBuffer* pCb;
            //m_CmdBuffMgr.CreateCommandBuffers< VKE_THREAD_SAFE >( 1, &pCb );
            Result res = m_pDeviceCtx->_CreateCommandBuffers( m_hCommandPool, 1, &pCb );
            if( VKE_SUCCEEDED( res ) )
            {
                SCommandBufferInitInfo Info;
                Info.pBaseCtx = this;
                Info.backBufferIdx = m_backBufferIdx;
                Info.initComputeShader = m_initComputeShader;
                Info.initGraphicsShaders = m_initGraphicsShaders;
                pCb->Init( Info );
            }
            return pCb;
        }

        CCommandBuffer* CContextBase::_GetCurrentCommandBuffer()
        {
            if( m_pCurrentCommandBuffer == nullptr )
            {
                m_pCurrentCommandBuffer = _CreateCommandBuffer();
                m_pCurrentCommandBuffer->Begin();
            }
            return m_pCurrentCommandBuffer;
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
            Result ret = this->m_pCurrentCommandBuffer->End( flags, phDDIOut );
            this->m_pCurrentCommandBuffer = _CreateCommandBuffer();
            this->m_pCurrentCommandBuffer->Begin();
            return ret;
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
                pCb->m_state = CCommandBuffer::States::END;
                pSubmitMgr->Submit( m_pDeviceCtx, m_hCommandPool, pCb );
                if( phDDIOut )
                {
                    VKE_ASSERT( pSubmitMgr->m_pCurrBatch != nullptr, "" );
                    *phDDIOut = pSubmitMgr->m_pCurrBatch->GetSignaledSemaphore();
                }

            }
            else if( flags & ExecuteCommandBufferFlags::EXECUTE )
            {
                pCb->m_state = CCommandBuffer::States::FLUSH;
                auto pBatch = pSubmitMgr->_GetNextBatch( m_pDeviceCtx, m_hCommandPool );
                
                pBatch->_Submit( pCb );
                
                ret = pSubmitMgr->ExecuteBatch( m_pDeviceCtx, m_pQueue, &pBatch );
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
                        }
                    }
                }
            }

            return ret;
        }

        CTransferContext* CContextBase::GetTransferContext()
        {
            return GetDeviceContext()->GetTransferContext();
        }

        Result CContextBase::UpdateBuffer( const SUpdateMemoryInfo& Info, BufferPtr* ppInOut )
        {
            Result ret = VKE_FAIL;
            CBuffer* pBuffer = ( *ppInOut ).Get();
            ret = m_pDeviceCtx->m_pBufferMgr->UpdateBuffer( Info, this, &pBuffer );
            return ret;
        }

        Result CContextBase::UpdateBuffer( const SUpdateMemoryInfo& Info, BufferHandle* phInOut )
        {
            Result ret = VKE_FAIL;
            CBuffer* pBuffer = m_pDeviceCtx->m_pBufferMgr->GetBuffer( *phInOut ).Get();
            ret = m_pDeviceCtx->m_pBufferMgr->UpdateBuffer( Info, this, &pBuffer );
            return ret;
        }

        uint32_t CContextBase::LockStagingBuffer(const uint32_t maxSize)
        {
            uint32_t ret = m_pDeviceCtx->m_pBufferMgr->LockStagingBuffer(maxSize);
            return ret;
        }

        void CContextBase::UpdateStagingBuffer(const uint32_t& hUpdateInfo, const void* pData, const uint32_t dataSize)
        {
            m_pDeviceCtx->m_pBufferMgr->UpdateStagingBufferMemory(hUpdateInfo, pData, dataSize);
        }

        Result CContextBase::UnlockStagingBuffer(CContextBase* pCtx, const SUnlockBufferInfo& Info)
        {
            return m_pDeviceCtx->m_pBufferMgr->UnlockStagingBuffer(pCtx, Info);
        }

        PipelinePtr CContextBase::BuildCurrentPipeline()
        {
            PipelinePtr pRet;
            /*PipelineLayoutPtr pLayout = m_pDeviceCtx->CreatePipelineLayout( this->m_pCurrentCommandBuffer->m_CurrentPipelineLayoutDesc );
            this->m_pCurrentCommandBuffer->m_CurrentPipelineDesc.Pipeline.hDDILayout = pLayout->GetDDIObject();
            pRet = m_pDeviceCtx->CreatePipeline( this->m_pCurrentCommandBuffer->m_CurrentPipelineDesc );*/
            this->m_pCurrentCommandBuffer->_UpdateCurrentPipeline();
            pRet = this->m_pCurrentCommandBuffer->m_pCurrentPipeline;
            return pRet;
        }

        void CContextBase::SetTextureState( const TextureHandle& hTex, const TEXTURE_STATE& state )
        {
            TexturePtr pTex = m_pDeviceCtx->GetTexture( hTex );
            VKE_ASSERT( pTex->GetState() != state, "" );
            //if( pTex->GetState() != state )
            {
                STextureBarrierInfo Info;
                pTex->SetState( state, &Info );
                _GetCurrentCommandBuffer()->Barrier( Info );
            }
        }

        void CContextBase::SetTextureState( const RenderTargetHandle& hRt, const TEXTURE_STATE& state )
        {
            RenderTargetPtr pRT = m_pDeviceCtx->GetRenderTarget( hRt );
            SetTextureState( pRT->GetTexture(), state );
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

        DescriptorSetHandle CContextBase::CreateResourceBindings( const SCreateBindingDesc& Desc )
        {
            DescriptorSetHandle ret = INVALID_HANDLE;

            auto hLayout = m_pDeviceCtx->CreateDescriptorSetLayout( Desc.LayoutDesc );
            if( hLayout != INVALID_HANDLE )
            {
                SDescriptorSetDesc SetDesc;
                SetDesc.vLayouts.PushBack( hLayout );
                ret = CreateDescriptorSet( SetDesc );
            }
            
            return ret;
        }

        DescriptorSetHandle CContextBase::CreateResourceBindings( const SUpdateBindingsHelper& Info )
        {
            DescriptorSetHandle ret = INVALID_HANDLE;
            SCreateBindingDesc Desc;
            
            for( uint32_t i = 0; i < Info.vRTs.GetCount(); ++i )
            {
                //const auto& Curr = Info.vRTs[i];
                //TextureHandle hTex = m_pDeviceCtx->GetTexture(Curr.)
                //Desc.AddTexture()
            }
            return ret;
        }

        void CContextBase::_DestroyDescriptorSets( DescriptorSetHandle* phSets, const uint32_t count )
        {
            if( count )
            {
                m_pDeviceCtx->m_pDescSetMgr->_DestroySets( phSets, count );
            }
        }

        void CContextBase::_FreeDescriptorSets( DescriptorSetHandle* phSets, uint32_t count )
        {
            if( count )
            {
                m_pDeviceCtx->m_pDescSetMgr->_FreeSets( phSets, count );
            }
        }

        void CContextBase::FreeDescriptorSet( const DescriptorSetHandle& hSet )
        {
            this->m_pCurrentCommandBuffer->_FreeDescriptorSet( hSet );
        }

    } // RenderSystem
} // VKE