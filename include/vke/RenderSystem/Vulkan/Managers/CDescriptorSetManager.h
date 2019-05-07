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
            

            using PoolDescArray = Utils::TCDynamicArray< SDescriptorPoolDesc >;
            using PoolBuffer = Utils::TSFreePool< DDIDescriptorPool, uint16_t >;

            struct SLayout
            {
                DDIDescriptorSetLayout  hDDILayout;
            };
            using LayoutMap = vke_hash_map< hash_t, SLayout >;


            using SetArray = Utils::TCDynamicArray< SDescriptorSet, Config::RenderSystem::Pipeline::MAX_DESCRIPTOR_SET_COUNT >;

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

                const SDescriptorSet*       GetSet( DescriptorSetHandle hSet );
                DDIDescriptorSetLayout      GetLayout( DescriptorSetLayoutHandle hLayout );

                DescriptorSetLayoutHandle   GetDefaultLayout() { return m_hDefaultLayout; }

                //DescriptorSetLayoutPtr      GetDefaultLayout() const { return m_pDefaultLayout; }

            protected:

                void                        _DestroyLayout( CDescriptorSetLayout** ppInOut );

            protected:

                CDeviceContext*             m_pCtx;
                PoolBuffer                  m_PoolBuffer;
                PoolDescArray               m_vPoolDescs;
                LayoutMap                   m_mLayouts;
                SetArray                    m_vSets;
                handle_t                    m_hDefaultPool;
                DescriptorSetLayoutHandle   m_hDefaultLayout;
        };
    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER