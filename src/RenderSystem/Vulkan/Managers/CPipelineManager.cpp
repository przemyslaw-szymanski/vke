#include "RenderSystem/Vulkan/Managers/CPipelineManager.h"
#if VKE_VULKAN_RENDERER
#include "RenderSystem/CDeviceContext.h"
#include "RenderSystem/CRenderSystem.h"
#include "CVkEngine.h"
#include "Core/Threads/CThreadPool.h"
#include "RenderSystem/Resources/CShader.h"
#include "RenderSystem/CRenderPass.h"

namespace VKE
{
    namespace RenderSystem
    {
        TaskState PipelineManagerTasks::SCreatePipelineTask::_OnStart(uint32_t tid)
        {
            return TaskStateBits::OK;
        }

        void PipelineManagerTasks::SCreatePipelineTask::_OnGet(void**)
        {

        }

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
            if( VKE_SUCCEEDED( m_PipelineMemMgr.Create( Desc.maxPipelineCount, sizeof( CPipeline ), 1 ) ) )
            {
                if( VKE_SUCCEEDED( m_PipelineLayoutMemMgr.Create( Desc.maxPipelineLayoutCount, sizeof( CPipelineLayout ), 1 ) ) )
                {
                    res = VKE_OK;
                }
                else
                {
                    goto ERR;
                }
            }
            {
                SDescriptorSetLayoutDesc SetLayoutDesc;
                SDescriptorSetLayoutDesc::Binding Binding;
                SetLayoutDesc.vBindings.PushBack(Binding);
                DescriptorSetLayoutRefPtr pDescSetLayout = m_pCtx->CreateDescriptorSetLayout( SetLayoutDesc );
                SPipelineLayoutDesc LayoutDesc;
                LayoutDesc.vDescriptorSetLayouts.PushBack( pDescSetLayout );
                PipelineLayoutRefPtr pLayout = CreateLayout( LayoutDesc );
            }
            
            return res;
ERR:
            Destroy();
            return res;
        }

        void CPipelineManager::Destroy()
        {
            m_PipelineLayoutMemMgr.Destroy();
            m_PipelineMemMgr.Destroy();
        }

        Result CPipelineManager::_CreatePipeline(const SPipelineDesc& Desc, CPipeline::SVkCreateDesc* pOut,
            VkPipeline* pVkOut)
        {
            Result res = VKE_OK;
            Vulkan::InitInfo( &pOut->ColorBlendState, VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO );
            pOut->ColorBlendState.attachmentCount = Desc.Blending.vBlendStates.GetCount();
            pOut->ColorBlendState.pAttachments = nullptr;
            Utils::TCDynamicArray< VkPipelineColorBlendAttachmentState, Config::RenderSystem::Pipeline::MAX_BLEND_STATE_COUNT > vVkBlendStates;
            const bool isGraphics = Desc.Shaders.pComputeShader.IsNull();

            PipelineLayoutPtr pLayout;
            if( Desc.hLayout == NULL_HANDLE )
            {
                auto& pDescLayout = m_pCtx->CreateDescriptorSetLayout( DEFAULT_CONSTRUCTOR_INIT );
                pLayout = CreateLayout( pDescLayout );
            }

            RenderPassHandle hPass = Desc.hRenderPass;
            if( Desc.hRenderPass == NULL_HANDLE )
            {
                CRenderPass* pPass = m_pCtx->GetRenderPass( NULL_HANDLE );
                hPass.handle = reinterpret_cast< handle_t >( pPass->GetNative() );
            }

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
                if( !vBlendStates.IsEmpty() )
                {
                    vVkBlendStates.Resize( vBlendStates.GetCount()) ;
                    auto& State = pOut->ColorBlendState;
                    Vulkan::InitInfo( &State, VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO );
                    {
                        for( uint32_t i = 0; i < vBlendStates.GetCount(); ++i )
                        {
                            auto& vkBlendState = vVkBlendStates[i];
                            vkBlendState.alphaBlendOp = Vulkan::Map::BlendOp( vBlendStates[i].Alpha.operation );
                            vkBlendState.blendEnable = vBlendStates[i].enable;
                            vkBlendState.colorBlendOp = Vulkan::Map::BlendOp( vBlendStates[i].Color.operation );
                            vkBlendState.colorWriteMask = Vulkan::Map::ColorComponent( vBlendStates[i].writeMask );
                            vkBlendState.dstAlphaBlendFactor = Vulkan::Map::BlendFactor( vBlendStates[i].Alpha.dst );
                            vkBlendState.dstColorBlendFactor = Vulkan::Map::BlendFactor( vBlendStates[i].Color.dst );
                            vkBlendState.srcAlphaBlendFactor = Vulkan::Map::BlendFactor( vBlendStates[i].Alpha.src );
                            vkBlendState.srcColorBlendFactor = Vulkan::Map::BlendFactor( vBlendStates[i].Color.src );
                        }

                        State.pAttachments = &vVkBlendStates[0];
                        State.attachmentCount = vVkBlendStates.GetCount();
                        State.flags = 0;
                        State.logicOp = Vulkan::Map::LogicOperation( Desc.Blending.logicOperation );
                        State.logicOpEnable = Desc.Blending.logicOperation != 0;
                        memset( State.blendConstants, 0, sizeof(float) * 4 );
                    }
                }
                if( Desc.Blending.enable )
                {
                    pOut->GraphicsCreateInfo.pColorBlendState = &pOut->ColorBlendState;
                }
            }

            if( Desc.DepthStencil.enable )
            {
                auto& State = pOut->DepthStencilState;
                Vulkan::InitInfo( &State, VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO );
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
                State.depthBoundsTestEnable = Desc.DepthStencil.DepthBounds.enable;
                State.depthCompareOp = Vulkan::Map::CompareOperation(Desc.DepthStencil.depthFunction);
                State.depthTestEnable = Desc.DepthStencil.enableDepthTest;
                State.depthWriteEnable = Desc.DepthStencil.enableDepthWrite;
                State.maxDepthBounds = Desc.DepthStencil.DepthBounds.max;
                State.minDepthBounds = Desc.DepthStencil.DepthBounds.min;
                State.stencilTestEnable = Desc.DepthStencil.enableStencilTest;

                pOut->GraphicsCreateInfo.pDepthStencilState = &State;
            }
            {
                auto& State = pOut->DynamicState;
                Vulkan::InitInfo( &State, VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO );
                {
                    State.dynamicStateCount = 0;
                    State.pDynamicStates = nullptr;
                }
            }
            if( Desc.Multisampling.enable )
            {
                auto& VkState = pOut->MultisampleState;
                Vulkan::InitInfo( &VkState, VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO );
                {
                    VkState.alphaToCoverageEnable = false;
                    VkState.alphaToOneEnable = false;
                    VkState.minSampleShading = 0;
                    VkState.pSampleMask = nullptr;
                    VkState.rasterizationSamples = Vulkan::Map::SampleCount(Desc.Multisampling.sampleCount);
                    VkState.sampleShadingEnable = false;
                    pOut->GraphicsCreateInfo.pMultisampleState = &VkState;
                }
            }

            {
                auto& VkState = pOut->RasterizationState;
                Vulkan::InitInfo( &VkState, VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO );
                {              
                    VkState.cullMode = Vulkan::Map::CullMode( Desc.Rasterization.Polygon.cullMode );
                    VkState.depthBiasClamp = Desc.Rasterization.Depth.biasClampFactor;
                    VkState.depthBiasConstantFactor = Desc.Rasterization.Depth.biasConstantFactor;
                    VkState.depthBiasEnable = Desc.Rasterization.Depth.biasConstantFactor != 0.0f;
                    VkState.depthBiasSlopeFactor = Desc.Rasterization.Depth.biasSlopeFactor;
                    VkState.depthClampEnable = Desc.Rasterization.Depth.enableClamp;
                    VkState.frontFace = Vulkan::Map::FrontFace( Desc.Rasterization.Polygon.frontFace );
                    VkState.lineWidth = 1;
                    VkState.polygonMode = Vulkan::Map::PolygonMode( Desc.Rasterization.Polygon.mode );
                    VkState.rasterizerDiscardEnable = true;

                    pOut->GraphicsCreateInfo.pRasterizationState = &pOut->RasterizationState;
                }
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
                auto pShader = Desc.Shaders.pTessDomainShader;
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
                auto pShader = Desc.Shaders.pTessHullShader;
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
                    VkState.flags = 0;
                    VkState.patchControlPoints = 0;
                    pOut->GraphicsCreateInfo.pTessellationState = &pOut->TessellationState;
                }
            }

            {
                auto& VkState = pOut->InputAssemblyState;
                Vulkan::InitInfo( &VkState, VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO );
                {
                    VkState.flags = 0;
                    VkState.primitiveRestartEnable = Desc.InputLayout.enablePrimitiveRestart;
                    VkState.topology = Vulkan::Map::PrimitiveTopology( Desc.InputLayout.topology );
                    pOut->GraphicsCreateInfo.pInputAssemblyState = &pOut->InputAssemblyState;
                }
            }
            {
                auto& VkState = pOut->VertexInputState;
                const auto& vAttribs = Desc.InputLayout.vVertexAttributes;
                if( !vAttribs.IsEmpty() )
                {
                    Vulkan::InitInfo( &VkState, VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO );
                    {
                        Utils::TCDynamicArray< VkVertexInputAttributeDescription, Config::RenderSystem::Pipeline::MAX_VERTEX_ATTRIBUTE_COUNT > vVkAttribs;
                        Utils::TCDynamicArray< VkVertexInputBindingDescription, Config::RenderSystem::Pipeline::MAX_VERTEX_INPUT_BINDING_COUNT > vVkBindings;
                        vVkAttribs.Resize( vAttribs.GetCount() );
                        vVkBindings.Resize( vAttribs.GetCount() );
                        SDescriptorSetLayoutDesc::BindingArray vBindings;
                        vBindings.Resize( vAttribs.GetCount() );
                        for( uint32_t i = 0; i < vAttribs.GetCount(); ++i )
                        {
                            auto& vkAttrib = vVkAttribs[ i ];
                            vkAttrib.binding = vAttribs[ i ].binding;
                            vkAttrib.format = Vulkan::Map::Format( vAttribs[ i ].format );
                            vkAttrib.location = vAttribs[ i ].location;
                            vkAttrib.offset = vAttribs[ i ].offset;

                            auto& vkBinding = vVkBindings[ i ];
                            vkBinding.binding = vAttribs[ i ].binding;
                            vkBinding.inputRate = Vulkan::Map::InputRate( vAttribs[i].inputRate );
                            vkBinding.stride = vAttribs[ i ].stride;
                        }

                        VkState.pVertexAttributeDescriptions = &vVkAttribs[0];
                        VkState.pVertexBindingDescriptions = &vVkBindings[0];
                        VkState.vertexAttributeDescriptionCount = vVkAttribs.GetCount();
                        VkState.vertexBindingDescriptionCount = vVkBindings.GetCount();

                        pOut->GraphicsCreateInfo.pVertexInputState = &VkState;
                    }
                }
                else
                {
                    VKE_LOG_ERR( "GraphicsPipeline has no InputLayout.VertexAttribute." );
                    res = VKE_FAIL;
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

            if( isGraphics )
            {
                pOut->GraphicsCreateInfo.layout = reinterpret_cast< VkPipelineLayout >( pLayout->GetHandle() );
                pOut->GraphicsCreateInfo.renderPass = reinterpret_cast< VkRenderPass >( hPass.handle );

                VkGraphicsPipelineCreateInfo& VkInfo = pOut->GraphicsCreateInfo;
                *pVkOut = ( m_pCtx->_GetDevice().CreatePipeline( VK_NULL_HANDLE, VkInfo, nullptr ) );
            }
            else
            {
                VkComputePipelineCreateInfo& VkInfo = pOut->ComputeCreateInfo;
                *pVkOut = m_pCtx->_GetDevice().CreatePipeline( VK_NULL_HANDLE, VkInfo, nullptr );
            }

            if ((*pVkOut) == VK_NULL_HANDLE)
            {
                res = VKE_FAIL;
            }

END:
            return res;
        }

        PipelineRefPtr CPipelineManager::CreatePipeline(const SPipelineCreateDesc& Desc)
        {
            PipelinePtr pPipeline;
            if (Desc.Create.async)
            {
                PipelineManagerTasks::SCreatePipelineTask* pTask;
                {
                    Threads::ScopedLock l( m_CreatePipelineSyncObj );
                    pTask = CreatePipelineTaskPoolHelper::GetTask( &m_CreatePipelineTaskPool );
                }
                pTask->pMgr = this;
                pTask->Desc = Desc;
                m_pCtx->GetRenderSystem()->GetEngine()->GetThreadPool()->AddTask( pTask );
            }
            else
            {
                _CreatePipelineTask( Desc.Pipeline, &pPipeline );
            }
            return PipelineRefPtr( pPipeline );
        }

        Result CPipelineManager::_CreatePipelineTask(const SPipelineDesc& Desc, PipelinePtr* ppOut)
        {
            Result res = VKE_FAIL;
            hash_t hash = _CalcHash( Desc );
            CPipeline* pPipeline = nullptr;
            PipelineBuffer::MapIterator Itr;
            if( !m_Buffer.Get( hash, &pPipeline, &Itr ) )
            {
                if( VKE_SUCCEEDED( Memory::CreateObject( &m_PipelineMemMgr, &pPipeline, this ) ) )
                {
                    if( m_Buffer.Add( pPipeline, hash, Itr ) )
                    {
                    }
                    else
                    {
                        VKE_LOG_ERR("Unable to add pipeline object to the buffer.");
                    }
                }
                else
                {
                    VKE_LOG_ERR("Unable to allocate memory for pipeline object.");
                }
            }
            if( pPipeline )
            {
                if( VKE_SUCCEEDED( _CreatePipeline( Desc, &pPipeline->m_CreateDesc, &pPipeline->m_vkPipeline ) ) &&
                    VKE_SUCCEEDED( pPipeline->Init( Desc ) ) )
                {
                    res = VKE_OK;
                }
                else
                {
                    m_Buffer.Free( pPipeline );
                    pPipeline = nullptr;
                }
            }

            return res;
        }

        hash_t CPipelineManager::_CalcHash(const SPipelineDesc& Desc)
        {
            hash_t hash = 0;
            /*hash ^= reinterpret_cast< uint64_t >( Desc.Shaders.pComputeShader.Get() );
            hash ^= reinterpret_cast< uint64_t >( Desc.Shaders.pVertexShader.Get() );
            hash ^= reinterpret_cast< uint64_t >( Desc.Shaders.pTessHullShader.Get() );
            hash ^= reinterpret_cast< uint64_t >( Desc.Shaders.pTessDomainShader.Get() );
            hash ^= reinterpret_cast< uint64_t >( Desc.Shaders.pGeometryShader.Get() );
            hash ^= reinterpret_cast< uint64_t >( Desc.Shaders.pPpixelShader.Get() );*/
            Hash::Combine( &hash, Desc.Shaders.pComputeShader.Get() );
            Hash::Combine( &hash, Desc.Shaders.pVertexShader.Get() );
            Hash::Combine( &hash, Desc.Shaders.pTessDomainShader.Get() );
            Hash::Combine( &hash, Desc.Shaders.pTessHullShader.Get() );
            Hash::Combine( &hash, Desc.Shaders.pGeometryShader.Get() );
            Hash::Combine( &hash, Desc.Shaders.pPpixelShader.Get() );

            hash_t blendingHash = 0;
            hash_t rasterHash = 0;
            hash_t viewportHash = 0;
            hash_t msHash = 0;
            hash_t dsHash = 0;
            hash_t ilHash = 0;

#define FLOAT_TO_INT(f) (static_cast<int32_t>((f)*1000))

            {
                //blendingHash = Desc.Blending.enableLogicOperation ^ ( Desc.Blending.logicOperation << 1);
                Hash::Combine( &hash, Desc.Blending.enable );
                Hash::Combine( &hash, Desc.Blending.enableLogicOperation );
                Hash::Combine( &hash, Desc.Blending.logicOperation );
                Hash::Combine( &hash, Desc.Blending.vBlendStates.GetCount() );

                for (uint32_t i = 0; i < Desc.Blending.vBlendStates.GetCount(); ++i)
                {
                    const auto& State = Desc.Blending.vBlendStates[i];
                    /*hash_t hash = (State.enable << 1) ^ ( State.writeMask << 1 );
                    hash ^= (State.Color.src << 1) ^ (State.Color.dst << 1) ^ (State.Color.operation << 1);
                    hash ^= (State.Alpha.src << 1) ^ (State.Alpha.dst << 1) ^ (State.Alpha.operation << 1);
                    blendingHash ^= hash << 1;*/
                    Hash::Combine( &hash, State.Alpha.dst );
                    Hash::Combine( &hash, State.Alpha.operation );
                    Hash::Combine( &hash, State.Alpha.src );
                    Hash::Combine( &hash, State.Color.dst );
                    Hash::Combine( &hash, State.Color.operation );
                    Hash::Combine( &hash, State.Color.src );
                    Hash::Combine( &hash, State.enable );
                    Hash::Combine( &hash, State.writeMask );
                }
            }
            {
                const auto& Raster = Desc.Rasterization;
                //rasterHash = FLOAT_TO_INT(Raster.Depth.biasClampFactor) ^ ( FLOAT_TO_INT(Raster.Depth.biasConstantFactor) << 1 );
                //rasterHash ^= ( FLOAT_TO_INT(Raster.Depth.biasSlopeFactor) << 1 ) ^ ( FLOAT_TO_INT(Raster.Depth.enableClamp) << 1 );
                //rasterHash ^= (Raster.Polygon.cullMode << 1) ^ (Raster.Polygon.frontFace << 1) ^ (Raster.Polygon.frontFace << 1);
                Hash::Combine( &hash, Raster.Depth.biasClampFactor );
                Hash::Combine( &hash, Raster.Depth.biasConstantFactor );
                Hash::Combine( &hash, Raster.Depth.biasClampFactor );
                Hash::Combine( &hash, Raster.Depth.enableClamp );
                Hash::Combine( &hash, Raster.Polygon.cullMode );
                Hash::Combine( &hash, Raster.Polygon.frontFace );
                Hash::Combine( &hash, Raster.Polygon.mode );
            }
            {
                //Viewport
            }
            {
                // Multisampling
                const auto& MS = Desc.Multisampling;
                Hash::Combine( &hash, MS.enable );
                Hash::Combine( &hash, MS.sampleCount );
            }
            {
                const auto& DS = Desc.DepthStencil;
                hash_t tmp = 0;
                {
                    const auto& Face = DS.BackFace;
                    Hash::Combine( &hash, Face.compareMask );
                    Hash::Combine( &hash, Face.compareOp );
                    Hash::Combine( &hash, Face.depthFailOp );
                    Hash::Combine( &hash, Face.failOp );
                    Hash::Combine( &hash, Face.passOp );
                    Hash::Combine( &hash, Face.reference );
                    Hash::Combine( &hash, Face.writeMask );
                }
                {
                    const auto& Face = DS.FrontFace;
                    Hash::Combine( &hash, Face.compareMask );
                    Hash::Combine( &hash, Face.compareOp );
                    Hash::Combine( &hash, Face.depthFailOp );
                    Hash::Combine( &hash, Face.failOp );
                    Hash::Combine( &hash, Face.passOp );
                    Hash::Combine( &hash, Face.reference );
                    Hash::Combine( &hash, Face.writeMask );
                }
                {
                    const auto& DB = DS.DepthBounds;
                    Hash::Combine( &hash, DB.enable );
                    Hash::Combine( &hash, DB.max );
                    Hash::Combine( &hash, DB.min );
                }
                Hash::Combine( &hash, DS.depthFunction );
                Hash::Combine( &hash, DS.enable );
                Hash::Combine( &hash, DS.enableDepthTest );
                Hash::Combine( &hash, DS.enableDepthWrite );
                Hash::Combine( &hash, DS.enableStencilTest );
                Hash::Combine( &hash, DS.enableStencilWrite );
            }
            {
                const auto& IL = Desc.InputLayout;
                for( uint32_t i = 0; i < IL.vVertexAttributes.GetCount(); ++i )
                {
                    const auto& Attr = IL.vVertexAttributes[ i ];
                    Hash::Combine( &hash, Attr.binding );
                    Hash::Combine( &hash, Attr.format );
                    Hash::Combine( &hash, Attr.inputRate );
                    Hash::Combine( &hash, Attr.location );
                    Hash::Combine( &hash, Attr.offset );
                    Hash::Combine( &hash, Attr.pName );
                    Hash::Combine( &hash, Attr.stride );
                }
                Hash::Combine( &hash, IL.vVertexAttributes.GetCount() );
            }

            return hash;
        }

        hash_t CPipelineManager::_CalcHash(const SPipelineLayoutDesc& Desc)
        {
            hash_t hash = 0;
            for( uint32_t i = 0; i < Desc.vDescriptorSetLayouts.GetCount(); ++i )
            {
                hash ^= ( reinterpret_cast< uint64_t >( Desc.vDescriptorSetLayouts[ i ]->GetNative() ) << 1 );
            }
            return hash;
        }

        PipelineLayoutRefPtr CPipelineManager::CreateLayout(const SPipelineLayoutDesc& Desc)
        {
            CPipelineLayout* pLayout = nullptr;
            hash_t hash = _CalcHash( Desc );
            PipelineLayoutBuffer::MapIterator Itr;
            if( !m_LayoutBuffer.Get( hash, &pLayout, &Itr ) )
            {
                if( VKE_SUCCEEDED( Memory::CreateObject( &m_PipelineLayoutMemMgr, &pLayout, this ) ) )
                {
                    if( m_LayoutBuffer.Add( pLayout, hash, Itr ) )
                    {
                        
                    }
                    else
                    {
                        VKE_LOG_ERR("Unable to add CPipelineLayout object to the resource buffer.");
                        Memory::DestroyObject( &m_PipelineLayoutMemMgr, &pLayout );
                    }
                }
            }
            if( pLayout )
            {
                if( pLayout->GetHandle() == NULL_HANDLE )
                {
                    VkPipelineLayout vkLayout;
                    VkPipelineLayoutCreateInfo ci;
                    Vulkan::InitInfo( &ci, VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO );
                    {
                        VKE_ASSERT( !Desc.vDescriptorSetLayouts.IsEmpty(), "There should be at least one DescriptorSetLayout." );
                        ci.setLayoutCount = Desc.vDescriptorSetLayouts.GetCount();

                        static const auto MAX_COUNT = Config::RenderSystem::Pipeline::MAX_PIPELINE_LAYOUT_DESCRIPTOR_SET_COUNT;
                        Utils::TCDynamicArray< VkDescriptorSetLayout, MAX_COUNT > vVkDescLayouts;
                        for( uint32_t i = 0; i < ci.setLayoutCount; ++i )
                        {
                            vVkDescLayouts.PushBack( Desc.vDescriptorSetLayouts[ i ]->GetNative() );
                        }
                        ci.pSetLayouts = &vVkDescLayouts[0];
                        ci.pPushConstantRanges = nullptr;
                        ci.pushConstantRangeCount = 0;

                        VkResult res = m_pCtx->_GetDevice().CreateObject( ci, nullptr, &vkLayout );
                        VK_ERR( res );
                        if( res == VK_SUCCESS )
                        {
                            pLayout->Init( Desc );
                            pLayout->m_hObjHandle = reinterpret_cast< handle_t >( vkLayout );
                        }
                        else
                        {
                            VKE_LOG_ERR("Unable to create VkPipelineLayout object.");
                            pLayout = nullptr;
                        }
                    }
                }
            }
            return PipelineLayoutRefPtr( pLayout );
        }

        PipelinePtr CPipelineManager::_CreateCurrPipeline()
        {
            if( m_CurrPipelineDirty )
            {
                m_CurrPipelineDesc.Create.async = false;
                m_pCurrPipeline = CreatePipeline( m_CurrPipelineDesc );
                m_CurrPipelineDirty = false;
            }
            return m_pCurrPipeline;
        }

        void CPipelineManager::SetShader( ShaderPtr pShader )
        {
            m_CurrPipelineDesc.Pipeline.Shaders.aShaders[ pShader->GetDesc().type ] = pShader;
            m_CurrPipelineDirty = true;
        }
    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER