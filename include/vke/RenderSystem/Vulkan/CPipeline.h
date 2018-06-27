#pragma once

#include "Common.h"
#include "RenderSystem/Vulkan/Vulkan.h"
#include "RenderSystem/Resources/CShader.h"

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

                SVkCreateDesc       m_CreateDesc;
                VkPipeline          m_vkPipeline = VK_NULL_HANDLE;
                CPipelineManager*   m_pMgr;
                PIPELINE_TYPE       m_type;
        };

        using PipelinePtr = Utils::TCWeakPtr< CPipeline >;
        using PipelineRefPtr = Utils::TCObjectSmartPtr< CPipeline >;

    } // RenderSystem
} // VKE