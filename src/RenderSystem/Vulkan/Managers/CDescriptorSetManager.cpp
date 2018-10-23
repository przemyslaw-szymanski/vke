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

            for( uint32_t i = 0; i < m_vVkDescPools.GetCount(); ++i )
            {
                auto& vkPool = m_vVkDescPools[ i ];
                {
                    VkDevice.DestroyObject( nullptr, &vkPool );
                }
            }
            m_vVkDescPools.Clear();
        }

        Result CDescriptorSetManager::Create(const SDescriptorSetManagerDesc& Desc)
        {
            Result ret = VKE_OK;
            
            ret = m_DescSetMemMgr.Create( Config::RenderSystem::Pipeline::MAX_DESCRIPTOR_SET_COUNT, sizeof( CDescriptorSet ), 1 );
            if( VKE_SUCCEEDED( ret ) )
            {
                ret = m_DescSetLayoutMemMgr.Create(Config::RenderSystem::Pipeline::MAX_DESCRIPTOR_SET_LAYOUT_COUNT, sizeof(CDescriptorSetLayout), 1 );
                if( VKE_SUCCEEDED( ret ) )
                {
                    VkDescriptorPool vkPool;
                    VkDescriptorPoolSizeArray vVkSizes;
                    vVkSizes.Resize( DESCRIPTOR_TYPE_COUNT );
                    for( uint32_t i = 0; i < DESCRIPTOR_TYPE_COUNT; ++i )
                    {
                        DESCRIPTOR_SET_TYPE descType = static_cast<DESCRIPTOR_SET_TYPE>( i );
                        
                        VkDescriptorPoolSize& VkPoolSize = vVkSizes[ i ];
                        VkPoolSize.descriptorCount = max( Desc.aMaxDescriptorSetCounts[ i ], Config::RenderSystem::Pipeline::MAX_DESCRIPTOR_TYPE_COUNT );
                        VkPoolSize.type = Vulkan::Map::DescriptorType( descType );
                    }
                    if( VKE_FAILED( _CreatePool( &vkPool, Desc.maxCount, vVkSizes ) ) )
                    {
                        ret = VKE_FAIL;
                    }
                    // Create default layout
                    {
                        SDescriptorSetLayoutDesc LayoutDesc;
                        SDescriptorSetLayoutDesc::Binding Binding;
                        LayoutDesc.vBindings.PushBack(Binding);
                        CreateLayout(LayoutDesc);
                    }
                }
                else
                {
                    VKE_LOG_ERR( "Unable to create memory pool for CDescriptorSetLayout." );
                }
            }
            else
            {
                VKE_LOG_ERR( "Unable to create memory pool for CDescriptorSet." );
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
                ai.descriptorPool = m_vVkDescPools.Back();
                ai.descriptorSetCount = 1;
                ai.pSetLayouts = &Desc.pLayout->GetNative();
                
                VkResult res = m_pCtx->_GetDevice().AllocateObjects( ai, &vkSet );
                VK_ERR( res );
                if( res == VK_SUCCESS )
                {
                    DescSetBuffer::MapIterator Itr;
                    auto& Buffer = m_avDescSetBuffers[ Desc.type ].Back();
                    // Add always unique descriptor
                    if( !Buffer.Get( ++hash, &pSet, &Itr ) )
                    {
                        if( VKE_SUCCEEDED( Memory::CreateObject( &m_DescSetMemMgr, &pSet, this ) ) )
                        {
                            if( VKE_SUCCEEDED( pSet->Init( Desc, hash ) ) )
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

                            }
                        }
                        else
                        {
                            VKE_LOG_ERR( "Unable to create CDescriptorSet object. No memory." );
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

        template<typename T>
        void HashCombine( hash_t* pInOut, const T& v )
        {
            std::hash< T > h;
            *pInOut ^= h( v ) + 0x9e3779b9 + ( *pInOut << 6 ) + ( *pInOut >> 2 );
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

                        //hash ^= ( Binding.idx << 1 ) + ( Binding.count << 1 ) + ( Binding.type << 1 ) + ( Binding.stages << 1 );
                        Hash::Combine( &hash, Binding.idx );
                        Hash::Combine( &hash, Binding.count );
                        Hash::Combine( &hash, Binding.type );
                        Hash::Combine( &hash, Binding.stages );
                    }
                    ci.pBindings = &vVkBindings[ 0 ];
                  
                    DescSetLayoutBuffer::MapIterator Itr;
                    if( !m_DescSetLayoutBuffer.Get( hash, &pLayout, &Itr ) )
                    {
                        if( VKE_SUCCEEDED( Memory::CreateObject( &m_DescSetLayoutMemMgr, &pLayout, this ) ) )
                        {
                            if( m_DescSetLayoutBuffer.Add( pLayout, hash, Itr ) )
                            {
                                VkResult res = m_pCtx->_GetDevice().CreateObject( ci, nullptr, &vkLayout );
                                VK_ERR( res );
                                if( res != VK_SUCCESS )
                                {
                                    VKE_LOG_ERR( "Unable to create VkDescriptorSetLayout." );
                                    goto ERR;
                                }
                            }
                            else
                            {
                                VKE_LOG_ERR( "Unable to add resource CDescriptorSetLayout to the resource buffer." );                                
                                goto ERR;
                            }
                        }
                        else
                        {
                            VKE_LOG_ERR( "Unable to create CDescriptorSetLayout object. No memory." );
                            goto ERR;
                        }
                    }
                    if( pLayout )
                    {
                        if( vkLayout != VK_NULL_HANDLE )
                        {
                            if( VKE_SUCCEEDED( pLayout->Init( Desc ) ) )
                            {
                                pLayout->m_vkDescriptorSetLayout = vkLayout;
                            }
                            else
                            {
                                goto ERR;
                            }
                        }
                    }
                    
                }
                else
                {
                    VKE_LOG_ERR( "Unable to allocate memory for DescriptorSetLayoutBindings." );
ERR:
                    Memory::DestroyObject( &m_DescSetLayoutMemMgr, &pLayout );
                }
            }
            return DescriptorSetLayoutRefPtr( pLayout );
        }

        Result CDescriptorSetManager::_CreatePool(VkDescriptorPool* pVkOut, uint32_t maxCount, const VkDescriptorPoolSizeArray& vVkSizes)
        {
            Result ret = VKE_FAIL;
            auto& VkDevice = m_pCtx->_GetDevice();
            VkDescriptorPoolCreateInfo ci;
            ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            ci.pNext = nullptr;
            ci.flags = 0;
            ci.maxSets = maxCount;
            ci.poolSizeCount = vVkSizes.GetCount();
            ci.pPoolSizes = &vVkSizes[ 0 ];
            VkDescriptorPool vkPool;
            VkResult res = VkDevice.CreateObject( ci, nullptr, &vkPool );
            VK_ERR( res );
            if( res == VK_SUCCESS )
            {
                m_vVkDescPools.PushBack( vkPool );
                ret = VKE_OK;
            }
            else
            {
                VKE_LOG_ERR( "Unable to create VkDescriptorPool object." );
            }
            return ret;
        }

        DescriptorSetRefPtr CDescriptorSetManager::GetDescriptorSet( DescriptorSetHandle hSet )
        {
            CDescriptorSet::SHandle Handle;
            Handle.value = hSet.handle;
            CDescriptorSet* pSet = m_avDescSetBuffers[ Handle.type ].Back().Find( Handle.value );
            return DescriptorSetRefPtr( pSet );
        }

        DescriptorSetLayoutRefPtr CDescriptorSetManager::GetDescriptorSetLayout( DescriptorSetLayoutHandle hLayout )
        {
            CDescriptorSetLayout::SHandle Handle;
            Handle.value = hLayout.handle;
            CDescriptorSetLayout* pLayout = m_DescSetLayoutBuffer.Find( Handle.hash );
            return DescriptorSetLayoutRefPtr( pLayout );
        }

    } // RenderSystem
} // VKE

#endif // VKE_VULKAN_RENDERER