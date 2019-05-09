#pragma once
#if VKE_VULKAN_RENDERER
#include "RenderSystem/Common.h"
#include "RenderSystem/Vulkan/Vulkan.h"
#include "RenderSystem/Vulkan/CDescriptorSet.h"
#include "Core/Utils/TCPoolFreeList.h"
#include "Core/Managers/CResourceManager.h"

namespace VKE
{
    namespace RenderSystem
    {
        
        struct SDescriptorSetManagerDesc
        {
            DescriptorSetCounts aMaxDescriptorSetCounts = { Config::RenderSystem::Pipeline::MAX_DESCRIPTOR_TYPE_COUNT };
            uint32_t            maxCount = Config::RenderSystem::Pipeline::MAX_DESCRIPTOR_SET_COUNT;
        };

        class CDescriptorSetManager
        {
            friend class CDeviceContext;
            friend class CDescriptorSet;
            friend class CDescriptorSetLayout;
            friend class CContextBase;

            using PoolHandle = uint16_t;
            using SetHandle = uint16_t;
            using LayoutHandle = uint32_t;

            union UDescSetHandle
            {
                struct
                {
                    SetHandle       index;
                    PoolHandle      hPool;
                    LayoutHandle    hLayout;
                };
                handle_t handle;
            };

            using SetArray = Utils::TCDynamicArray< SDescriptorSet, Config::RenderSystem::Pipeline::MAX_DESCRIPTOR_SET_COUNT >;
            using DDISetArray = Utils::TCDynamicArray< DDIDescriptorSet, Config::RenderSystem::Pipeline::MAX_DESCRIPTOR_SET_COUNT >;
            using DDISetPool = Utils::TSFreePool< DDIDescriptorSet, SetHandle, Config::RenderSystem::Pipeline::MAX_DESCRIPTOR_SET_COUNT >;

            struct SPool
            {
                SPool() {}
                SPool( DDIDescriptorPool hDDI ) : hDDIObject{ hDDI } { SetPool.Add( {} ); }

                DDIDescriptorPool   hDDIObject;
                DDISetPool          SetPool;
            };
            

            using PoolDescArray = Utils::TCDynamicArray< SDescriptorPoolDesc >;
            using DDIPoolBuffer = Utils::TSFreePool< DDIDescriptorPool, uint16_t >;
            using PoolBuffer = Utils::TSFreePool< SPool, PoolHandle >;
            using SetHandleArray = Utils::TCDynamicArray< DescriptorSetHandle >;
            
            using SetMap = vke_hash_map< LayoutHandle, SetHandleArray >;

            struct SLayout
            {
                DDIDescriptorSetLayout  hDDILayout;
                SetMap                  mFreeSets;

                handle_t                hLastUsedPool = NULL_HANDLE;
                SetHandleArray*         pLastUsedPool;
            };
            using LayoutMap = vke_hash_map< hash_t, SLayout >;



            public:

                CDescriptorSetManager(CDeviceContext* pCtx);
                ~CDescriptorSetManager();

                Result Create(const SDescriptorSetManagerDesc& Desc);
                void Destroy();

                handle_t                    CreatePool( const SDescriptorPoolDesc& Desc );
                void                        DestroyPool( handle_t* phInOut );
                DescriptorSetHandle         CreateSet(const handle_t& hPool, const SDescriptorSetDesc& Desc);
                void                        DestroySet(DescriptorSetPtr pSet);
                DescriptorSetLayoutHandle   CreateLayout(const SDescriptorSetLayoutDesc& Desc);
                void                        DestroyLayout(DescriptorSetLayoutPtr pLayout);

                const DDIDescriptorSet&     GetSet( DescriptorSetHandle hSet );
                DescriptorSetLayoutHandle   GetLayout( DescriptorSetHandle hSet );
                DDIDescriptorSetLayout      GetLayout( DescriptorSetLayoutHandle hLayout );

                DescriptorSetLayoutHandle   GetDefaultLayout() { return m_hDefaultLayout; }

                //DescriptorSetLayoutPtr      GetDefaultLayout() const { return m_pDefaultLayout; }

            protected:

                void                        _DestroyLayout( CDescriptorSetLayout** ppInOut );
                void                        _DestroySets( DescriptorSetHandle* phSets, const uint32_t count );
                void                        _FreeSets( DescriptorSetHandle* phSets, uint32_t count );

            protected:

                CDeviceContext*             m_pCtx;
                PoolBuffer                  m_PoolBuffer;
                PoolDescArray               m_vPoolDescs;
                LayoutMap                   m_mLayouts;
                handle_t                    m_hDefaultPool;
                DescriptorSetLayoutHandle   m_hDefaultLayout;
                Threads::SyncObject         m_SyncObj;
                std::mutex                  m_mtx;
        };
    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER