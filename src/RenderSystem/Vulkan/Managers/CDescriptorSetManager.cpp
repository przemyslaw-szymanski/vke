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
            for( auto& Pair : m_mLayouts )
            {
                m_pCtx->DDI().DestroyObject( &Pair.second.hDDILayout, nullptr );
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
            m_mLayouts[NULL_HANDLE] = {};
            
            SDescriptorPoolDesc PoolDesc;
            PoolDesc.maxSetCount = Config::RenderSystem::Pipeline::MAX_DESCRIPTOR_SET_COUNT;
            for( uint32_t i = 0; i < DescriptorSetTypes::_MAX_COUNT; ++i )
            {
                SDescriptorPoolDesc::SSize Size;
                Size.count = 16;
                Size.type = static_cast<DESCRIPTOR_SET_TYPE>(i);
                PoolDesc.vPoolSizes.PushBack( Size );
            }
            m_hDefaultPool = CreatePool( PoolDesc );
            if( m_hDefaultPool != NULL_HANDLE )
            {
                SDescriptorSetLayoutDesc LayoutDesc;
                SDescriptorSetLayoutDesc::SBinding Binding;
                Binding.count = 1;
                Binding.idx = 0;
                Binding.stages = PipelineStages::VERTEX | PipelineStages::PIXEL;
                Binding.type = BindingTypes::UNIFORM_BUFFER;
                LayoutDesc.vBindings.PushBack( Binding );
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
            handle_t hRet = NULL_HANDLE;

            DDIDescriptorPool hPool = m_pCtx->DDI().CreateObject( Desc, nullptr );
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
            m_pCtx->DDI().DestroyObject( &hDDIPool, nullptr );
            Pool.SetPool.Clear();
            m_PoolBuffer.Free( static_cast< PoolHandle >( *phInOut ) );
            *phInOut = NULL_HANDLE;
        }

        DescriptorSetHandle CDescriptorSetManager::CreateSet( const handle_t& hPool, const SDescriptorSetDesc& Desc )
        {
            DDIDescriptorSet hDDISet;
            DescriptorSetHandle hRet = NULL_HANDLE;

            DescriptorSetLayoutHandle hLayout = Desc.vLayouts[ 0 ];
            //DDIDescriptorSetLayout hDDILayout = m_mLayouts[ hLayout.handle ].hDDILayout;
            auto& Layout = m_mLayouts[hLayout.handle];
            {
                //Threads::ScopedLock l( m_SyncObj );
                //Layout.vFreeSets.PopBack( &hRet );
                //Layout.mFreeSets[hPool].PopBack( &hRet );
            }
            if( hRet == NULL_HANDLE )
            {
                SPool& Pool = m_PoolBuffer[ static_cast< PoolHandle >( hPool ) ];

                CDDI::AllocateDescs::SDescSet SetDesc;
                SetDesc.count = 1;
                SetDesc.hPool = Pool.hDDIObject;
                SetDesc.phLayouts = &Layout.hDDILayout;
                Result res = m_pCtx->DDI().AllocateObjects( SetDesc, &hDDISet );
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
            DescriptorSetLayoutHandle ret = NULL_HANDLE;
            SHash Hash;
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
                DDIDescriptorSetLayout hDDILayout = m_pCtx->DDI().CreateObject( Desc, nullptr );
                if( hDDILayout != DDI_NULL_HANDLE )
                {
                    ret.handle = hLayout;
                    SLayout Layout;
                    Layout.hDDILayout = hDDILayout;
                    m_mLayouts[hLayout] = Layout;
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
            PoolHandle hPool = static_cast< PoolHandle >( NULL_HANDLE );
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
                    m_pCtx->DDI().FreeObjects( Sets );
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
                m_pCtx->DDI().FreeObjects( Sets );
                vDDISets.Clear();
            }
        }

        void CDescriptorSetManager::_FreeSets( DescriptorSetHandle* phSets, uint32_t )
        {
            //handle_t hTmpLayout = NULL_HANDLE;
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
            return m_mLayouts[hLayout.handle].hDDILayout;
        }

        DescriptorSetLayoutHandle CDescriptorSetManager::GetLayout( const DescriptorSetHandle& hSet )
        {
            UDescSetHandle hDescSet;
            hDescSet.handle = hSet.handle;
            return DescriptorSetLayoutHandle{ hDescSet.hLayout };
        }

    } // RenderSystem
} // VKE

#endif // VKE_VULKAN_RENDERER