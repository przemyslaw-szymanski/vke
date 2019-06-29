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
        class VKE_API CPipelineLayout
        {
            friend class CPipelineManager;
            VKE_ADD_DDI_OBJECT( DDIPipelineLayout );
            VKE_DECL_BASE_OBJECT( PipelineLayoutHandle );

            public:

                CPipelineLayout(CPipelineManager* pMgr) : m_pMgr( pMgr ) {}
                Result Init(const SPipelineLayoutDesc& Desc);

                const SPipelineLayoutDesc& GetDesc() const { return m_Desc; }

            protected:

                SPipelineLayoutDesc m_Desc;
                CPipelineManager*   m_pMgr;
        };

        using PipelineLayoutPtr = Utils::TCWeakPtr< CPipelineLayout >;
        using PipelineLayoutRefPtr = Utils::TCObjectSmartPtr< CPipelineLayout >;

        class VKE_API CPipeline
        {
            friend class CPipelineManager;
            friend class CDeviceContext;
            friend class CGraphicsContext;
            friend class CComputeContext;
            friend class CCommandBuffer;

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

            VKE_ADD_DDI_OBJECT( DDIPipeline );
            VKE_DECL_BASE_OBJECT( PipelineHandle );
            VKE_DECL_BASE_RESOURCE();

            public:

                        CPipeline(CPipelineManager*);
                        ~CPipeline();

                Result          Init(const SPipelineDesc& Desc);
                
                PIPELINE_TYPE   GetType() const { return m_type; }

                const SPipelineDesc& GetDesc() const { return m_Desc; }

                PipelineLayoutPtr   GetLayout() const { return m_pLayout; }

            protected:

                void            _Destroy();

            protected:

                SVkCreateDesc           m_CreateDesc;
                SPipelineDesc           m_Desc;
                //VkPipeline              m_vkPipeline = VK_NULL_HANDLE;
                PipelineLayoutRefPtr    m_pLayout;
                CPipelineManager*       m_pMgr;
                PIPELINE_TYPE           m_type;
                bool                    m_isActive = false;
        };

        using PipelinePtr = Utils::TCWeakPtr< CPipeline >;
        using PipelineRefPtr = Utils::TCObjectSmartPtr< CPipeline >;

    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER