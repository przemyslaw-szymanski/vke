#pragma once
#if VKE_VULKAN_RENDERER
#include "Common.h"
#include "RenderSystem/Vulkan/Vulkan.h"
#include "RenderSystem/Resources/CShader.h"
#include "RenderSystem/CDescriptorSet.h"

namespace VKE
{
    namespace RenderSystem
    {
        struct PipelineTypes
        {
            enum TYPE
            {
                GRAPHICS,
                COMPUTE,
                _MAX_COUNT
            };
        };
        using PIPELINE_TYPE = PipelineTypes::TYPE;

        struct SPipelineLayoutDesc
        {
            static const auto MAX_COUNT = Config::RenderSystem::Pipeline::MAX_PIPELINE_LAYOUT_DESCRIPTOR_SET_COUNT;
            using DescSetLayoutArray = Utils::TCDynamicArray< DescriptorSetLayoutRefPtr, MAX_COUNT >;

            DescSetLayoutArray  vDescriptorSetLayouts;
        };

        class VKE_API CPipelineLayout : public Core::CObject
        {
            friend class CPipelineManager;
            public:

                CPipelineLayout(CPipelineManager* pMgr) : m_pMgr( pMgr ) {}
                Result Init(const SPipelineLayoutDesc& Desc);

                const VkPipelineLayout& GetNative() const { return m_vkLayout; }

            protected:

                SPipelineLayoutDesc m_Desc;
                CPipelineManager*   m_pMgr;
                VkPipelineLayout    m_vkLayout = VK_NULL_HANDLE;
        };

        using PipelineLayoutPtr = Utils::TCWeakPtr< CPipelineLayout >;
        using PipelineLayoutRefPtr = Utils::TCObjectSmartPtr< CPipelineLayout >;

        class VKE_API CPipeline : public Core::CObject
        {
            friend class CPipelineManager;
            friend class CDeviceContext;
            friend class CGraphicsContext;
            friend class CComputeContext;

            struct SVkCreateDesc
            {
                VkGraphicsPipelineCreateInfo            GraphicsCreateInfo = {};
                VkComputePipelineCreateInfo             ComputeCreateInfo = {};
                VkPipelineShaderStageCreateInfo         Stages[ ShaderTypes::_MAX_COUNT ] = {};
                VkPipelineVertexInputStateCreateInfo    VertexInputState;
                VkPipelineInputAssemblyStateCreateInfo  InputAssemblyState;
                VkPipelineTessellationStateCreateInfo   TessellationState;
                VkPipelineViewportStateCreateInfo       ViewportState;
                VkPipelineRasterizationStateCreateInfo  RasterizationState;
                VkPipelineMultisampleStateCreateInfo    MultisampleState;
                VkPipelineDepthStencilStateCreateInfo   DepthStencilState;
                VkPipelineColorBlendStateCreateInfo     ColorBlendState;
                VkPipelineDynamicStateCreateInfo        DynamicState;
            };

            public:

                        CPipeline(CPipelineManager*);
                        ~CPipeline();

                Result          Init(const SPipelineDesc& Desc);
                void            Destroy();
                PIPELINE_TYPE   GetType() const { return m_type; }


            protected:

                SVkCreateDesc           m_CreateDesc;
                VkPipeline              m_vkPipeline = VK_NULL_HANDLE;
                PipelineLayoutRefPtr    m_pLayout;
                CPipelineManager*       m_pMgr;
                PIPELINE_TYPE           m_type;
        };

        using PipelinePtr = Utils::TCWeakPtr< CPipeline >;
        using PipelineRefPtr = Utils::TCObjectSmartPtr< CPipeline >;

    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER