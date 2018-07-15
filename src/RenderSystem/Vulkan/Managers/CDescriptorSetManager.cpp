#if VKE_VULKAN_RENDERER
#include "RenderSystem/Vulkan/Managers/CDescriptorSetManager.h"
#include "Rendersystem/CDeviceContext.h"

namespace VKE
{
    namespace RenderSystem
    {
        CDescriptorSetManager::CDescriptorSetManager(CDeviceContext* pCtx) :
            m_pCtx( pCtx )
        {}

        CDescriptorSetManager::~CDescriptorSetManager()
        {
            Destroy();
        }

        void CDescriptorSetManager::Destroy()
        {

        }

        Result CDescriptorSetManager::Create(const SDescriptorSetManagerDesc& Desc)
        {
            Result ret = VKE_FAIL;
            
            return ret;
        }

        DescriptorSetRefPtr CDescriptorSetManager::CreateSet(const SDescriptorSetDesc& Desc)
        {

        }

        DescriptorSetLayoutRefPtr CDescriptorSetManager::CreateLayout(const SDescriptorSetLayoutDesc& Desc)
        {

        }

        Result CDescriptorSetManager::_CreatePool(VkDescriptorPool* pVkOut, uint32_t maxCount,
                                                  const VkDescriptorPoolSize& VkPoolSize, DESCRIPTOR_SET_TYPE descType)
        {
            Result ret = VKE_FAIL;
            auto& ICD = m_pCtx->_GetICD().Device;
            VkDescriptorPoolCreateInfo ci;
            Vulkan::InitInfo( &ci, VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO );
            {
                ci.maxSets = maxCount;
                ci.poolSizeCount = 1;
                ci.pPoolSizes = &VkPoolSize;
                VkDescriptorPool vkPool;
                VkResult res = ICD.vkCreateDescriptorPool( m_pCtx->_GetDevice().GetHandle(), &ci, nullptr, &vkPool );
                VK_ERR( res );
                if( res == VK_SUCCESS )
                {
                    uint32_t pos = m_avVkDescPools[ descType ].PushBack( vkPool );
                }
                else
                {
                    VKE_LOG_ERR( "Unable to create VkDescriptorPool with maxCount: " << maxCount << " of types: " << descType );
                }
            }
            return ret;
        }
    } // RenderSystem
} // VKE

#endif // VKE_VULKAN_RENDERER