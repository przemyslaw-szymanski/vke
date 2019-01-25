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

            static const uint32_t DESCRIPTOR_TYPE_COUNT = BindingTypes::_MAX_COUNT;
            static const uint32_t DESCRIPTOR_SET_COUNT = Config::RenderSystem::Pipeline::MAX_DESCRIPTOR_SET_COUNT;
            static const uint32_t DESCRIPTOR_SET_LAYOUT_COUNT = Config::RenderSystem::Pipeline::MAX_DESCRIPTOR_SET_LAYOUT_COUNT;
            using DescSetBuffer = Core::TSResourceBuffer< CDescriptorSet*, CDescriptorSet*, DESCRIPTOR_SET_COUNT >;
            using DescSetLayoutBuffer = Core::TSResourceBuffer< CDescriptorSetLayout*, CDescriptorSetLayout*, DESCRIPTOR_SET_COUNT >;
            using DescSetBufferArray = Utils::TCDynamicArray< DescSetBuffer, 2 >;
            using VkDescriptorPoolArray = Utils::TCDynamicArray< DDIDescriptorPool, 2 >;
            using DescSetMemoryPool = Memory::CFreeListPool; //Utils::TCFreeList< CDescriptorSet, DESCRIPTOR_SET_COUNT >;
            using DescSetLayoutMemoryPool = Memory::CFreeListPool; //Utils::TCFreeList< CDescriptorSetLayout, DESCRIPTOR_SET_LAYOUT_COUNT >;
            using VkDescriptorPoolSizeArray = Utils::TCDynamicArray< VkDescriptorPoolSize, DESCRIPTOR_TYPE_COUNT >;

            public:

                CDescriptorSetManager(CDeviceContext* pCtx);
                ~CDescriptorSetManager();

                Result Create(const SDescriptorSetManagerDesc& Desc);
                void Destroy();

                DescriptorSetRefPtr         CreateSet(const SDescriptorSetDesc& Desc);
                void                        DestroySet(DescriptorSetPtr pSet);
                DescriptorSetLayoutRefPtr   CreateLayout(const SDescriptorSetLayoutDesc& Desc);
                void                        DestroyLayout(DescriptorSetLayoutPtr pLayout);

                DescriptorSetRefPtr         GetDescriptorSet( DescriptorSetHandle hSet );
                DescriptorSetLayoutRefPtr   GetDescriptorSetLayout( DescriptorSetLayoutHandle hLayout );

            protected:

                void                        _DestroyPool(VkDescriptorPool* pVkOut);
                void                        _DestroyLayout( CDescriptorSetLayout** ppInOut );

            protected:

                CDeviceContext*             m_pCtx;
                DescSetBufferArray          m_avDescSetBuffers[ DESCRIPTOR_TYPE_COUNT ];
                DescSetLayoutBuffer         m_DescSetLayoutBuffer;
                VkDescriptorPoolArray       m_hDescPools;
                DescSetLayoutMemoryPool     m_DescSetLayoutMemMgr;
                DescSetMemoryPool           m_DescSetMemMgr;
        };
    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER