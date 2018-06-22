#include "RenderSystem/Vulkan/Managers/CPipelineManager.h"
#include "RenderSystem/CDeviceContext.h"

namespace VKE
{
    namespace RenderSystem
    {
        CPipelineManager::CPipelineManager(CDeviceContext* pCtx) :
            m_pCtx( pCtx )
        {

        }

        CPipelineManager::~CPipelineManager()
        {
            Destroy();
        }

        Result CPipelineManager::Create(const SPipelineManagerDesc& Desc)
        {
            Result res = VKE_FAIL;
            if( VKE_SUCCEEDED( m_PipelineFreeListPool.Create( Desc.maxPipelineCount, sizeof( CPipeline ), 1 ) ) )
            {
                res = VKE_OK;
            }
            return res;
        }

        void CPipelineManager::Destroy()
        {

        }

        Result CPipelineManager::_CreatePipeline(const SPipelineDesc& Desc, CPipeline::SVkCreateDesc* pOut)
        {
            Result res = VKE_OK;
            Vulkan::InitInfo( &pOut->ColorBlendState, VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO );
            pOut->ColorBlendState.attachmentCount = Desc.Blending.vBlendStates.GetCount();
            pOut->ColorBlendState.pAttachments = nullptr;
            Utils::TCDynamicArray< VkPipelineColorBlendAttachmentState, Config::RenderSystem::Pipeline::MAX_BLEND_STATE_COUNT > vVkBlendStates;
            const bool isGraphics = Desc.Shaders.pComputeShader.IsNull();

            {
                auto& Info = pOut->GraphicsCreateInfo;
                Vulkan::InitInfo( &Info, VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO );
            }
            {
                auto& Info = pOut->ComputeCreateInfo;
                Vulkan::InitInfo( &Info, VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO );
            }
            
            {
                const auto& vBlendStates = Desc.Blending.vBlendStates;
                vVkBlendStates.Resize(vBlendStates.GetCount());

                for (uint32_t i = 0; i < vBlendStates.GetCount(); ++i)
                {
                    vVkBlendStates[i].alphaBlendOp = Vulkan::Map::BlendOp(vBlendStates[i].Alpha.operation);
                    vVkBlendStates[i].blendEnable = vBlendStates[i].enable;
                    vVkBlendStates[i].colorBlendOp = Vulkan::Map::BlendOp(vBlendStates[i].Color.operation);
                    vVkBlendStates[i].colorWriteMask = Vulkan::Map::ColorComponent(vBlendStates[i].writeMask);
                    vVkBlendStates[i].dstAlphaBlendFactor = Vulkan::Map::BlendFactor(vBlendStates[i].Alpha.dst);
                    vVkBlendStates[i].dstColorBlendFactor = Vulkan::Map::BlendFactor(vBlendStates[i].Color.dst);
                    vVkBlendStates[i].srcAlphaBlendFactor = Vulkan::Map::BlendFactor(vBlendStates[i].Alpha.src);
                    vVkBlendStates[i].srcColorBlendFactor = Vulkan::Map::BlendFactor(vBlendStates[i].Color.src);
                }

                pOut->ColorBlendState.pAttachments = &vVkBlendStates[0];
                pOut->ColorBlendState.logicOp = Vulkan::Map::LogicOperation(Desc.Blending.logicOperation);
                pOut->ColorBlendState.logicOpEnable = Desc.Blending.logicOperation != 0;

                if( Desc.Blending.enable )
                {
                    pOut->GraphicsCreateInfo.pColorBlendState = &pOut->ColorBlendState;
                }
            }
            if( Desc.DepthStencil.enable )
            {
                Vulkan::InitInfo(&pOut->DepthStencilState, VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO);
                {
                    auto& VkFace = pOut->DepthStencilState.back;
                    const auto& Face = Desc.DepthStencil.BackFace;

                    VkFace.compareMask = Face.compareMask;
                    VkFace.compareOp = Vulkan::Map::CompareOperation(Face.compareOp);
                    VkFace.depthFailOp = Vulkan::Map::StencilOperation(Face.depthFailOp);
                    VkFace.failOp = Vulkan::Map::StencilOperation(Face.failOp);
                    VkFace.passOp = Vulkan::Map::StencilOperation(Face.passOp);
                    VkFace.reference = Face.reference;
                    VkFace.writeMask = Face.writeMask;
                }
                {
                    auto& VkFace = pOut->DepthStencilState.front;
                    const auto& Face = Desc.DepthStencil.FrontFace;

                    VkFace.compareMask = Desc.DepthStencil.BackFace.compareMask;
                    VkFace.compareOp = Vulkan::Map::CompareOperation(Face.compareOp);
                    VkFace.depthFailOp = Vulkan::Map::StencilOperation(Face.depthFailOp);
                    VkFace.failOp = Vulkan::Map::StencilOperation(Face.failOp);
                    VkFace.passOp = Vulkan::Map::StencilOperation(Face.passOp);
                    VkFace.reference = Face.reference;
                    VkFace.writeMask = Face.writeMask;
                }
                pOut->DepthStencilState.depthBoundsTestEnable = Desc.DepthStencil.DepthBounds.enable;
                pOut->DepthStencilState.depthCompareOp = Vulkan::Map::CompareOperation(Desc.DepthStencil.depthFunction);
                pOut->DepthStencilState.depthTestEnable = Desc.DepthStencil.enableDepthTest;
                pOut->DepthStencilState.depthWriteEnable = Desc.DepthStencil.enableDepthWrite;
                pOut->DepthStencilState.maxDepthBounds = Desc.DepthStencil.DepthBounds.max;
                pOut->DepthStencilState.minDepthBounds = Desc.DepthStencil.DepthBounds.min;
                pOut->DepthStencilState.stencilTestEnable = Desc.DepthStencil.enableStencilTest;

                pOut->GraphicsCreateInfo.pDepthStencilState = &pOut->DepthStencilState;
            }
            Vulkan::InitInfo( &pOut->DynamicState, VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO );
            {
                auto& State = pOut->DynamicState;
                State.dynamicStateCount = 0;
                State.pDynamicStates = nullptr;
            }

            Vulkan::InitInfo( &pOut->InputAssemblyState, VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO );
            {
                auto& State = pOut->InputAssemblyState;
                State.primitiveRestartEnable = Desc.InputLayout.enablePrimitiveRestart;
                State.topology = Vulkan::Map::PrimitiveTopology( Desc.InputLayout.topology );

                pOut->GraphicsCreateInfo.pInputAssemblyState = &pOut->InputAssemblyState;
            }

            if (Desc.Multisampling.enable)
            {
                Vulkan::InitInfo(&pOut->MultisampleState, VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO);
                {
                    auto& VkState = pOut->MultisampleState;
                    VkState.alphaToCoverageEnable = false;
                    VkState.alphaToOneEnable = false;
                    VkState.minSampleShading = 0;
                    VkState.pSampleMask = nullptr;
                    VkState.rasterizationSamples = Vulkan::Map::SampleCount(Desc.Multisampling.sampleCount);
                    VkState.sampleShadingEnable = false;
                    pOut->GraphicsCreateInfo.pMultisampleState = &pOut->MultisampleState;
                }
            }

            Vulkan::InitInfo( &pOut->RasterizationState, VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO );
            {
                auto& VkState = pOut->RasterizationState;
                VkState.cullMode = Vulkan::Map::CullMode( Desc.Rasterization.Polygon.cullMode );
                VkState.depthBiasClamp = Desc.Rasterization.Depth.biasClampFactor;
                VkState.depthBiasConstantFactor = Desc.Rasterization.Depth.biasConstantFactor;
                VkState.depthBiasEnable = Desc.Rasterization.Depth.biasConstantFactor != 0.0f;
                VkState.depthBiasSlopeFactor = Desc.Rasterization.Depth.biasSlopeFactor;
                VkState.depthClampEnable = Desc.Rasterization.Depth.enableClamp;
                VkState.frontFace = Vulkan::Map::FrontFace( Desc.Rasterization.Polygon.frontFace );
                VkState.lineWidth = 1;
                VkState.polygonMode = Vulkan::Map::PolygonMode( Desc.Rasterization.Polygon.mode );
                VkState.rasterizerDiscardEnable = false;

                pOut->GraphicsCreateInfo.pRasterizationState = &pOut->RasterizationState;
            }

            VkShaderStageFlags vkShaderStages = 0;
            uint32_t stageCount = 0;
            {
                auto pShader = Desc.Shaders.pComputeShader;
                const auto& type = ShaderTypes::COMPUTE;
                if( pShader.IsValid() )
                {
                    auto& VkState = pOut->Stages[ type ];
                    Vulkan::InitInfo( &VkState, VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO );
                    {
                        res = pShader->Compile();
                        if( VKE_FAILED( res ) )
                        {
                            goto END;
                        }
                        VkState.module = pShader->GetNative();
                        VkState.pName = pShader->GetDesc().pEntryPoint;
                        VkState.stage = Vulkan::Map::ShaderStage( type );
                        VkState.pSpecializationInfo = nullptr;
                        vkShaderStages |= VkState.stage;
                        pOut->ComputeCreateInfo.stage = VkState;
                    }
                }
            }

            {
                auto pShader = Desc.Shaders.pDomainShader;
                const auto& type = ShaderTypes::TESS_DOMAIN;
                if( pShader.IsValid() )
                {
                    auto& VkState = pOut->Stages[ type ];
                    Vulkan::InitInfo( &VkState, VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO );
                    {
                        res = pShader->Compile();
                        if( VKE_FAILED( res ) )
                        {
                            goto END;
                        }
                        VkState.module = pShader->GetNative();
                        VkState.pName = pShader->GetDesc().pEntryPoint;
                        VkState.stage = Vulkan::Map::ShaderStage( type );
                        VkState.pSpecializationInfo = nullptr;
                        vkShaderStages |= VkState.stage;
                        stageCount++;
                    }
                }
            }

            {
                auto pShader = Desc.Shaders.pGeometryShader;
                const auto& type = ShaderTypes::GEOMETRY;
                if( pShader.IsValid() )
                {
                    auto& VkState = pOut->Stages[ type ];
                    Vulkan::InitInfo( &VkState, VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO );
                    {
                        res = pShader->Compile();
                        if( VKE_FAILED( res ) )
                        {
                            goto END;
                        }
                        VkState.module = pShader->GetNative();
                        VkState.pName = pShader->GetDesc().pEntryPoint;
                        VkState.stage = Vulkan::Map::ShaderStage( type );
                        VkState.pSpecializationInfo = nullptr;
                        vkShaderStages |= VkState.stage;
                        stageCount++;
                    }
                }
            }

            {
                auto pShader = Desc.Shaders.pHullShader;
                const auto& type = ShaderTypes::TESS_HULL;
                if( pShader.IsValid() )
                {
                    auto& VkState = pOut->Stages[ type ];
                    Vulkan::InitInfo( &VkState, VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO );
                    {
                        res = pShader->Compile();
                        if( VKE_FAILED( res ) )
                        {
                            goto END;
                        }
                        VkState.module = pShader->GetNative();
                        VkState.pName = pShader->GetDesc().pEntryPoint;
                        VkState.stage = Vulkan::Map::ShaderStage( type );
                        VkState.pSpecializationInfo = nullptr;
                        vkShaderStages |= VkState.stage;
                        stageCount++;
                    }
                }
            }

            {
                auto pShader = Desc.Shaders.pPpixelShader;
                const auto& type = ShaderTypes::PIXEL;
                if( pShader.IsValid() )
                {
                    auto& VkState = pOut->Stages[ type ];
                    Vulkan::InitInfo( &VkState, VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO );
                    {
                        res = pShader->Compile();
                        if( VKE_FAILED( res ) )
                        {
                            goto END;
                        }
                        VkState.module = pShader->GetNative();
                        VkState.pName = pShader->GetDesc().pEntryPoint;
                        VkState.stage = Vulkan::Map::ShaderStage( type );
                        VkState.pSpecializationInfo = nullptr;
                        vkShaderStages |= VkState.stage;
                        stageCount++;
                    }
                }
            }

            {
                auto pShader = Desc.Shaders.pVertexShader;
                const auto& type = ShaderTypes::VERTEX;
                if( pShader.IsValid() )
                {
                    auto& VkState = pOut->Stages[ type ];
                    Vulkan::InitInfo( &VkState, VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO );
                    {
                        res = pShader->Compile();
                        if( VKE_FAILED( res ) )
                        {
                            goto END;
                        }
                        VkState.module = pShader->GetNative();
                        VkState.pName = pShader->GetDesc().pEntryPoint;
                        VkState.stage = Vulkan::Map::ShaderStage( type );
                        VkState.pSpecializationInfo = nullptr;
                        vkShaderStages |= VkState.stage;
                        stageCount++;
                    }
                }
            }

            pOut->GraphicsCreateInfo.pStages = pOut->Stages;
            pOut->GraphicsCreateInfo.stageCount = stageCount;

            if( Desc.Tesselation.enable )
            {
                auto& VkState = pOut->TessellationState;
                Vulkan::InitInfo( &VkState, VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO );
                {
                    pOut->GraphicsCreateInfo.pTessellationState = &pOut->TessellationState;
                }
            }

            {
                auto& VkState = pOut->InputAssemblyState;
                Vulkan::InitInfo( &VkState, VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO );
                {
                    VkState.primitiveRestartEnable = Desc.InputLayout.enablePrimitiveRestart;
                    VkState.topology = Vulkan::Map::PrimitiveTopology( Desc.InputLayout.topology );
                    pOut->GraphicsCreateInfo.pInputAssemblyState = &pOut->InputAssemblyState;
                }
            }
            {
                auto& VkState = pOut->VertexInputState;
                const auto& vAttribs = Desc.InputLayout.vVertexAttributes;

                Vulkan::InitInfo( &VkState, VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO );
                {
                    Utils::TCDynamicArray< VkVertexInputAttributeDescription, Config::RenderSystem::Pipeline::MAX_VERTEX_ATTRIBUTE_COUNT > vVkAttribs;
                    Utils::TCDynamicArray< VkVertexInputBindingDescription, Config::RenderSystem::Pipeline::MAX_VERTEX_INPUT_BINDING_COUNT > vVkBindings;
                    vVkAttribs.Resize( vAttribs.GetCount() );
                    vVkBindings.Resize( vAttribs.GetCount() );
                    
                    for( uint32_t i = 0; i < vAttribs.GetCount(); ++i )
                    {
                        vVkAttribs[ i ].binding = vAttribs[ i ].binding;
                        vVkAttribs[ i ].format = Vulkan::Map::Format( vAttribs[ i ].format );
                        vVkAttribs[ i ].location = vAttribs[ i ].location;
                        vVkAttribs[ i ].offset = vAttribs[ i ].offset;

                        vVkBindings[ i ].binding = vAttribs[ i ].binding;
                        vVkBindings[ i ].inputRate = Vulkan::Map::InputRate( vAttribs[i].inputRate );
                    }

                    VkState.pVertexAttributeDescriptions = &vVkAttribs[0];
                    VkState.pVertexBindingDescriptions = &vVkBindings[0];
                    VkState.vertexAttributeDescriptionCount = vVkAttribs.GetCount();
                    VkState.vertexBindingDescriptionCount = vVkBindings.GetCount();

                    pOut->GraphicsCreateInfo.pVertexInputState = &VkState;
                }
            }

            if( Desc.Viewport.enable )
            {
                auto& VkState = pOut->ViewportState;
                Vulkan::InitInfo( &VkState, VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO );
                {
                    using VkViewportArray = Utils::TCDynamicArray< VkViewport, Config::RenderSystem::Pipeline::MAX_VIEWPORT_COUNT >;
                    using VkScissorArray = Utils::TCDynamicArray< VkRect2D, Config::RenderSystem::Pipeline::MAX_SCISSOR_COUNT >;
                    VkViewportArray vVkViewports;
                    VkScissorArray vVkScissors;

                    for( uint32_t i = 0; i < Desc.Viewport.vViewports.GetCount(); ++i )
                    {
                        const auto& Viewport = Desc.Viewport.vViewports[ i ];
                        VkViewport vkViewport;
                        vkViewport.x = Viewport.Position.x;
                        vkViewport.y = Viewport.Position.y;
                        vkViewport.width = Viewport.Size.width;
                        vkViewport.height = Viewport.Size.height;
                        vkViewport.minDepth = Viewport.MinMaxDepth.begin;
                        vkViewport.maxDepth = Viewport.MinMaxDepth.end;

                        vVkViewports.PushBack( vkViewport );
                    }

                    for( uint32_t i = 0; i < Desc.Viewport.vScissors.GetCount(); ++i )
                    {
                        const auto& Scissor = Desc.Viewport.vScissors[ i ];
                        VkRect2D vkScissor;
                        vkScissor.extent.width = Scissor.Size.width;
                        vkScissor.extent.height = Scissor.Size.height;
                        vkScissor.offset.x = Scissor.Position.x;
                        vkScissor.offset.y = Scissor.Position.y;

                        vVkScissors.PushBack( vkScissor );
                    }
                    VkState.pViewports = &vVkViewports[ 0 ];
                    VkState.viewportCount = vVkViewports.GetCount();
                    VkState.pScissors = &vVkScissors[ 0 ];
                    VkState.scissorCount = vVkScissors.GetCount();
                    pOut->GraphicsCreateInfo.pViewportState = &pOut->ViewportState;
                }
            }

            if (isGraphics)
            {
                VkGraphicsPipelineCreateInfo& VkInfo = pOut->GraphicsCreateInfo;
                m_pCtx->_GetDevice().CreatePipeline();
            }
            else
            {
                m_ICD.Device.Device.vkCreateComputePipelines();
            }

END:
            return res;
        }

        PipelinePtr CPipelineManager::CreatePipeline(const SPipelineDesc& Desc)
        {
            hash_t hash = _CalcHash( Desc );
            CPipeline* pPipeline = nullptr;
            PipelineBuffer::MapIterator Itr;
            if( !m_Buffer.Get( hash, &pPipeline, &Itr ) )
            {
                if( VKE_SUCCEEDED( Memory::CreateObject( &m_PipelineFreeListPool, &pPipeline, this ) ) )
                {
                    if( m_Buffer.Add( pPipeline, hash, Itr ) )
                    {

                    }
                    else
                    {
                        VKE_LOG_ERR( "Unable to add pipeline object to the buffer." );
                    }
                }
                else
                {
                    VKE_LOG_ERR( "Unable to allocate memory for pipeline object." );
                }
            }
            if( pPipeline )
            {
                if( VKE_SUCCEEDED( _CreatePipeline( Desc, &pPipeline->m_CreateDesc ) ) && VKE_SUCCEEDED( pPipeline->Init( Desc ) ) )
                {

                }
                else
                {
                    m_Buffer.Free( pPipeline );
                    pPipeline = nullptr;
                }
            }
            return PipelinePtr( pPipeline );
        }

        hash_t CPipelineManager::_CalcHash(const SPipelineDesc& Desc)
        {
            hash_t hash = 0;
            hash ^= reinterpret_cast< uint64_t >( Desc.Shaders.pComputeShader.Get() );
            hash ^= reinterpret_cast< uint64_t >( Desc.Shaders.pVertexShader.Get() );
            hash ^= reinterpret_cast< uint64_t >( Desc.Shaders.pHullShader.Get() );
            hash ^= reinterpret_cast< uint64_t >( Desc.Shaders.pDomainShader.Get() );
            hash ^= reinterpret_cast< uint64_t >( Desc.Shaders.pGeometryShader.Get() );
            hash ^= reinterpret_cast< uint64_t >( Desc.Shaders.pPpixelShader.Get() );

            hash_t blendingHash = 0;
            hash_t rasterHash = 0;
            hash_t viewportHash = 0;
            hash_t msHash = 0;
            hash_t dsHash = 0;
            hash_t ilHash = 0;

#define FLOAT_TO_INT(f) (static_cast<int32_t>((f)*1000))

            {
                blendingHash = Desc.Blending.enableLogicOperation ^ ( Desc.Blending.logicOperation << 1);
                for (uint32_t i = 0; i < Desc.Blending.vBlendStates.GetCount(); ++i)
                {
                    const auto& State = Desc.Blending.vBlendStates[i];
                    hash_t hash = (State.enable << 1) ^ ( State.writeMask << 1 );
                    hash ^= (State.Color.src << 1) ^ (State.Color.dst << 1) ^ (State.Color.operation << 1);
                    hash ^= (State.Alpha.src << 1) ^ (State.Alpha.dst << 1) ^ (State.Alpha.operation << 1);
                    blendingHash ^= hash << 1;
                }
            }
            {
                const auto& Raster = Desc.Rasterization;
                rasterHash = FLOAT_TO_INT(Raster.Depth.biasClampFactor) ^ ( FLOAT_TO_INT(Raster.Depth.biasConstantFactor) << 1 );
                rasterHash ^= ( FLOAT_TO_INT(Raster.Depth.biasSlopeFactor) << 1 ) ^ ( FLOAT_TO_INT(Raster.Depth.enableClamp) << 1 );
                rasterHash ^= (Raster.Polygon.cullMode << 1) ^ (Raster.Polygon.frontFace << 1) ^ (Raster.Polygon.frontFace << 1);
            }
            {
                //Viewport
            }
            {
                // Multisampling
                const auto& MS = Desc.Multisampling;
                msHash ^= (MS.sampleCount << 1);
            }
            {
                const auto& DS = Desc.DepthStencil;
                hash_t tmp = 0;
                {
                    const auto& Face = DS.BackFace;
                    tmp = ( Face.compareMask << 1 ) ^ ( Face.compareOp << 1 ) ^ ( Face.depthFailOp < 1 );
                    tmp ^= ( Face.failOp << 1 ) ^ ( Face.passOp << 1 ) ^ ( Face.reference << 1 ) ^ ( Face.writeMask << 1 );
                    dsHash ^= ( tmp << 1 );
                }
                {
                    const auto& Face = DS.FrontFace;
                    tmp = (Face.compareMask << 1) ^ (Face.compareOp << 1) ^ (Face.depthFailOp < 1);
                    tmp ^= (Face.failOp << 1) ^ (Face.passOp << 1) ^ (Face.reference << 1) ^ (Face.writeMask << 1);
                    dsHash ^= (tmp << 1);
                }
                {
                    const auto& DB = DS.DepthBounds;
                    tmp = ( DB.enable << 1 ) ^ ( FLOAT_TO_INT( DB.max ) << 1 ) ^ ( FLOAT_TO_INT( DB.min ) << 1 );
                    dsHash ^= ( tmp << 1 );
                }
                {
                    dsHash ^= ( DS.depthFunction << 1 ) ^ ( DS.enableDepthTest << 1 ) ^ ( DS.enableDepthWrite << 1 );
                    dsHash ^= ( DS.enableStencilTest << 1 ) ^ ( DS.enableStencilWrite << 1 );
                }
            }
            {
                const auto& IL = Desc.InputLayout;
                for( uint32_t i = 0; i < IL.vVertexAttributes.GetCount(); ++i )
                {
                    const auto& Attr = IL.vVertexAttributes[ i ];
                    ilHash ^= ( Attr.binding << 1 ) ^ ( Attr.format << 1 ) ^ ( Attr.inputRate << 1 );
                    ilHash ^= ( Attr.location << 1 ) ^ ( Attr.offset << 1 );
                    ilHash ^= ( Attr.stride << 1 );
                }
            }

            hash ^= ( ilHash << 1 ) ^ ( dsHash << 1 ) ^ ( msHash << 1 ) ^ ( rasterHash << 1 ) ^ ( blendingHash << 1 );
            hash ^= ( viewportHash << 1 );

            return hash;
        }
    }
}