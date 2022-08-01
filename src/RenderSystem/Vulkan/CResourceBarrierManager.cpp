#include "RenderSystem/Vulkan/CResourceBarrierManager.h"
#if VKE_VULKAN_RENDER_SYSTEM
#include "RenderSystem/CDeviceContext.h"
namespace VKE
{
    namespace RenderSystem
    {
        CResourceBarrierManager::CResourceBarrierManager()
        {

        }

        CResourceBarrierManager::~CResourceBarrierManager()
        {}


        void CResourceBarrierManager::AddBarrier(const SImageBarrierInfo& Info)
        {
            const VkImage vkImg = Info.vkImg;
            bool imageFound = false;
            for( uint32_t i = 0; i < m_vImageBarriers.GetCount(); ++i )
            {
                auto& CurrBarrier = m_vImageBarriers[ i ];
                if( CurrBarrier.vkImg == vkImg )
                {
                    CurrBarrier.vkNewLayout = Info.vkNewLayout;
                    imageFound = true;
                    break;
                }
            }
            if( !imageFound )
            {
                m_vImageBarriers.PushBack( Info );
            }
        }

        void CResourceBarrierManager::ExecuteBarriers(SBarrierData* pOut)
        {
            VkImageMemoryBarrier VkImgBarrier;
            Vulkan::InitInfo( &VkImgBarrier, VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER );
            VkImgBarrier.srcAccessMask = 0;
            VkImgBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
            VkImgBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            VkImgBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            VkImgBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            VkImgBarrier.subresourceRange.baseArrayLayer = 0;
            VkImgBarrier.subresourceRange.baseMipLevel = 0;
            VkImgBarrier.subresourceRange.layerCount = 1;
            VkImgBarrier.subresourceRange.levelCount = 1;

            for( uint32_t i = 0; i < m_vImageBarriers.GetCount(); ++i )
            {
                const auto& CurrInfo = m_vImageBarriers[ i ];
                VkImgBarrier.image = CurrInfo.vkImg;
                VkImgBarrier.oldLayout = CurrInfo.vkCurrentLayout;
                VkImgBarrier.newLayout = CurrInfo.vkNewLayout;
                pOut->vImageBarriers.PushBack( VkImgBarrier );
            }


            m_vMemoryBarriers.Clear();
            m_vBufferBarriers.Clear();
            m_vImageBarriers.Clear();
        }

    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDER_SYSTEM