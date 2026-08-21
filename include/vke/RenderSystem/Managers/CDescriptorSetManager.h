#pragma once

#include "Core/Managers/CResourceManager.h"
#include "Core/Utils/TCPoolFreeList.h"

#include "RenderSystem/CDescriptorSet.h"
#include "RenderSystem/Common.h"

namespace VKE
{
    namespace RenderSystem
    {

        struct SDescriptorSetManagerDesc
        {
            DescriptorSetCounts aMaxDescriptorSetCounts = { Config::RenderSystem::Pipeline::MAX_DESCRIPTOR_TYPE_COUNT };
            uint32_t            maxCount                = Config::RenderSystem::Pipeline::MAX_DESCRIPTOR_SET_COUNT;
        };

        class CDescriptorSetManager
        {
            friend class CDeviceContext;
            friend class CDescriptorSet;
            friend class CDescriptorSetLayout;
            friend class CContextBase;

            using PoolHandle   = uint16_t;
            using SetHandle    = uint16_t;
            using LayoutHandle = uint32_t;

            union UDescSetHandle
            {
                struct
                {
                    SetHandle    index;
                    PoolHandle   hPool;
                    LayoutHandle hLayout;
                };

                handle_t handle;
            };

            using SetArray =
                Utils::TCDynamicArray< SDescriptorSet, Config::RenderSystem::Pipeline::MAX_DESCRIPTOR_SET_COUNT >;
            using DDISetArray = Utils::TCDynamicArray< RHI::DescriptorSet,
                                                       Config::RenderSystem::Pipeline::MAX_DESCRIPTOR_SET_COUNT >;
            using DDISetPool  = Utils::TSFreePool< RHI::DescriptorSet, SetHandle,
                                                   Config::RenderSystem::Pipeline::MAX_DESCRIPTOR_SET_COUNT >;

            struct SPool
            {
                SPool()
                {
                }

                SPool( RHI::DescriptorPool hDDI ) : hDDIObject{ hDDI }
                {
                    SetPool.Add( {} );
                }

                RHI::DescriptorPool hDDIObject;
                DDISetPool                SetPool;
            };

            using PoolDescArray  = Utils::TCDynamicArray< SDescriptorPoolDesc >;
            using DDIPoolBuffer  = Utils::TSFreePool< RHI::DescriptorPool, uint16_t >;
            using PoolBuffer     = Utils::TSFreePool< SPool, PoolHandle >;
            using SetHandleArray = Utils::TCDynamicArray< DescriptorSetHandle >;

            using SetMap = vke_hash_map< LayoutHandle, SetHandleArray >;

            struct SLayout
            {
                RHI::DescriptorSetLayout hDDILayout;
                SDescriptorSetLayoutDesc       Desc;
                SetMap                   mFreeSets = {};

                handle_t        hLastUsedPool = INVALID_HANDLE;
                SetHandleArray* pLastUsedPool = nullptr;
            };

            using LayoutMap = vke_hash_map< hash_t, SLayout >;

        public:
            CDescriptorSetManager( CDeviceContext* pCtx );
            ~CDescriptorSetManager();

            Result Create( const SDescriptorSetManagerDesc& Desc );
            void   Destroy();

            handle_t                  CreatePool( const SDescriptorPoolDesc& Desc );
            void                      DestroyPool( handle_t* phInOut );
            DescriptorSetHandle       CreateSet( handle_t hPool, const SDescriptorSetDesc& Desc );
            void                      DestroySet( DescriptorSetPtr pSet );
            DescriptorSetLayoutHandle CreateLayout( const SDescriptorSetLayoutDesc& Desc );
            void                      DestroyLayout( DescriptorSetLayoutPtr pLayout );

            const RHI::DescriptorSet& GetSet( const DescriptorSetHandle& hSet );
            DescriptorSetLayoutHandle       GetLayout( const DescriptorSetHandle& hSet );
            RHI::DescriptorSetLayout  GetLayout( const DescriptorSetLayoutHandle& hLayout );
            DescriptorSetLayoutHandle       GetLayout( const SDescriptorSetLayoutDesc& Desc );

            // DescriptorSetLayoutPtr      GetDefaultLayout() const { return m_pDefaultLayout; }

        protected:
            void _DestroyLayout( CDescriptorSetLayout** ppInOut );
            void _DestroySets( DescriptorSetHandle* phSets, const uint32_t count );
            void _FreeSets( DescriptorSetHandle* phSets, uint32_t count );

            DESCRIPTOR_POOL_TYPE _GetPoolType( const SLayout& Layout ) const
            {
                return BindingTypeToPoolType( Layout.Desc.vBindings[ 0 ].type );
            }

        protected:
            CDeviceContext*           m_pCtx;
            SDescriptorPoolDesc       m_DefaultPoolDesc;
            PoolBuffer                m_PoolBuffer;
            PoolDescArray             m_vPoolDescs;
            LayoutMap                 m_mLayouts;
            SDescriptorPoolDesc       m_aDefaultPoolDescs[ DescriptorPoolTypes::_MAX_COUNT ];
            handle_t                  m_ahDefaultPools[ DescriptorPoolTypes::_MAX_COUNT ];
            Threads::SyncObject       m_SyncObj;
            std::mutex                m_mtx;
        };
    } // namespace RenderSystem
} // namespace VKE
