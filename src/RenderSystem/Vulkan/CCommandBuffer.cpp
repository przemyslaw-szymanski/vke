#include "RenderSystem/Vulkan/CCommandBuffer.h"
#if VKE_VULKAN_RENDERER
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

        void CCommandBuffer::Init(const VkICD::Device* pICD, const VkCommandBuffer& vkCb)
        {
            m_pICD = pICD;
            m_vkCommandBuffer = vkCb;
        }

        void CCommandBuffer::Begin()
        {
            assert( m_state == States::UNKNOWN || m_state == States::FLUSH );
            VkCommandBufferBeginInfo VkBeginInfo;
            Vulkan::InitInfo( &VkBeginInfo, VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO );
            VkBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            VkBeginInfo.pInheritanceInfo = nullptr;
            VK_ERR( m_pICD->vkResetCommandBuffer( m_vkCommandBuffer, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT ) );
            VK_ERR( m_pICD->vkBeginCommandBuffer( m_vkCommandBuffer, &VkBeginInfo ) );
            m_state = States::BEGIN;
        }

        void CCommandBuffer::End()
        {
            assert( m_state == States::BEGIN );
            m_pICD->vkEndCommandBuffer( m_vkCommandBuffer );
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

            m_pICD->vkCmdPipelineBarrier( m_vkCommandBuffer, srcStage, dstStage, 0,
                                                 Data.vMemoryBarriers.GetCount(), pMemBarriers,
                                                 Data.vBufferBarriers.GetCount(), pBuffBarriers,
                                                 Data.vImageBarriers.GetCount(), pImgBarriers );
        }

        void CCommandBuffer::Flush()
        {
            assert( m_state == States::END );
            m_state = States::FLUSH;
        }
        
    } // rendersystem
} // vke
#endif // VKE_VULKAN_RENDERER