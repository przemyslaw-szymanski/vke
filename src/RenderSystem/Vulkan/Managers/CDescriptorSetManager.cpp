#if VKE_VULKAN_RENDER_SYSTEM
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
            for( auto& Pair : m_mLayouts )
            {
                m_pCtx->NativeAPI().DestroyDescriptorSetLayout( &Pair.second.hDDILayout, nullptr );
            }
            m_mLayouts.clear();

            for( uint32_t i = 0; i < m_PoolBuffer.vPool.GetCount(); ++i )
            {
                handle_t h = i;
                {
                    DestroyPool( &h );
                }
            }
            m_PoolBuffer.Clear();
        }

        Result CDescriptorSetManager::Create(const SDescriptorSetManagerDesc&)
        {
            Result ret = VKE_OK;
            // Push null element
            m_PoolBuffer.Add( static_cast<DDIDescriptorPool>(DDI_NULL_HANDLE) );
            m_mLayouts[INVALID_HANDLE] = {};
            
            SDescriptorPoolDesc PoolDesc;
            PoolDesc.maxSetCount = Config::RenderSystem::Pipeline::MAX_DESCRIPTOR_SET_COUNT;
            for( uint32_t i = 0; i < DescriptorSetTypes::_MAX_COUNT; ++i )
            {
                SDescriptorPoolDesc::SSize Size;
                Size.count = 16;
                Size.type = static_cast<DESCRIPTOR_SET_TYPE>(i);
                PoolDesc.vPoolSizes.PushBack( Size );
            }
            PoolDesc.SetDebugName( "VKE_DefaultDescriptorPool" );
            m_hDefaultPool = CreatePool( PoolDesc );
            if( m_hDefaultPool != INVALID_HANDLE )
            {
                SDescriptorSetLayoutDesc LayoutDesc;
                SDescriptorSetLayoutDesc::SBinding Binding;
                Binding.count = 1;
                Binding.idx = 0;
                Binding.stages = PipelineStages::VERTEX | PipelineStages::PIXEL;
                Binding.type = BindingTypes::CONSTANT_BUFFER;
                LayoutDesc.vBindings.PushBack( Binding );
                LayoutDesc.SetDebugName( "VKE_DefaultDescriptorLayout" );
                m_hDefaultLayout = CreateLayout( LayoutDesc );
            }

            if( ret == VKE_FAIL )
            {
                Destroy();
            }

            return ret;
        }

        handle_t CDescriptorSetManager::CreatePool( const SDescriptorPoolDesc& Desc )
        {
            handle_t hRet = INVALID_HANDLE;

            DDIDescriptorPool hPool = m_pCtx->NativeAPI().CreateDescriptorPool( Desc, nullptr );
            if( hPool != DDI_NULL_HANDLE )
            {
                hRet = m_PoolBuffer.Add( { hPool } );
            }
            return hRet;
        }

        void CDescriptorSetManager::DestroyPool( handle_t* phInOut )
        {
            SPool& Pool = m_PoolBuffer[ static_cast<PoolHandle>( *phInOut ) ];
            DDIDescriptorPool& hDDIPool = Pool.hDDIObject;
            m_pCtx->NativeAPI().DestroyDescriptorPool( &hDDIPool, nullptr );
            Pool.SetPool.Clear();
            m_PoolBuffer.Free( static_cast< PoolHandle >( *phInOut ) );
            *phInOut = INVALID_HANDLE;
        }

        DescriptorSetHandle CDescriptorSetManager::CreateSet( const handle_t& hPool,
            const SDescriptorSetDesc& Desc )
        {
            DDIDescriptorSet hDDISet;
            DescriptorSetHandle hRet = INVALID_HANDLE;

            DescriptorSetLayoutHandle hLayout = Desc.hLayout;
            //DDIDescriptorSetLayout hDDILayout = m_mLayouts[ hLayout.handle ].hDDILayout;
            auto& Layout = m_mLayouts[(hash_t)hLayout.handle];
            {
                //Threads::ScopedLock l( m_SyncObj );
                //Layout.vFreeSets.PopBack( &hRet );
                //Layout.mFreeSets[hPool].PopBack( &hRet );
            }
            if( hRet == INVALID_HANDLE )
            {
                SPool& Pool = m_PoolBuffer[ static_cast< PoolHandle >( hPool ) ];

                CDDI::AllocateDescs::SDescSet SetDesc;
                SetDesc.count = 1;
                SetDesc.hPool = Pool.hDDIObject;
                SetDesc.phLayouts = &Layout.hDDILayout;
                SetDesc.SetDebugName( Desc.GetDebugName() );
                Result res = m_pCtx->NativeAPI().AllocateObjects( SetDesc, &hDDISet );
                if( VKE_SUCCEEDED( res ) )
                {
                    SDescriptorSet Set;
                    Set.hPool = hPool;
                    Set.hDDISet = hDDISet;
                    //Set.hSetLayout = Desc.vLayouts[0];
                    Set.hSetLayout = hLayout;

                    UDescSetHandle hSet;
                    hSet.hLayout = static_cast<LayoutHandle>( hLayout.handle );
                    hSet.hPool = static_cast< PoolHandle >( hPool );
                    hSet.index = Pool.SetPool.Add( hDDISet );
                    hRet.handle = hSet.handle;
                }
                else if(res == VKE_ENOMEMORY)
                {
                    // Create new pool
                    auto hTmpPool = CreatePool( m_DefaultPoolDesc );
                    res = m_pCtx->NativeAPI().AllocateObjects( SetDesc, &hDDISet );
                    
                    if(VKE_SUCCEEDED(res))
                    {
                        m_hDefaultPool = hTmpPool;
                        SDescriptorSet Set;
                        Set.hPool = hPool;
                        Set.hDDISet = hDDISet;
                        // Set.hSetLayout = Desc.vLayouts[0];
                        Set.hSetLayout = hLayout;
                        UDescSetHandle hSet;
                        hSet.hLayout = static_cast<LayoutHandle>( hLayout.handle );
                        hSet.hPool = static_cast<PoolHandle>( hPool );
                        hSet.index = Pool.SetPool.Add( hDDISet );
                        hRet.handle = hSet.handle;
                    }
                    // If still no memory try to allocate pool that fits with layout
                    else if(res == VKE_ENOMEMORY)
                    {
                        const auto& LayoutDesc = m_mLayouts[ hLayout.handle ].Desc;
                        SDescriptorPoolDesc PoolDesc;
                        PoolDesc.vPoolSizes.Reserve( LayoutDesc.vBindings.GetCount() );
                        PoolDesc.maxSetCount = 16; /// TODO: de-hardcode this
                        PoolDesc.SetDebugName( Desc.GetDebugName() );

                        for( uint32_t i = 0; i < LayoutDesc.vBindings.GetCount(); ++i )
                        {
                            SDescriptorPoolDesc::SSize Size =
                            {
                                .type = LayoutDesc.vBindings[i].type,
                                .count = LayoutDesc.vBindings[i].count
                            };
                            PoolDesc.vPoolSizes.PushBack( Size );
                        }
                        hTmpPool = CreatePool( PoolDesc );
                        hRet = CreateSet( hTmpPool, Desc );
                    }
                }
                else
                {
                    VKE_LOG_ERR( "Unable to allocate DescriptorSet: " << Desc.GetDebugName() );
                }
            }
            else
            {
                VKE_LOG( "Pop set: " << hRet.handle );
            }
            return hRet;
        }

        template<typename T>
        void HashCombine( hash_t* pInOut, const T& v )
        {
            std::hash< T > h;
            *pInOut ^= h( v ) + 0x9e3779b9 + ( *pInOut << 6 ) + ( *pInOut >> 2 );
        }

        DescriptorSetLayoutHandle CDescriptorSetManager::CreateLayout(const SDescriptorSetLayoutDesc& Desc)
        {
            DescriptorSetLayoutHandle ret = INVALID_HANDLE;
            Utils::SHash Hash;
            Hash += Desc.vBindings.GetCount();
            for( uint32_t i = 0; i < Desc.vBindings.GetCount(); ++i )
            {
                const auto& Binding = Desc.vBindings[i];
                Hash.Combine( Binding.count, Binding.idx, Binding.stages, Binding.type );
            }
            LayoutHandle hLayout = static_cast< LayoutHandle >( Hash.value );

            auto Itr = m_mLayouts.find( hLayout );
            if( Itr != m_mLayouts.end() )
            {
                ret.handle = hLayout;
            }
            else
            {
                DDIDescriptorSetLayout hDDILayout = m_pCtx->NativeAPI().CreateDescriptorSetLayout( Desc, nullptr );
                if( hDDILayout != DDI_NULL_HANDLE )
                {
                    ret.handle = hLayout;
                    m_mLayouts[ hLayout ] = { .hDDILayout = hDDILayout, .Desc = Desc };
                }
            }
            return ret;
        }

        void CDescriptorSetManager::_DestroyLayout( CDescriptorSetLayout** )
        {

        }

        void CDescriptorSetManager::_DestroySets( DescriptorSetHandle* phSets, const uint32_t count )
        {
            DDISetArray vDDISets;
            PoolHandle hPool = static_cast< PoolHandle >( INVALID_HANDLE );
            for( uint32_t i = 0; i < count; ++i )
            {
                UDescSetHandle hSet;
                hSet.handle = phSets[ i ].handle;

                if( hPool != hSet.hPool && !vDDISets.IsEmpty() )
                {
                    SPool& Pool = m_PoolBuffer[ hPool ];
                    CDDI::FreeDescs::SDescSet Sets;
                    Sets.count = vDDISets.GetCount();
                    Sets.hPool = Pool.hDDIObject;
                    Sets.phSets = vDDISets.GetData();
                    m_pCtx->NativeAPI().FreeObjects( Sets );
                    vDDISets.Clear();
                }

                {
                    hPool = hSet.hPool;
                    SPool& Pool = m_PoolBuffer[hPool];
                    vDDISets.PushBack( Pool.SetPool[hSet.index] );
                    Pool.SetPool.Free( hSet.index );
                }
            }
            if( !vDDISets.IsEmpty() )
            {
                SPool& Pool = m_PoolBuffer[ hPool ];
                CDDI::FreeDescs::SDescSet Sets;
                Sets.count = vDDISets.GetCount();
                Sets.hPool = Pool.hDDIObject;
                Sets.phSets = vDDISets.GetData();
                m_pCtx->NativeAPI().FreeObjects( Sets );
                vDDISets.Clear();
            }
        }

        void CDescriptorSetManager::_FreeSets( DescriptorSetHandle* phSets, uint32_t )
        {
            //handle_t hTmpLayout = INVALID_HANDLE;
            //SLayout* pTmpLayout = nullptr;

            
        }

        const DDIDescriptorSet& CDescriptorSetManager::GetSet( const DescriptorSetHandle& hSet )
        {
            UDescSetHandle hDescSet;
            hDescSet.handle = hSet.handle;
            return m_PoolBuffer[ hDescSet.hPool ].SetPool[ hDescSet.index ];
        }

        DDIDescriptorSetLayout CDescriptorSetManager::GetLayout( const DescriptorSetLayoutHandle& hLayout )
        {
            return m_mLayouts[(const hash_t)hLayout.handle].hDDILayout;
        }

        DescriptorSetLayoutHandle CDescriptorSetManager::GetLayout( const DescriptorSetHandle& hSet )
        {
            UDescSetHandle hDescSet;
            hDescSet.handle = hSet.handle;
            return DescriptorSetLayoutHandle{ hDescSet.hLayout };
        }

        DescriptorSetLayoutHandle CDescriptorSetManager::GetLayout( const SDescriptorSetLayoutDesc& Desc )
        {
            auto hRet = CreateLayout( Desc );
            return hRet;
        }

    } // RenderSystem
} // VKE

#endif // VKE_VULKAN_RENDER_SYSTEM