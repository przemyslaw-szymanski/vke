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
            if( usage & BufferUsages::UNIFORM_BUFFER )
            {
                ret = BindingTypes::UNIFORM_BUFFER;
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
            /*SInfo Info;
            Info.type = BufferUsageToBindingType( pBuffer->GetDesc().usage );
            Info.stages = stages;
            Info.Binding = Binding;

            vBindings.PushBack( Info );*/
            SDescriptorSetLayoutDesc::SBinding BindInfo;
            BindInfo.count = Binding.count;
            BindInfo.idx = Binding.index;
            BindInfo.stages = Binding.stages;
            BindInfo.type = BufferUsageToBindingType( pBuffer->GetDesc().usage );
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

            m_vDescPools.PushBack( NULL_HANDLE );


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
                    if( hPool != NULL_HANDLE )
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
                for( uint32_t i = 0; i < m_vDescPools.GetCount(); ++i )
                {
                    m_pDeviceCtx->m_pDescSetMgr->DestroyPool( &m_vDescPools[i] );
                }
                m_pDeviceCtx = nullptr;
                m_hCommandPool = NULL_HANDLE;
                m_vDescPools.Clear();
            }
        }

        DescriptorSetHandle CContextBase::CreateDescriptorSet( const SDescriptorSetDesc& Desc )
        {
            DescriptorSetHandle hRet = NULL_HANDLE;
            handle_t hPool;
            if( m_vDescPools.IsEmpty() )
            {
                hPool = m_pDeviceCtx->m_pDescSetMgr->CreatePool( m_DescPoolDesc );
            }
            else
            {
                hPool = m_vDescPools.Back();
            }
            if( hPool )
            {
                VKE_ASSERT( hPool != NULL_HANDLE, "" );
                hRet = m_pDeviceCtx->m_pDescSetMgr->CreateSet( hPool, Desc );
                if( hRet == NULL_HANDLE )
                {
                    m_pDeviceCtx->m_pDescSetMgr->CreatePool( m_DescPoolDesc );
                    hRet = CreateDescriptorSet( Desc );
                }
            }
            return hRet;
        }

        const SDescriptorSet* CContextBase::GetDescriptorSet( const DescriptorSetHandle& hSet )
        {
            return m_pDeviceCtx->m_pDescSetMgr->GetSet( hSet );
        }

        void CContextBase::UpdateDescriptorSet( BufferPtr pBuffer, DescriptorSetHandle* phInOut )
        {
            DescriptorSetHandle& hSet = *phInOut;
            const DDIDescriptorSet& hDDISet = m_pDeviceCtx->m_pDescSetMgr->GetSet( hSet )->hDDISet;
            SUpdateBufferDescriptorSetInfo Info;
            Info.count = 1;
            Info.binding = 0;
            Info.hDDISet = hDDISet;
            Info.vBufferInfos.PushBack( { pBuffer->GetDDIObject(), 0, pBuffer->GetSize() } );
            m_DDI.Update( Info );
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

        Result CContextBase::_EndCommandBuffer( CCommandBuffer** ppInOut, COMMAND_BUFFER_END_FLAGS flags )
        {
            Result ret = VKE_OK;
            CCommandBuffer* pCb = *ppInOut;
            m_DDI.EndCommandBuffer( pCb->GetDDIObject() );

            auto pSubmitMgr = m_pQueue->_GetSubmitManager();

            if( flags & CommandBufferEndFlags::END )
            {
                pCb->m_state = CCommandBuffer::States::END;
                pSubmitMgr->Submit( m_pDeviceCtx, m_hCommandPool, pCb );
            }
            else if( flags & CommandBufferEndFlags::EXECUTE )
            {
                pCb->m_state = CCommandBuffer::States::FLUSH;
                auto pBatch = pSubmitMgr->_GetNextBatch( m_pDeviceCtx, m_hCommandPool );
                pSubmitMgr->m_signalSemaphore = ( flags & CommandBufferEndFlags::DONT_SIGNAL_SEMAPHORE ) == 0;
                pBatch->_Submit( pCb );
                ret = pSubmitMgr->ExecuteBatch( m_pQueue, &pBatch );
                if( flags & CommandBufferEndFlags::WAIT )
                {
                    if( VKE_SUCCEEDED( ret ) )
                    {
                        ret = m_DDI.WaitForFences( pBatch->m_hDDIFence, UINT64_MAX );
                    }
                }
            }

            return ret;
        }

        Result CContextBase::UpdateBuffer( const SUpdateMemoryInfo& Info, BufferPtr* ppInOut )
        {
            Result ret = VKE_FAIL;
            CBuffer* pBuffer = ( *ppInOut ).Get();
            ret = m_pDeviceCtx->m_pBufferMgr->UpdateBuffer( Info, this, &pBuffer );
            return ret;
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
            DescriptorSetHandle ret = NULL_HANDLE;

            auto hLayout = m_pDeviceCtx->CreateDescriptorSetLayout( Desc.LayoutDesc );
            if( hLayout != NULL_HANDLE )
            {
                SDescriptorSetDesc SetDesc;
                SetDesc.vLayouts.PushBack( hLayout );
                ret = CreateDescriptorSet( SetDesc );
            }
            
            return ret;
        }

    } // RenderSystem
} // VKE