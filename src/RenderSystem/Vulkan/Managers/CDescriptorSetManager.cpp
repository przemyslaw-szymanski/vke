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
            using VkBindingArray = Utils::TCDynamicArray< VkDescriptorSetLayoutBinding, Config::RenderSystem::Pipeline::MAX_DESCRIPTOR_BINDING_COUNT >;
            CDescriptorSetLayout* pLayout = nullptr;
            VkDescriptorSetLayout vkLayout = VK_NULL_HANDLE;
            VkDescriptorSetLayoutCreateInfo ci;
            Vulkan::InitInfo( &ci, VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO );
            {
                ci.bindingCount = Desc.vBindings.GetCount();
                VkBindingArray vVkBindings;
                hash_t hash = ( ci.bindingCount << 1 );
                if( vVkBindings.Resize( ci.bindingCount ) )
                {
                    for( uint32_t i = 0; i < ci.bindingCount; ++i )
                    {
                        auto& VkBinding = vVkBindings[ i ];
                        const auto& Binding = Desc.vBindings[ i ];
                        VkBinding.binding = Binding.idx;
                        VkBinding.descriptorCount = Binding.count;
                        VkBinding.descriptorType = Vulkan::Map::DescriptorType( Binding.type );
                        VkBinding.pImmutableSamplers = nullptr;
                        VkBinding.stageFlags = Vulkan::Convert::PipelineStages( Binding.stages );

                        hash ^= ( Binding.idx << 1 ) ^ ( Binding.count << 1 ) ^ ( Binding.type << 1 ) ^ ( Binding.stages << 1 );
                    }
                    ci.pBindings = &vVkBindings[ 0 ];

                    
                    VkResult res = m_pCtx->_GetDevice().CreateObject( ci, nullptr, &vkLayout );
                    VK_ERR( res );
                    if( res == VK_SUCCESS )
                    {
                        DescSetLayoutBuffer::MapIterator Itr;
                        if( m_DescSetLayoutBuffer.Get( hash, &pLayout, &Itr ) )
                        {
                            if( VKE_SUCCEEDED( pLayout->Init( Desc ) ) )
                            {

                            }
                            else
                            {
                                VKE_LOG_ERR( "Unable to initialize CDescriptorSetLayout" );
                                goto ERR;
                            }
                        }
                        else
                        {
                            VKE_LOG_ERR( "Unable to create memory for CDescriptorSetLayout" );
                            goto ERR;
                        }
                    }
                    else
                    {
                        VKE_LOG_ERR("Unable to create VkDescriptorSetLayout");
                        goto ERR;
                    }
                }
                else
                {
                    VKE_LOG_ERR( "Unable to allocate memory for DescriptorSetLayoutBindings" );
ERR:
                    m_pCtx->_GetDevice().DestroyObject( nullptr, &vkLayout );
                }
            }
            return DescriptorSetLayoutRefPtr( pLayout );
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
                //VkResult res = ICD.vkCreateDescriptorPool( m_pCtx->_GetDevice().GetHandle(), &ci, nullptr, &vkPool );
                VkResult res = m_pCtx->_GetDevice().CreateObject( ci, nullptr, &vkPool );
                VK_ERR( res );
                if( res == VK_SUCCESS )
                {
                    uint32_t pos = m_avVkDescPools[ descType ].PushBack( vkPool );
                    if (pos != Utils::INVALID_POSITION)
                    {
                        ret = VKE_OK;
                    }
                    else
                    {
                        m_pCtx->_GetDevice().DestroyObject( nullptr, &vkPool );
                        goto ERR;
                    }
                }
                else
                {
ERR:
                    VKE_LOG_ERR( "Unable to create VkDescriptorPool with maxCount: " << maxCount << " of types: " << descType );
                }
            }
            return ret;
        }
    } // RenderSystem
} // VKE

#endif // VKE_VULKAN_RENDERER