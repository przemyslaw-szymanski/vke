#pragma once

#include "Common.h"
#include "RenderSystem/Vulkan/Vulkan.h"
#include "RenderSystem/Resources/CShader.h"

namespace VKE
{
    namespace RenderSystem
    {
        class CPipeline
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

                Result  Init(const SPipelineDesc&);
                void    Destroy();

            protected:

                SVkCreateDesc       m_CreateDesc;
                VkPipeline          m_vkPipeline = VK_NULL_HANDLE;
                CPipelineManager*   m_pMgr;
        };

        using PipelinePtr = Utils::TCWeakPtr< CPipeline >;
        using PipelineRefPtr = Utils::TCObjectSmartPtr< CPipeline >;

    } // RenderSystem
} // VKE