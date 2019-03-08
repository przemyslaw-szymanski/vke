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
            //const auto& ICD = m_pCtx->_GetICD().Device;
            //const auto& VkDevice = m_pCtx->_GetDevice();

            for( uint32_t i = 0; i < m_hDescPools.GetCount(); ++i )
            {
                auto& hPool = m_hDescPools[ i ];
                {
                    //VkDevice.DestroyObject( nullptr, &vkPool );
                    m_pCtx->_GetDDI().DestroyObject( &hPool, nullptr );
                }
            }
            m_hDescPools.Clear();
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
                    SDescriptorPoolDesc PoolDesc;
                    PoolDesc.vPoolSizes.Resize( DESCRIPTOR_TYPE_COUNT );
                    for( uint32_t i = 0; i < DESCRIPTOR_TYPE_COUNT; ++i )
                    {
                        /*DESCRIPTOR_SET_TYPE descType = static_cast<DESCRIPTOR_SET_TYPE>( i );
                        
                        VkDescriptorPoolSize& VkPoolSize = vVkSizes[ i ];
                        VkPoolSize.descriptorCount = max( Desc.aMaxDescriptorSetCounts[ i ], Config::RenderSystem::Pipeline::MAX_DESCRIPTOR_TYPE_COUNT );
                        VkPoolSize.type = Vulkan::Map::DescriptorType( descType );*/
                        auto& Size = PoolDesc.vPoolSizes;
                        Size[i].type = static_cast<DESCRIPTOR_SET_TYPE>(i);
                        Size[i].count = std::max<uint32_t>( Desc.aMaxDescriptorSetCounts[i], Config::RenderSystem::Pipeline::MAX_DESCRIPTOR_TYPE_COUNT );
                    }

                    
                    PoolDesc.maxSetCount = Desc.maxCount;
                    DDIDescriptorPool hPool = m_pCtx->_GetDDI().CreateObject( PoolDesc, nullptr );
                    if( hPool == DDI_NULL_HANDLE )
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
            // Default
            {
                SDescriptorSetLayoutDesc LayoutDesc;
                m_pDefaultLayout = CreateLayout( LayoutDesc );
                VKE_ASSERT( m_pDefaultLayout.IsValid(), "Invalid default descriptor set layout" );
            }
            return ret;
        }

        DescriptorSetRefPtr CDescriptorSetManager::CreateSet(const SDescriptorSetDesc& Desc)
        {
            CDescriptorSet* pSet = nullptr;
            SHash Hash;
            Utils::TCDynamicArray< DDIDescriptorSetLayout > vLayouts;

            for( uint32_t i = 0; i < Desc.vpLayouts.GetCount(); ++i )
            {
                vLayouts.PushBack( Desc.vpLayouts[ i ]->GetDDIObject() );
                Hash += Desc.vpLayouts[ i ]->GetHandle();
            }

            CDDI::AllocateDescs::SDescSet AllocDesc;
            AllocDesc.hPool = m_hDescPools.Back();
            AllocDesc.count = vLayouts.GetCount();
            AllocDesc.phLayouts = &vLayouts[ 0 ];
            
            Hash += AllocDesc.hPool;
            Hash += AllocDesc.count;

            DDIDescriptorSet hSet;

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
            SHash Hash;
            Hash += Desc.vBindings.GetCount();

            for( uint32_t i = 0; i < Desc.vBindings.GetCount(); ++i )
            {
                const auto& Binding = Desc.vBindings[i];
                Hash.Combine( Binding.idx, Binding.count, Binding.type, Binding.stages );
            }

            DescSetLayoutBuffer::MapIterator Itr;
            CDescriptorSetLayout* pLayout = nullptr;
            DDIDescriptorSetLayout hLayout = DDI_NULL_HANDLE;
            if( !m_DescSetLayoutBuffer.Get( Hash.value, &pLayout, &Itr ) )
            {
                hLayout = m_pCtx->_GetDDI().CreateObject( Desc, nullptr );
                if( hLayout != DDI_NULL_HANDLE )
                {
                    if( VKE_SUCCEEDED( Memory::CreateObject( &m_DescSetLayoutMemMgr, &pLayout, this ) ) )
                    {
                        if( m_DescSetLayoutBuffer.Add( pLayout, Hash.value, Itr ) )
                        {

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
                else
                {
                    goto ERR;
                }
            }
            if( pLayout )
            {
                if( hLayout != DDI_NULL_HANDLE )
                {
                    if( VKE_SUCCEEDED( pLayout->Init( Desc ) ) )
                    {
                        pLayout->m_hDDIObject = hLayout;
                    }
                    else
                    {
                        goto ERR;
                    }
                }
            }

            return DescriptorSetLayoutRefPtr( pLayout );

        ERR:
            _DestroyLayout( &pLayout );
            return DescriptorSetLayoutRefPtr();
        }

        void CDescriptorSetManager::_DestroyLayout( CDescriptorSetLayout** ppInOut )
        {
            CDescriptorSetLayout* pLayout = *ppInOut;
            if( pLayout != nullptr )
            {
                m_pCtx->_GetDDI().DestroyObject( &pLayout->m_hDDIObject, nullptr );
                Memory::DestroyObject( &HeapAllocator, &pLayout );
            }
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