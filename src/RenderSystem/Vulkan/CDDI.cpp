#include "RenderSystem/CDDI.h"
#if VKE_VULKAN_RENDERER
#include "RenderSystem/CDeviceContext.h"
#include "RenderSystem/CGraphicsContext.h"
#include "RenderSystem/Resources/CTexture.h"
#include "RenderSystem/Resources/CBuffer.h"
#include "RenderSystem/CRenderPass.h"
#include "Core/Platform/CWindow.h"
#include "Core/Managers/CFileManager.h"

#include "ThirdParty/glslang/glslang/Include/ShHandle.h"
#include "ThirdParty/glslang/glslang/Public/ShaderLang.h"
#include "ThirdParty/glslang/glslang/OSDependent/osinclude.h"
#include "ThirdParty/glslang/SPIRV/GlslangToSpv.h"


namespace VKE
{

#define DDI_CREATE_OBJECT(_name, _CreateInfo, _pAllocator, _phObj) \
    m_ICD.vkCreate##_name( m_hDevice, &(_CreateInfo), static_cast<const VkAllocationCallbacks*>(_pAllocator), (_phObj) );

#define DDI_DESTROY_OBJECT(_name, _phObj, _pAllocator) \
    if( (_phObj) && (*_phObj) != DDI_NULL_HANDLE ) \
    { \
        m_ICD.vkDestroy##_name( m_hDevice, (*_phObj), static_cast<const VkAllocationCallbacks*>(_pAllocator) ); \
        (*_phObj) = DDI_NULL_HANDLE; \
    }

    namespace RenderSystem
    {
        VkICD::Global CDDI::sGlobalICD;
        VkICD::Instance CDDI::sInstanceICD;
        handle_t CDDI::shICD = 0;
        VkInstance CDDI::sVkInstance = VK_NULL_HANDLE;
        CDDI::AdapterArray CDDI::svAdapters;


        namespace Map
        {
            VkImageType ImageType( RenderSystem::TEXTURE_TYPE type )
            {
                static const VkImageType aVkImageTypes[] =
                {
                    VK_IMAGE_TYPE_1D,
                    VK_IMAGE_TYPE_2D,
                    VK_IMAGE_TYPE_3D,
                };
                return aVkImageTypes[type];
            }

            VkImageViewType ImageViewType( RenderSystem::TEXTURE_VIEW_TYPE type )
            {
                static const VkImageViewType aVkTypes[] =
                {
                    VK_IMAGE_VIEW_TYPE_1D,
                    VK_IMAGE_VIEW_TYPE_2D,
                    VK_IMAGE_VIEW_TYPE_3D,
                    VK_IMAGE_VIEW_TYPE_1D_ARRAY,
                    VK_IMAGE_VIEW_TYPE_2D_ARRAY,
                    VK_IMAGE_VIEW_TYPE_CUBE,
                    VK_IMAGE_VIEW_TYPE_CUBE_ARRAY
                };
                return aVkTypes[type];
            }

            VkImageUsageFlags ImageUsage( RenderSystem::TEXTURE_USAGES usage )
            {
                /*static const VkImageUsageFlags aVkUsages[] =
                {
                VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_IMAGE_USAGE_STORAGE_BIT,
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT
                };
                return aVkUsages[ usage ];*/
                using namespace RenderSystem;
                VkImageUsageFlags flags = 0;
                if( usage & TextureUsages::SAMPLED )
                {
                    flags |= VK_IMAGE_USAGE_SAMPLED_BIT;
                }
                if( usage & TextureUsages::COLOR_RENDER_TARGET )
                {
                    flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
                }
                else if( usage & TextureUsages::DEPTH_STENCIL_RENDER_TARGET )
                {
                    flags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
                }
                if( usage & TextureUsages::STORAGE )
                {
                    flags |= VK_IMAGE_USAGE_STORAGE_BIT;
                }
                if( usage & TextureUsages::TRANSFER_DST )
                {
                    flags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
                }
                if( usage & TextureUsages::TRANSFER_SRC )
                {
                    flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
                }

                return flags;
            }

            VkImageLayout ImageLayout( RenderSystem::TEXTURE_LAYOUT layout )
            {
                static const VkImageLayout aVkLayouts[] =
                {
                    VK_IMAGE_LAYOUT_UNDEFINED,
                    VK_IMAGE_LAYOUT_GENERAL,
                    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                    VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
                };
                return aVkLayouts[layout];
            }

            VkImageAspectFlags ImageAspect( RenderSystem::TEXTURE_ASPECT aspect )
            {
                static const VkImageAspectFlags aVkAspects[] =
                {
                    // UNKNOWN
                    0,
                    // COLOR
                    VK_IMAGE_ASPECT_COLOR_BIT,
                    // DEPTH
                    VK_IMAGE_ASPECT_DEPTH_BIT,
                    // STENCIL
                    VK_IMAGE_ASPECT_STENCIL_BIT,
                    // DEPTH_STENCIL
                    VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT
                };
                return aVkAspects[aspect];
            }

            VkMemoryPropertyFlags MemoryPropertyFlags( RenderSystem::MEMORY_USAGES usages )
            {
                using namespace RenderSystem;
                VkMemoryPropertyFlags flags = 0;
                if( usages & MemoryUsages::CPU_ACCESS )
                {
                    flags |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
                }
                if( usages & MemoryUsages::CPU_CACHED )
                {
                    flags |= VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
                }
                if( usages & MemoryUsages::CPU_NO_FLUSH )
                {
                    flags |= VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
                }
                if( usages & MemoryUsages::GPU_ACCESS )
                {
                    flags |= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
                }
                return flags;
            }

            VkBlendOp BlendOp( const RenderSystem::BLEND_OPERATION& op )
            {
                static const VkBlendOp aVkOps[] =
                {
                    VK_BLEND_OP_ADD,
                    VK_BLEND_OP_SUBTRACT,
                    VK_BLEND_OP_REVERSE_SUBTRACT,
                    VK_BLEND_OP_MIN,
                    VK_BLEND_OP_MAX
                };
                return aVkOps[op];
            }

            VkColorComponentFlags ColorComponent( const RenderSystem::ColorComponent& component )
            {
                VkColorComponentFlags vkComponent = 0;
                if( component & RenderSystem::ColorComponents::ALL )
                {
                    vkComponent = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
                }
                else
                {
                    if( component & RenderSystem::ColorComponents::ALPHA )
                    {
                        vkComponent |= VK_COLOR_COMPONENT_A_BIT;
                    }
                    if( component & RenderSystem::ColorComponents::BLUE )
                    {
                        vkComponent |= VK_COLOR_COMPONENT_B_BIT;
                    }
                    if( component & RenderSystem::ColorComponents::GREEN )
                    {
                        vkComponent |= VK_COLOR_COMPONENT_G_BIT;
                    }
                    if( component & RenderSystem::ColorComponents::RED )
                    {
                        vkComponent |= VK_COLOR_COMPONENT_R_BIT;
                    }
                }
                return vkComponent;
            }

            VkBlendFactor BlendFactor( const RenderSystem::BLEND_FACTOR& factor )
            {
                static const VkBlendFactor aVkFactors[] =
                {
                    VK_BLEND_FACTOR_ZERO,
                    VK_BLEND_FACTOR_ONE,
                    VK_BLEND_FACTOR_SRC_COLOR,
                    VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR,
                    VK_BLEND_FACTOR_DST_COLOR,
                    VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR,
                    VK_BLEND_FACTOR_SRC_ALPHA,
                    VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                    VK_BLEND_FACTOR_DST_ALPHA,
                    VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA,
                    VK_BLEND_FACTOR_CONSTANT_COLOR,
                    VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR,
                    VK_BLEND_FACTOR_CONSTANT_ALPHA,
                    VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA,
                    VK_BLEND_FACTOR_SRC_ALPHA_SATURATE,
                    VK_BLEND_FACTOR_SRC1_COLOR,
                    VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR,
                    VK_BLEND_FACTOR_SRC1_ALPHA,
                    VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA
                };
                return aVkFactors[factor];
            }

            VkLogicOp LogicOperation( const RenderSystem::LOGIC_OPERATION& op )
            {
                static const VkLogicOp aVkOps[] =
                {
                    VK_LOGIC_OP_CLEAR,
                    VK_LOGIC_OP_AND,
                    VK_LOGIC_OP_AND_REVERSE,
                    VK_LOGIC_OP_COPY,
                    VK_LOGIC_OP_AND_INVERTED,
                    VK_LOGIC_OP_NO_OP,
                    VK_LOGIC_OP_XOR,
                    VK_LOGIC_OP_OR,
                    VK_LOGIC_OP_NOR,
                    VK_LOGIC_OP_EQUIVALENT,
                    VK_LOGIC_OP_INVERT,
                    VK_LOGIC_OP_OR_REVERSE,
                    VK_LOGIC_OP_COPY_INVERTED,
                    VK_LOGIC_OP_OR_INVERTED,
                    VK_LOGIC_OP_NAND,
                    VK_LOGIC_OP_SET
                };
                return aVkOps[op];
            }

            VkStencilOp StencilOperation( const RenderSystem::STENCIL_OPERATION& op )
            {
                static const VkStencilOp aVkOps[] =
                {
                    VK_STENCIL_OP_KEEP,
                    VK_STENCIL_OP_ZERO,
                    VK_STENCIL_OP_REPLACE,
                    VK_STENCIL_OP_INCREMENT_AND_CLAMP,
                    VK_STENCIL_OP_DECREMENT_AND_CLAMP,
                    VK_STENCIL_OP_INVERT,
                    VK_STENCIL_OP_INCREMENT_AND_WRAP,
                    VK_STENCIL_OP_DECREMENT_AND_WRAP
                };
                return aVkOps[op];
            }

            VkCompareOp CompareOperation( const RenderSystem::COMPARE_FUNCTION& op )
            {
                static const VkCompareOp aVkOps[] =
                {
                    VK_COMPARE_OP_NEVER,
                    VK_COMPARE_OP_LESS,
                    VK_COMPARE_OP_EQUAL,
                    VK_COMPARE_OP_LESS_OR_EQUAL,
                    VK_COMPARE_OP_GREATER,
                    VK_COMPARE_OP_NOT_EQUAL,
                    VK_COMPARE_OP_GREATER_OR_EQUAL,
                    VK_COMPARE_OP_ALWAYS
                };
                return aVkOps[op];
            }

            VkPrimitiveTopology PrimitiveTopology( const RenderSystem::PRIMITIVE_TOPOLOGY& topology )
            {
                static const VkPrimitiveTopology aVkTopologies[] =
                {
                    VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
                    VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
                    VK_PRIMITIVE_TOPOLOGY_LINE_STRIP,
                    VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
                    VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
                    VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN,
                    VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY,
                    VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY,
                    VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY,
                    VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY,
                    VK_PRIMITIVE_TOPOLOGY_PATCH_LIST
                };
                return aVkTopologies[topology];
            }

            VkSampleCountFlagBits SampleCount( const RenderSystem::SAMPLE_COUNT& count )
            {
                static const VkSampleCountFlagBits aVkSamples[] =
                {
                    VK_SAMPLE_COUNT_1_BIT,
                    VK_SAMPLE_COUNT_2_BIT,
                    VK_SAMPLE_COUNT_4_BIT,
                    VK_SAMPLE_COUNT_8_BIT,
                    VK_SAMPLE_COUNT_16_BIT,
                    VK_SAMPLE_COUNT_32_BIT,
                    VK_SAMPLE_COUNT_64_BIT
                };
                return aVkSamples[count];
            }

            VkCullModeFlags CullMode( const RenderSystem::CULL_MODE& mode )
            {
                static const VkCullModeFlagBits aVkModes[] =
                {
                    VK_CULL_MODE_NONE,
                    VK_CULL_MODE_FRONT_BIT,
                    VK_CULL_MODE_BACK_BIT,
                    VK_CULL_MODE_FRONT_AND_BACK
                };
                return aVkModes[mode];
            }

            VkFrontFace FrontFace( const RenderSystem::FRONT_FACE& face )
            {
                static const VkFrontFace aVkFaces[] =
                {
                    VK_FRONT_FACE_COUNTER_CLOCKWISE,
                    VK_FRONT_FACE_CLOCKWISE
                };
                return aVkFaces[face];
            }

            VkPolygonMode PolygonMode( const RenderSystem::POLYGON_MODE& mode )
            {
                static const VkPolygonMode aVkModes[] =
                {
                    VK_POLYGON_MODE_FILL,
                    VK_POLYGON_MODE_LINE,
                    VK_POLYGON_MODE_POINT
                };
                return aVkModes[mode];
            }

            VkShaderStageFlagBits ShaderStage( const RenderSystem::SHADER_TYPE& type )
            {
                static const VkShaderStageFlagBits aVkBits[] =
                {
                    VK_SHADER_STAGE_VERTEX_BIT,
                    VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT,
                    VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT,
                    VK_SHADER_STAGE_GEOMETRY_BIT,
                    VK_SHADER_STAGE_FRAGMENT_BIT,
                    VK_SHADER_STAGE_COMPUTE_BIT
                };
                return aVkBits[type];
            }

            VkVertexInputRate InputRate( const RenderSystem::VERTEX_INPUT_RATE& rate )
            {
                static const VkVertexInputRate aVkRates[] =
                {
                    VK_VERTEX_INPUT_RATE_VERTEX,
                    VK_VERTEX_INPUT_RATE_INSTANCE
                };
                return aVkRates[rate];
            }

            VkDescriptorType DescriptorType( const RenderSystem::DESCRIPTOR_SET_TYPE& type )
            {
                static const VkDescriptorType aVkDescriptorType[] =
                {
                    VK_DESCRIPTOR_TYPE_SAMPLER,
                    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                    VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                    VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,
                    VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,
                    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
                    VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC,
                    VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT
                };
                return aVkDescriptorType[type];
            }

            VkIndexType IndexType( const INDEX_TYPE& type )
            {
                static const VkIndexType aVkTypes[] =
                {
                    VK_INDEX_TYPE_UINT16,
                    VK_INDEX_TYPE_UINT32
                };
                return aVkTypes[type];
            }

            VkCommandBufferLevel CommandBufferLevel( const RenderSystem::COMMAND_BUFFER_LEVEL& level )
            {
                static const VkCommandBufferLevel aVkLevels[] =
                {
                    VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                    VK_COMMAND_BUFFER_LEVEL_SECONDARY
                };
                return aVkLevels[ level ];
            }

        } // Map

        namespace Convert
        {
            VkImageAspectFlags UsageToAspectMask( VkImageUsageFlags usage )
            {
                if( usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT )
                {
                    return VK_IMAGE_ASPECT_COLOR_BIT;
                }
                if( usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT )
                {
                    return VK_IMAGE_ASPECT_DEPTH_BIT;
                }
                VKE_LOG_ERR( "Invalid image usage: " << usage << " to use for aspectMask" );
                assert( 0 && "Invalid image usage" );
                return VK_IMAGE_ASPECT_COLOR_BIT;
            }

            VkImageViewType ImageTypeToViewType( VkImageType type )
            {
                static const VkImageViewType aTypes[] =
                {
                    VK_IMAGE_VIEW_TYPE_1D,
                    VK_IMAGE_VIEW_TYPE_2D,
                    VK_IMAGE_VIEW_TYPE_3D
                };
                assert( type <= VK_IMAGE_TYPE_3D && "Invalid image type for image view type" );
                return aTypes[type];
            }

            VkAttachmentLoadOp UsageToLoadOp( RenderSystem::RENDER_PASS_ATTACHMENT_USAGE usage )
            {
                static const VkAttachmentLoadOp aLoads[] =
                {
                    VK_ATTACHMENT_LOAD_OP_DONT_CARE, // undefined
                    VK_ATTACHMENT_LOAD_OP_LOAD, // color
                    VK_ATTACHMENT_LOAD_OP_CLEAR, // color clear
                    VK_ATTACHMENT_LOAD_OP_LOAD, // color store
                    VK_ATTACHMENT_LOAD_OP_CLEAR, // color clear store
                    VK_ATTACHMENT_LOAD_OP_LOAD, // depth
                    VK_ATTACHMENT_LOAD_OP_CLEAR, // depth clear
                    VK_ATTACHMENT_LOAD_OP_LOAD, // depth store
                    VK_ATTACHMENT_LOAD_OP_CLEAR, // depth clear store
                };
                return aLoads[usage];
            }


            VkAttachmentStoreOp UsageToStoreOp( RenderSystem::RENDER_PASS_ATTACHMENT_USAGE usage )
            {
                static const VkAttachmentStoreOp aStores[] =
                {
                    VK_ATTACHMENT_STORE_OP_STORE, // undefined
                    VK_ATTACHMENT_STORE_OP_STORE, // color
                    VK_ATTACHMENT_STORE_OP_STORE, // color clear
                    VK_ATTACHMENT_STORE_OP_STORE, // color store
                    VK_ATTACHMENT_STORE_OP_STORE, // color clear store
                    VK_ATTACHMENT_STORE_OP_STORE, // depth
                    VK_ATTACHMENT_STORE_OP_STORE, // depth clear
                    VK_ATTACHMENT_STORE_OP_STORE, // depth store
                    VK_ATTACHMENT_STORE_OP_STORE, // depth clear store
                };
                return aStores[usage];
            }

            VkImageLayout ImageUsageToLayout( VkImageUsageFlags vkFlags )
            {
                const auto imgSampled = vkFlags & VK_IMAGE_USAGE_SAMPLED_BIT;
                const auto inputAttachment = vkFlags & VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
                const auto isReadOnly = imgSampled || inputAttachment;

                if( vkFlags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT )
                {
                    return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                }
                else if( vkFlags & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT )
                {
                    if( isReadOnly )
                        return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
                    return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                }
                else if( vkFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT )
                {
                    return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                }
                else if( vkFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT )
                {
                    return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                }
                else if( isReadOnly )
                {
                    return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                }
                assert( 0 && "Invalid image usage flags" );
                VKE_LOG_ERR( "Usage flags: " << vkFlags << " are invalid." );
                return VK_IMAGE_LAYOUT_UNDEFINED;
            }

            VkImageLayout ImageUsageToInitialLayout( VkImageUsageFlags vkFlags )
            {
                if( vkFlags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT )
                {
                    return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                }
                else if( vkFlags & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT )
                {
                    return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                }
                assert( 0 && "Invalid image usage flags" );
                VKE_LOG_ERR( "Usage flags: " << vkFlags << " are invalid." );
                return VK_IMAGE_LAYOUT_UNDEFINED;
            }

            VkImageLayout ImageUsageToFinalLayout( VkImageUsageFlags vkFlags )
            {
                const auto imgSampled = vkFlags & VK_IMAGE_USAGE_SAMPLED_BIT;
                const auto inputAttachment = vkFlags & VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
                bool isReadOnly = imgSampled || inputAttachment;

                if( vkFlags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT )
                {
                    if( isReadOnly )
                        return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                }
                else if( vkFlags & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT )
                {
                    if( isReadOnly )
                        return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
                    return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                }

                assert( 0 && "Invalid image usage flags" );
                VKE_LOG_ERR( "Usage flags: " << vkFlags << " are invalid." );
                return VK_IMAGE_LAYOUT_UNDEFINED;
            }

            VkImageLayout NextAttachmentLayoutRread( VkImageLayout currLayout )
            {
                if( currLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL )
                {
                    return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                }
                if( currLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL )
                {
                    return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
                }
                assert( 0 && "Incorrect initial layout for attachment." );
                return VK_IMAGE_LAYOUT_UNDEFINED;
            }

            VkImageLayout NextAttachmentLayoutOptimal( VkImageLayout currLayout )
            {
                if( currLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL )
                {
                    return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                }
                if( currLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL )
                {
                    return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                }
                assert( 0 && "Incorrect initial layout for attachment." );
                return VK_IMAGE_LAYOUT_UNDEFINED;
            }

            RenderSystem::TEXTURE_FORMAT ImageFormat( VkFormat vkFormat )
            {
                switch( vkFormat )
                {
                    case VK_FORMAT_B8G8R8A8_UNORM: return RenderSystem::Formats::B8G8R8A8_UNORM;
                    case VK_FORMAT_B8G8R8A8_SRGB: return RenderSystem::Formats::B8G8R8A8_SRGB;
                    case VK_FORMAT_R8G8B8A8_UNORM: return RenderSystem::Formats::R8G8B8A8_UNORM;
                    case VK_FORMAT_R8G8B8A8_SRGB: return RenderSystem::Formats::R8G8B8A8_SRGB;
                }
                char buff[128];
                sprintf_s( buff, "Cannot convert VkFormat: %d to Engine format.", vkFormat );
                VKE_ASSERT( 0, buff );
                return RenderSystem::Formats::UNDEFINED;
            }

            VkPipelineStageFlags PipelineStages( const RenderSystem::PIPELINE_STAGES& stages )
            {
                VkPipelineStageFlags vkFlags = 0;
                if( stages & RenderSystem::PipelineStages::COMPUTE )
                {
                    vkFlags |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
                }
                if( stages & RenderSystem::PipelineStages::GEOMETRY )
                {
                    vkFlags |= VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT;
                }
                if( stages & RenderSystem::PipelineStages::PIXEL )
                {
                    vkFlags |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                }
                if( stages & RenderSystem::PipelineStages::TESS_DOMAIN )
                {
                    vkFlags |= VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT;
                }
                if( stages & RenderSystem::PipelineStages::TESS_HULL )
                {
                    vkFlags |= VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT;
                }
                if( stages & RenderSystem::PipelineStages::VERTEX )
                {
                    vkFlags |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
                }
                return vkFlags;
            }

            VkBufferUsageFlags BufferUsage( const RenderSystem::BUFFER_USAGE& usage )
            {
                VkBufferUsageFlags vkFlags = 0;
                if( usage & RenderSystem::BufferUsageBits::INDEX_BUFFER )
                {
                    vkFlags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
                }
                if( usage & RenderSystem::BufferUsageBits::VERTEX_BUFFER )
                {
                    vkFlags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
                }
                if( usage & RenderSystem::BufferUsageBits::UNIFORM_BUFFER )
                {
                    vkFlags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
                }
                if( usage & RenderSystem::BufferUsageBits::TRANSFER_DST )
                {
                    vkFlags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
                }
                if( usage & RenderSystem::BufferUsageBits::TRANSFER_SRC )
                {
                    vkFlags |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
                }
                if( usage & RenderSystem::BufferUsageBits::INDIRECT_BUFFER )
                {
                    vkFlags |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
                }
                if( usage & RenderSystem::BufferUsageBits::STORAGE_BUFFER )
                {
                    vkFlags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
                }
                if( usage & RenderSystem::BufferUsageBits::STORAGE_TEXEL_BUFFER )
                {
                    vkFlags |= VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT;
                }
                if( usage & RenderSystem::BufferUsageBits::UNIFORM_TEXEL_BUFFER )
                {
                    vkFlags |= VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT;
                }

                return vkFlags;
            }

            VkImageTiling ImageUsageToTiling( const RenderSystem::TEXTURE_USAGES& usage )
            {
                VkImageTiling vkTiling = VK_IMAGE_TILING_OPTIMAL;
                if( usage & RenderSystem::TextureUsages::FILE_IO )
                {
                    vkTiling = VK_IMAGE_TILING_LINEAR;
                }
                return vkTiling;
            }

            static const VkMemoryPropertyFlags g_aRequiredMemoryFlags[] =
            {
                0, // unknown
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, // gpu
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, // cpu access
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT // cpu access optimal
            };

            VkMemoryPropertyFlags MemoryUsagesToVkMemoryPropertyFlags( const RenderSystem::MEMORY_USAGES& usages )
            {
                VkMemoryPropertyFlags flags = 0;
                if( usages & RenderSystem::MemoryUsages::GPU_ACCESS )
                {
                    flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
                }
                else
                {
                    if( usages & RenderSystem::MemoryUsages::CPU_ACCESS )
                    {
                        flags |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
                    }
                    if( usages & RenderSystem::MemoryUsages::CPU_NO_FLUSH )
                    {
                        flags |= VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
                    }
                    if( usages & RenderSystem::MemoryUsages::CPU_CACHED )
                    {
                        flags |= VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
                    }
                }
                return flags;
            }

            VkPipelineBindPoint PipelineTypeToBindPoint( const PIPELINE_TYPE& type )
            {
                static const VkPipelineBindPoint aVkBindPoints[] =
                {
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    VK_PIPELINE_BIND_POINT_COMPUTE
                };
                return aVkBindPoints[ type ];
            }

            void ClearValues( const SClearValue* pSrc, const uint32_t count, VkClearValue* pDst )
            {
                for( uint32_t i = 0; i < count; ++i )
                {
                    const SClearValue& Src = pSrc[i];
                    VkClearValue& Dst = pDst[i];

                    Dst.color.float32[0] = Src.Color.r;
                    Dst.color.float32[1] = Src.Color.g;
                    Dst.color.float32[2] = Src.Color.b;
                    Dst.color.float32[3] = Src.Color.a;
                    Dst.depthStencil.depth = Src.DepthStencil.depth;
                    Dst.depthStencil.stencil = Src.DepthStencil.stencil;
                }
            }

            void TextureSubresourceRange( VkImageSubresourceRange* pVkDst, const STextureSubresourceRange& Src )
            {
                pVkDst->aspectMask = Map::ImageAspect( Src.aspect );
                pVkDst->baseArrayLayer = Src.beginArrayLayer;
                pVkDst->baseMipLevel = Src.beginMipmapLevel;
                pVkDst->layerCount = Src.layerCount;
                pVkDst->levelCount = Src.mipmapLevelCount;
            }

            VkAccessFlags AccessMask( const MEMORY_ACCESS_TYPE& type )
            {
                VkAccessFlags vkFlags = 0;
                if( type & MemoryAccessTypes::COLOR_ATTACHMENT_READ )
                {
                    vkFlags |= VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
                }
                if( type & MemoryAccessTypes::COLOR_ATTACHMENT_WRITE )
                {
                    vkFlags |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                }
                if( type & MemoryAccessTypes::CPU_MEMORY_READ )
                {
                    vkFlags |= VK_ACCESS_HOST_READ_BIT;
                }
                if( type & MemoryAccessTypes::CPU_MEMORY_WRITE )
                {
                    vkFlags |= VK_ACCESS_HOST_WRITE_BIT;
                }
                if( type & MemoryAccessTypes::DATA_TRANSFER_READ )
                {
                    vkFlags |= VK_ACCESS_TRANSFER_READ_BIT;
                }
                if( type & MemoryAccessTypes::DATA_TRANSFER_WRITE )
                {
                    vkFlags |= VK_ACCESS_TRANSFER_WRITE_BIT;
                }
                if( type & MemoryAccessTypes::DEPTH_STENCIL_ATTACHMENT_READ )
                {
                    vkFlags |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
                }
                if( type & MemoryAccessTypes::GPU_MEMORY_READ )
                {
                    vkFlags |= VK_ACCESS_MEMORY_READ_BIT;
                }
                if( type & MemoryAccessTypes::GPU_MEMORY_WRITE )
                {
                    vkFlags |= VK_ACCESS_MEMORY_WRITE_BIT;
                }
                if( type & MemoryAccessTypes::INDEX_READ )
                {
                    vkFlags |= VK_ACCESS_INDEX_READ_BIT;
                }
                if( type & MemoryAccessTypes::INDIRECT_BUFFER_READ )
                {
                    vkFlags |= VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
                }
                if( type & MemoryAccessTypes::INPUT_ATTACHMENT_READ )
                {
                    vkFlags |= VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
                }
                if( type & MemoryAccessTypes::SHADER_READ )
                {
                    vkFlags |= VK_ACCESS_SHADER_READ_BIT;
                }
                if( type & MemoryAccessTypes::SHADER_WRITE )
                {
                    vkFlags |= VK_ACCESS_SHADER_WRITE_BIT;
                }
                if( type & MemoryAccessTypes::UNIFORM_READ )
                {
                    vkFlags |= VK_ACCESS_UNIFORM_READ_BIT;
                }
                if( type & MemoryAccessTypes::VERTEX_ATTRIBUTE_READ )
                {
                    vkFlags |= VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
                }
                return vkFlags;
            }

            void Barrier( VkMemoryBarrier* pOut, const SMemoryBarrierInfo& Info )
            {
                pOut->srcAccessMask = Convert::AccessMask( Info.srcMemoryAccess );
                pOut->dstAccessMask = Convert::AccessMask( Info.dstMemoryAccess );
            }

            void Barrier( VkImageMemoryBarrier* pOut, const STextureBarrierInfo& Info )
            {
                pOut->srcAccessMask = Convert::AccessMask( Info.srcMemoryAccess );
                pOut->dstAccessMask = Convert::AccessMask( Info.dstMemoryAccess );
                pOut->image = Info.hTexture;
                pOut->oldLayout = Map::ImageLayout( Info.currentLayout );
                pOut->newLayout = Map::ImageLayout( Info.newLayout );
                Convert::TextureSubresourceRange( &pOut->subresourceRange, Info.SubresourceRange );
            }

            void Barrier( VkBufferMemoryBarrier* pOut, const SBufferBarrierInfo& Info )
            {
                pOut->srcAccessMask = Convert::AccessMask( Info.srcMemoryAccess );
                pOut->dstAccessMask = Convert::AccessMask( Info.dstMemoryAccess );
                pOut->buffer = Info.hBuffer;
                pOut->offset = Info.offset;
                pOut->size = Info.size;
            }

        } // Convert

        namespace Helper
        {
            struct SCompilerData
            {
                using ShaderBinaryData = vke_vector < uint32_t >;

                uint8_t             ShaderMemory[sizeof( glslang::TShader )];
                uint8_t				ProgramMemory[sizeof( glslang::TProgram )];
                glslang::TShader*   pShader = nullptr;
                glslang::TProgram*	pProgram = nullptr;

                ~SCompilerData()
                {
                    Destroy();
                }

                void Create( EShLanguage lang )
                {
                    pShader = ::new(ShaderMemory) glslang::TShader( lang );
                    pProgram = ::new(ProgramMemory) glslang::TProgram();
                }

                void Destroy()
                {
                    if( pProgram )
                    {
                        pProgram->~TProgram();
                    }
                    if( pShader )
                    {
                        pShader->~TShader();
                    }
                    pProgram = nullptr;
                    pShader = nullptr;
                }
            };

            struct SShaderCompiler
            {

                Result ProcessShaderIncludes( /*CFileManager* pFileMgr*/ )
                {
                    Result res = VKE_FAIL;

                    return res;
                }
            };
        } // Helper

        Result GetInstanceValidationLayers( bool bEnable, const VkICD::Global& Global,
            vke_vector<VkLayerProperties>* pvProps, CStrVec* pvNames )
        {
            if( !bEnable )
                return VKE_OK;

            static const char* apNames[] =
            {
                "VK_LAYER_LUNARG_parameter_validation",
                "VK_LAYER_LUNARG_device_limits",
                "VK_LAYER_LUNARG_object_tracker",
                "VK_LAYER_LUNARG_core_validation",
                "VK_LAYER_LUNARG_swapchain"
            };
            /*vNames.push_back("VK_LAYER_GOOGLE_threading");
            vNames.push_back("VK_LAYER_LUNARG_parameter_validation");
            vNames.push_back("VK_LAYER_LUNARG_device_limits");
            vNames.push_back("VK_LAYER_LUNARG_object_tracker");
            vNames.push_back("VK_LAYER_LUNARG_image");
            vNames.push_back("VK_LAYER_LUNARG_core_validation");
            vNames.push_back("VK_LAYER_LUNARG_swapchain");
            vNames.push_back("VK_LAYER_GOOGLE_unique_objects");*/

            uint32_t count = 0;
            auto& vProps = *pvProps;
            VK_ERR( Global.vkEnumerateInstanceLayerProperties( &count, nullptr ) );
            vProps.resize( count );
            VK_ERR( Global.vkEnumerateInstanceLayerProperties( &count, &vProps[0] ) );

            for( uint32_t i = 0; i < ARRAYSIZE( apNames ); ++i )
            {
                auto pName = apNames[i];
                for( auto& Prop : vProps )
                {
                    if( strcmp( Prop.layerName, pName ) == 0 )
                    {
                        pvNames->push_back( pName );
                    }
                }
            }
            return VKE_OK;
        }

        CStrVec GetInstanceExtensionNames( const VkICD::Global& Global )
        {
            CStrVec vNames;
            vke_vector< VkExtensionProperties > vProps;
            uint32_t count = 0;
            VK_ERR( Global.vkEnumerateInstanceExtensionProperties( "", &count, nullptr ) );
            vProps.resize( count );
            VK_ERR( Global.vkEnumerateInstanceExtensionProperties( "", &count, &vProps[0] ) );

            // Required extensions
            vNames.push_back( VK_EXT_DEBUG_REPORT_EXTENSION_NAME );
            vNames.push_back( VK_KHR_SURFACE_EXTENSION_NAME );
#if VKE_WINDOWS
            vNames.push_back( VK_KHR_WIN32_SURFACE_EXTENSION_NAME );
#elif VKE_LINUX
            vNames.push_back( VK_KHR_XCB_SURFACE_EXTENSION_NAME );
#elif VKE_ANDROID
            vNames.push_back( VK_KHR_ANDROID_SURFACE_EXTENSION_NAME );
#endif

            // Check extension availability
            for( auto& pExt : vNames )
            {
                bool bAvailable = false;
                for( auto& Prop : vProps )
                {
                    if( strcmp( Prop.extensionName, pExt ) == 0 )
                    {
                        bAvailable = true;
                        break;
                    }
                }
                if( !bAvailable )
                {
                    VKE_LOG_ERR( "There is no required extension: " << pExt << " supported by this GPU" );
                    vNames.clear();
                    return vNames;
                }
            }
            return vNames;
        }

        Result EnableInstanceExtensions( bool bEnable )
        {
            (void)bEnable;
            return VKE_OK;
        }

        Result EnableInstanceLayers( bool bEnable )
        {
            (void)bEnable;
            return VKE_OK;
        }

        Result QueryAdapterProperties( const VkPhysicalDevice& vkPhysicalDevice, SDeviceProperties* pOut )
        {
            auto& InstanceICD = CDDI::GetInstantceICD();

            uint32_t propCount = 0;
            InstanceICD.vkGetPhysicalDeviceQueueFamilyProperties( vkPhysicalDevice, &propCount, nullptr );
            if( propCount == 0 )
            {
                VKE_LOG_ERR( "No device queue family properties" );
                return VKE_FAIL;
            }

            pOut->vQueueFamilyProperties.Resize( propCount );
            auto& aProperties = pOut->vQueueFamilyProperties;
            auto& vQueueFamilies = pOut->vQueueFamilies;

            InstanceICD.vkGetPhysicalDeviceQueueFamilyProperties( vkPhysicalDevice, &propCount, &aProperties[0] );
            // Choose a family index
            for( uint32_t i = 0; i < propCount; ++i )
            {
                //uint32_t isGraphics = aProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT;
                uint32_t isCompute = aProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT;
                uint32_t isTransfer = aProperties[i].queueFlags & VK_QUEUE_TRANSFER_BIT;
                uint32_t isSparse = aProperties[i].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT;
                uint32_t isGraphics = aProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT;
                VkBool32 isPresent = VK_FALSE;
#if VKE_USE_VULKAN_WINDOWS
                isPresent = InstanceICD.vkGetPhysicalDeviceWin32PresentationSupportKHR( vkPhysicalDevice, i );
#elif VKE_USE_VULKAN_LINUX
                isPresent = InstanceICD.vkGetPhysicalDeviceXcbPresentationSupportKHR( s_physical_device, i,
                    xcb_connection, visual_id );
#elif VKE_USE_VULKAN_ANDROID
#error "implement"
#endif

                SQueueFamilyInfo Family;
                Family.vQueues.Resize( aProperties[i].queueCount );
                Family.vPriorities.Resize( aProperties[i].queueCount, 1.0f );
                Family.index = i;
                Family.type = 0;
                /*Family.isGraphics = isGraphics != 0;
                Family.isCompute = isCompute != 0;
                Family.isTransfer = isTransfer != 0;
                Family.isSparse = isSparse != 0;
                Family.isPresent = isPresent == VK_TRUE;*/
                if( isCompute )
                {
                    Family.type |= QueueTypeBits::COMPUTE;
                }
                if( isTransfer )
                {
                    Family.type |= QueueTypeBits::TRANSFER;
                }
                if( isSparse )
                {
                    Family.type |= QueueTypeBits::SPARSE;
                }
                if( isGraphics )
                {
                    Family.type |= QueueTypeBits::GRAPHICS;
                }
                if( isPresent )
                {
                    Family.type |= QueueTypeBits::PRESENT;
                }

                vQueueFamilies.PushBack( Family );
            }

            InstanceICD.vkGetPhysicalDeviceMemoryProperties( vkPhysicalDevice, &pOut->vkMemProperties );
            InstanceICD.vkGetPhysicalDeviceFeatures( vkPhysicalDevice, &pOut->vkFeatures );

            for( uint32_t i = 0; i < RenderSystem::Formats::_MAX_COUNT; ++i )
            {
                const auto& fmt = RenderSystem::g_aFormats[i];
                InstanceICD.vkGetPhysicalDeviceFormatProperties( vkPhysicalDevice, fmt,
                    &pOut->aFormatProperties[i] );
            }

            return VKE_OK;
        }

//        Result CDDI::QueryAdapterProperties( const SAdapterInfo& Info, SAdapterProperties* pOut )
//        {
//            auto& InstanceICD = CDDI::GetInstantceICD();
//
//            uint32_t propCount = 0;
//            InstanceICD.vkGetPhysicalDeviceQueueFamilyProperties( Info.hDDIAdapter, &propCount, nullptr );
//            if( propCount == 0 )
//            {
//                VKE_LOG_ERR( "No device queue family properties" );
//                return VKE_FAIL;
//            }
//
//            Utils::TCDynamicArray< VkQueueFamilyProperties, 64 > vVkQueueProperties;
//            vVkQueueProperties.Resize( propCount );
//            auto& aProperties = vVkQueueProperties;
//            auto& vQueueFamilies = pOut->vQueueInfos;
//
//            InstanceICD.vkGetPhysicalDeviceQueueFamilyProperties( Info.hDDIAdapter, &propCount, &aProperties[0] );
//            // Choose a family index
//            for( uint32_t i = 0; i < propCount; ++i )
//            {
//                //uint32_t isGraphics = aProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT;
//                uint32_t isCompute = aProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT;
//                uint32_t isTransfer = aProperties[i].queueFlags & VK_QUEUE_TRANSFER_BIT;
//                uint32_t isSparse = aProperties[i].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT;
//                uint32_t isGraphics = aProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT;
//                VkBool32 isPresent = VK_FALSE;
//#if VKE_USE_VULKAN_WINDOWS
//                isPresent = InstanceICD.vkGetPhysicalDeviceWin32PresentationSupportKHR( Info.hDDIAdapter, i );
//#elif VKE_USE_VULKAN_LINUX
//                isPresent = InstanceICD.vkGetPhysicalDeviceXcbPresentationSupportKHR( s_physical_device, i,
//                    xcb_connection, visual_id );
//#elif VKE_USE_VULKAN_ANDROID
//#error "implement"
//#endif
//
//                SQueueFamilyInfo Family;
//                //Family.vQueues.Resize( aProperties[i].queueCount );
//                Family.vPriorities.Resize( aProperties[i].queueCount, 1.0f );
//                Family.index = i;
//                Family.isGraphics = isGraphics != 0;
//                Family.isCompute = isCompute != 0;
//                Family.isTransfer = isTransfer != 0;
//                Family.isSparse = isSparse != 0;
//                Family.isPresent = isPresent == VK_TRUE;
//
//                vQueueFamilies.PushBack( Family );
//            }
//        }

        Result CheckDeviceExtensions( VkPhysicalDevice vkPhysicalDevice,
            const Utils::TCDynamicArray<const char *>& vExtensions )
        {
            auto& InstanceICD = CDDI::GetInstantceICD();
            uint32_t count = 0;
            VK_ERR( InstanceICD.vkEnumerateDeviceExtensionProperties( vkPhysicalDevice, nullptr, &count, nullptr ) );

            Utils::TCDynamicArray< VkExtensionProperties > vProperties( count );

            VK_ERR( InstanceICD.vkEnumerateDeviceExtensionProperties( vkPhysicalDevice, nullptr, &count,
                &vProperties[0] ) );

            std::string ext;
            Result err = VKE_OK;

            for( uint32_t e = 0; e < vExtensions.GetCount(); ++e )
            {
                ext = vExtensions[e];
                bool found = false;
                for( uint32_t p = 0; p < count; ++p )
                {
                    if( ext == vProperties[p].extensionName )
                    {
                        found = true;
                        break;
                    }
                }
                if( !found )
                {
                    VKE_LOG_ERR( "Extension: " << ext << " is not supported by the device." );
                    err = VKE_ENOTFOUND;
                }
            }

            return err;
        }

        //Result CDDI::QueryQueues( QueueFamilyInfoArray* pOut )
        //{
        //    //m_ICD.vkGetDeviceQueue()
        //}

        Result CDDI::CreateDevice( CDeviceContext* pCtx )
        {
            m_pCtx = pCtx;
            Utils::TCDynamicArray< const char* > vExtensions =
            {
                VK_KHR_SWAPCHAIN_EXTENSION_NAME
            };

            auto hAdapter = m_pCtx->m_Desc.pAdapterInfo->hDDIAdapter;
            VKE_ASSERT( hAdapter != NULL_HANDLE, "" );
            m_hAdapter = reinterpret_cast< VkPhysicalDevice >( hAdapter );
            //VkInstance vkInstance = reinterpret_cast<VkInstance>(Desc.hAPIInstance);

            {
                sInstanceICD.vkGetPhysicalDeviceFeatures( m_hAdapter, &m_DeviceInfo.Features );
                //ICD.Instance.vkGetPhysicalDeviceFormatProperties( vkPhysicalDevice, &m_DeviceInfo.FormatProperties );
                sInstanceICD.vkGetPhysicalDeviceMemoryProperties( m_hAdapter, &m_DeviceInfo.MemoryProperties );
                sInstanceICD.vkGetPhysicalDeviceProperties( m_hAdapter, &m_DeviceInfo.Properties );
            }

            VKE_RETURN_IF_FAILED( QueryAdapterProperties( m_hAdapter, &m_DeviceProperties ) );
            VKE_RETURN_IF_FAILED( CheckDeviceExtensions( m_hAdapter, vExtensions ) );

            Utils::TCDynamicArray<VkDeviceQueueCreateInfo> vQis;
            for( auto& Family : m_DeviceProperties.vQueueFamilies )
            {
                if( !Family.vQueues.IsEmpty() )
                {
                    VkDeviceQueueCreateInfo qi;
                    Vulkan::InitInfo( &qi, VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO );
                    qi.flags = 0;
                    qi.pQueuePriorities = &Family.vPriorities[0];
                    qi.queueFamilyIndex = Family.index;
                    qi.queueCount = static_cast<uint32_t>(Family.vQueues.GetCount());
                    vQis.PushBack( qi );
                }
            }

            //VkPhysicalDeviceFeatures df = {};

            VkDeviceCreateInfo di;
            Vulkan::InitInfo( &di, VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO );
            di.enabledExtensionCount = vExtensions.GetCount();
            di.enabledLayerCount = 0;
            di.pEnabledFeatures = nullptr;
            di.ppEnabledExtensionNames = &vExtensions[0];
            di.ppEnabledLayerNames = nullptr;
            di.pQueueCreateInfos = &vQis[0];
            di.queueCreateInfoCount = static_cast<uint32_t>(vQis.GetCount());
            di.flags = 0;

            VK_ERR( sInstanceICD.vkCreateDevice( m_hAdapter, &di, nullptr, &m_hDevice ) );

            VKE_RETURN_IF_FAILED( Vulkan::LoadDeviceFunctions( m_hDevice, sInstanceICD, &m_ICD ) );

            for( SQueueFamilyInfo& Family : m_DeviceProperties.vQueueFamilies )
            {
                for( uint32_t q = 0; q < Family.vQueues.GetCount(); ++q )
                {
                    VkQueue vkQueue;
                    m_ICD.vkGetDeviceQueue( m_hDevice, Family.index, q, &vkQueue );
                    Family.vQueues[q] = vkQueue;
                }
            }
        
            return VKE_OK;
        }

        void CDDI::DestroyDevice()
        {
            if( m_hDevice != DDI_NULL_HANDLE )
            {
                sInstanceICD.vkDestroyDevice( m_hDevice, nullptr );
            }
            m_hDevice = DDI_NULL_HANDLE;
            m_pCtx = nullptr;
        }

        Result CDDI::LoadICD( const SDDILoadInfo& Info )
        {
            Result ret = VKE_OK;
            shICD = Platform::DynamicLibrary::Load( "vulkan-1.dll" );
            if( shICD != 0 )
            {
                ret = Vulkan::LoadGlobalFunctions( shICD, &sGlobalICD );
                if( VKE_SUCCEEDED( ret ) )
                {
                    bool bEnabled = true;
                    auto vExtNames = GetInstanceExtensionNames( sGlobalICD );
                    if( vExtNames.empty() )
                    {
                        return VKE_FAIL;
                    }

                    CStrVec vLayerNames;
                    vke_vector< VkLayerProperties > vLayerProps;
                    ret = GetInstanceValidationLayers( bEnabled, sGlobalICD, &vLayerProps, &vLayerNames );
                    if( VKE_SUCCEEDED( ret ) )
                    {
                        VkApplicationInfo vkAppInfo;
                        vkAppInfo.apiVersion = VK_API_VERSION_1_0;
                        vkAppInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
                        vkAppInfo.pNext = nullptr;
                        vkAppInfo.applicationVersion = Info.AppInfo.applicationVersion;
                        vkAppInfo.engineVersion = Info.AppInfo.engineVersion;
                        vkAppInfo.pApplicationName = Info.AppInfo.pApplicationName;
                        vkAppInfo.pEngineName = Info.AppInfo.pEngineName;

                        VkInstanceCreateInfo InstInfo;
                        Vulkan::InitInfo( &InstInfo, VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO );
                        InstInfo.enabledExtensionCount = static_cast<uint32_t>(vExtNames.size());
                        InstInfo.enabledLayerCount = static_cast<uint32_t>(vLayerNames.size());
                        InstInfo.flags = 0;
                        InstInfo.pApplicationInfo = &vkAppInfo;
                        InstInfo.ppEnabledExtensionNames = vExtNames.data();
                        InstInfo.ppEnabledLayerNames = vLayerNames.data();

                        VkResult vkRes = sGlobalICD.vkCreateInstance( &InstInfo, nullptr, &sVkInstance );
                        VK_ERR( vkRes );
                        if( vkRes == VK_SUCCESS )
                        {
                            ret = Vulkan::LoadInstanceFunctions( sVkInstance, sGlobalICD, &sInstanceICD );
                        }
                        else
                        {
                            ret = VKE_FAIL;
                            VKE_LOG_ERR( "Unable to create Vulkan instance: " << vkRes );
                        }
                    }
                    else
                    {
                        VKE_LOG_ERR( "Unable to get Vulkan instance validation layers." );
                    }
                }
                else
                {
                    VKE_LOG_ERR("Unable to load Vulkan global function pointers.");
                }
            }
            else
            {
                VKE_LOG_ERR( "Unable to load library: vulkan-1.dll" );
            }
            return ret;
        }

        void CDDI::CloseICD()
        {
            //sGlobalICD.vkDestroyInstance( sVkInstance, nullptr );
            sInstanceICD.vkDestroyInstance( sVkInstance, nullptr );
            sVkInstance = VK_NULL_HANDLE;
            Platform::DynamicLibrary::Close( shICD );
        }

        Result CDDI::QueryAdapters( AdapterInfoArray* pOut )
        {
            Result ret = VKE_FAIL;
            uint32_t count = 0;
            VkResult vkRes = sInstanceICD.vkEnumeratePhysicalDevices( sVkInstance, &count, nullptr );
            VK_ERR( vkRes );
            if( vkRes == VK_SUCCESS )
            {
                if( count > 0 )
                {
                    svAdapters.Resize( count );
                    vkRes = sInstanceICD.vkEnumeratePhysicalDevices( sVkInstance, &count, &svAdapters[ 0 ] );
                    VK_ERR( vkRes );
                    if( vkRes == VK_SUCCESS )
                    {
                        const uint32_t nameLen = Min( VK_MAX_PHYSICAL_DEVICE_NAME_SIZE, Constants::MAX_NAME_LENGTH );

                        for( size_t i = 0; i < svAdapters.GetCount(); ++i )
                        {
                            const auto& vkPhysicalDevice = svAdapters[ i ];

                            VkPhysicalDeviceProperties Props;
                            sInstanceICD.vkGetPhysicalDeviceProperties( vkPhysicalDevice, &Props );
                            RenderSystem::SAdapterInfo Info = {};
                            Info.apiVersion = Props.apiVersion;
                            Info.deviceID = Props.deviceID;
                            Info.driverVersion = Props.driverVersion;
                            Info.type = static_cast<RenderSystem::ADAPTER_TYPE>(Props.deviceType);
                            Info.vendorID = Props.vendorID;
                            Info.hDDIAdapter = reinterpret_cast<handle_t>(vkPhysicalDevice);
                            Memory::Copy( Info.name, sizeof( Info.name ), Props.deviceName, nameLen );

                            pOut->PushBack( Info );
                        }
                        ret = VKE_OK;
                    }
                    else
                    {
                        VKE_LOG_ERR( "No physical device available for this machine" );
                    }
                }
                else
                {
                    VKE_LOG_ERR( "No physical device available for this machine" );
                    VKE_LOG_ERR( "Vulkan is not supported for this GPU" );
                }
            }
            else
            {
                VKE_LOG_ERR( "Unable to enumerate Vulkan physical devices: " << vkRes );
            }
            return ret;
        }

        DDIBuffer   CDDI::CreateObject( const SBufferDesc& Desc, const void* pAllocator )
        {
            VkBufferCreateInfo ci;
            DDIBuffer hBuffer = VK_NULL_HANDLE;
            Vulkan::InitInfo( &ci, VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO );
            {
                ci.flags = 0;
                ci.pQueueFamilyIndices = nullptr;
                ci.queueFamilyIndexCount = 0;
                ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                ci.size = Desc.size;
                ci.usage = Vulkan::Convert::BufferUsage( Desc.usage );
                VkResult vkRes = DDI_CREATE_OBJECT( Buffer, ci, pAllocator, &hBuffer );
                VK_ERR( vkRes );
            }
            return hBuffer;
        }

        void CDDI::DestroyObject( DDIBuffer* phBuffer, const void* pAllocator )
        {
            DDI_DESTROY_OBJECT( Buffer, phBuffer, pAllocator );
        }

        DDIBufferView CDDI::CreateObject( const SBufferViewDesc& Desc, const void* pAllocator )
        {
            DDIBufferView hView = DDI_NULL_HANDLE;
            VkBufferViewCreateInfo ci;
            {
                ci.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
                ci.pNext = nullptr;
                ci.flags = 0;
                ci.format = Vulkan::Map::Format( Desc.format );
                ci.buffer = m_pCtx->GetBuffer( Desc.hBuffer )->GetDDIObject();
                ci.offset = Desc.offset;
            }
            VkResult vkRes = DDI_CREATE_OBJECT( BufferView, ci, pAllocator, &hView );
            VK_ERR( vkRes );
            return hView;
        }

        void CDDI::DestroyObject( DDIBufferView* phBufferView, const void* pAllocator )
        {
            DDI_DESTROY_OBJECT( BufferView, phBufferView, pAllocator );
        }

        DDITexture CDDI::CreateObject( const STextureDesc& Desc, const void* pAllocator )
        {
            DDITexture hImage = DDI_NULL_HANDLE;
            VkImageCreateInfo ci;
            {
                ci.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
                ci.pNext = nullptr;
                ci.flags = 0;
                ci.format = Vulkan::Map::Format( Desc.format );
                ci.imageType = Vulkan::Map::ImageType( Desc.type );
                ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                ci.mipLevels = Desc.mipLevelCount;
                ci.samples = Vulkan::Map::SampleCount( Desc.multisampling );
                ci.pQueueFamilyIndices = nullptr;
                ci.queueFamilyIndexCount = 0;
                ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                ci.tiling = Vulkan::Convert::ImageUsageToTiling( Desc.usage );
                ci.arrayLayers = 1;
                ci.extent.width = Desc.Size.width;
                ci.extent.height = Desc.Size.height;
                ci.extent.depth = 1;
                ci.usage = Vulkan::Map::ImageUsage( Desc.usage );
            }
            VkResult vkRes = DDI_CREATE_OBJECT( Image, ci, pAllocator, &hImage );
            VK_ERR( vkRes );
            return hImage;
        }

        void CDDI::DestroyObject( DDITexture* phImage, const void* pAllocator )
        {
            DDI_DESTROY_OBJECT( Image, phImage, pAllocator );
        }

        DDITextureView CDDI::CreateObject( const STextureViewDesc& Desc, const void* pAllocator )
        {
            static const VkComponentMapping DefaultMapping = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
            DDITextureView hView = DDI_NULL_HANDLE;
            VkImageViewCreateInfo ci;
            {
                Convert::TextureSubresourceRange( &ci.subresourceRange, Desc.SubresourceRange );

                TextureRefPtr pTex = m_pCtx->GetTexture( Desc.hTexture );
                ci.components = DefaultMapping;
                ci.flags = 0;
                ci.format = Vulkan::Map::Format( Desc.format );
                ci.image = pTex->GetDDIObject();
                ci.pNext = nullptr;
                ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                ci.viewType = Vulkan::Map::ImageViewType( Desc.type );
            }
            
            VkResult vkRes = DDI_CREATE_OBJECT( ImageView, ci, pAllocator, &hView );
            VK_ERR( vkRes );
            return hView;
        }

        void CDDI::DestroyObject( DDITextureView* phImageView, const void* pAllocator )
        {
            DDI_DESTROY_OBJECT( ImageView, phImageView, pAllocator );
        }

        DDIFramebuffer CDDI::CreateObject( const SFramebufferDesc& Desc, const void* pAllocator )
        {
            Utils::TCDynamicArray< DDITextureView > vAttachments;
            const uint32_t attachmentCount = Desc.vAttachments.GetCount();
            vAttachments.Resize( attachmentCount );
            
            for( uint32_t i = 0; i < attachmentCount; ++i )
            {
                DDITextureView hView = reinterpret_cast< DDITextureView >( Desc.vAttachments[i].handle );
                //vAttachments[i] = m_pCtx->GetTextureView( Desc.vAttachments[i] )->GetDDIObject();
                vAttachments[ i ] = hView;
            }

            VkFramebufferCreateInfo ci;
            ci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            ci.pNext = nullptr;
            ci.flags = 0;
            ci.width = Desc.Size.width;
            ci.height = Desc.Size.height;
            ci.layers = 1;
            ci.attachmentCount = Desc.vAttachments.GetCount();
            ci.pAttachments = &vAttachments[0];
            ci.renderPass = reinterpret_cast< DDIRenderPass >( Desc.hRenderPass.handle );
            //ci.renderPass = m_pCtx->GetRenderPass( Desc.hRenderPass )->GetDDIObject();

            DDIFramebuffer hFramebuffer = DDI_NULL_HANDLE;
            VkResult vkRes = DDI_CREATE_OBJECT( Framebuffer, ci, pAllocator, &hFramebuffer );
            VK_ERR( vkRes );
            return hFramebuffer;
        
        }

        void CDDI::DestroyObject( DDIFramebuffer* phFramebuffer, const void* pAllocator )
        {
            DDI_DESTROY_OBJECT( Framebuffer, phFramebuffer, pAllocator );
        }

        DDIFence CDDI::CreateObject( const SFenceDesc& Desc, const void* pAllocator )
        {
            VkFenceCreateInfo ci;
            ci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            ci.pNext = nullptr;
            ci.flags = Desc.isSignaled;
            DDIFence hObj = DDI_NULL_HANDLE;
            VkResult res = DDI_CREATE_OBJECT( Fence, ci, pAllocator, &hObj );
            VK_ERR( res );
            return hObj;
        }

        void CDDI::DestroyObject( DDIFence* phFence, const void* pAllocator )
        {
            DDI_DESTROY_OBJECT( Fence, phFence, pAllocator );
        }

        DDISemaphore CDDI::CreateObject( const SSemaphoreDesc& Desc, const void* pAllocator )
        {
            DDISemaphore hSemaphore = DDI_NULL_HANDLE;
            VkSemaphoreCreateInfo ci;
            ci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            ci.pNext = nullptr;
            ci.flags = 0;
            VK_ERR( DDI_CREATE_OBJECT( Semaphore, ci, pAllocator, &hSemaphore ) );
            return hSemaphore;
        }

        void CDDI::DestroyObject( DDISemaphore* phSemaphore, const void* pAllocator )
        {
            DDI_DESTROY_OBJECT( Semaphore, phSemaphore, pAllocator );
        }

        DDICommandBufferPool CDDI::CreateObject( const SCommandBufferPoolDesc& Desc, const void* pAllocator )
        {
            DDICommandBufferPool hPool = DDI_NULL_HANDLE;
            VkCommandPoolCreateInfo ci;
            ci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            ci.pNext = nullptr;
            ci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            ci.queueFamilyIndex = Desc.queueFamilyIndex;
            VkResult res = DDI_CREATE_OBJECT( CommandPool, ci, pAllocator, &hPool );
            VK_ERR( res );
            return hPool;
        }

        void CDDI::DestroyObject( DDICommandBufferPool* phPool, const void* pAllocator )
        {
            DDI_DESTROY_OBJECT( CommandPool, phPool, pAllocator );
        }

        static
        int32_t FindTextureHandle( const SRenderPassDesc::AttachmentDescArray& vAttachments,
            const TextureViewHandle& hTexView )
        {
            int32_t res = -1;
            for( uint32_t a = 0; a < vAttachments.GetCount(); ++a )
            {
                if( hTexView == vAttachments[a].hTextureView )
                {
                    res = a;
                    break;
                }
            }
            return res;
        }

        static
        bool MakeAttachmentRef( const SRenderPassDesc::AttachmentDescArray& vAttachments,
            const SSubpassAttachmentDesc& SubpassAttachmentDesc,
            VkAttachmentReference* pRefOut )
        {
            int32_t idx = FindTextureHandle( vAttachments, SubpassAttachmentDesc.hTextureView );
            bool res = false;
            if( idx >= 0 )
            {
                pRefOut->attachment = idx;
                pRefOut->layout = Vulkan::Map::ImageLayout( SubpassAttachmentDesc.layout );
                res = true;
            }
            return res;
        }

        DDIRenderPass CDDI::CreateObject( const SRenderPassDesc& Desc, const void* pAllocator )
        {
            DDIRenderPass hPass = DDI_NULL_HANDLE;
            using VkAttachmentDescriptionArray = Utils::TCDynamicArray< VkAttachmentDescription, 8 >;
            using VkAttachmentRefArray = Utils::TCDynamicArray< VkAttachmentReference >;

            struct SSubpassDesc
            {
                VkAttachmentRefArray vInputAttachmentRefs;
                VkAttachmentRefArray vColorAttachmentRefs;
                VkAttachmentReference vkDepthStencilRef;
                VkAttachmentReference* pVkDepthStencilRef = nullptr;
            };

            using SubpassDescArray = Utils::TCDynamicArray< SSubpassDesc >;
            using VkSubpassDescArray = Utils::TCDynamicArray< VkSubpassDescription >;
            using VkClearValueArray = Utils::TCDynamicArray< VkClearValue >;
            using VkImageViewArray = Utils::TCDynamicArray< VkImageView >;

            VkAttachmentDescriptionArray vVkAttachmentDescriptions;
            SubpassDescArray vSubpassDescs;
            VkSubpassDescArray vVkSubpassDescs;
            //VkClearValueArray vVkClearValues;

            for( uint32_t a = 0; a < Desc.vAttachments.GetCount(); ++a )
            {
                const SRenderPassAttachmentDesc& AttachmentDesc = Desc.vAttachments[a];
                //const VkImageCreateInfo& vkImgInfo = ResMgr.GetTextureDesc( AttachmentDesc.hTextureView );
                VkAttachmentDescription vkAttachmentDesc;
                vkAttachmentDesc.finalLayout = Vulkan::Map::ImageLayout( AttachmentDesc.endLayout );
                vkAttachmentDesc.flags = 0;
                vkAttachmentDesc.format = Vulkan::Map::Format( AttachmentDesc.format );
                vkAttachmentDesc.initialLayout = Vulkan::Map::ImageLayout( AttachmentDesc.beginLayout );
                vkAttachmentDesc.loadOp = Vulkan::Convert::UsageToLoadOp( AttachmentDesc.usage );
                vkAttachmentDesc.storeOp = Vulkan::Convert::UsageToStoreOp( AttachmentDesc.usage );
                vkAttachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                vkAttachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                vkAttachmentDesc.samples = Vulkan::Map::SampleCount( AttachmentDesc.sampleCount );
                vVkAttachmentDescriptions.PushBack( vkAttachmentDesc );

                /*VkImageView vkView = ResMgr.GetTextureView( AttachmentDesc.hTextureView );
                m_vImageViews.PushBack( vkView );
                VkImage vkImg = ResMgr.GetTextureViewDesc( AttachmentDesc.hTextureView ).image;
                m_vImages.PushBack( vkImg );*/

                //VkClearValue vkClear;
                //AttachmentDesc.ClearColor.CopyToNative( &vkClear );
                //Convert( AttachmentDesc.ClearValue, &vkClear );
                //vVkClearValues.PushBack( vkClear );
            }

            for( uint32_t s = 0; s < Desc.vSubpasses.GetCount(); ++s )
            {
                SSubpassDesc SubDesc;

                const auto& SubpassDesc = Desc.vSubpasses[s];
                for( uint32_t r = 0; r < SubpassDesc.vRenderTargets.GetCount(); ++r )
                {
                    const auto& RenderTargetDesc = SubpassDesc.vRenderTargets[r];

                    // Find attachment
                    VkAttachmentReference vkRef;
                    if( MakeAttachmentRef( Desc.vAttachments, RenderTargetDesc, &vkRef ) )
                    {
                        SubDesc.vColorAttachmentRefs.PushBack( vkRef );
                    }
                }

                for( uint32_t t = 0; t < SubpassDesc.vTextures.GetCount(); ++t )
                {
                    const auto& TexDesc = SubpassDesc.vTextures[t];
                    // Find attachment
                    VkAttachmentReference vkRef;
                    if( MakeAttachmentRef( Desc.vAttachments, TexDesc, &vkRef ) )
                    {
                        SubDesc.vInputAttachmentRefs.PushBack( vkRef );
                    }
                }

                // Find attachment
                VkAttachmentReference* pVkDepthStencilRef = nullptr;
                if( SubpassDesc.DepthBuffer.hTextureView != NULL_HANDLE )
                {
                    VkAttachmentReference vkRef;
                    if( MakeAttachmentRef( Desc.vAttachments, SubpassDesc.DepthBuffer, &vkRef ) )
                    {
                        SubDesc.vkDepthStencilRef = vkRef;
                        SubDesc.pVkDepthStencilRef = &SubDesc.vkDepthStencilRef;
                    }
                }

                VkSubpassDescription VkSubpassDesc;
                const auto colorCount = SubDesc.vColorAttachmentRefs.GetCount();
                const auto inputCount = SubDesc.vInputAttachmentRefs.GetCount();

                VkSubpassDesc.colorAttachmentCount = colorCount;
                VkSubpassDesc.pColorAttachments = (colorCount > 0) ? &SubDesc.vColorAttachmentRefs[0] : nullptr;
                VkSubpassDesc.inputAttachmentCount = inputCount;
                VkSubpassDesc.pInputAttachments = (inputCount > 0) ? &SubDesc.vInputAttachmentRefs[0] : nullptr;
                VkSubpassDesc.pDepthStencilAttachment = pVkDepthStencilRef;
                VkSubpassDesc.flags = 0;
                VkSubpassDesc.pResolveAttachments = nullptr;
                VkSubpassDesc.preserveAttachmentCount = 0;
                VkSubpassDesc.pPreserveAttachments = nullptr;
                VkSubpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

                vSubpassDescs.PushBack( SubDesc );
                vVkSubpassDescs.PushBack( VkSubpassDesc );
            }

            {
                VkRenderPassCreateInfo ci;
                ci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
                ci.pNext = nullptr;
                ci.attachmentCount = vVkAttachmentDescriptions.GetCount();
                ci.pAttachments = &vVkAttachmentDescriptions[0];
                ci.dependencyCount = 0;
                ci.pDependencies = nullptr;
                ci.subpassCount = vVkSubpassDescs.GetCount();
                ci.pSubpasses = &vVkSubpassDescs[0];
                ci.flags = 0;
                VkResult res = m_ICD.vkCreateRenderPass( m_hDevice, &ci, nullptr, &hPass );
                VK_ERR( res );
            }
            return hPass;
        }

        void CDDI::DestroyObject( DDIRenderPass* phRenderPass, const void* pAllocator )
        {
            DDI_DESTROY_OBJECT( RenderPass, phRenderPass, pAllocator );
        }

        DDIDescriptorPool CDDI::CreateObject( const SDescriptorPoolDesc& Desc, const void* pAllocator )
        {
            DDIDescriptorPool hPool = DDI_NULL_HANDLE;
            VkDescriptorPoolCreateInfo ci;
            ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            ci.pNext = nullptr;
            ci.flags = 0;
            ci.maxSets = Desc.maxSetCount;
            ci.poolSizeCount = Desc.vPoolSizes.GetCount();
            
            Utils::TCDynamicArray< VkDescriptorPoolSize > vVkSizes;
            vVkSizes.Resize( ci.poolSizeCount );
            for( uint32_t i = 0; i < ci.poolSizeCount; ++i )
            {
                vVkSizes[i].descriptorCount = Desc.vPoolSizes[i].count;
                vVkSizes[i].type = Map::DescriptorType( Desc.vPoolSizes[i].type );
            }
            ci.pPoolSizes = &vVkSizes[0];
            
            //VkResult res = m_ICD.vkCreateDescriptorPool( m_hDevice, &ci, pVkAllocator, &hPool );
            VkResult res = DDI_CREATE_OBJECT( DescriptorPool, ci, pAllocator, &hPool );
            VK_ERR( res );
            return hPool;
        }

        void CDDI::DestroyObject( DDIDescriptorPool* phPool, const void* pAllocator )
        {
            DDI_DESTROY_OBJECT( DescriptorPool, phPool, pAllocator );
        }

        DDIPipeline CDDI::CreateObject( const SPipelineDesc& Desc, const void* pAllocator)
        {
            DDIPipeline hPipeline = DDI_NULL_HANDLE;
            VkResult vkRes = VK_ERROR_OUT_OF_HOST_MEMORY;
            const VkAllocationCallbacks* pVkCallbacks = reinterpret_cast<const VkAllocationCallbacks*>(pAllocator);

            //Utils::TCDynamicArray< VkPipelineColorBlendAttachmentState, Config::RenderSystem::Pipeline::MAX_BLEND_STATE_COUNT > vVkBlendStates;
            const bool isGraphics = Desc.Shaders.apShaders[ShaderTypes::COMPUTE].IsNull();

            VkGraphicsPipelineCreateInfo VkGraphicsInfo = {};
            VkComputePipelineCreateInfo VkComputeInfo = {};

            if( isGraphics )
            {
                VkGraphicsPipelineCreateInfo& ci = VkGraphicsInfo;
                ci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
                ci.pNext = nullptr;
                ci.flags = 0;

                VkPipelineColorBlendStateCreateInfo VkColorBlendState;
                {
                    auto& State = VkColorBlendState;
                    State.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
                    State.pNext = nullptr;
                    State.flags = 0;
                    const auto& vBlendStates = Desc.Blending.vBlendStates;
                    if( !vBlendStates.IsEmpty() )
                    {
                        Utils::TCDynamicArray< VkPipelineColorBlendAttachmentState > vVkBlendStates;
                        vVkBlendStates.Resize( vBlendStates.GetCount() );
                        {
                            for( uint32_t i = 0; i < vBlendStates.GetCount(); ++i )
                            {
                                auto& vkBlendState = vVkBlendStates[i];
                                vkBlendState.alphaBlendOp = Map::BlendOp( vBlendStates[i].Alpha.operation );
                                vkBlendState.blendEnable = vBlendStates[i].enable;
                                vkBlendState.colorBlendOp = Map::BlendOp( vBlendStates[i].Color.operation );
                                vkBlendState.colorWriteMask = Map::ColorComponent( vBlendStates[i].writeMask );
                                vkBlendState.dstAlphaBlendFactor = Map::BlendFactor( vBlendStates[i].Alpha.dst );
                                vkBlendState.dstColorBlendFactor = Map::BlendFactor( vBlendStates[i].Color.dst );
                                vkBlendState.srcAlphaBlendFactor = Map::BlendFactor( vBlendStates[i].Alpha.src );
                                vkBlendState.srcColorBlendFactor = Map::BlendFactor( vBlendStates[i].Color.src );
                            }

                            State.pAttachments = &vVkBlendStates[0];
                            State.attachmentCount = vVkBlendStates.GetCount();
                            State.flags = 0;
                            State.logicOp = Map::LogicOperation( Desc.Blending.logicOperation );
                            State.logicOpEnable = Desc.Blending.logicOperation != 0;
                            memset( State.blendConstants, 0, sizeof( float ) * 4 );
                        }
                    }
                }
                ci.pColorBlendState = &VkColorBlendState;

                VkPipelineDepthStencilStateCreateInfo VkDepthStencil;
                {
                    auto& State = VkDepthStencil;
                    State.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
                    State.pNext = nullptr;
                    State.flags = 0;

                    if( Desc.DepthStencil.enable )
                    {
                        {
                            VkStencilOpState VkFace;
                            const auto& Face = Desc.DepthStencil.BackFace;

                            VkFace.compareMask = Face.compareMask;
                            VkFace.compareOp = Map::CompareOperation( Face.compareOp );
                            VkFace.depthFailOp = Map::StencilOperation( Face.depthFailOp );
                            VkFace.failOp = Map::StencilOperation( Face.failOp );
                            VkFace.passOp = Map::StencilOperation( Face.passOp );
                            VkFace.reference = Face.reference;
                            VkFace.writeMask = Face.writeMask;
                        }
                        {
                            VkStencilOpState VkFace;
                            const auto& Face = Desc.DepthStencil.FrontFace;

                            VkFace.compareMask = Desc.DepthStencil.BackFace.compareMask;
                            VkFace.compareOp = Map::CompareOperation( Face.compareOp );
                            VkFace.depthFailOp = Map::StencilOperation( Face.depthFailOp );
                            VkFace.failOp = Map::StencilOperation( Face.failOp );
                            VkFace.passOp = Map::StencilOperation( Face.passOp );
                            VkFace.reference = Face.reference;
                            VkFace.writeMask = Face.writeMask;
                        }
                        State.depthBoundsTestEnable = Desc.DepthStencil.DepthBounds.enable;
                        State.depthCompareOp = Map::CompareOperation( Desc.DepthStencil.depthFunction );
                        State.depthTestEnable = Desc.DepthStencil.enableDepthTest;
                        State.depthWriteEnable = Desc.DepthStencil.enableDepthWrite;
                        State.maxDepthBounds = Desc.DepthStencil.DepthBounds.max;
                        State.minDepthBounds = Desc.DepthStencil.DepthBounds.min;
                        State.stencilTestEnable = Desc.DepthStencil.enableStencilTest;

                        ci.pDepthStencilState = &VkDepthStencil;
                    }
                }

                VkPipelineDynamicStateCreateInfo VkDynamicState = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
                {
                    auto& State = VkDynamicState;
                    ci.pDynamicState = nullptr;
                }

                VkPipelineMultisampleStateCreateInfo VkMultisampling;
                {
                    auto& State = VkMultisampling;
                    State.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
                    State.pNext = nullptr;
                    State.flags = 0;
                    if( Desc.Multisampling.enable )
                    {
                        State.alphaToCoverageEnable = false;
                        State.alphaToOneEnable = false;
                        State.minSampleShading = 0;
                        State.pSampleMask = nullptr;
                        State.rasterizationSamples = Vulkan::Map::SampleCount( Desc.Multisampling.sampleCount );
                        State.sampleShadingEnable = false;
                        
                        ci.pMultisampleState = &VkMultisampling;
                    }
                }

                VkPipelineRasterizationStateCreateInfo VkRasterization;
                {
                    auto& State = VkRasterization;
                    State.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
                    State.pNext = nullptr;
                    State.flags = 0;

                    State.cullMode = Map::CullMode( Desc.Rasterization.Polygon.cullMode );
                    State.depthBiasClamp = Desc.Rasterization.Depth.biasClampFactor;
                    State.depthBiasConstantFactor = Desc.Rasterization.Depth.biasConstantFactor;
                    State.depthBiasEnable = Desc.Rasterization.Depth.biasConstantFactor != 0.0f;
                    State.depthBiasSlopeFactor = Desc.Rasterization.Depth.biasSlopeFactor;
                    State.depthClampEnable = Desc.Rasterization.Depth.enableClamp;
                    State.frontFace = Map::FrontFace( Desc.Rasterization.Polygon.frontFace );
                    State.lineWidth = 1;
                    State.polygonMode = Map::PolygonMode( Desc.Rasterization.Polygon.mode );
                    State.rasterizerDiscardEnable = true;

                    ci.pRasterizationState = &VkRasterization;
                }

                VkShaderStageFlags vkShaderStages = 0;
                Utils::TCDynamicArray< VkPipelineShaderStageCreateInfo, ShaderTypes::_MAX_COUNT > vVkStages;
                uint32_t stageCount = 0;
                {
                    for( uint32_t i = 0; i < ShaderTypes::_MAX_COUNT; ++i )
                    {
                        if( Desc.Shaders.apShaders[i].IsValid() )
                        {
                            auto pShader = Desc.Shaders.apShaders[ i ]; //m_pCtx->GetShader( Desc.Shaders.apShaders[i] );
                            VkPipelineShaderStageCreateInfo State;
                            State.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
                            State.pName = nullptr;
                            State.flags = 0;
                            {
                                if( VKE_FAILED( pShader->Compile() ) )
                                {
                                }
                                State.module = pShader->GetDDIObject();
                                State.pName = pShader->GetDesc().pEntryPoint;
                                State.stage = Map::ShaderStage( static_cast<SHADER_TYPE>(i) );
                                State.pSpecializationInfo = nullptr;
                                vkShaderStages |= State.stage;
                                stageCount++;
                                vVkStages.PushBack( State );
                            }
                        }
                    }
                }

                ci.stageCount = stageCount;
                ci.pStages = &vVkStages[0];

                VkPipelineTessellationStateCreateInfo VkTesselation = { VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO };
                if( Desc.Tesselation.enable )
                {
                    auto& State = VkTesselation;
                    {
                        State.patchControlPoints = 0;
                    }
                    ci.pTessellationState = &VkTesselation;
                }

                VkPipelineInputAssemblyStateCreateInfo VkInputAssembly = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
                {
                    auto& State = VkInputAssembly;
                    {
                        State.primitiveRestartEnable = Desc.InputLayout.enablePrimitiveRestart;
                        State.topology = Map::PrimitiveTopology( Desc.InputLayout.topology );
                    }
                }
                ci.pInputAssemblyState = &VkInputAssembly;

                VkPipelineVertexInputStateCreateInfo VkVertexInput = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO  };
                {
                    auto& State = VkVertexInput;
                    const auto& vAttribs = Desc.InputLayout.vVertexAttributes;
                    if( !vAttribs.IsEmpty() )
                    {           
                        {
                            Utils::TCDynamicArray< VkVertexInputAttributeDescription, Config::RenderSystem::Pipeline::MAX_VERTEX_ATTRIBUTE_COUNT > vVkAttribs;
                            Utils::TCDynamicArray< VkVertexInputBindingDescription, Config::RenderSystem::Pipeline::MAX_VERTEX_INPUT_BINDING_COUNT > vVkBindings;
                            vVkAttribs.Resize( vAttribs.GetCount() );
                            vVkBindings.Resize( vAttribs.GetCount() );
                            SDescriptorSetLayoutDesc::BindingArray vBindings;
                            vBindings.Resize( vAttribs.GetCount() );
                            for( uint32_t i = 0; i < vAttribs.GetCount(); ++i )
                            {
                                auto& vkAttrib = vVkAttribs[i];
                                vkAttrib.binding = vAttribs[i].binding;
                                vkAttrib.format = Vulkan::Map::Format( vAttribs[i].format );
                                vkAttrib.location = vAttribs[i].location;
                                vkAttrib.offset = vAttribs[i].offset;

                                auto& vkBinding = vVkBindings[i];
                                vkBinding.binding = vAttribs[i].binding;
                                vkBinding.inputRate = Vulkan::Map::InputRate( vAttribs[i].inputRate );
                                vkBinding.stride = vAttribs[i].stride;
                            }

                            State.pVertexAttributeDescriptions = &vVkAttribs[0];
                            State.pVertexBindingDescriptions = &vVkBindings[0];
                            State.vertexAttributeDescriptionCount = vVkAttribs.GetCount();
                            State.vertexBindingDescriptionCount = vVkBindings.GetCount();
                        }
                    }
                    else
                    {
                        VKE_LOG_ERR( "GraphicsPipeline has no InputLayout.VertexAttribute." );
                    }
                }
                ci.pVertexInputState = &VkVertexInput;

                VkPipelineViewportStateCreateInfo VkViewportState = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
                if( Desc.Viewport.enable )
                {
                    auto& State = VkViewportState;
                    {
                        using VkViewportArray = Utils::TCDynamicArray< VkViewport, Config::RenderSystem::Pipeline::MAX_VIEWPORT_COUNT >;
                        using VkScissorArray = Utils::TCDynamicArray< VkRect2D, Config::RenderSystem::Pipeline::MAX_SCISSOR_COUNT >;
                        VkViewportArray vVkViewports;
                        VkScissorArray vVkScissors;

                        for( uint32_t i = 0; i < Desc.Viewport.vViewports.GetCount(); ++i )
                        {
                            const auto& Viewport = Desc.Viewport.vViewports[i];
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
                            const auto& Scissor = Desc.Viewport.vScissors[i];
                            VkRect2D vkScissor;
                            vkScissor.extent.width = Scissor.Size.width;
                            vkScissor.extent.height = Scissor.Size.height;
                            vkScissor.offset.x = Scissor.Position.x;
                            vkScissor.offset.y = Scissor.Position.y;

                            vVkScissors.PushBack( vkScissor );
                        }
                        State.pViewports = &vVkViewports[0];
                        State.viewportCount = vVkViewports.GetCount();
                        State.pScissors = &vVkScissors[0];
                        State.scissorCount = vVkScissors.GetCount();
                    }
                    ci.pViewportState = &VkViewportState;
                }

                //VkGraphicsInfo.layout = reinterpret_cast< VkPipelineLayout >( Desc.hLayout.handle );
                //VkGraphicsInfo.renderPass = reinterpret_cast< VkRenderPass >( Desc.hRenderPass.handle );
                VkGraphicsInfo.layout = m_pCtx->GetPipelineLayout( Desc.hLayout )->GetDDIObject();
                VkGraphicsInfo.renderPass = m_pCtx->GetRenderPass( Desc.hRenderPass )->GetDDIObject();

                vkRes = m_ICD.vkCreateGraphicsPipelines( m_hDevice, VK_NULL_HANDLE, 1, &VkGraphicsInfo, nullptr, &hPipeline );
            }
            else
            {
                VkComputePipelineCreateInfo& ci = VkComputeInfo;
                ci.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
                ci.pNext = nullptr;
                ci.flags = 0;
                ci.basePipelineHandle = VK_NULL_HANDLE;
                ci.basePipelineIndex = -1;
                
                //auto pShader = m_pCtx->GetShader( Desc.Shaders.apShaders[ ShaderTypes::COMPUTE ] );
                auto pShader = Desc.Shaders.apShaders[ ShaderTypes::COMPUTE ];
                
                ci.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
                ci.stage.pName = nullptr;
                ci.stage.flags = 0;
                {
                    if( VKE_FAILED( pShader->Compile() ) )
                    {
                    }
                    ci.stage.module = pShader->GetDDIObject();
                    ci.stage.pName = pShader->GetDesc().pEntryPoint;
                    ci.stage.stage = Map::ShaderStage( static_cast<SHADER_TYPE>(ShaderTypes::COMPUTE) );
                    ci.stage.pSpecializationInfo = nullptr;
                }
                
                VkComputeInfo.layout = reinterpret_cast<VkPipelineLayout>(Desc.hLayout.handle);
                vkRes = m_ICD.vkCreateComputePipelines( m_hDevice, VK_NULL_HANDLE, 1, &VkComputeInfo, pVkCallbacks, &hPipeline );
            }

            VK_ERR( vkRes );
        
            return hPipeline;
        }

        void CDDI::DestroyObject( DDIPipeline* phPipeline, const void* pAllocator )
        {
            DDI_DESTROY_OBJECT( Pipeline, phPipeline, pAllocator );
        }

        DDIDescriptorSetLayout CDDI::CreateObject( const SDescriptorSetLayoutDesc& Desc, const void* pAllocator )
        {
            DDIDescriptorSetLayout hLayout = DDI_NULL_HANDLE;

            VkDescriptorSetLayoutCreateInfo ci;
            ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            ci.pNext = nullptr;
            ci.flags = 0;
            ci.bindingCount = Desc.vBindings.GetCount();

            using VkBindingArray = Utils::TCDynamicArray< VkDescriptorSetLayoutBinding, Config::RenderSystem::Pipeline::MAX_DESCRIPTOR_BINDING_COUNT >;
            VkBindingArray vVkBindings;
            if( vVkBindings.Resize( ci.bindingCount ) )
            {
                for( uint32_t i = 0; i < ci.bindingCount; ++i )
                {
                    auto& VkBinding = vVkBindings[i];
                    const auto& Binding = Desc.vBindings[i];
                    VkBinding.binding = Binding.idx;
                    VkBinding.descriptorCount = Binding.count;
                    VkBinding.descriptorType = Vulkan::Map::DescriptorType( Binding.type );
                    VkBinding.pImmutableSamplers = nullptr;
                    VkBinding.stageFlags = Vulkan::Convert::PipelineStages( Binding.stages );

                    //hash ^= ( Binding.idx << 1 ) + ( Binding.count << 1 ) + ( Binding.type << 1 ) + ( Binding.stages << 1 );
                    /*Hash::Combine( &hash, Binding.idx );
                    Hash::Combine( &hash, Binding.count );
                    Hash::Combine( &hash, Binding.type );
                    Hash::Combine( &hash, Binding.stages );*/
                }
                ci.pBindings = &vVkBindings[0];

                VK_ERR( DDI_CREATE_OBJECT( DescriptorSetLayout, ci, pAllocator, &hLayout ) );
            }

            return hLayout;
        }

        void CDDI::DestroyObject( DDIDescriptorSetLayout* phLayout, const void* pAllocator )
        {
            DDI_DESTROY_OBJECT( DescriptorSetLayout, phLayout, pAllocator );
        }
        
        DDIPipelineLayout CDDI::CreateObject( const SPipelineLayoutDesc& Desc, const void* pAllocator )
        {
            DDIPipelineLayout hLayout = DDI_NULL_HANDLE;
            VkPipelineLayoutCreateInfo ci;
            ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            ci.pNext = nullptr;
            ci.flags = 0;
            
            //VKE_ASSERT( !Desc.vDescriptorSetLayouts.IsEmpty(), "There should be at least one DescriptorSetLayout." );
            ci.setLayoutCount = Desc.vDescriptorSetLayouts.GetCount();
            static const auto MAX_COUNT = Config::RenderSystem::Pipeline::MAX_PIPELINE_LAYOUT_DESCRIPTOR_SET_COUNT;
            Utils::TCDynamicArray< VkDescriptorSetLayout, MAX_COUNT > vVkDescLayouts;
            for( uint32_t i = 0; i < ci.setLayoutCount; ++i )
            {
                vVkDescLayouts.PushBack( reinterpret_cast<DDIDescriptorSetLayout>(Desc.vDescriptorSetLayouts[i].handle) );
            }
            ci.pSetLayouts = vVkDescLayouts.GetData();
            ci.pPushConstantRanges = nullptr;
            ci.pushConstantRangeCount = 0;

            VK_ERR( DDI_CREATE_OBJECT( PipelineLayout, ci, pAllocator, &hLayout ) );

            return hLayout;
        }

        void CDDI::DestroyObject( DDIPipelineLayout* phLayout, const void* pAllocator )
        {
            DDI_DESTROY_OBJECT( PipelineLayout, phLayout, pAllocator );
        }

        DDIShader CDDI::CreateObject( const SShaderData& Data, const void* pAllocator )
        {
            VKE_ASSERT( Data.state == ShaderStates::COMPILED_IR_BINARY &&
                Data.codeSize > 0 && Data.codeSize % 4 == 0 &&
                Data.pCode != nullptr, "Invalid shader data." );

            DDIShader hShader = DDI_NULL_HANDLE;
            VkShaderModuleCreateInfo ci;
            ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            ci.pNext = nullptr;
            ci.flags = 0;
            ci.pCode = reinterpret_cast< const uint32_t* >( Data.pCode );
            ci.codeSize = Data.codeSize;
            VK_ERR( DDI_CREATE_OBJECT( ShaderModule, ci, pAllocator, &hShader ) );
            return hShader;
        }

        void CDDI::DestroyObject( DDIShader* phShader, const void* pAllocator )
        {
            DDI_DESTROY_OBJECT( ShaderModule, phShader, pAllocator );
        }

        Result CDDI::AllocateObjects(const AllocateDescs::SDescSet& Info, DDIDescriptorSet* pSets )
        {
            Result ret = VKE_FAIL;
            VkDescriptorSetAllocateInfo ai;
            ai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            ai.pNext = nullptr;
            ai.descriptorPool = Info.hPool;
            ai.descriptorSetCount = Info.count;
            ai.pSetLayouts = Info.phLayouts;
            VkResult res = m_ICD.vkAllocateDescriptorSets( m_hDevice, &ai, pSets );
            VK_ERR( res );
            if( res == VK_SUCCESS )
            {
                ret = VKE_OK;
            }
            return ret;
        }

        void CDDI::FreeObjects( const FreeDescs::SDescSet& Desc )
        {
            m_ICD.vkFreeDescriptorSets( m_hDevice, Desc.hPool, Desc.count, Desc.phSets );
        }

        Result CDDI::AllocateObjects( const SAllocateCommandBufferInfo& Info, DDICommandBuffer* pBuffers )
        {
            Result ret = VKE_FAIL;
            VkCommandBufferAllocateInfo ai;
            ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            ai.pNext = nullptr;
            ai.level = Map::CommandBufferLevel( Info.level );
            ai.commandBufferCount = Info.count;
            ai.commandPool = Info.hDDIPool;
            VkResult res = m_ICD.vkAllocateCommandBuffers( m_hDevice, &ai, pBuffers );
            VK_ERR( res );
            ret = res == VK_SUCCESS ? VKE_OK : VKE_ENOMEMORY;
            return ret;
        }

        void CDDI::FreeObjects( const SFreeCommandBufferInfo& Info )
        {
            m_ICD.vkFreeCommandBuffers( m_hDevice, Info.hDDIPool, Info.count, Info.pDDICommandBuffers );
        }

        vke_force_inline
        int32_t FindMemoryTypeIndex( const VkPhysicalDeviceMemoryProperties* pMemProps,
            uint32_t requiredMemBits,
            VkMemoryPropertyFlags requiredProperties )
        {
            const uint32_t memCount = pMemProps->memoryTypeCount;
            for( uint32_t memIdx = 0; memIdx < memCount; ++memIdx )
            {
                const uint32_t memTypeBits = (1 << memIdx);
                const bool isRequiredMemType = requiredMemBits & memTypeBits;
                const VkMemoryPropertyFlags props = pMemProps->memoryTypes[memIdx].propertyFlags;
                const bool hasRequiredProps = (props & requiredProperties) == requiredProperties;
                if( isRequiredMemType && hasRequiredProps )
                    return static_cast<int32_t>(memIdx);
            }
            return -1;
        }

        Result CDDI::_Allocate( const AllocateDescs::SMemory& Desc, const VkMemoryRequirements& vkRequirements,
            const void* pAllocator, SMemoryAllocateData* pData )
        {
            DDIMemory hMemory = DDI_NULL_HANDLE;
            VkMemoryPropertyFlags vkPropertyFlags = Vulkan::Convert::MemoryUsagesToVkMemoryPropertyFlags( Desc.memoryUsages );
            const int32_t idx = FindMemoryTypeIndex( &m_vkMemoryProperties, vkRequirements.memoryTypeBits, vkPropertyFlags );
            VkResult res = VK_NOT_READY;
            if( idx >= 0 )
            {
                VkMemoryAllocateInfo ai;
                ai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
                ai.pNext = nullptr;
                ai.allocationSize = Desc.size;
                ai.memoryTypeIndex = idx;
                VkResult res = m_ICD.vkAllocateMemory( m_hDevice, &ai,
                    reinterpret_cast<const VkAllocationCallbacks*>( pAllocator ), &hMemory );
                VK_ERR( res );
                pData->hMemory = hMemory;
                pData->alignment = static_cast<VkDeviceSize>(vkRequirements.alignment);
                pData->size = static_cast<VkDeviceSize>(vkRequirements.size);
            }
            return res == VK_SUCCESS ? VKE_OK : VKE_FAIL;
        }

        void CDDI::Free( DDIMemory* phMemory, const void* pAllocator )
        {
            if( *phMemory != DDI_NULL_HANDLE )
            {
                m_ICD.vkFreeMemory( m_hDevice, *phMemory, reinterpret_cast<const VkAllocationCallbacks*>(pAllocator) );
            }
            *phMemory = DDI_NULL_HANDLE;
        }

        bool CDDI::IsReady( const DDIFence& hFence )
        {
            return WaitForFences( hFence, 0 ) == VKE_OK;
        }

        void CDDI::Reset( DDIFence* phFence )
        {
            VK_ERR( m_ICD.vkResetFences( m_hDevice, 1, phFence ) );
        }

        Result CDDI::WaitForFences( const DDIFence& hFence, uint64_t timeout )
        {
            VkResult res = m_ICD.vkWaitForFences( m_hDevice, 1, &hFence, VK_TRUE, timeout );
            VK_ERR( res );
            Result ret = VKE_FAIL;
            switch( res )
            {
                case VK_SUCCESS:
                    ret = VKE_OK;
                    break;
                case VK_TIMEOUT:
                    ret = VKE_TIMEOUT;
                    break;
            
                default:
                    break;
            };
            return ret;
        }

        Result CDDI::WaitForQueue( const DDIQueue& hQueue )
        {
            VkResult res = m_ICD.vkQueueWaitIdle( hQueue );
            VK_ERR( res );
            return res == VK_SUCCESS ? VKE_OK : VKE_FAIL;
        }

        Result CDDI::WaitForDevice()
        {
            VkResult res = m_ICD.vkDeviceWaitIdle( m_hDevice );
            VK_ERR( res );
            return res == VK_SUCCESS ? VKE_OK : VKE_FAIL;
        }

        Result CDDI::Submit( const SSubmitInfo& Info )
        {
            Result ret = VKE_FAIL;

            static const VkPipelineStageFlags vkWaitMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            VkSubmitInfo si;
            si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            si.pNext = nullptr;
            si.pSignalSemaphores = Info.pDDISignalSemaphores;
            si.signalSemaphoreCount = Info.signalSemaphoreCount;
            si.pWaitSemaphores = Info.pDDIWaitSemaphores;
            si.waitSemaphoreCount = Info.waitSemaphoreCount;
            si.pWaitDstStageMask = &vkWaitMask;
            si.commandBufferCount = Info.commandBufferCount;
            si.pCommandBuffers = &Info.pDDICommandBuffers[0];
            //VK_ERR( m_pQueue->Submit( ICD, si, pSubmit->m_hDDIFence ) );
            VkResult res = m_ICD.vkQueueSubmit( Info.hDDIQueue, 1, &si, Info.hDDIFence );
            VK_ERR( res );
            ret = res == VK_SUCCESS ? VKE_OK : VKE_FAIL;
            return ret;
        }

        Result CDDI::Present( const SPresentData& Info )
        {
            VkPresentInfoKHR pi;
            pi.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
            pi.pNext = nullptr;
            pi.pImageIndices = &Info.vImageIndices[0];
            pi.pSwapchains = &Info.vSwapchains[0];
            pi.pWaitSemaphores = Info.vWaitSemaphores.GetData();
            pi.pResults = nullptr;
            pi.swapchainCount = Info.vSwapchains.GetCount();
            pi.waitSemaphoreCount = Info.vWaitSemaphores.GetCount();
            
            VkResult res = m_ICD.vkQueuePresentKHR( Info.hQueue, &pi );
            VK_ERR( res );
            return res == VK_SUCCESS ? VKE_OK : VKE_FAIL;
        }

        Result ConvertVkSurfaceFormatToPresentSurfaceFormat( const VkSurfaceFormatKHR& vkFormat,
            SPresentSurfaceFormat* pOut )
        {
            Result ret = VKE_OK;
            switch( vkFormat.colorSpace )
            {
                case VK_COLOR_SPACE_SRGB_NONLINEAR_KHR:
                {
                    pOut->colorSpace = ColorSpaces::SRGB;
                }
                break;
                default:
                {
                    ret = VKE_FAIL;
                }
                break;
            };
            //pOut->format = Vulkan::Convert::ImageFormat( vkFormat.format );
            pOut->format = Convert::ImageFormat( vkFormat.format );
            return ret;
        }

        Result CDDI::CreateSwapChain( const SSwapChainDesc& Desc, const void* pAllocator, SDDISwapChain* pOut )
        {
            Result ret = VKE_FAIL;
            VkResult vkRes;
            DDIPresentSurface hSurface = DDI_NULL_HANDLE;
            uint16_t elementCount = Desc.elementCount;

            if( pOut->hSurface == DDI_NULL_HANDLE )
            {
#if VKE_USE_VULKAN_WINDOWS
                HINSTANCE hInst = reinterpret_cast<HINSTANCE>(Desc.pWindow->GetDesc().hProcess);
                HWND hWnd = reinterpret_cast<HWND>(Desc.pWindow->GetDesc().hWnd);
                VkWin32SurfaceCreateInfoKHR SurfaceCI;
                Vulkan::InitInfo( &SurfaceCI, VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR );
                SurfaceCI.flags = 0;
                SurfaceCI.hinstance = hInst;
                SurfaceCI.hwnd = hWnd;
                vkRes = sInstanceICD.vkCreateWin32SurfaceKHR( sVkInstance, &SurfaceCI, nullptr, &hSurface );
#elif VKE_USE_VULKAN_LINUX
                VkXcbSurfaceCreateInfoKHR SurfaceCI;
                Vulkan::InitInfo( &SurfaceCI, VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR );
                SurfaceCI.flags = 0;
                SurfaceCI.connection = reinterpret_cast<xcb_connection_t*>(m_Desc.hPlatform);
                SurfaceCI.window = m_Desc.hWnd;
                EXPECT_SUCCESS( Vk.vkCreateXcbSurfaceKHR( s_instance, &SurfaceCI, NO_ALLOC_CALLBACK, &s_surface ) )
#elif VKE_USE_VULKAN_ANDROID
                VkAndroidSurfaceCreateInfoKHR SurfaceCI;
                Vulkan::InitInfo( &SurfaceCI, VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR );
                SurfaceCI.flags = 0;
                SurfaceCI.window = m_Desc.hWnd;
                EXPECT_SUCCESS( Vk.vkCreateAndroidSurfaceKHR( s_instance, s_window.window->getNativeHandle(), NO_ALLOC_CALLBACK, &s_surface ) );
#endif
                VK_ERR( vkRes );
                if( vkRes == VK_SUCCESS )
                {
                    VkBool32 isSurfaceSupported = VK_FALSE;
                    const auto queueIndex = Desc.queueFamilyIndex;
                    VK_ERR( sInstanceICD.vkGetPhysicalDeviceSurfaceSupportKHR( m_hAdapter, queueIndex,
                        hSurface, &isSurfaceSupported ) );
                    if( !isSurfaceSupported )
                    {
                        VKE_LOG_ERR( "Queue index: " << queueIndex << " does not support the surface." );
                        sInstanceICD.vkDestroySurfaceKHR( sVkInstance, hSurface,
                            reinterpret_cast<const VkAllocationCallbacks*>(pAllocator) );
                    }
                }

                SPresentSurfaceCaps& Caps = pOut->Caps;
                ret = QueryPresentSurfaceCaps( hSurface, &Caps );
                if( !Caps.canBeUsedAsRenderTarget )
                {
                    VKE_LOG_ERR( "Created present surface can't be used as render target." );
                    goto ERR;
                }
                bool found = false;
                for( auto& format : Caps.vFormats )
                {
                    if( format.colorSpace == Desc.colorSpace )
                    {
                        if( Desc.format == Formats::UNDEFINED || format.format == Desc.format )
                        {
                            pOut->Format = format;
                            found = true;
                            break;
                        }
                    }
                }
                if( !found )
                {
                    VKE_LOG_ERR( "Requested format: " << Desc.format << " / " << Desc.colorSpace <<" is not supported for present surface." );
                    goto ERR;
                }
                
                found = false;
                for( auto& mode : Caps.vModes )
                {
                    if( Desc.mode == PresentModes::UNDEFINED || mode == Desc.mode )
                    {
                        pOut->mode = mode;
                        found = true;
                        break;
                    }
                }
                if( !found )
                {
                    VKE_LOG_ERR( "Requested presentation mode is not supported for presentation surface." );
                    goto ERR;
                }

                pOut->Size = Caps.CurrentSize;
                pOut->hSurface = hSurface;
                if( Constants::_SOptimal::IsOptimal( elementCount ) )
                {
                    elementCount = std::min<uint64_t>( Caps.minImageCount, 2ul );
                }
                else
                {
                    elementCount = std::min<uint64_t>( elementCount, Caps.maxImageCount );
                }
            }

            static const VkColorSpaceKHR aVkColorSpaces[] =
            {
                VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
            };

            static const VkPresentModeKHR aVkModes[] =
            {
                VK_PRESENT_MODE_FIFO_KHR, // as undefined
                VK_PRESENT_MODE_IMMEDIATE_KHR, // immediate
                VK_PRESENT_MODE_MAILBOX_KHR, // mailbox
                VK_PRESENT_MODE_FIFO_KHR, // fifo
                VK_PRESENT_MODE_FIFO_KHR
            };

            static const VkComponentMapping vkDefaultMapping =
            {
                //VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A
                VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
            };

            VkSwapchainKHR hSwapChain = DDI_NULL_HANDLE;
            uint32_t familyIndex = Desc.queueFamilyIndex;
            VkResult res;
            VkSwapchainCreateInfoKHR SwapChainCI;
            {
                auto& ci = SwapChainCI;
                ci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
                ci.pNext = nullptr;
                ci.clipped = VK_TRUE;
                ci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
                ci.flags = 0;
                ci.imageArrayLayers = 1;
                ci.imageColorSpace = aVkColorSpaces[ pOut->Format.colorSpace ];
                ci.imageExtent.width = Desc.Size.width;
                ci.imageExtent.height = Desc.Size.height;
                ci.imageFormat = Vulkan::Map::Format( pOut->Format.format );
                ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
                ci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
                ci.minImageCount = elementCount;
                ci.oldSwapchain = pOut->hSwapChain;
                ci.pQueueFamilyIndices = &familyIndex;
                ci.queueFamilyIndexCount = 1;
                ci.presentMode = aVkModes[ pOut->mode ];
                ci.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
                ci.surface = pOut->hSurface;

                res = m_ICD.vkCreateSwapchainKHR( m_hDevice, &ci,
                    reinterpret_cast<const VkAllocationCallbacks*>(pAllocator), &hSwapChain );
            }

            VK_ERR( res );
            if( res == VK_SUCCESS )
            {
                uint32_t imgCount = 0;
                res = m_ICD.vkGetSwapchainImagesKHR( m_hDevice, hSwapChain, &imgCount, nullptr );
                VK_ERR( res );
                if( res == VK_SUCCESS )
                {
                    if( imgCount <= Desc.elementCount )
                    {
                        pOut->vImages.Resize( imgCount );
                        pOut->vImageViews.Resize( imgCount );
                        res = m_ICD.vkGetSwapchainImagesKHR( m_hDevice, hSwapChain, &imgCount, &pOut->vImages[0] );
                        VK_ERR( res );
                        if( res == VK_SUCCESS )
                        {
                            Utils::TCDynamicArray< VkImageMemoryBarrier > vVkBarriers;

                            for( uint32_t i = 0; i < imgCount; ++i )
                            {
                                VkImageViewCreateInfo ci;
                                ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                                ci.pNext = nullptr;
                                ci.flags = 0;
                                ci.format = SwapChainCI.imageFormat;
                                ci.image = pOut->vImages[i];
                                ci.components = vkDefaultMapping;
                                ci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                                ci.subresourceRange.baseArrayLayer = 0;
                                ci.subresourceRange.baseMipLevel = 0;
                                ci.subresourceRange.layerCount = 1;
                                ci.subresourceRange.levelCount = 1;
                                ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
                                
                                DDITextureView hView;
                                res = m_ICD.vkCreateImageView( m_hDevice, &ci, reinterpret_cast<const VkAllocationCallbacks*>(pAllocator), &hView );
                                VK_ERR( res );
                                if( res != VK_SUCCESS )
                                {
                                    VKE_LOG_ERR( "Unable to create ImageView for SwapChain image." );
                                    goto ERR;
                                }
                                pOut->vImageViews[i] = hView;

                                // Do a barrier for image
                                VkImageMemoryBarrier vkBarrier;
                                vkBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                                vkBarrier.pNext = nullptr;
                                vkBarrier.image = pOut->vImages[i];
                                vkBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                                vkBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
                                vkBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
                                vkBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                                vkBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                                vkBarrier.srcAccessMask = 0;
                                vkBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                                vkBarrier.subresourceRange.baseArrayLayer = 0;
                                vkBarrier.subresourceRange.baseMipLevel = 0;
                                vkBarrier.subresourceRange.layerCount = 1;
                                vkBarrier.subresourceRange.levelCount = 1;
                                vVkBarriers.PushBack( vkBarrier );
                            }
                            {
                                // Change image layout UNDEFINED -> PRESENT
                                VKE_ASSERT( Desc.pCtx != nullptr, "GraphicsContext must be set." );
                                //CommandBufferPtr pCmdBuffer = Desc.pCtx->CreateCommandBuffer( DDI_NULL_HANDLE );
                                //if( pCmdBuffer.IsNull() )
                                //{
                                //    goto ERR;
                                //}
                                //pCmdBuffer->Begin();
                                //DDICommandBuffer hCb = pCmdBuffer->GetDDIObject();
                                //m_ICD.vkCmdPipelineBarrier( hCb,
                                //    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                //    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                //    0, 0, nullptr, 0, nullptr,
                                //    vVkBarriers.GetCount(), &vVkBarriers[0] );
                                //pCmdBuffer->End();
                                //pCmdBuffer->Flush();
                                ////CGraphicsContext::CommandBufferArray vCmdBuffers = { pCmdBuffer };
                                ////Desc.pCtx->_SubmitCommandBuffers( vCmdBuffers, DDI_NULL_HANDLE );
                                //Desc.pCtx->ExecuteCommandBuffers();
                                //Desc.pCtx->Wait();
                                //Desc.pCtx->_FreeCommandBuffer( pCmdBuffer );
                            }
                        }
                        else
                        {
                            VKE_LOG_ERR( "Unable to get Vulkan SwapChain images." );
                            goto ERR;
                        }
                    }
                    else
                    {
                        VKE_LOG_ERR( "imgCount > Desc.elementCount" );
                        goto ERR;
                    }
                }
                else
                {
                    VKE_LOG_ERR( "Unable to get Vulkan SwapChain images." );
                    goto ERR;
                }
            }
            else
            {
                VKE_LOG_ERR( "Unable to create a SwapChain Vulkan object." );
                goto ERR;
            }

            pOut->hSwapChain = hSwapChain;

            ret = VKE_OK;
            return ret;

        ERR:
            for( uint32_t i = 0; i < pOut->vImageViews.GetCount(); ++i )
            {
                DestroyObject( &pOut->vImageViews[i], reinterpret_cast<const VkAllocationCallbacks*>(pAllocator) );
            }
            if( hSwapChain != DDI_NULL_HANDLE )
            {
                m_ICD.vkDestroySwapchainKHR( m_hDevice, hSwapChain, reinterpret_cast<const VkAllocationCallbacks*>(pAllocator) );
            }
            if( hSurface != DDI_NULL_HANDLE )
            {
                sInstanceICD.vkDestroySurfaceKHR( sVkInstance, hSurface,
                    reinterpret_cast<const VkAllocationCallbacks*>( pAllocator ) );
            }
            return ret;
        }

        Result CDDI::QueryPresentSurfaceCaps( const DDIPresentSurface& hSurface, SPresentSurfaceCaps* pOut )
        {
            Result ret = VKE_FAIL;
            VkResult res;

            VkSurfaceCapabilitiesKHR vkSurfaceCaps;
            sInstanceICD.vkGetPhysicalDeviceSurfaceCapabilitiesKHR( m_hAdapter, hSurface, &vkSurfaceCaps );
            auto hasColorAttachment = vkSurfaceCaps.supportedUsageFlags | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

            // Select surface format
            Utils::TCDynamicArray<VkSurfaceFormatKHR> vSurfaceFormats;
            uint32_t formatCount = 0;
            res = sInstanceICD.vkGetPhysicalDeviceSurfaceFormatsKHR( m_hAdapter, hSurface, &formatCount,
                nullptr );
            VK_ERR( res );

            if( res == VK_SUCCESS )
            {
                if( formatCount > 0 )
                {
                    vSurfaceFormats.Resize( formatCount );
                    res = sInstanceICD.vkGetPhysicalDeviceSurfaceFormatsKHR( m_hAdapter, hSurface, &formatCount,
                        &vSurfaceFormats[0] );
                    VK_ERR( res );
                    if( res == VK_SUCCESS )
                    {
                        for( VkSurfaceFormatKHR& vkFormat : vSurfaceFormats )
                        {
                            SPresentSurfaceFormat Format;
                            if( VKE_SUCCEEDED( ConvertVkSurfaceFormatToPresentSurfaceFormat( vkFormat, &Format ) ) )
                            {
                                pOut->vFormats.PushBack( Format );
                            }
                        }
                    }
                }

                // Select present mode
                uint32_t presentCount = 0;
                Utils::TCDynamicArray< VkPresentModeKHR, 8 > vPresents;
                res = sInstanceICD.vkGetPhysicalDeviceSurfacePresentModesKHR( m_hAdapter, hSurface,
                    &presentCount, nullptr );
                VK_ERR( res );
                if( res == VK_SUCCESS )
                {
                    if( presentCount > 0 )
                    {
                        vPresents.Resize( presentCount );
                        res = sInstanceICD.vkGetPhysicalDeviceSurfacePresentModesKHR( m_hAdapter, hSurface,
                            &presentCount, &vPresents[0] );
                        VK_ERR( res );
                        if( res == VK_SUCCESS )
                        {
                            static const PRESENT_MODE aModes[] =
                            {
                                PresentModes::IMMEDIATE,
                                PresentModes::MAILBOX,
                                PresentModes::FIFO,
                                PresentModes::FIFO, // VK FIFO RELAXED
                            };

                            for( VkPresentModeKHR& vkMode : vPresents )
                            {
                                pOut->vModes.PushBack( aModes[vkMode] );
                            }
                        }
                    }
                }

                if( vkSurfaceCaps.maxImageCount == 0 )
                {
                    vkSurfaceCaps.maxImageCount = Constants::RenderSystem::MAX_SWAP_CHAIN_ELEMENTS;
                }

                pOut->MinSize.width = vkSurfaceCaps.minImageExtent.width;
                pOut->MinSize.height = vkSurfaceCaps.minImageExtent.height;
                pOut->MaxSize.width = vkSurfaceCaps.maxImageExtent.width;
                pOut->MaxSize.height = vkSurfaceCaps.maxImageExtent.height;
                pOut->CurrentSize.width = vkSurfaceCaps.currentExtent.width;
                pOut->CurrentSize.height = vkSurfaceCaps.currentExtent.height;
                pOut->minImageCount = vkSurfaceCaps.minImageCount;
                pOut->maxImageCount = vkSurfaceCaps.maxImageCount;
                pOut->canBeUsedAsRenderTarget = hasColorAttachment;
                //pOut->transform = vkSurfaceCaps.currentTransform
            }
            return ret;
        }

        void CDDI::DestroySwapChain( SDDISwapChain* pInOut, const void* pAllocator )
        {
            const VkAllocationCallbacks* pVkAllocator = reinterpret_cast<const VkAllocationCallbacks*>(pAllocator);
            for( uint32_t i = 0; i < pInOut->vImageViews.GetCount(); ++i )
            {
                DestroyObject( &pInOut->vImageViews[i], pVkAllocator );
            }
            if( pInOut->hSwapChain != DDI_NULL_HANDLE )
            {
                m_ICD.vkDestroySwapchainKHR( m_hDevice, pInOut->hSwapChain, pVkAllocator );
                pInOut->hSwapChain = DDI_NULL_HANDLE;
            }
            if( pInOut->hSurface != DDI_NULL_HANDLE )
            {
                sInstanceICD.vkDestroySurfaceKHR( sVkInstance, pInOut->hSurface, pVkAllocator );
                pInOut->hSurface = DDI_NULL_HANDLE;
            }
        }

        uint32_t CDDI::GetCurrentBackBufferIndex( const SDDISwapChain& SwapChain, const SDDIGetBackBufferInfo& Info )
        {
            uint32_t idx;
            VkResult res = m_ICD.vkAcquireNextImageKHR( m_hDevice, SwapChain.hSwapChain, Info.waitTimeout,
                Info.hAcquireSemaphore, Info.hFence, &idx );
            VK_ERR( res );
            return idx;
        }

        void CDDI::Reset( const DDICommandBuffer& hCommandBuffer )
        {
            const auto flags = VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT;
            VK_ERR( m_ICD.vkResetCommandBuffer( hCommandBuffer, flags ) );
        }

        void CDDI::BeginCommandBuffer( const DDICommandBuffer& hCommandBuffer )
        {
            VkCommandBufferBeginInfo bi;
            bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            bi.pNext = nullptr;
            bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            bi.pInheritanceInfo = nullptr;
            VK_ERR( m_ICD.vkBeginCommandBuffer( hCommandBuffer, &bi ) );
        }

        void CDDI::EndCommandBuffer( const DDICommandBuffer& hCommandBuffer )
        {
            VK_ERR( m_ICD.vkEndCommandBuffer( hCommandBuffer ) );
        }

        void CDDI::BeginRenderPass( const DDICommandBuffer& hCommandBuffer, const SBeginRenderPassInfo& Info )
        {
            VkRenderPassBeginInfo bi;
            bi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            bi.pNext = nullptr;
            bi.clearValueCount = Info.vDDIClearValues.GetCount();
            bi.pClearValues = ( &Info.vDDIClearValues[ 0 ] );
            bi.renderArea.extent.width = Info.RenderArea.Size.width;
            bi.renderArea.extent.height = Info.RenderArea.Size.height;
            bi.renderArea.offset = { Info.RenderArea.Offset.x, Info.RenderArea.Offset.y };

            //bi.renderPass = reinterpret_cast<DDIRenderPass>(Info.hPass.handle);
            //bi.framebuffer = reinterpret_cast<DDIFramebuffer>(Info.hFramebuffer.handle);
            bi.renderPass = Info.hDDIRenderPass;
            bi.framebuffer = Info.hDDIFramebuffer;
            m_ICD.vkCmdBeginRenderPass( hCommandBuffer, &bi, VK_SUBPASS_CONTENTS_INLINE );
        }

        void CDDI::EndRenderPass( const DDICommandBuffer& hCommandBuffer )
        {
            m_ICD.vkCmdEndRenderPass( hCommandBuffer );
        }

        void CDDI::Bind( const SBindPipelineInfo& Info )
        {
            m_ICD.vkCmdBindPipeline( Info.pCmdBuffer->GetDDIObject(),
                Convert::PipelineTypeToBindPoint( Info.pPipeline->GetType() ), Info.pPipeline->GetDDIObject() );
        }

        void CDDI::Bind( const SBindRenderPassInfo& Info )
        {
            if( Info.pRenderPassInfo != nullptr )
            {
                BeginRenderPass( Info.pCmdBuffer->GetDDIObject(), *Info.pRenderPassInfo );
            }
            else
            {
                m_ICD.vkCmdEndRenderPass( Info.pCmdBuffer->GetDDIObject() );
            }
        }

        void CDDI::Bind( const SBindDescriptorSetsInfo& Info )
        {
            m_ICD.vkCmdBindDescriptorSets( Info.pCmdBuffer->GetDDIObject(),
                Convert::PipelineTypeToBindPoint( Info.type ), Info.pPipelineLayout->GetDDIObject(),
                Info.firstSet, Info.setCount, Info.aDDISetHandles, Info.dynamicOffsetCount,
                Info.aDynamicOffsets );
        }

        void CDDI::Bind( const SBindVertexBufferInfo& Info )
        {
            m_ICD.vkCmdBindVertexBuffers( Info.pCmdBuffer->GetDDIObject(), 0, 1, &Info.pBuffer->GetDDIObject(),
                &Info.offset );
        }

        void CDDI::Bind( const SBindIndexBufferInfo& Info )
        {
            m_ICD.vkCmdBindIndexBuffer( Info.pCmdBuffer->GetDDIObject(), Info.pBuffer->GetDDIObject(), Info.offset,
                Map::IndexType( Info.pBuffer->GetIndexType() ) );
        }

        void CDDI::Barrier( const DDICommandBuffer& hCommandBuffer, const SBarrierInfo& Info )
        {
            VkMemoryBarrier* pVkMemBarriers = nullptr;
            VkImageMemoryBarrier* pVkImgBarriers = nullptr;
            VkBufferMemoryBarrier* pVkBuffBarrier = nullptr;
            VkPipelineStageFlags srcStage;
            VkPipelineStageFlags dstStage;

            Utils::TCDynamicArray< VkMemoryBarrier, SBarrierInfo::MAX_BARRIER_COUNT > vVkMemBarriers( Info.vMemoryBarriers.GetCount() );
            Utils::TCDynamicArray< VkImageMemoryBarrier, SBarrierInfo::MAX_BARRIER_COUNT > vVkImgBarriers( Info.vTextureBarriers.GetCount() );
            Utils::TCDynamicArray< VkBufferMemoryBarrier, SBarrierInfo::MAX_BARRIER_COUNT > vVkBufferBarriers( Info.vBufferBarriers.GetCount() );

            {
                const auto& Barriers = Info.vMemoryBarriers;
                if( !Barriers.IsEmpty() )
                {
                    for( uint32_t i = 0; i < Barriers.GetCount(); ++i )
                    {
                        vVkMemBarriers[i] = { VK_STRUCTURE_TYPE_MEMORY_BARRIER };
                        Convert::Barrier( &vVkMemBarriers[i], Barriers[i] );
                    }
                }
            }
            {
                const auto& Barriers = Info.vTextureBarriers;
                if( !Barriers.IsEmpty() )
                {
                    for( uint32_t i = 0; i < Barriers.GetCount(); ++i )
                    {
                        vVkImgBarriers[i] = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
                        Convert::Barrier( &vVkImgBarriers[i], Barriers[i] );
                        vVkImgBarriers[i].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                        vVkImgBarriers[i].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    }
                }
            }
            {
                const auto& Barriers = Info.vBufferBarriers;
                if( !Barriers.IsEmpty() )
                {
                    for( uint32_t i = 0; i < Barriers.GetCount(); ++i )
                    {
                        vVkBufferBarriers[i] = { VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
                        Convert::Barrier( &vVkBufferBarriers[i], Barriers[i] );
                        vVkBufferBarriers[i].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                        vVkBufferBarriers[i].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    }
                }
            }

            m_ICD.vkCmdPipelineBarrier( hCommandBuffer, srcStage, dstStage, 0,
                Info.vMemoryBarriers.GetCount(), pVkMemBarriers,
                Info.vBufferBarriers.GetCount(), pVkBuffBarrier,
                Info.vTextureBarriers.GetCount(), pVkImgBarriers );
        }

        Result CDDI::CompileShader( const SCompileShaderInfo& Info, SCompileShaderData* pOut )
        {
            static const EShLanguage aNativeTypes[] =
            {
                EShLangVertex,
                EShLangTessControl,
                EShLangTessEvaluation,
                EShLangGeometry,
                EShLangFragment,
                EShLangCompute
            };

            struct CompilerOptions
            {
                enum TOptions
                {
                    EOptionNone = 0,
                    EOptionIntermediate = (1 << 0),
                    EOptionSuppressInfolog = (1 << 1),
                    EOptionMemoryLeakMode = (1 << 2),
                    EOptionRelaxedErrors = (1 << 3),
                    EOptionGiveWarnings = (1 << 4),
                    EOptionLinkProgram = (1 << 5),
                    EOptionMultiThreaded = (1 << 6),
                    EOptionDumpConfig = (1 << 7),
                    EOptionDumpReflection = (1 << 8),
                    EOptionSuppressWarnings = (1 << 9),
                    EOptionDumpVersions = (1 << 10),
                    EOptionSpv = (1 << 11),
                    EOptionHumanReadableSpv = (1 << 12),
                    EOptionVulkanRules = (1 << 13),
                    EOptionDefaultDesktop = (1 << 14),
                    EOptionOutputPreprocessed = (1 << 15),
                    EOptionOutputHexadecimal = (1 << 16),
                    EOptionReadHlsl = (1 << 17),
                    EOptionCascadingErrors = (1 << 18),
                    EOptionAutoMapBindings = (1 << 19),
                    EOptionFlattenUniformArrays = (1 << 20),
                    EOptionNoStorageFormat = (1 << 21),
                    EOptionKeepUncalled = (1 << 22),
                    EOptionHlslOffsets = (1 << 23),
                    EOptionHlslIoMapping = (1 << 24),
                    EOptionAutoMapLocations = (1 << 25),
                    EOptionDebug = (1 << 26),
                    EOptionStdin = (1 << 27),
                    EOptionOptimizeDisable = (1 << 28),
                    EOptionOptimizeSize = (1 << 29)
                };
            };
            const TBuiltInResource DefaultTBuiltInResource = {
                /* .MaxLights = */ 32,
                /* .MaxClipPlanes = */ 6,
                /* .MaxTextureUnits = */ 32,
                /* .MaxTextureCoords = */ 32,
                /* .MaxVertexAttribs = */ 64,
                /* .MaxVertexUniformComponents = */ 4096,
                /* .MaxVaryingFloats = */ 64,
                /* .MaxVertexTextureImageUnits = */ 32,
                /* .MaxCombinedTextureImageUnits = */ 80,
                /* .MaxTextureImageUnits = */ 32,
                /* .MaxFragmentUniformComponents = */ 4096,
                /* .MaxDrawBuffers = */ 32,
                /* .MaxVertexUniformVectors = */ 128,
                /* .MaxVaryingVectors = */ 8,
                /* .MaxFragmentUniformVectors = */ 16,
                /* .MaxVertexOutputVectors = */ 16,
                /* .MaxFragmentInputVectors = */ 15,
                /* .MinProgramTexelOffset = */ -8,
                /* .MaxProgramTexelOffset = */ 7,
                /* .MaxClipDistances = */ 8,
                /* .MaxComputeWorkGroupCountX = */ 65535,
                /* .MaxComputeWorkGroupCountY = */ 65535,
                /* .MaxComputeWorkGroupCountZ = */ 65535,
                /* .MaxComputeWorkGroupSizeX = */ 1024,
                /* .MaxComputeWorkGroupSizeY = */ 1024,
                /* .MaxComputeWorkGroupSizeZ = */ 64,
                /* .MaxComputeUniformComponents = */ 1024,
                /* .MaxComputeTextureImageUnits = */ 16,
                /* .MaxComputeImageUniforms = */ 8,
                /* .MaxComputeAtomicCounters = */ 8,
                /* .MaxComputeAtomicCounterBuffers = */ 1,
                /* .MaxVaryingComponents = */ 60,
                /* .MaxVertexOutputComponents = */ 64,
                /* .MaxGeometryInputComponents = */ 64,
                /* .MaxGeometryOutputComponents = */ 128,
                /* .MaxFragmentInputComponents = */ 128,
                /* .MaxImageUnits = */ 8,
                /* .MaxCombinedImageUnitsAndFragmentOutputs = */ 8,
                /* .MaxCombinedShaderOutputResources = */ 8,
                /* .MaxImageSamples = */ 0,
                /* .MaxVertexImageUniforms = */ 0,
                /* .MaxTessControlImageUniforms = */ 0,
                /* .MaxTessEvaluationImageUniforms = */ 0,
                /* .MaxGeometryImageUniforms = */ 0,
                /* .MaxFragmentImageUniforms = */ 8,
                /* .MaxCombinedImageUniforms = */ 8,
                /* .MaxGeometryTextureImageUnits = */ 16,
                /* .MaxGeometryOutputVertices = */ 256,
                /* .MaxGeometryTotalOutputComponents = */ 1024,
                /* .MaxGeometryUniformComponents = */ 1024,
                /* .MaxGeometryVaryingComponents = */ 64,
                /* .MaxTessControlInputComponents = */ 128,
                /* .MaxTessControlOutputComponents = */ 128,
                /* .MaxTessControlTextureImageUnits = */ 16,
                /* .MaxTessControlUniformComponents = */ 1024,
                /* .MaxTessControlTotalOutputComponents = */ 4096,
                /* .MaxTessEvaluationInputComponents = */ 128,
                /* .MaxTessEvaluationOutputComponents = */ 128,
                /* .MaxTessEvaluationTextureImageUnits = */ 16,
                /* .MaxTessEvaluationUniformComponents = */ 1024,
                /* .MaxTessPatchComponents = */ 120,
                /* .MaxPatchVertices = */ 32,
                /* .MaxTessGenLevel = */ 64,
                /* .MaxViewports = */ 16,
                /* .MaxVertexAtomicCounters = */ 0,
                /* .MaxTessControlAtomicCounters = */ 0,
                /* .MaxTessEvaluationAtomicCounters = */ 0,
                /* .MaxGeometryAtomicCounters = */ 0,
                /* .MaxFragmentAtomicCounters = */ 8,
                /* .MaxCombinedAtomicCounters = */ 8,
                /* .MaxAtomicCounterBindings = */ 1,
                /* .MaxVertexAtomicCounterBuffers = */ 0,
                /* .MaxTessControlAtomicCounterBuffers = */ 0,
                /* .MaxTessEvaluationAtomicCounterBuffers = */ 0,
                /* .MaxGeometryAtomicCounterBuffers = */ 0,
                /* .MaxFragmentAtomicCounterBuffers = */ 1,
                /* .MaxCombinedAtomicCounterBuffers = */ 1,
                /* .MaxAtomicCounterBufferSize = */ 16384,
                /* .MaxTransformFeedbackBuffers = */ 4,
                /* .MaxTransformFeedbackInterleavedComponents = */ 64,
                /* .MaxCullDistances = */ 8,
                /* .MaxCombinedClipAndCullDistances = */ 8,
                /* .MaxSamples = */ 4,
                /* .limits = */{
                /* .nonInductiveForLoops = */ 1,
                /* .whileLoops = */ 1,
                /* .doWhileLoops = */ 1,
                /* .generalUniformIndexing = */ 1,
                /* .generalAttributeMatrixVectorIndexing = */ 1,
                /* .generalVaryingIndexing = */ 1,
                /* .generalSamplerIndexing = */ 1,
                /* .generalVariableIndexing = */ 1,
                /* .generalConstantMatrixVectorIndexing = */ 1,
            } };


            Result ret = VKE_FAIL;
            EShLanguage type = aNativeTypes[ Info.type ];
            //SCompilerData* pCompilerData = reinterpret_cast<SCompilerData*>(Info.pCompilerData);
            Helper::SCompilerData CompilerData;
            CompilerData.Create( type );
            
            CompilerData.pShader->setEntryPoint( Info.pEntryPoint );
            CompilerData.pShader->setStrings( &Info.pBuffer, 1 );

            EShMessages messages = static_cast<EShMessages>(EShMsgSpvRules | EShMsgVulkanRules);
            bool result = CompilerData.pShader->parse( &DefaultTBuiltInResource, 110, false, messages );
            if( result )
            {
                CompilerData.pProgram->addShader( CompilerData.pShader );
                EShMessages messages = static_cast<EShMessages>(EShMsgSpvRules | EShMsgVulkanRules);
                result = CompilerData.pProgram->link( messages );
                if( result )
                {
                    result = CompilerData.pProgram->buildReflection();
                    if( result )
                    {
                        glslang::SpvOptions Options;
#if VKE_RENDERER_DEBUG
                        Options.disableOptimizer = true;
                        Options.generateDebugInfo = true;
                        Options.optimizeSize = false;
#else
                        Options.disableOptimizer = false;
                        Options.generateDebugInfo = false;
                        Options.optimizeSize = true;
#endif
                        spv::SpvBuildLogger Logger;

                        glslang::TIntermediate* pIntermediate = CompilerData.pProgram->getIntermediate( type );
                        if( pIntermediate )
                        {
                            auto& vData = pOut->vShaderBinary;
                            vData.reserve( Config::RenderSystem::Shader::DEFAULT_SHADER_BINARY_SIZE );
                            glslang::GlslangToSpv( *pIntermediate, vData, &Logger, &Options );
                            pOut->codeByteSize = sizeof( SCompileShaderData::BinaryElement ) * vData.size();
                        }
#if VKE_RENDERER_DEBUG
                        CompilerData.pProgram->dumpReflection();
#endif // VKE_RENDERER_DEBUG
                        ret = VKE_OK;
                    }
                    else
                    {
                        VKE_LOG_ERR( "Failed to build linker reflection.\n" << CompilerData.pProgram->getInfoLog() );
                    }
                }
                else
                {
                    VKE_LOG_ERR( "Failed to link shaders.\n" << CompilerData.pProgram->getInfoLog() );
                }
            }
            else
            {
                VKE_LOG_ERR( "Compiile shader: " << Info.pName << " failed.\n" << CompilerData.pShader->getInfoLog() );

            }
            return ret;
        }


        void CDDI::Convert( const SClearValue& In, DDIClearValue* pOut )
        {
            /*pOut->color.float32[0] = In.Color.floats[0];
            pOut->color.float32[1] = In.Color.floats[1];
            pOut->color.float32[2] = In.Color.floats[2];
            pOut->color.float32[3] = In.Color.floats[3];*/
            Memory::Copy( &pOut->color, sizeof(pOut->color), &In.Color, sizeof(In.Color) );
            pOut->depthStencil.depth = In.DepthStencil.depth;
            pOut->depthStencil.stencil = In.DepthStencil.stencil;
        }

    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER