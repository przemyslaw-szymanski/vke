#include "RenderSystem/Vulkan/CCommandBuffer.h"
#include "RenderSystem/Resources/CShader.h"
#include "RenderSystem/CGraphicsContext.h"
#if VKE_VULKAN_RENDERER
#include "RenderSystem/Vulkan/Managers/CPipelineManager.h"

namespace VKE
{
    namespace RenderSystem
    {
        CCommandBuffer::CCommandBuffer(CGraphicsContext* pCtx) :
            m_pCtx( pCtx ),
            m_ICD( pCtx->_GetICD() )
        {
        }

        CCommandBuffer::~CCommandBuffer()
        {
        }

        void CCommandBuffer::Init(const VkCommandBuffer& vkCb)
        {
            m_PipelineDesc.Create.async = false;
            m_vkCommandBuffer = vkCb;
        }

        void CCommandBuffer::Begin()
        {
            assert( m_state == States::UNKNOWN || m_state == States::FLUSH );
            VkCommandBufferBeginInfo VkBeginInfo;
            Vulkan::InitInfo( &VkBeginInfo, VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO );
            VkBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            VkBeginInfo.pInheritanceInfo = nullptr;
            VK_ERR( m_ICD.Device.vkResetCommandBuffer( m_vkCommandBuffer, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT ) );
            VK_ERR( m_ICD.Device.vkBeginCommandBuffer( m_vkCommandBuffer, &VkBeginInfo ) );
            m_state = States::BEGIN;
        }

        void CCommandBuffer::End()
        {
            assert( m_state == States::BEGIN );
            m_ICD.Device.vkEndCommandBuffer( m_vkCommandBuffer );
            m_state = States::END;
        }

        void CCommandBuffer::Barrier(const CResourceBarrierManager::SImageBarrierInfo& Barrier)
        {
            assert( m_state == States::BEGIN );
            m_BarrierMgr.AddBarrier( Barrier );
        }

        void CCommandBuffer::ExecuteBarriers()
        {
            CResourceBarrierManager::SBarrierData Data;
            m_BarrierMgr.ExecuteBarriers( &Data );
            
            const VkMemoryBarrier* pMemBarriers = ( !Data.vMemoryBarriers.IsEmpty() ) ? &Data.vMemoryBarriers[ 0 ] : nullptr;
            const VkBufferMemoryBarrier* pBuffBarriers = ( !Data.vBufferBarriers.IsEmpty() ) ? &Data.vBufferBarriers[ 0 ] : nullptr;
            const VkImageMemoryBarrier* pImgBarriers = ( !Data.vImageBarriers.IsEmpty() ) ? &Data.vImageBarriers[ 0 ] : nullptr;
        
            const VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            const VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

            m_ICD.Device.vkCmdPipelineBarrier( m_vkCommandBuffer, srcStage, dstStage, 0,
                Data.vMemoryBarriers.GetCount(), pMemBarriers,
                Data.vBufferBarriers.GetCount(), pBuffBarriers,
                Data.vImageBarriers.GetCount(), pImgBarriers );
        }

        void CCommandBuffer::Flush()
        {
            assert( m_state == States::END );
            m_state = States::FLUSH;
        }

        void CCommandBuffer::SetShader(ShaderPtr pShader)
        {
            switch( pShader->GetDesc().type )
            {
                case ShaderTypes::COMPUTE:
                    m_PipelineDesc.Pipeline.Shaders.pComputeShader = pShader;
                break;
                case ShaderTypes::GEOMETRY:
                    m_PipelineDesc.Pipeline.Shaders.pGeometryShader = pShader;
                break;
                case ShaderTypes::PIXEL:
                    m_PipelineDesc.Pipeline.Shaders.pPpixelShader = pShader;
                break;
                case ShaderTypes::TESS_DOMAIN:
                    m_PipelineDesc.Pipeline.Shaders.pTessDomainShader = pShader;
                break;
                case ShaderTypes::TESS_HULL:
                    m_PipelineDesc.Pipeline.Shaders.pTessHullShader = pShader;
                break;
                case ShaderTypes::VERTEX:
                    m_PipelineDesc.Pipeline.Shaders.pVertexShader = pShader;
                break;
            }
            m_needNewPipeline = true;
        }

        void CCommandBuffer::SetDepthStencil(const SPipelineDesc::SDepthStencil& DepthStencil)
        {
            m_PipelineDesc.Pipeline.DepthStencil = DepthStencil;
            m_needNewPipeline = true;
        }

        void CCommandBuffer::SetRasterization(const SPipelineDesc::SRasterization& Rasterization)
        {
            m_PipelineDesc.Pipeline.Rasterization = Rasterization;
            m_needNewPipeline = true;
        }

        Result CCommandBuffer::_DrawProlog()
        {
            Result res = VKE_FAIL;
            //if( m_needNewPipeline )
            {
                PipelinePtr pPipeline = m_pCtx->CreatePipeline( m_PipelineDesc );
                /// TODO: perf log, count pipeline creation at drawtime
                if( pPipeline.IsValid() )
                {
                    m_pCtx->SetPipeline( CommandBufferPtr( this ), pPipeline );
                    m_needNewPipeline = false;
                    res = VKE_OK;
                }
            }
            return res;
        }

        void CCommandBuffer::Draw(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex,
            uint32_t vertexOffset, uint32_t firstInstance)
        {
            if( m_needNewPipeline )
            {
                _DrawProlog();
            }
            m_pCtx->_GetICD().Device.vkCmdDrawIndexed( m_vkCommandBuffer, indexCount, instanceCount, firstIndex,
                vertexOffset, firstInstance );
        }

        void CCommandBuffer::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex,
            uint32_t firstInstance)
        {
            if( m_needNewPipeline )
            {
                _DrawProlog();
            }
            m_pCtx->_GetICD().Device.vkCmdDraw( m_vkCommandBuffer, vertexCount, instanceCount, firstVertex,
                firstInstance );
        }
        
    } // rendersystem
} // vke
#endif // VKE_VULKAN_RENDERER