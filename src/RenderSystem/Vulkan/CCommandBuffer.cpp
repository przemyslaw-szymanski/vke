#include "RenderSystem/Vulkan/CCommandBuffer.h"
#include "RenderSystem/Resources/CShader.h"
#include "RenderSystem/CDeviceContext.h"
#if VKE_VULKAN_RENDERER
#include "RenderSystem/Vulkan/Managers/CPipelineManager.h"
#include "RenderSystem/Vulkan/Managers/CSubmitManager.h"
#include "RenderSystem/CRenderPass.h"

namespace VKE
{
    namespace RenderSystem
    {
        CCommandBuffer::CCommandBuffer()
        {
        }

        CCommandBuffer::~CCommandBuffer()
        {
        }

        void CCommandBuffer::Init(const SCommandBufferInitInfo& Info)
        {
            m_pCtx = Info.pCtx;
            m_pBatch = Info.pBatch;
            //m_pICD = &m_pCtx->_GetICD();
            m_CurrentPipelineDesc.Create.async = false;
            this->m_hDDIObject = Info.hDDIObject;
            m_CurrentPipelineDesc.Pipeline = SPipelineDesc( DEFAULT_CONSTRUCTOR_INIT );
        }

        void CCommandBuffer::Begin()
        {
            assert( m_state == States::UNKNOWN || m_state == States::FLUSH );

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
                Set( m_pCurrentPipelineLayout );
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

        void CCommandBuffer::End()
        {
            assert( m_state == States::BEGIN );
            //m_pICD->Device.vkEndCommandBuffer( this->m_hDDIObject );
            m_pCtx->_GetDDI().EndCommandBuffer( this->GetDDIObject() );
            m_state = States::END;
        }

        void CCommandBuffer::Barrier(const CResourceBarrierManager::SImageBarrierInfo& Barrier)
        {
            assert( m_state == States::BEGIN );
            m_BarrierMgr.AddBarrier( Barrier );
        }

        void CCommandBuffer::Barrier( const SMemoryBarrierInfo& Info )
        {
            m_BarrierInfo.vMemoryBarriers.PushBack( Info );
        }

        void CCommandBuffer::Barrier( const SBufferBarrierInfo& Info )
        {
            m_BarrierInfo.vBufferBarriers.PushBack( Info );
        }

        void CCommandBuffer::Barrier( const STextureBarrierInfo& Info )
        {
            m_BarrierInfo.vTextureBarriers.PushBack( Info );
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
        }

        void CCommandBuffer::Flush()
        {
            VKE_ASSERT( m_state == States::END, "CommandBuffer must be Ended in order to submit." );
            m_state = States::FLUSH;
            VKE_ASSERT( m_pBatch != nullptr, "CommandBufferBatch must be set in order to submit." );
            m_pBatch->_Submit( CommandBufferPtr{ this } );
            m_pBatch = nullptr; // Clear batch as this command buffer is no longer valid for 
        }

        void CCommandBuffer::Set( PipelineLayoutPtr pLayout )
        {
            m_pCurrentPipelineLayout = pLayout;
            m_CurrentPipelineDesc.Pipeline.hLayout = PipelineLayoutHandle{ m_pCurrentPipelineLayout->GetHandle() };
            //m_CurrentPipelineDesc.Pipeline.hRenderPass.handle = reinterpret_cast< handle_t >( m_pCurrentRenderPass->GetDDIObject() );
            VKE_ASSERT( m_CurrentPipelineDesc.Pipeline.hLayout != NULL_HANDLE, "Invalid pipeline object." );
            m_needNewPipeline = true;
        }

        void CCommandBuffer::Set( RenderPassPtr pRenderPass )
        {
            m_pCurrentRenderPass = pRenderPass;
            m_CurrentPipelineDesc.Pipeline.hRenderPass = RenderPassHandle{ m_pCurrentRenderPass->GetHandle() };
            m_needNewPipeline = true;
        }

        void CCommandBuffer::Set( PipelinePtr pPipeline )
        {
            SBindPipelineInfo Info;
            Info.pCmdBuffer = this;
            Info.pPipeline = pPipeline.Get();
            m_pCurrentPipeline = pPipeline;
            m_pCtx->DDI().Bind( Info );
        }

        void CCommandBuffer::Set(ShaderPtr pShader)
        {
            const auto type = pShader->GetDesc().type;
            const ShaderHandle hShader( pShader->GetHandle() );
            {
                //m_CurrentPipelineDesc.Pipeline.Shaders.apShaders[ type ] = hShader;
                m_CurrentPipelineDesc.Pipeline.Shaders.apShaders[ type ] = pShader;
                m_needNewPipeline = true;
            }
        }

        void CCommandBuffer::Set( VertexBufferPtr pBuffer )
        {
            SBindVertexBufferInfo Info;
            Info.pCmdBuffer = this;
            Info.pBuffer = pBuffer.Get();
            Info.offset = 0;
            m_pCtx->DDI().Bind( Info );
        }

        void CCommandBuffer::Set(const SPipelineDesc::SDepthStencil& DepthStencil)
        {
            m_CurrentPipelineDesc.Pipeline.DepthStencil = DepthStencil;
            m_needNewPipeline = true;
        }

        void CCommandBuffer::Set(const SPipelineDesc::SRasterization& Rasterization)
        {
            m_CurrentPipelineDesc.Pipeline.Rasterization = Rasterization;
            m_needNewPipeline = true;
        }

        Result CCommandBuffer::_DrawProlog()
        {
            Result res = VKE_FAIL;
            {
                m_pCurrentPipeline = m_pCtx->CreatePipeline( m_CurrentPipelineDesc );
                m_needNewPipeline = false;
                VKE_ASSERT( m_pCurrentPipeline.IsValid(), "Pipeline was not created successfully." );
                Set( m_pCurrentPipeline );
            }
            return res;
        }

        void CCommandBuffer::DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex,
            uint32_t vertexOffset, uint32_t firstInstance)
        {
            if( m_needNewPipeline )
            {
                _DrawProlog();
            }
            m_pCtx->_GetDDI().GetICD().vkCmdDrawIndexed( this->m_hDDIObject, indexCount, instanceCount, firstIndex,
                vertexOffset, firstInstance );
        }

        void CCommandBuffer::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex,
            uint32_t firstInstance)
        {
            if( m_needNewPipeline )
            {
                _DrawProlog();
            }
            /*m_pCtx->_GetICD().Device.vkCmdDraw( this->m_hDDIObject, vertexCount, instanceCount, firstVertex,
                firstInstance );*/
            m_pCtx->_GetDDI().GetICD().vkCmdDraw( this->m_hDDIObject, vertexCount, instanceCount, firstVertex,
                firstInstance );
        }
        
    } // rendersystem
} // vke
#endif // VKE_VULKAN_RENDERER