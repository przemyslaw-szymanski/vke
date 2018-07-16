#pragma once
#if VKE_VULKAN_RENDERER
#include "RenderSystem/Common.h"
#include "Core/Utils/TCSmartPtr.h"
#include "RenderSystem/Vulkan/Vulkan.h"

namespace VKE
{
    namespace RenderSystem
    {
        struct BindingTypes
        {
            enum TYPE
            {
                SAMPLER,
                COMBINED_IMAGE_SAMPLER,
                SAMPLED_TEXTURE,
                STORAGE_TEXTURE,
                UNIFORM_TEXEL_BUFFER,
                STORAGE_TEXEL_BUFFER,
                UNIFORM_BUFFER,
                STORAGE_BUFFER,
                UNIFORM_BUFFER_DYNAMIC,
                STORAGE_BUFFER_DYNAMIC,
                INPUT_ATTACHMENT,
                _MAX_COUNT
            };
        };
        using BINDING_TYPE = BindingTypes::TYPE;
        using DESCRIPTOR_SET_TYPE = BINDING_TYPE;
        using DescriptorSetTypes = BindingTypes;

        using PipelineStages = ShaderTypes;
        using PIPELINE_STAGE = SHADER_TYPE;

        struct SDescriptorSetLayoutDesc
        {
            struct Binding
            {
                uint32_t        idx = 0;
                BINDING_TYPE    type = BindingTypes::SAMPLED_TEXTURE;
                uint32_t        count = 1;
                PIPELINE_STAGES stages = PipelineStages::VERTEX;
            };

            using BindingArray = Utils::TCDynamicArray< Binding, Config::RenderSystem::Pipeline::MAX_DESCRIPTOR_BINDING_COUNT >;
            BindingArray    vBindings;
        };

        class CDescriptorSetLayout : public Core::CObject
        {
            friend class CDescriptorSetManager;
            friend class CDeviceContext;

            public:

                Result  Init(const SDescriptorSetLayoutDesc& Desc);

            protected:

                SDescriptorSetLayoutDesc    m_Desc;
                VkDescriptorSet             m_vkDescriptorSetLayout = VK_NULL_HANDLE;
        };

        using DeescriptorSetLayoutPtr = Utils::TCWeakPtr< CDescriptorSetLayout >;
        using DescriptorSetLayoutRefPtr = Utils::TCObjectSmartPtr< CDescriptorSetLayout >;

        struct SDescriptorSetDesc
        {

        };

        class CDescriptorSet : public Core::CObject
        {
            friend class CDeviceContext;
            friend class CDescriptorSetManager;

            public:

                Result  Init(const SDescriptorSetDesc& Desc);

            protected:

                DescriptorSetLayoutRefPtr   m_pLayout;
        };

        using DescriptorSetPtr = Utils::TCWeakPtr< CDescriptorSet >;
        using DescriptorSetRefPtr = Utils::TCObjectSmartPtr< CDescriptorSet >;

    } // RenderSystem
} // VKE

#endif // VKE_VULKAN_RENDERER