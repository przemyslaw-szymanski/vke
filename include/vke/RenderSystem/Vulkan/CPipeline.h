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
        class VKE_API CPipelineLayout : public Core::CObject
        {
            friend class CPipelineManager;
            VKE_ADD_DDI_OBJECT( DDIPipelineLayout );
            public:

                CPipelineLayout(CPipelineManager* pMgr) : m_pMgr( pMgr ) {}
                Result Init(const SPipelineLayoutDesc& Desc);

            protected:

                SPipelineLayoutDesc m_Desc;
                CPipelineManager*   m_pMgr;
        };

        using PipelineLayoutPtr = Utils::TCWeakPtr< CPipelineLayout >;
        using PipelineLayoutRefPtr = Utils::TCObjectSmartPtr< CPipelineLayout >;

        class VKE_API CPipeline : public Core::CObject
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

            public:

                        CPipeline(CPipelineManager*);
                        ~CPipeline();

                Result          Init(const SPipelineDesc& Desc);
                void            Destroy();
                PIPELINE_TYPE   GetType() const { return m_type; }

            protected:


            protected:

                SVkCreateDesc           m_CreateDesc;
                //VkPipeline              m_vkPipeline = VK_NULL_HANDLE;
                PipelineLayoutRefPtr    m_pLayout;
                CPipelineManager*       m_pMgr;
                Threads::SyncObject     m_SyncObj;
                PIPELINE_TYPE           m_type;
                bool                    m_isActive = false;
        };

        using PipelinePtr = Utils::TCWeakPtr< CPipeline >;
        using PipelineRefPtr = Utils::TCObjectSmartPtr< CPipeline >;

    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER