#include "RenderSystem/Managers/CDescriptorSetManager.h"
#include "Rendersystem/CDeviceContext.h"

namespace VKE
{
    namespace RenderSystem
    {
        CDescriptorSetManager::CDescriptorSetManager( CDeviceContext* pCtx ) : m_pCtx( pCtx )
        {
        }

        CDescriptorSetManager::~CDescriptorSetManager()
        {
            Destroy();
        }

        void CDescriptorSetManager::Destroy()
        {
            for( auto& Pair: m_mLayouts )
            {
                m_pCtx->RHI().DestroyDescriptorSetLayout( &Pair.second.hDDILayout );
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

        Result CDescriptorSetManager::Create( const SDescriptorSetManagerDesc& )
        {
            Result ret = VKE_OK;
            // Push null element
            m_PoolBuffer.Add( static_cast< NativeAPI::DescriptorPool >( NativeAPI::Null ) );
            m_mLayouts[ INVALID_HANDLE ] = {};

            {
                SDescriptorPoolDesc& Desc = m_aDefaultPoolDescs[ DescriptorPoolTypes::SAMPLER ];
                Desc.SetDebugName( "Sampler" );
                Desc.vPoolSizes                                  = { { DescriptorSetTypes::SAMPLER,
                                                                       Config::RenderSystem::Bindings::MAX_SAMPLER_DESCRIPTOR_COUNT } };
                m_ahDefaultPools[ DescriptorPoolTypes::SAMPLER ] = CreatePool( Desc );
            }
            {
                SDescriptorPoolDesc& Desc = m_aDefaultPoolDescs[ DescriptorPoolTypes::TEXTURE_BUFFER_CBUFFER ];
                Desc.SetDebugName( "TextureBufferCBuffer" );
                Desc.vPoolSizes = {
                    { DescriptorSetTypes::BUFFER, Config::RenderSystem::Bindings::MAX_BUFFER_DESCRIPTOR_COUNT },
                    { DescriptorSetTypes::CONSTANT_BUFFER,
                      Config::RenderSystem::Bindings::MAX_CONSTANT_BUFFER_DESCRIPTOR_COUNT },
                    { DescriptorSetTypes::DYNAMIC_BUFFER, Config::RenderSystem::Bindings::MAX_BUFFER_DESCRIPTOR_COUNT },
                    { DescriptorSetTypes::DYNAMIC_CONSTANT_BUFFER,
                      Config::RenderSystem::Bindings::MAX_CONSTANT_BUFFER_DESCRIPTOR_COUNT },
                    { DescriptorSetTypes::READ_ONLY_TEXEL_BUFFER,
                      Config::RenderSystem::Bindings::MAX_BUFFER_DESCRIPTOR_COUNT },
                    { DescriptorSetTypes::READ_WRITE_TEXEL_BUFFER,
                      Config::RenderSystem::Bindings::MAX_READ_WRITE_BUFFER_DESCRIPTOR_COUNT },
                    { DescriptorSetTypes::STORAGE_TEXTURE,
                      Config::RenderSystem::Bindings::MAX_STORAGE_TEXTURE_DESCRIPTOR_COUNT },
                    { DescriptorSetTypes::TEXTURE, Config::RenderSystem::Bindings::MAX_TEXTURE_DESCRIPTOR_COUNT }
                };
                m_ahDefaultPools[ DescriptorPoolTypes::TEXTURE_BUFFER_CBUFFER ] = CreatePool( Desc );
            }
            {
                SDescriptorPoolDesc& Desc = m_aDefaultPoolDescs[ DescriptorPoolTypes::COLOR_RENDER_TARGET ];
                Desc.SetDebugName( "ColorRenderTarget" );
                Desc.maxSetCount = Config::RenderSystem::Bindings::MAX_COLOR_RENDER_TARGET_DESCRIPTOR_COUNT;
                Desc.vPoolSizes  = {
                    { DescriptorSetTypes::RENDER_TARGET,
                       Config::RenderSystem::Bindings::MAX_COLOR_RENDER_TARGET_DESCRIPTOR_COUNT },
                };
                m_ahDefaultPools[ DescriptorPoolTypes::COLOR_RENDER_TARGET ] = CreatePool( Desc );
            }
            {
                SDescriptorPoolDesc& Desc = m_aDefaultPoolDescs[ DescriptorPoolTypes::DEPTH_STENCIL ];
                Desc.SetDebugName( "DepthStencil" );
                Desc.maxSetCount = Config::RenderSystem::Bindings::MAX_DEPTH_STENCIL_DESCRIPTOR_COUNT;
                Desc.vPoolSizes  = {
                    { DescriptorSetTypes::DEPTH_STENCIL,
                       Config::RenderSystem::Bindings::MAX_DEPTH_STENCIL_DESCRIPTOR_COUNT },
                };
                m_ahDefaultPools[ DescriptorPoolTypes::DEPTH_STENCIL ] = CreatePool( Desc );
            }
            for( uint32_t i = 0; i < DescriptorPoolTypes::_MAX_COUNT; ++i )
            {
                if( m_ahDefaultPools[ i ] == INVALID_HANDLE )
                {
                    ret = VKE_FAIL;
                    break;
                }
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

            NativeAPI::DescriptorPool hPool = m_pCtx->RHI().CreateDescriptorPool( Desc );
            if( hPool != NativeAPI::Null )
            {
                hRet = m_PoolBuffer.Add( { hPool } );
            }
            return hRet;
        }

        void CDescriptorSetManager::DestroyPool( handle_t* phInOut )
        {
            SPool&                     Pool     = m_PoolBuffer[ static_cast< PoolHandle >( *phInOut ) ];
            NativeAPI::DescriptorPool& hDDIPool = Pool.hDDIObject;
            m_pCtx->RHI().DestroyDescriptorPool( &hDDIPool );
            Pool.SetPool.Clear();
            m_PoolBuffer.Free( static_cast< PoolHandle >( *phInOut ) );
            *phInOut = INVALID_HANDLE;
        }

        DescriptorSetHandle CDescriptorSetManager::CreateSet( handle_t hPool, const SDescriptorSetDesc& Desc )
        {
            NativeAPI::DescriptorSet hDDISet;
            DescriptorSetHandle      hRet = INVALID_HANDLE;

            DescriptorSetLayoutHandle hLayout = Desc.hLayout;
            // NativeAPI::DescriptorSetLayout hDDILayout = m_mLayouts[ hLayout.handle ].hDDILayout;
            auto& Layout = m_mLayouts[ (hash_t)hLayout.handle ];
            if( hPool == INVALID_HANDLE )
            {
                const auto poolType = _GetPoolType( Layout );
                hPool               = m_ahDefaultPools[ poolType ];
            }
            if( hRet == INVALID_HANDLE )
            {
                SPool& Pool = m_PoolBuffer[ static_cast< PoolHandle >( hPool ) ];

                AllocateDescs::SDescSet SetDesc;
                SetDesc.count     = 1;
                SetDesc.hPool     = Pool.hDDIObject;
                SetDesc.phLayouts = &Layout.hDDILayout;
                SetDesc.SetDebugName( Desc.GetDebugName() );
                Result res = m_pCtx->RHI().CreateDescriptorSets( SetDesc, &hDDISet );
                if( VKE_SUCCEEDED( res ) )
                {
                    SDescriptorSet Set;
                    Set.hPool   = hPool;
                    Set.hDDISet = hDDISet;
                    // Set.hSetLayout = Desc.vLayouts[0];
                    Set.hSetLayout = hLayout;

                    UDescSetHandle hSet;
                    hSet.hLayout = static_cast< LayoutHandle >( hLayout.handle );
                    hSet.hPool   = static_cast< PoolHandle >( hPool );
                    hSet.index   = Pool.SetPool.Add( hDDISet );
                    hRet.handle  = hSet.handle;
                }
                else if( res == VKE_ENOMEMORY )
                {
                    // Create new pool
                    const auto poolType = _GetPoolType( Layout );
                    auto       hTmpPool = CreatePool( m_aDefaultPoolDescs[ poolType ] );

                    if( VKE_SUCCEEDED( res ) )
                    {
                        m_ahDefaultPools[ poolType ] = hTmpPool;
                        SDescriptorSet Set;
                        Set.hPool   = hPool;
                        Set.hDDISet = hDDISet;
                        // Set.hSetLayout = Desc.vLayouts[0];
                        Set.hSetLayout = hLayout;
                        UDescSetHandle hSet;
                        hSet.hLayout = static_cast< LayoutHandle >( hLayout.handle );
                        hSet.hPool   = static_cast< PoolHandle >( hPool );
                        hSet.index   = Pool.SetPool.Add( hDDISet );
                        hRet.handle  = hSet.handle;
                    }
                    // If still no memory try to allocate pool that fits with layout
                    else if( res == VKE_ENOMEMORY )
                    {
                        const auto&         LayoutDesc = m_mLayouts[ hLayout.handle ].Desc;
                        SDescriptorPoolDesc PoolDesc;
                        PoolDesc.vPoolSizes.Reserve( LayoutDesc.vBindings.GetCount() );
                        PoolDesc.maxSetCount = 16; /// TODO: de-hardcode this
                        PoolDesc.SetDebugName( Desc.GetDebugName() );

                        for( uint32_t i = 0; i < LayoutDesc.vBindings.GetCount(); ++i )
                        {
                            SDescriptorPoolDesc::SSize Size = { .type  = LayoutDesc.vBindings[ i ].type,
                                                                .count = LayoutDesc.vBindings[ i ].count };
                            PoolDesc.vPoolSizes.PushBack( Size );
                        }
                        hTmpPool = CreatePool( PoolDesc );
                        hRet     = CreateSet( hTmpPool, Desc );
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

        template< typename T >
        void HashCombine( hash_t* pInOut, const T& v )
        {
            std::hash< T > h;
            *pInOut ^= h( v ) + 0x9e3779b9 + ( *pInOut << 6 ) + ( *pInOut >> 2 );
        }

        DescriptorSetLayoutHandle CDescriptorSetManager::CreateLayout( const SDescriptorSetLayoutDesc& Desc )
        {
            DescriptorSetLayoutHandle ret = INVALID_HANDLE;
            Utils::SHash              Hash;
            Hash += Desc.vBindings.GetCount();
            for( uint32_t i = 0; i < Desc.vBindings.GetCount(); ++i )
            {
                const auto& Binding = Desc.vBindings[ i ];
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
                NativeAPI::DescriptorSetLayout hDDILayout =
                    m_pCtx->RHI().CreateDescriptorSetLayout( Desc );
                if( hDDILayout != NativeAPI::Null )
                {
                    ret.handle            = hLayout;
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
            PoolHandle  hPool = static_cast< PoolHandle >( INVALID_HANDLE );
            for( uint32_t i = 0; i < count; ++i )
            {
                UDescSetHandle hSet;
                hSet.handle = phSets[ i ].handle;

                if( hPool != hSet.hPool && !vDDISets.IsEmpty() )
                {
                    SPool&                    Pool = m_PoolBuffer[ hPool ];
                    FreeDescs::SDescSet Sets;
                    Sets.count  = vDDISets.GetCount();
                    Sets.hPool  = Pool.hDDIObject;
                    Sets.phSets = vDDISets.GetData();
                    m_pCtx->RHI().FreeObjects( Sets );
                    vDDISets.Clear();
                }

                {
                    hPool       = hSet.hPool;
                    SPool& Pool = m_PoolBuffer[ hPool ];
                    vDDISets.PushBack( Pool.SetPool[ hSet.index ] );
                    Pool.SetPool.Free( hSet.index );
                }
            }
            if( !vDDISets.IsEmpty() )
            {
                SPool&                    Pool = m_PoolBuffer[ hPool ];
                FreeDescs::SDescSet Sets;
                Sets.count  = vDDISets.GetCount();
                Sets.hPool  = Pool.hDDIObject;
                Sets.phSets = vDDISets.GetData();
                m_pCtx->RHI().FreeObjects( Sets );
                vDDISets.Clear();
            }
        }

        void CDescriptorSetManager::_FreeSets( DescriptorSetHandle* phSets, uint32_t )
        {
            // handle_t hTmpLayout = INVALID_HANDLE;
            // SLayout* pTmpLayout = nullptr;
        }

        const NativeAPI::DescriptorSet& CDescriptorSetManager::GetSet( const DescriptorSetHandle& hSet )
        {
            UDescSetHandle hDescSet;
            hDescSet.handle = hSet.handle;
            return m_PoolBuffer[ hDescSet.hPool ].SetPool[ hDescSet.index ];
        }

        NativeAPI::DescriptorSetLayout CDescriptorSetManager::GetLayout( const DescriptorSetLayoutHandle& hLayout )
        {
            return m_mLayouts[ (const hash_t)hLayout.handle ].hDDILayout;
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

    } // namespace RenderSystem
} // namespace VKE
