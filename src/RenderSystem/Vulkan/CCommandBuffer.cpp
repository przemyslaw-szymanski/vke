#include "RenderSystem/Vulkan/CCommandBuffer.h"
#include "RenderSystem/Resources/CShader.h"
#include "RenderSystem/CDeviceContext.h"
#if VKE_VULKAN_RENDERER
#include "RenderSystem/Vulkan/Managers/CPipelineManager.h"
#include "RenderSystem/Vulkan/Managers/CSubmitManager.h"
#include "RenderSystem/CRenderPass.h"
#include "RenderSystem/CSwapChain.h"
#include "Core/Utils/CProfiler.h"

namespace VKE
{
    namespace RenderSystem
    {
        static CPipeline g_sDummyPipeline = CPipeline(nullptr);

        CCommandBuffer::CCommandBuffer() :
            m_pCurrentPipeline( PipelinePtr( &g_sDummyPipeline ) )
        {
        }

        CCommandBuffer::~CCommandBuffer()
        {
        }

        void CCommandBuffer::Init(const SCommandBufferInitInfo& Info)
        {
            m_pCtx = Info.pCtx;
            m_pDDI = &m_pCtx->DDI();
            m_pBatch = Info.pBatch;
            //m_pICD = &m_pCtx->_GetICD();
            m_CurrentPipelineDesc.Create.async = false;
            this->m_hDDIObject = Info.hDDIObject;
            m_CurrentPipelineDesc.Pipeline = SPipelineDesc( DEFAULT_CONSTRUCTOR_INIT );
            m_CurrentPipelineDesc.Pipeline.Shaders.apShaders[ ShaderTypes::VERTEX ] = m_pCtx->GetDefaultShader( ShaderTypes::VERTEX );
            m_CurrentPipelineDesc.Pipeline.Shaders.apShaders[ ShaderTypes::PIXEL ] = m_pCtx->GetDefaultShader( ShaderTypes::PIXEL );
            m_CurrentPipelineDesc.Pipeline.hLayout = PipelineLayoutHandle{ m_pCtx->GetDefaultPipelineLayout()->GetHandle() };
        }

        void CCommandBuffer::Begin()
        {
            assert( m_state == States::UNKNOWN || m_state == States::FLUSH || m_state == States::END );

            //if( m_pCurrentRenderPass.IsNull() )
            {
                //m_pCurrentRenderPass = m_pCtx
            }
            if( m_pCurrentPipelineLayout.IsNull() )
            {
                {
                    SPipelineLayoutDesc Desc;
                    PipelineLayoutPtr pLayout = m_pCtx->CreatePipelineLayout( Desc );
                    VKE_ASSERT( pLayout.IsValid(), "Invalid pipeline layout." );
                    m_pCurrentPipelineLayout = pLayout;
                }
            }
            if( m_pCurrentPipeline.IsNull() )
            {
                Bind( m_pCurrentPipelineLayout );
            }
            
           /* VkCommandBufferBeginInfo VkBeginInfo;
            Vulkan::InitInfo( &VkBeginInfo, VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO );
            VkBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            VkBeginInfo.pInheritanceInfo = nullptr;*/
            //VK_ERR( m_pICD->Device.vkResetCommandBuffer( m_vkCommandBuffer, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT ) );
            //VK_ERR( m_pICD->Device.vkBeginCommandBuffer( m_vkCommandBuffer, &VkBeginInfo ) );
            m_pCtx->DDI().Reset( this->GetDDIObject() );
            m_pCtx->_GetDDI().BeginCommandBuffer( this->GetDDIObject() );
            m_state = States::BEGIN;
        }

        void CCommandBuffer::End(COMMAND_BUFFER_END_FLAG flag)
        {
            assert( m_state == States::BEGIN );
            if( m_needUnbindRenderPass )
            {
                Bind( RenderPassPtr() );
            }
            if( m_needExecuteBarriers )
            {
                ExecuteBarriers();
            }
            
            m_pCtx->_GetDDI().EndCommandBuffer( this->GetDDIObject() );
            
            if( flag == CommandBufferEndFlags::END )
            {
                m_state = States::END;
                m_pBatch->_Submit( CommandBufferPtr{ this } );
            }
            /*else if( flag == CommandBufferEndFlags::EXECUTE )
            {
                m_pBatch->_Flush( 0 );
                m_state = States::FLUSH;
            }
            else if( flag == CommandBufferEndFlags::EXECUTE_AND_WAIT )
            {
                m_pBatch->_Flush( UINT64_MAX );
                m_state = States::FLUSH;
            }*/

            
            _Reset();
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
            //CResourceBarrierManager::SBarrierData Data;
            //m_BarrierMgr.ExecuteBarriers( &Data );
            
            /*const VkMemoryBarrier* pMemBarriers = ( !Data.vMemoryBarriers.IsEmpty() ) ? &Data.vMemoryBarriers[ 0 ] : nullptr;
            const VkBufferMemoryBarrier* pBuffBarriers = ( !Data.vBufferBarriers.IsEmpty() ) ? &Data.vBufferBarriers[ 0 ] : nullptr;
            const VkImageMemoryBarrier* pImgBarriers = ( !Data.vImageBarriers.IsEmpty() ) ? &Data.vImageBarriers[ 0 ] : nullptr;
        
            const VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            const VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;*/

            /*m_pICD->Device.vkCmdPipelineBarrier( this->m_hDDIObject, srcStage, dstStage, 0,
                Data.vMemoryBarriers.GetCount(), pMemBarriers,
                Data.vBufferBarriers.GetCount(), pBuffBarriers,
                Data.vImageBarriers.GetCount(), pImgBarriers );*/
            m_pCtx->_GetDDI().Barrier( this->GetDDIObject(), m_BarrierInfo );
            m_BarrierInfo.vBufferBarriers.Clear();
            m_BarrierInfo.vMemoryBarriers.Clear();
            m_BarrierInfo.vTextureBarriers.Clear();
            m_needExecuteBarriers = false;
        }

        /*Result CCommandBuffer::Flush( const uint64_t& timeout )
        {
            VKE_ASSERT( m_pBatch != nullptr, "" );
            return m_pBatch->_Flush( timeout );
        }*/

        void CCommandBuffer::_Reset()
        {
            m_pBatch = nullptr; // Clear batch as this command buffer is no longer valid for
            m_pCurrentPipeline = nullptr;
            m_pCurrentRenderPass = nullptr;
            m_isPipelineBound = false;
            m_needExecuteBarriers = false;
            m_needNewPipeline = true;
            m_needNewPipelineLayout = true;
            m_needUnbindRenderPass = false;
            m_CurrentPipelineDesc.Pipeline.hRenderPass = NULL_HANDLE;
            m_CurrentPipelineDesc.Pipeline.hDDIRenderPass = DDI_NULL_HANDLE;
        }

        void CCommandBuffer::Bind( PipelineLayoutPtr pLayout )
        {
            m_pCurrentPipelineLayout = pLayout;
            m_CurrentPipelineDesc.Pipeline.hLayout = PipelineLayoutHandle{ m_pCurrentPipelineLayout->GetHandle() };
            //m_CurrentPipelineDesc.Pipeline.hRenderPass.handle = reinterpret_cast< handle_t >( m_pCurrentRenderPass->GetDDIObject() );
            VKE_ASSERT( m_CurrentPipelineDesc.Pipeline.hLayout != NULL_HANDLE, "Invalid pipeline object." );
            m_needNewPipeline = true;
        }

        void CCommandBuffer::Bind( RenderPassPtr pRenderPass )
        {
            SBindRenderPassInfo Info;
            Info.hDDICommandBuffer = GetDDIObject();

            if( pRenderPass.IsValid() )
            {
                if( m_needExecuteBarriers )
                {
                    ExecuteBarriers();
                }
                // Close current render pass
                if( m_pCurrentRenderPass != nullptr && !m_pCurrentRenderPass->IsActive() )
                {
                    Info.pBeginInfo = nullptr;
                    m_pCtx->DDI().Bind( Info );
                    m_pCurrentRenderPass->_IsActive( false );
                }

                m_pCurrentRenderPass = pRenderPass;
                const auto hPass = RenderPassHandle{ m_pCurrentRenderPass->GetHandle() };
                m_CurrentPipelineDesc.Pipeline.hRenderPass = hPass;
                m_CurrentPipelineDesc.Pipeline.hDDIRenderPass = DDI_NULL_HANDLE;
                m_needNewPipeline = m_CurrentPipelineDesc.Pipeline.hRenderPass != hPass;
                
                m_pCurrentRenderPass->_IsActive( true );
                
                Info.pBeginInfo = &pRenderPass->GetBeginInfo();
                m_pCtx->DDI().Bind( Info );
            }
            else
            {
                m_pCtx->DDI().Unbind( GetDDIObject(), (DDIRenderPass)(DDI_NULL_HANDLE) );
                m_needUnbindRenderPass = false;
            }
        }

        void CCommandBuffer::Bind( PipelinePtr pPipeline )
        {
            VKE_ASSERT( pPipeline.IsValid(), "Pipeline must be valid." );
            SBindPipelineInfo Info;
            Info.pCmdBuffer = this;
            Info.pPipeline = pPipeline.Get();
            m_pCurrentPipeline = pPipeline;
            m_isPipelineBound = true;
            m_pCtx->DDI().Bind( Info );
        }

        void CCommandBuffer::Bind(ShaderPtr pShader)
        {
            const auto type = pShader->GetDesc().type;
            VKE_ASSERT( type != ShaderTypes::_MAX_COUNT, "" );
            const ShaderHandle hShader( pShader->GetHandle() );
            {
                //m_CurrentPipelineDesc.Pipeline.Shaders.apShaders[ type ] = hShader;
                m_CurrentPipelineDesc.Pipeline.Shaders.apShaders[ type ] = pShader;
                m_needNewPipeline = true;
            }
        }

        void CCommandBuffer::Bind( VertexBufferPtr pBuffer )
        {
            SBindVertexBufferInfo Info;
            Info.pCmdBuffer = this;
            Info.pBuffer = pBuffer.Get();
            Info.offset = 0;
            m_pCtx->DDI().Bind( Info );
        }

        void CCommandBuffer::Bind( CSwapChain* pSwapChain )
        {
            Bind( pSwapChain->m_DDISwapChain );
        }

        void CCommandBuffer::Bind( const SDDISwapChain& SwapChain )
        {
            SBindRenderPassInfo Info;
            SBeginRenderPassInfo BeginInfo;
            const auto idx = GetBackBufferIndex();
            BeginInfo.hDDIFramebuffer = SwapChain.vFramebuffers[idx];
            BeginInfo.hDDIRenderPass = SwapChain.hRenderPass;
            BeginInfo.RenderArea.Size = SwapChain.Size;
            BeginInfo.RenderArea.Offset = { 0,0 };
            BeginInfo.vDDIClearValues.PushBack( {0,0,1,1} );

            Info.hDDICommandBuffer = GetDDIObject();
            Info.pBeginInfo = &BeginInfo;

            m_pCtx->DDI().Bind( Info );

            m_needNewPipeline = m_CurrentPipelineDesc.Pipeline.hDDIRenderPass != SwapChain.hRenderPass;
            m_CurrentPipelineDesc.Pipeline.hRenderPass = NULL_HANDLE;
            m_CurrentPipelineDesc.Pipeline.hDDIRenderPass = SwapChain.hRenderPass;

            m_needUnbindRenderPass = true;
        }

        void CCommandBuffer::SetState(const SPipelineDesc::SDepthStencil& DepthStencil)
        {
            m_CurrentPipelineDesc.Pipeline.DepthStencil = DepthStencil;
            m_needNewPipeline = true;
        }

        void CCommandBuffer::SetState(const SPipelineDesc::SRasterization& Rasterization)
        {
            m_CurrentPipelineDesc.Pipeline.Rasterization = Rasterization;
            m_needNewPipeline = true;
        }

        void CCommandBuffer::SetState( const SPipelineDesc::SInputLayout& InputLayout )
        {
            m_CurrentPipelineDesc.Pipeline.InputLayout = InputLayout;
            m_needNewPipeline = true;
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
            return 0;
        }

        void CCommandBuffer::SetState( const SVertexInputLayoutDesc& VertexInputLayout )
        {
            uint32_t currOffset = 0;
            uint32_t currBinding = 0;
            uint32_t currLocation = 0;
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
                VA.binding = currBinding;
                VA.offset = currOffset;
                VA.location = currLocation;
                VA.format = static_cast<FORMAT>( Curr.type );
                VA.pName = Curr.pName;
                VA.stride = vertexSize;

                m_CurrentPipelineDesc.Pipeline.InputLayout.vVertexAttributes.PushBack( VA );
            }

            m_needNewPipeline = true;
        }

        Result CCommandBuffer::_DrawProlog()
        {
            Result ret = VKE_FAIL;
            if( m_needNewPipeline )
            {
                m_pCurrentPipeline = m_pCtx->CreatePipeline( m_CurrentPipelineDesc );
                m_needNewPipeline = false;
                m_isPipelineBound = false;
            }
            VKE_ASSERT( m_pCurrentPipeline.IsValid(), "Pipeline was not created successfully." );
            if( !m_isPipelineBound )
            {
                Bind( m_pCurrentPipeline );
            }
            ret = VKE_OK;
            return ret;
        }

        void CCommandBuffer::DrawIndexed( const uint32_t& indexCount, const uint32_t& instanceCount, const uint32_t& firstIndex,
            const uint32_t& vertexOffset, const uint32_t& firstInstance)
        {
            //if( m_needNewPipeline )
            {
                _DrawProlog();
            }
            m_pCtx->_GetDDI().GetICD().vkCmdDrawIndexed( this->m_hDDIObject, indexCount, instanceCount, firstIndex,
                vertexOffset, firstInstance );
        }

        void CCommandBuffer::Draw( const uint32_t& vertexCount, const uint32_t& instanceCount, const uint32_t& firstVertex,
            const uint32_t& firstInstance)
        {
            //if( m_needNewPipeline )
            {
                _DrawProlog();
            }
            VKE_ASSERT( m_isPipelineBound, "Pipeline must be set." );
            //VKE_SIMPLE_PROFILE();
            m_pDDI->Draw( GetDDIObject(), vertexCount, instanceCount, firstVertex, firstInstance );
        }
        
    } // rendersystem
} // vke
#endif // VKE_VULKAN_RENDERER