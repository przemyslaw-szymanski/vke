#pragma once

#include "Common.h"
#include "RenderSystem/Vulkan/Vulkan.h"
#include "RenderSystem/Resources/CShader.h"

namespace VKE
{
    namespace RenderSystem
    {

        struct SPipelineDesc
        {
            SPipelineDesc()
            {
            }

            struct SShaders
            {
                ShaderRefPtr pVertexShader;
                ShaderRefPtr pHullShader;
                ShaderRefPtr pDomainShader;
                ShaderRefPtr pGeometryShader;
                ShaderRefPtr pPpixelShader;
                ShaderRefPtr pComputeShader;
            };

            struct SBlending
            {
                using BlendStateArray = Utils::TCDynamicArray< SBlendState, Config::RenderSystem::Pipeline::MAX_BLEND_STATE_COUNT >;
                BlendStateArray vBlendStates;
                LOGIC_OPERATION logicOperation = LogicOperations::NO_OPERATION;
                bool            enableLogicOperation = false;
                bool            enable = false;
            };

            struct SViewport
            {
                bool enable = false;
            };

            struct SRasterization
            {
                struct  
                {
                    POLYGON_MODE    mode = PolygonModes::FILL;
                    CULL_MODE       cullMode = CullModes::BACK;
                    FRONT_FACE      frontFace = FrontFaces::COUNTER_CLOCKWISE;
                } Polygon;
                
                struct 
                {
                    float   biasConstantFactor = 0.0f;
                    float   biasClampFactor = 0.0f;
                    float   biasSlopeFactor = 0.0f;
                    bool    enableClamp = false;
                } Depth;
            };

            struct SMultisampling
            {
                SAMPLE_COUNT    sampleCount = SampleCounts::SAMPLE_1;
                bool            enable = false;
            };

            struct SDepthStencil
            {
                bool                    enable = false;
                bool                    enableDepthTest = true;
                bool                    enableDepthWrite = true;
                bool                    enableStencilTest = false;
                bool                    enableStencilWrite = false;
                COMPARE_FUNCTION        depthFunction = CompareFunctions::LESS_EQUAL;
                SStencilOperationDesc   FrontFace;
                SStencilOperationDesc   BackFace;
                struct
                {
                    bool        enable = false;
                    float       min = 0.0f;
                    float       max = 0.0f;
                } DepthBounds;
            };

            struct SInputLayout
            {
                struct SVertexAttribute
                {
                    cstr_t              pName = "";
                    FORMAT              format;
                    uint16_t            binding;
                    uint16_t            location;
                    uint16_t            offset;
                    uint16_t            stride;
                    VERTEX_INPUT_RATE   inputRate = VertexInputRates::VERTEX;
                };
                using SVertexAttributeArray = Utils::TCDynamicArray< SVertexAttribute, Config::RenderSystem::Pipeline::MAX_VERTEX_ATTRIBUTE_COUNT >;
                SVertexAttributeArray   vVertexAttributes;
                PRIMITIVE_TOPOLOGY      topology;
                bool                    enablePrimitiveRestart = false;
            };

            struct STesselation
            {
                bool enable = false;
            };

            SShaders        Shaders;
            SBlending       Blending;
            SRasterization  Rasterization;
            SViewport       Viewport;
            SMultisampling  Multisampling;
            SDepthStencil   DepthStencil;
            SInputLayout    InputLayout;
            STesselation    Tesselation;
        };

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