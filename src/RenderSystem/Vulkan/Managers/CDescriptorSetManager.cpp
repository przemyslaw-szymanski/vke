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

        Result CDescriptorSetManager::Create(const SDescriptorSetManagerDesc& Desc)
        {
            Result ret = VKE_OK;
            // Push null element
            m_PoolBuffer.Add( static_cast<DDIDescriptorPool>(DDI_NULL_HANDLE) );
            m_mLayouts[NULL_HANDLE] = {};
            m_vSets.PushBack( {} );
            
            SDescriptorPoolDesc PoolDesc;
            PoolDesc.maxSetCount = 1024;
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
                hRet = m_PoolBuffer.Add( hPool );
            }
            return hRet;
        }

        void CDescriptorSetManager::DestroyPool( handle_t* phInOut )
        {
            DDIDescriptorPool& hDDIPool = m_PoolBuffer[ *phInOut ];
            m_pCtx->DDI().DestroyObject( &hDDIPool, nullptr );
            *phInOut = NULL_HANDLE;
        }

        DescriptorSetHandle CDescriptorSetManager::CreateSet( const handle_t& hPool, const SDescriptorSetDesc& Desc )
        {
            DDIDescriptorSet hDDISet;
            DescriptorSetLayoutHandle hLayout = Desc.vLayouts[ 0 ];
            DDIDescriptorSetLayout hDDILayout = m_mLayouts[ hLayout.handle ].hDDILayout;
            CDDI::AllocateDescs::SDescSet SetDesc;
            SetDesc.count = 1;
            SetDesc.hPool = m_PoolBuffer[ hPool ];
            SetDesc.phLayouts = &hDDILayout;
            Result res = m_pCtx->DDI().AllocateObjects( SetDesc, &hDDISet );
            DescriptorSetHandle ret = NULL_HANDLE;
            if( VKE_SUCCEEDED( res ) )
            {
                SDescriptorSet Set;
                Set.hPool = hPool;
                Set.hDDISet = hDDISet;
                //Set.hSetLayout = Desc.vLayouts[0];
                Set.hSetLayout = hLayout;
                ret.handle = m_vSets.PushBack( Set );
            }
            return ret;
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
            auto Itr = m_mLayouts.find( Hash.value );
            if( Itr != m_mLayouts.end() )
            {
                ret.handle = Hash.value;
            }
            else
            {
                DDIDescriptorSetLayout hDDILayout = m_pCtx->DDI().CreateObject( Desc, nullptr );
                if( hDDILayout != DDI_NULL_HANDLE )
                {
                    ret.handle = Hash.value;
                    SLayout Layout;
                    Layout.hDDILayout = hDDILayout;
                    m_mLayouts[Hash.value] = Layout;
                }
            }
            return ret;
        }

        void CDescriptorSetManager::_DestroyLayout( CDescriptorSetLayout** ppInOut )
        {
            
        }

        const SDescriptorSet* CDescriptorSetManager::GetSet( DescriptorSetHandle hSet )
        {
            return &m_vSets[ hSet.handle ];
        }

        DDIDescriptorSetLayout CDescriptorSetManager::GetLayout( DescriptorSetLayoutHandle hLayout )
        {
            return m_mLayouts[hLayout.handle].hDDILayout;
        }

    } // RenderSystem
} // VKE

#endif // VKE_VULKAN_RENDERER