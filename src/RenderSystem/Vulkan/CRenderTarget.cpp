#include "RenderSystem/CRenderTarget.h"
#if VKE_VULKAN_RENDERER
#include "RenderSystem/CGraphicsContext.h"

namespace VKE
{
    namespace RenderSystem
    {
        CRenderTarget::CRenderTarget(CDeviceContext* pCtx) :
            m_pCtx(pCtx)
        {}

        CRenderTarget::~CRenderTarget()
        {
            //Destroy();
        }

        //void CRenderTarget::Destroy()
        //{
        //    if( m_vkFramebuffer == VK_NULL_HANDLE )
        //    {
        //        return;
        //    }

        //    //m_pCtx->_DestroyFramebuffer(&m_vkFramebuffer);
        //    //m_pCtx->_DestroyRenderPass(&m_vkRenderPass);
        //}

        //Result CRenderTarget::Create(const SRenderTargetDesc& Desc)
        //{
        //    if( m_vkFramebuffer != VK_NULL_HANDLE )
        //    {
        //        return VKE_OK;
        //    }

        //    m_Desc = Desc;
        //    auto& Device = m_pCtx->_GetDevice();

        //    {
        //        VkImageCreateInfo ci;
        //        Vulkan::InitInfo( &ci, VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO );
        //        ci.arrayLayers = 1;
        //        ci.flags = 0;
        //        ci.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
        //        ci.mipLevels = 1;
        //        ci.queueFamilyIndexCount = 1;
        //        ci.pQueueFamilyIndices = &m_pCtx->m_pQueue->familyIndex;
        //        ci.samples = VK_SAMPLE_COUNT_1_BIT;
        //        ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        //        ci.tiling = VK_IMAGE_TILING_OPTIMAL;
        //        ci.extent.depth = 1;
        //        //ci.usage = VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        //        for( uint32_t i = 0; i < m_Desc.vTextureDescs.GetCount(); ++i )
        //        {
        //            auto& TexDesc = m_Desc.vTextureDescs[ i ];
        //            ci.extent.width = TexDesc.Size.width;
        //            ci.extent.height = TexDesc.Size.height;
        //            ci.usage = VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
        //            ci.imageType = Vulkan::GetImageType(TexDesc.type);
        //            ci.usage = Vulkan::GetImageUsage(TexDesc.usage);
        //            
        //        }
        //    }

        //    return VKE_OK;
        //}
    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER