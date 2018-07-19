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
            const auto& ICD = m_pCtx->_GetICD().Device;
            const auto& VkDevice = m_pCtx->_GetDevice();

            for( uint32_t i = 0; i < DESCRIPTOR_TYPE_COUNT; ++i )
            {
                auto& Buffer = m_avVkDescPools[ i ];
                for( uint32_t j = 0; j < Buffer.GetCount(); ++j )
                {
                    VkDescriptorPool& vkPool = Buffer[ j ];
                    VkDevice.DestroyObject( nullptr, &vkPool );
                }
                Buffer.Destroy();
            }
        }

        Result CDescriptorSetManager::Create(const SDescriptorSetManagerDesc& Desc)
        {
            Result ret = VKE_OK;
            for( uint32_t i = 0; i < DESCRIPTOR_TYPE_COUNT; ++i )
            {
                DESCRIPTOR_SET_TYPE descType = static_cast< DESCRIPTOR_SET_TYPE >( i );
                VkDescriptorPool vkPool;
                uint32_t maxCount = Desc.aMaxDescriptorSetCounts[ i ];
                VkDescriptorPoolSize VkPoolSize;
                VkPoolSize.descriptorCount = DESCRIPTOR_SET_COUNT;
                VkPoolSize.type = Vulkan::Map::DescriptorType( descType );
                if( VKE_FAILED( _CreatePool(  &vkPool, maxCount, VkPoolSize, descType ) ) )
                {
                    ret = VKE_FAIL;
                    break;
                }
            }

            // Create default layout
            {
                SDescriptorSetLayoutDesc LayoutDesc;
                SDescriptorSetLayoutDesc::Binding Binding;
                LayoutDesc.vBindings.PushBack( Binding );
                CreateLayout( LayoutDesc );
            }

            if( ret == VKE_FAIL )
            {
                Destroy();
            }
            return ret;
        }

        DescriptorSetRefPtr CDescriptorSetManager::CreateSet(const SDescriptorSetDesc& Desc)
        {
            CDescriptorSet* pSet = nullptr;
            VkDescriptorSet vkSet = VK_NULL_HANDLE;
            VkDescriptorSetAllocateInfo ai;
            Vulkan::InitInfo( &ai, VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO );
            {
                static hash_t hash = 0;
                ai.descriptorPool = m_avVkDescPools[ Desc.type ].Back();
                ai.descriptorSetCount = 1;
                ai.pSetLayouts = &Desc.pLayout->GetNative();
                
                VkResult res = m_pCtx->_GetDevice().AllocateObjects( ai, &vkSet );
                VK_ERR( res );
                if( res == VK_SUCCESS )
                {
                    DescSetBuffer::MapIterator Itr;
                    auto& Buffer = m_avDescSetBuffers[ Desc.type ].Back();
                    // Add always unique descriptor
                    if( Buffer.Get( ++hash, &pSet, &Itr, &m_DescSetMemMgr, this ) )
                    {
                        if( !Buffer.Add( pSet, hash, Itr ) )
                        {
                            VKE_LOG_ERR( "Unable to add CDescriptorSet to the buffer." );
                            Memory::DestroyObject( &m_DescSetMemMgr, &pSet );
                            goto ERR;
                        }
                    }
                    else
                    {
ERR:
                        VKE_LOG_ERR( "Unable to create CDescriptorSet object." );
                        m_pCtx->_GetDevice().FreeObjects( ai.descriptorPool, 1, &vkSet );
                    }
                }
                else
                {
                    VKE_LOG_ERR( "Unable to allocate VkDescriptorSet object." );
                }
            }
            return DescriptorSetRefPtr( pSet );
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
                        if( m_DescSetLayoutBuffer.Get( hash, &pLayout, &Itr, &m_DescSetLayoutMemMgr, this ) )
                        {
                            if( !m_DescSetLayoutBuffer.Add( pLayout, hash, Itr ) )
                            {
                                VKE_LOG_ERR( "Unable to add resource CDescriptorSetLayout to the resource buffer." );
                                Memory::DestroyObject( &m_DescSetLayoutMemMgr, &pLayout );
                                goto ERR;
                            }
                        }
                        if( pLayout )
                        {
                            if( VKE_SUCCEEDED( pLayout->Init( Desc ) ) )
                            {

                            }
                            else
                            {
                                VKE_LOG_ERR( "Unable to initialize CDescriptorSetLayout." );
                                goto ERR;
                            }
                        }
                    }
                    else
                    {
                        VKE_LOG_ERR("Unable to create VkDescriptorSetLayout.");
                        goto ERR;
                    }
                }
                else
                {
                    VKE_LOG_ERR( "Unable to allocate memory for DescriptorSetLayoutBindings." );
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