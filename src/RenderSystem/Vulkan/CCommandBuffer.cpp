#include "RenderSystem/Vulkan/CCommandBuffer.h"
#include "RenderSystem/Resources/CShader.h"
#include "RenderSystem/CDeviceContext.h"
#if VKE_VULKAN_RENDERER
#include "RenderSystem/Vulkan/Managers/CPipelineManager.h"
#include "RenderSystem/Vulkan/Managers/CSubmitManager.h"
#include "RenderSystem/CRenderPass.h"
#include "RenderSystem/CSwapChain.h"
#include "Core/Utils/CProfiler.h"
#include "RenderSystem/CContextBase.h"

namespace VKE
{
    namespace RenderSystem
    {
        static CPipeline g_sDummyPipeline = CPipeline(nullptr);
        static PipelinePtr g_spDummyPipeline( &g_sDummyPipeline );

        CCommandBuffer::CCommandBuffer() :
            m_pCurrentPipeline( g_spDummyPipeline )
        {
        }

        CCommandBuffer::~CCommandBuffer()
        {
        }

        void CCommandBuffer::Init(const SCommandBufferInitInfo& Info)
        {
            VKE_ASSERT( m_hDDIObject != DDI_NULL_HANDLE, "" );
            VKE_ASSERT( Info.pBaseCtx != nullptr, "" );

            _Reset();

            m_pBaseCtx = Info.pBaseCtx;
            // Init pipeline desc only one time
            if( m_pBaseCtx != nullptr )
            {
#if !VKE_ENABLE_SIMPLE_COMMAND_BUFFER
                m_CurrentPipelineDesc.Create.async = false;
                m_CurrentPipelineDesc.Pipeline = SPipelineDesc();
                if( Info.initGraphicsShaders )
                {
                    m_CurrentPipelineDesc.Pipeline.Shaders.apShaders[ShaderTypes::VERTEX] = m_pBaseCtx->m_pDeviceCtx->GetDefaultShader( ShaderTypes::VERTEX );
                    m_CurrentPipelineDesc.Pipeline.Shaders.apShaders[ShaderTypes::PIXEL] = m_pBaseCtx->m_pDeviceCtx->GetDefaultShader( ShaderTypes::PIXEL );
                    m_CurrentPipelineDesc.Pipeline.hLayout = PipelineLayoutHandle{ m_pBaseCtx->m_pDeviceCtx->GetDefaultPipelineLayout()->GetHandle() };

                }
                else if( Info.initComputeShader )
                {
                    m_CurrentPipelineDesc.Pipeline.hLayout = PipelineLayoutHandle{ m_pBaseCtx->m_pDeviceCtx->GetDefaultPipelineLayout()->GetHandle() };
                }
#endif
            }
        }

        bool CCommandBuffer::IsExecuted()
        {
            bool ret = m_hDDIFence != DDI_NULL_HANDLE && m_pBaseCtx->m_DDI.IsReady( m_hDDIFence );
            return ret;
        }

        void CCommandBuffer::AddWaitOnSemaphore( const DDISemaphore& hDDISemaphore )
        {
            m_vDDIWaitOnSemaphores.PushBack( hDDISemaphore );
        }

        void CCommandBuffer::_BeginProlog()
        {
            //_Reset();

            /*if( m_pCurrentPipelineLayout.IsNull() )
            {
                {
                    SPipelineLayoutDesc Desc;
                    PipelineLayoutPtr pLayout = m_pBaseCtx->m_pDeviceCtx->CreatePipelineLayout( Desc );
                    VKE_ASSERT( pLayout.IsValid(), "Invalid pipeline layout." );
                    m_pCurrentPipelineLayout = pLayout;
                }
            }
            if( m_pCurrentPipeline.IsNull() )
            {
                SetState( m_pCurrentPipelineLayout );
            }*/
        }

        void CCommandBuffer::Begin()
        {
            VKE_ASSERT( m_state == States::UNKNOWN || m_state == States::FLUSH || m_state == States::END, "" );

            _BeginProlog();
            
            auto pThis = this;
            
            m_pBaseCtx->_BeginCommandBuffer( &pThis );
            m_state = States::BEGIN;
        }

        Result CCommandBuffer::End( COMMAND_BUFFER_END_FLAGS flag, DDISemaphore* phDDIOut )
        {
            Result ret = VKE_OK;
            if( m_state == States::BEGIN )
            {
                if( m_isRenderPassBound )
                {
                    Bind( RenderPassPtr() );
                }
                if( m_needExecuteBarriers )
                {
                    ExecuteBarriers();
                }

                auto pThis = this;
                ret = m_pBaseCtx->_EndCommandBuffer( flag, &pThis, phDDIOut );
            }
            return ret;
        }

        void CCommandBuffer::Barrier( const SMemoryBarrierInfo& Info )
        {
            VKE_ASSERT( m_state == States::BEGIN, "Command buffer must Begun" );
            m_BarrierInfo.vMemoryBarriers.PushBack( Info );
            m_needExecuteBarriers = true;
        }

        void CCommandBuffer::Barrier( const SBufferBarrierInfo& Info )
        {
            VKE_ASSERT( m_state == States::BEGIN, "Command buffer must Begun" );
            m_BarrierInfo.vBufferBarriers.PushBack( Info );
            m_needExecuteBarriers = true;
        }

        void CCommandBuffer::Barrier( const STextureBarrierInfo& Info )
        {
            VKE_ASSERT( m_state == States::BEGIN, "Command buffer must Begun" );
            m_BarrierInfo.vTextureBarriers.PushBack( Info );
            m_needExecuteBarriers = true;
        }

        void CCommandBuffer::ExecuteBarriers()
        {
            m_pBaseCtx->m_pDeviceCtx->_GetDDI().Barrier( this->GetDDIObject(), m_BarrierInfo );
            m_BarrierInfo.vBufferBarriers.Clear();
            m_BarrierInfo.vMemoryBarriers.Clear();
            m_BarrierInfo.vTextureBarriers.Clear();
            m_needExecuteBarriers = false;
        }

        void CCommandBuffer::Bind( const RenderTargetHandle& hRT )
        {
#if !VKE_ENABLE_SIMPLE_COMMAND_BUFFER
            RenderTargetPtr pRT = m_pBaseCtx->m_pDeviceCtx->GetRenderTarget( hRT );
            const auto& Desc = pRT->GetDesc();
            m_CurrentRenderPassDesc.vRenderTargets.PushBack( Desc );
            m_CurrentRenderPassDesc.Size = pRT->GetSize();
            m_needNewRenderPass = true;
#endif
        }

        void CCommandBuffer::_Reset()
        {
#if !VKE_ENABLE_SIMPLE_COMMAND_BUFFER
            m_CurrentPipelineDesc.Pipeline.hRenderPass = NULL_HANDLE;
            m_CurrentPipelineDesc.Pipeline.hDDIRenderPass = DDI_NULL_HANDLE;
            m_CurrentPipelineDesc.Pipeline.Viewport.vViewports.Clear();
            m_CurrentPipelineDesc.Pipeline.Viewport.vScissors.Clear();
            m_CurrentPipelineLayoutDesc.vDescriptorSetLayouts.Clear();

            m_pCurrentPipeline = nullptr;
            m_pCurrentPipelineLayout = nullptr;
            m_pCurrentRenderPass = nullptr;

            m_hCurrentdRenderPass = NULL_HANDLE;
            m_hDDILastUsedLayout = DDI_NULL_HANDLE;
            m_CurrentRenderPassDesc.vRenderTargets.Clear();
            m_CurrentRenderPassDesc.vSubpasses.Clear();
#endif
            m_isPipelineBound = false;
            m_needExecuteBarriers = false;
            m_needNewPipeline = true;
            m_needNewPipelineLayout = true;
            m_needNewRenderPass = false;
            m_isRenderPassBound = false;
            
            m_isDirty = false;

            m_hDDIFence = DDI_NULL_HANDLE;


            m_pBaseCtx->_DestroyDescriptorSets( m_vUsedSets.GetData(), m_vUsedSets.GetCount() );
            m_vUsedSets.Clear();
            m_vDDIWaitOnSemaphores.Clear();
        }

        void CCommandBuffer::SetState( PipelineLayoutPtr pLayout )
        {
#if !VKE_ENABLE_SIMPLE_COMMAND_BUFFER
            m_pCurrentPipelineLayout = pLayout;
            m_CurrentPipelineDesc.Pipeline.hLayout = PipelineLayoutHandle{ m_pCurrentPipelineLayout->GetHandle() };
            m_CurrentPipelineDesc.Pipeline.hDDILayout = m_pCurrentPipelineLayout->GetDDIObject();
            //m_CurrentPipelineDesc.Pipeline.hRenderPass.handle = reinterpret_cast< handle_t >( m_pCurrentRenderPass->GetDDIObject() );
            VKE_ASSERT( m_CurrentPipelineDesc.Pipeline.hLayout != NULL_HANDLE, "Invalid pipeline object." );
            m_needNewPipeline = true;
            m_needNewPipelineLayout = false;
#endif
        }

        void CCommandBuffer::Bind( RenderPassPtr pRenderPass )
        {
            SBindRenderPassInfo Info;
            Info.hDDICommandBuffer = GetDDIObject();

            if( pRenderPass.IsValid() )
            {
                if( m_isRenderPassBound )
                {
                    // Unbind pipeline
                    m_isPipelineBound = false;
                    // If there is already render pass bound end it
                    m_pBaseCtx->m_pDeviceCtx->DDI().Unbind( GetDDIObject(), (DDIRenderPass)(DDI_NULL_HANDLE) );
                }

                if( m_needExecuteBarriers )
                {
                    ExecuteBarriers();
                }
                // Close current render pass
                if( m_pCurrentRenderPass != nullptr && !m_pCurrentRenderPass->IsActive() )
                {
                    Info.pBeginInfo = nullptr;
                    m_pBaseCtx->m_pDeviceCtx->DDI().Bind( Info );
                    m_pCurrentRenderPass->_IsActive( false );
                }

                m_pCurrentRenderPass = pRenderPass;
                m_hCurrentdRenderPass = pRenderPass->GetHandle();
                m_pCurrentRenderPass->_IsActive( true );
                Info.pBeginInfo = &pRenderPass->GetBeginInfo();
                m_pBaseCtx->m_pDeviceCtx->DDI().Bind( Info );
                m_isRenderPassBound = true;
#if !VKE_ENABLE_SIMPLE_COMMAND_BUFFER
                const auto hPass = RenderPassHandle{ m_pCurrentRenderPass->GetHandle() };
                m_CurrentPipelineDesc.Pipeline.hRenderPass = hPass;
                m_CurrentPipelineDesc.Pipeline.hDDIRenderPass = DDI_NULL_HANDLE;
                m_needNewPipeline = true; // m_CurrentPipelineDesc.Pipeline.hRenderPass != hPass;

                VKE_ASSERT( m_pCurrentRenderPass->GetHandle() == m_hCurrentdRenderPass.handle, "" );
                VKE_ASSERT( m_pCurrentRenderPass->GetDDIObject() == m_pBaseCtx->m_pDeviceCtx->GetRenderPass( m_hCurrentdRenderPass )->GetDDIObject(), "" );
#endif
            }
            else
            {
                m_pBaseCtx->m_pDeviceCtx->DDI().Unbind( GetDDIObject(), (DDIRenderPass)(DDI_NULL_HANDLE) );
                m_isRenderPassBound = false;
                m_pCurrentRenderPass = nullptr;
                m_hCurrentdRenderPass = NULL_HANDLE;
            }

        }

        void CCommandBuffer::Bind( PipelinePtr pPipeline )
        {
            if( pPipeline.IsValid() && pPipeline->IsReady() )
            {
                SetState( pPipeline->GetLayout() );
#if !VKE_ENABLE_SIMPLE_COMMAND_BUFFER
                m_needNewPipeline = false;
#endif
                SBindPipelineInfo Info;
                Info.pCmdBuffer = this;
                Info.pPipeline = pPipeline.Get();
                m_isPipelineBound = true;
                m_pCurrentPipeline = pPipeline;
                m_pBaseCtx->m_pDeviceCtx->DDI().Bind( Info );
                m_pBaseCtx->m_DDI.SetState( GetDDIObject(), m_CurrViewport );
                m_pBaseCtx->m_DDI.SetState( GetDDIObject(), m_CurrScissor );
            }
        }

        void CCommandBuffer::SetState( ShaderPtr pShader )
        {
#if !VKE_ENABLE_SIMPLE_COMMAND_BUFFER
            if( pShader.IsValid() )
            {
                const auto type = pShader->GetDesc().type;
                VKE_ASSERT( type != ShaderTypes::_MAX_COUNT, "" );
                const ShaderHandle hShader( pShader->GetHandle() );
                {
                    m_CurrentPipelineDesc.Pipeline.Shaders.apShaders[ type ] = pShader;
                    m_needNewPipeline = true;
                }
            }
#endif
        }

        void CCommandBuffer::SetState( const ShaderHandle& hShader )
        {
#if !VKE_ENABLE_SIMPLE_COMMAND_BUFFER
            ShaderPtr pShader = m_pBaseCtx->m_pDeviceCtx->GetShader( hShader );
            SetState( pShader );
#endif
        }

        void CCommandBuffer::Bind( VertexBufferPtr pBuffer, const uint32_t offset )
        {
            m_pBaseCtx->m_pDeviceCtx->DDI().Bind( this->GetDDIObject(), pBuffer->GetDDIObject(), offset );
        }

        void CCommandBuffer::Bind( const VertexBufferHandle& hBuffer, const uint32_t offset )
        {
            const auto& hDDIBuffer = m_pBaseCtx->m_pDeviceCtx->GetBuffer( hBuffer )->GetDDIObject();
            m_pBaseCtx->m_DDI.Bind( this->GetDDIObject(), hDDIBuffer, offset );
        }

        void CCommandBuffer::Bind( const IndexBufferHandle& hBuffer, const uint32_t offset )
        {
            auto pBuffer = m_pBaseCtx->m_pDeviceCtx->GetBuffer( hBuffer );
            const auto& hDDIBuffer = pBuffer->GetDDIObject();
            INDEX_TYPE type = pBuffer->GetDesc().indexType;
            m_pBaseCtx->m_DDI.Bind( this->GetDDIObject(), hDDIBuffer, offset, type );
        }

        void CCommandBuffer::Bind( CSwapChain* pSwapChain )
        {
            Bind( pSwapChain->m_DDISwapChain );
            SetState( pSwapChain->m_CurrViewport );
            SetState( pSwapChain->m_CurrScissor );
        }

        void CCommandBuffer::Bind( const SDDISwapChain& SwapChain )
        {
            if( m_isRenderPassBound )
            {
                Bind( RenderPassPtr{} );
            }
            if( m_needExecuteBarriers )
            {
                ExecuteBarriers();
            }

            SBindRenderPassInfo Info;
            SBeginRenderPassInfo BeginInfo;
            const auto idx = GetBackBufferIndex();
            BeginInfo.hDDIFramebuffer = SwapChain.vFramebuffers[idx];
            BeginInfo.hDDIRenderPass = SwapChain.hDDIRenderPass;
            BeginInfo.RenderArea.Size = SwapChain.Size;
            BeginInfo.RenderArea.Offset = { 0,0 };
            BeginInfo.vDDIClearValues.PushBack( {0,0,1,1} );

            Info.hDDICommandBuffer = GetDDIObject();
            Info.pBeginInfo = &BeginInfo;

            m_pBaseCtx->m_pDeviceCtx->DDI().Bind( Info );

#if !VKE_ENABLE_SIMPLE_COMMAND_BUFFER
            m_needNewPipeline = m_CurrentPipelineDesc.Pipeline.hDDIRenderPass != SwapChain.hDDIRenderPass;
            m_CurrentPipelineDesc.Pipeline.hRenderPass = NULL_HANDLE;
            m_CurrentPipelineDesc.Pipeline.hDDIRenderPass = SwapChain.hDDIRenderPass;
#endif
            m_isRenderPassBound = true;
        }

        void CCommandBuffer::Bind( const DescriptorSetHandle& hSet, const uint32_t offset )
        {
            m_vBindings.PushBack( hSet );
            const DDIDescriptorSet& hDDISet = m_pBaseCtx->GetDescriptorSet( hSet );
            DescriptorSetLayoutHandle hLayout = m_pBaseCtx->GetDescriptorSetLayout( hSet );

            m_vDDIBindings.PushBack( hDDISet );
            m_vBindingOffsets.PushBack( offset );

#if !VKE_ENABLE_SIMPLE_COMMAND_BUFFER
            m_CurrentPipelineLayoutDesc.vDescriptorSetLayouts.PushBack( hLayout );
            m_needNewPipelineLayout = true;
#endif
        }

        void CCommandBuffer::Bind( const uint32_t& index, const DescriptorSetHandle& hDescSet,
                                   const uint32_t& offset )
        {
            VKE_ASSERT( m_pCurrentPipeline != nullptr, "Pipeline must be already bound to call this function." );
            SBindDDIDescriptorSetsInfo Info;
            const DDIDescriptorSet& hDDIDescSet = m_pBaseCtx->GetDescriptorSet( hDescSet );

            Info.aDDISetHandles = &hDDIDescSet;
            Info.aDynamicOffsets = &offset;
            Info.dynamicOffsetCount = 1;
            Info.firstSet = (uint16_t)index;
            Info.setCount = 1;
            Info.hDDICommandBuffer = GetDDIObject();
            Info.hDDIPipelineLayout = m_pCurrentPipeline->GetLayout()->GetDDIObject();
            Info.pipelineType = m_pCurrentPipeline->GetType();

            m_pBaseCtx->m_DDI.Bind( Info );
        }

        void CCommandBuffer::Bind( const SBindDDIDescriptorSetsInfo& Info )
        {
            m_pBaseCtx->m_DDI.Bind( Info );
        }

        void CCommandBuffer::SetState(const SPipelineDesc::SDepthStencil& DepthStencil)
        {
#if !VKE_ENABLE_SIMPLE_COMMAND_BUFFER
            m_CurrentPipelineDesc.Pipeline.DepthStencil = DepthStencil;
            m_needNewPipeline = true;
#endif
        }

        void CCommandBuffer::SetState(const SPipelineDesc::SRasterization& Rasterization)
        {
#if !VKE_ENABLE_SIMPLE_COMMAND_BUFFER
            m_CurrentPipelineDesc.Pipeline.Rasterization = Rasterization;
            m_needNewPipeline = true;
#endif
        }

        void CCommandBuffer::SetState( const SPipelineDesc::SInputLayout& InputLayout )
        {
#if !VKE_ENABLE_SIMPLE_COMMAND_BUFFER
            m_CurrentPipelineDesc.Pipeline.InputLayout = InputLayout;
            m_needNewPipeline = true;
#endif
        }

        void CCommandBuffer::SetState( const SViewportDesc& Viewport )
        {
            uint32_t h = Viewport.CalcHash();
            if( m_currViewportHash != h )
            {
                m_currViewportHash = h;
                //m_pBaseCtx->m_DDI.SetState( m_hDDIObject, Viewport );
                m_CurrViewport = Viewport;
                //m_CurrentPipelineDesc.Pipeline.Viewport.vViewports.PushBack( Viewport );
            }
        }

        void CCommandBuffer::SetState( const SScissorDesc& Scissor )
        {
            uint32_t h = Scissor.CalcHash();
            if( m_currScissorHash != h )
            {
                m_currScissorHash = h;
                //m_pBaseCtx->m_DDI.SetState( m_hDDIObject, Scissor );
                m_CurrScissor = Scissor;
                //m_CurrentPipelineDesc.Pipeline.Viewport.vScissors[0] = Scissor;
            }
        }

        void CCommandBuffer::SetState( const SViewportDesc& Viewport, bool )
        {
            uint32_t h = Viewport.CalcHash();
            if( m_currViewportHash != h )
            {
                m_currViewportHash = h;
                m_pBaseCtx->m_DDI.SetState( GetDDIObject(), Viewport );
                m_CurrViewport = Viewport;
            }
        }

        void CCommandBuffer::SetState( const SScissorDesc& Scissor, bool )
        {
            uint32_t h = Scissor.CalcHash();
            if( m_currScissorHash != h )
            {
                m_currScissorHash = h;
                m_pBaseCtx->m_DDI.SetState( GetDDIObject(), Scissor );
                m_CurrScissor = Scissor;
            }
        }

        void CCommandBuffer::SetState( const PRIMITIVE_TOPOLOGY& topology )
        {
#if !VKE_ENABLE_SIMPLE_COMMAND_BUFFER
            m_CurrentPipelineDesc.Pipeline.InputLayout.topology = topology;
            m_needNewPipeline = true;
#endif
        }

        uint32_t ConvertFormatToSize( const FORMAT& format )
        {
            switch( format )
            {
                case Formats::R8_SINT:
                case Formats::R8_UINT:
                case Formats::R8_SNORM:
                case Formats::R8_UNORM:
                    return sizeof( uint8_t );
                break;
                case Formats::R8G8_SINT:
                case Formats::R8G8_UINT:
                case Formats::R8G8_SNORM:
                case Formats::R8G8_UNORM:
                    return sizeof( uint8_t ) * 2;
                break;
                case Formats::R8G8B8_SINT:
                case Formats::R8G8B8_UINT:
                case Formats::R8G8B8_SNORM:
                case Formats::R8G8B8_UNORM:
                    return sizeof( uint8_t ) * 3;
                break;
                case Formats::R8G8B8A8_SINT:
                case Formats::R8G8B8A8_UINT:
                case Formats::R8G8B8A8_SNORM:
                case Formats::R8G8B8A8_UNORM:
                    return sizeof( uint8_t ) * 4;
                break;
                
                case Formats::R16_SINT:
                case Formats::R16_UINT:
                case Formats::R16_SNORM:
                case Formats::R16_UNORM:
                    return sizeof( uint16_t );
                    break;
                case Formats::R16G16_SINT:
                case Formats::R16G16_UINT:
                case Formats::R16G16_SNORM:
                case Formats::R16G16_UNORM:
                    return sizeof( uint16_t ) * 2;
                    break;
                case Formats::R16G16B16_SINT:
                case Formats::R16G16B16_UINT:
                case Formats::R16G16B16_SNORM:
                case Formats::R16G16B16_UNORM:
                    return sizeof( uint16_t ) * 3;
                    break;
                case Formats::R16G16B16A16_SINT:
                case Formats::R16G16B16A16_UINT:
                case Formats::R16G16B16A16_SNORM:
                case Formats::R16G16B16A16_UNORM:
                    return sizeof( uint16_t ) * 4;
                    break;

                case Formats::R32_SINT:
                case Formats::R32_UINT:
                case Formats::R32_SFLOAT:
                    return sizeof( float );
                    break;
                case Formats::R32G32_SINT:
                case Formats::R32G32_UINT:
                case Formats::R32G32_SFLOAT:
                    return sizeof( float ) * 2;
                    break;
                case Formats::R32G32B32_SINT:
                case Formats::R32G32B32_UINT:
                case Formats::R32G32B32_SFLOAT:
                    return sizeof( float ) * 3;
                    break;
                case Formats::R32G32B32A32_SINT:
                case Formats::R32G32B32A32_UINT:
                case Formats::R32G32B32A32_SFLOAT:
                    return sizeof( float ) * 4;
                    break;

                default:
                    return 0;
                break;
            }
        }

        void CCommandBuffer::SetState( const SVertexInputLayoutDesc& VertexInputLayout )
        {
#if !VKE_ENABLE_SIMPLE_COMMAND_BUFFER
            uint16_t currOffset = 0;
            uint16_t currLocation = 0;
            uint32_t vertexSize = 0;
            
            for( uint32_t i = 0; i < VertexInputLayout.vAttributes.GetCount(); ++i )
            {
                vertexSize += ConvertFormatToSize( static_cast<FORMAT>( VertexInputLayout.vAttributes[ i ].type ) );
            }

            m_CurrentPipelineDesc.Pipeline.InputLayout.vVertexAttributes.Clear();
            m_CurrentPipelineDesc.Pipeline.InputLayout.topology = VertexInputLayout.topology;
            m_CurrentPipelineDesc.Pipeline.InputLayout.enablePrimitiveRestart = VertexInputLayout.enablePrimitiveRestart;

            for( uint32_t i = 0; i < VertexInputLayout.vAttributes.GetCount(); ++i )
            {
                const auto& Curr = VertexInputLayout.vAttributes[i];

                SPipelineDesc::SInputLayout::SVertexAttribute VA;
                VA.vertexBufferBindingIndex = Curr.vertexBufferBinding;
                VA.offset = currOffset;
                VA.location = currLocation;
                VA.format = static_cast<FORMAT>( Curr.type );
                VA.pName = Curr.pName;
                VA.stride = static_cast< uint16_t >( vertexSize );

                currOffset += static_cast< uint16_t >( ConvertFormatToSize( static_cast< FORMAT >( VertexInputLayout.vAttributes[ i ].type ) ) );
                ++currLocation;

                m_CurrentPipelineDesc.Pipeline.InputLayout.vVertexAttributes.PushBack( VA );
            }

            m_needNewPipeline = true;
#endif
        }

        Result CCommandBuffer::_DrawProlog()
        {
            Result ret = VKE_FAIL;
            
            if( m_needExecuteBarriers )
            {
                ExecuteBarriers();
            }
#if !VKE_ENABLE_SIMPLE_COMMAND_BUFFER
            if( _UpdateCurrentRenderPass() == VKE_OK )
            {
                Bind( m_pCurrentRenderPass );
            }
#endif
            ret = _UpdateCurrentPipeline();
            VKE_ASSERT( m_pCurrentPipeline.IsValid(), "Pipeline was not created successfully." );
#if !VKE_ENABLE_SIMPLE_COMMAND_BUFFER
            if( !m_isPipelineBound )
            {
                Bind( m_pCurrentPipeline );
            }
#endif
            if( !m_vBindings.IsEmpty() )
            {
                if( VKE_SUCCEEDED( ret ) )
                {
                    _BindDescriptorSets();
                }
                m_vBindings.Clear();
                m_vDDIBindings.Clear();
                m_vBindingOffsets.Clear();
            }
            VKE_ASSERT( m_isRenderPassBound == true, "Render pass must be bound before drawcall." );

            return ret;
        }

        void CCommandBuffer::_BindDescriptorSets()
        {
#if !VKE_ENABLE_SIMPLE_COMMAND_BUFFER
            VKE_ASSERT( m_pCurrentPipelineLayout->GetDDIObject() == m_pCurrentPipeline->GetDesc().hDDILayout, "" );
#endif
            SBindDDIDescriptorSetsInfo Info;
            Info.aDDISetHandles = m_vDDIBindings.GetData();
            Info.aDynamicOffsets = m_vBindingOffsets.GetData();
            Info.dynamicOffsetCount = static_cast< uint16_t >( m_vBindingOffsets.GetCount() );
            Info.firstSet = 0;
            Info.hDDICommandBuffer = GetDDIObject();
            Info.hDDIPipelineLayout = m_pCurrentPipeline->GetLayout()->GetDDIObject();
            Info.setCount = static_cast< uint16_t >( m_vDDIBindings.GetCount() );
            Info.pipelineType = m_pCurrentPipeline->GetType();
            m_pBaseCtx->m_DDI.Bind( Info );

            /*for( uint32_t i = 0; i < m_vBindings.GetCount(); ++i )
            {
                if( m_vUsedSets.Find( m_vBindings[i] ) < 0 )
                {
                    m_vUsedSets.PushBack( m_vBindings[i] );
                }
            }*/
        }

        void CCommandBuffer::DrawIndexedWithCheck( const SDrawParams& Params )
        {
            if( VKE_SUCCEEDED( _DrawProlog() ) )
            {
                m_pBaseCtx->m_pDeviceCtx->DDI().DrawIndexed( this->m_hDDIObject, Params );
            }
        }

        void CCommandBuffer::DrawWithCheck( const uint32_t& vertexCount, const uint32_t& instanceCount, const uint32_t& firstVertex,
            const uint32_t& firstInstance)
        {
            //if( m_needNewPipeline )
            {
                _DrawProlog();
            }
            VKE_ASSERT( m_isPipelineBound, "Pipeline must be set." );
            //VKE_SIMPLE_PROFILE();
            m_pBaseCtx->m_DDI.Draw( GetDDIObject(), vertexCount, instanceCount, firstVertex, firstInstance );
        }

        void CCommandBuffer::DrawIndexedFast( const SDrawParams& Params )
        {
            VKE_ASSERT( m_isPipelineBound, "Pipeline must be set." );
            m_pBaseCtx->m_pDeviceCtx->DDI().DrawIndexed( this->m_hDDIObject, Params );
        }

        void CCommandBuffer::DrawFast( const uint32_t& vertexCount, const uint32_t& instanceCount, const uint32_t& firstVertex,
            const uint32_t& firstInstance )
        {
            VKE_ASSERT( m_isPipelineBound, "Pipeline must be set." );
            m_pBaseCtx->m_DDI.Draw( GetDDIObject(), vertexCount, instanceCount, firstVertex, firstInstance );
        }
        

        void CCommandBuffer::Copy( const SCopyBufferInfo& Info )
        {
            if( m_needExecuteBarriers )
            {
                ExecuteBarriers();
            }
            m_pBaseCtx->m_DDI.Copy( m_hDDIObject, Info );
        }

        void CCommandBuffer::Copy( const SCopyTextureInfoEx& Info )
        {
            if( m_needExecuteBarriers )
            {
                ExecuteBarriers();
            }
            m_pBaseCtx->m_DDI.Copy( m_hDDIObject, Info );
        }

        void CCommandBuffer::_FreeDescriptorSet( const DescriptorSetHandle& hSet )
        {
            m_vUsedSets.PushBack( hSet );
        }

        Result CCommandBuffer::_UpdateCurrentPipeline()
        {
            Result ret = VKE_OK;
#if !VKE_ENABLE_SIMPLE_COMMAND_BUFFER
            if( m_needNewPipelineLayout )
            {
                m_pCurrentPipelineLayout = m_pBaseCtx->m_pDeviceCtx->CreatePipelineLayout( m_CurrentPipelineLayoutDesc );
                if( m_pCurrentPipelineLayout.IsValid() )
                {
                    m_CurrentPipelineLayoutDesc.vDescriptorSetLayouts.Clear();
                    const DDIPipelineLayout& hDDILayout = m_pCurrentPipelineLayout->GetDDIObject();
                    // If pipeline layout didn't change do not to try to create new pipeline
                    if( hDDILayout != m_hDDILastUsedLayout )
                    {
                        //m_CurrentPipelineDesc.Pipeline.hDDILayout = m_pCurrentPipelineLayout->GetDDIObject();
                        //m_needNewPipeline = true;
                        SetState( m_pCurrentPipelineLayout );
                        m_hDDILastUsedLayout = hDDILayout;
                    }

                    m_needNewPipelineLayout = false;
                }
                else
                {
                    ret = VKE_FAIL;
                }
            }
            if( (m_needNewPipeline && ret == VKE_OK) ||
                m_pCurrentPipeline.IsNull() )
            {
                //m_CurrentPipelineDesc.Pipeline.hDDILayout = m_pCurrentPipelineLayout->GetDDIObject();
                VKE_ASSERT( m_CurrentPipelineDesc.Pipeline.hDDILayout != DDI_NULL_HANDLE, "" );
                m_pCurrentPipeline = m_pBaseCtx->m_pDeviceCtx->CreatePipeline( m_CurrentPipelineDesc );
                if( m_pCurrentPipeline.IsNull() )
                {
                    ret = VKE_FAIL;
                }
                m_needNewPipeline = false;
                m_isPipelineBound = false;
            }
#endif
            if( !m_pCurrentPipeline->IsReady() )
            {
                ret = VKE_FAIL;
            }
            return ret;
        }

        Result CCommandBuffer::_UpdateCurrentRenderPass()
        {
            Result ret = VKE_FAIL;
#if !VKE_ENABLE_SIMPLE_COMMAND_BUFFER
            if( m_needNewRenderPass )
            {   
                auto hPass = m_pBaseCtx->m_pDeviceCtx->CreateRenderPass( m_CurrentRenderPassDesc );
                RenderPassPtr pPass = m_pBaseCtx->m_pDeviceCtx->GetRenderPass( hPass );
                m_needNewRenderPass = false;
                if( m_hCurrentdRenderPass != hPass )
                {
                    ret = VKE_OK; // render pass changed
                    m_hCurrentdRenderPass = hPass;
                    m_pCurrentRenderPass = pPass;
                }
            }
            else if( !m_isRenderPassBound )
            {
                
            }
#endif
            return ret;
        }


        void CCommandBuffer::SetEvent( const DDIEvent& hDDIEvent, const PIPELINE_STAGES& stages )
        {
            m_pBaseCtx->m_DDI.SetEvent( GetDDIObject(), hDDIEvent, stages );
        }

        void CCommandBuffer::ResetEvent( const DDIEvent& hDDIEvent, const PIPELINE_STAGES& stages )
        {
            m_pBaseCtx->m_DDI.Reset( GetDDIObject(), hDDIEvent, stages );
        }

        void CCommandBuffer::SetEvent( const EventHandle& hEvent, const PIPELINE_STAGES& stages )
        {
            auto hDDIEvent = m_pBaseCtx->m_pDeviceCtx->GetEvent( hEvent );
            SetEvent( hDDIEvent, stages );
        }

        void CCommandBuffer::ResetEvent( const EventHandle& hEvent, const PIPELINE_STAGES& stages )
        {
            auto hDDIEvent = m_pBaseCtx->m_pDeviceCtx->GetEvent( hEvent );
            ResetEvent( hDDIEvent, stages );
        }

    } // rendersystem
} // vke
#endif // VKE_VULKAN_RENDERER