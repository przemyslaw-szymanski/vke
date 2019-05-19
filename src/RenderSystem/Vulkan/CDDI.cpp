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
        VkDebugReportCallbackEXT CDDI::sVkDebugReportCallback = VK_NULL_HANDLE;
        VkDebugUtilsMessengerEXT CDDI::sVkDebugMessengerCallback = VK_NULL_HANDLE;
        CDDI::AdapterArray CDDI::svAdapters;

        VKAPI_ATTR VkBool32 VKAPI_CALL VkDebugCallback( VkDebugReportFlagsEXT msgFlags,
                                                        VkDebugReportObjectTypeEXT objType,
                                                        uint64_t srcObject, size_t location,
                                                        int32_t msgCode, const char *pLayerPrefix,
                                                        const char *pMsg, void *pUserData );

        VKAPI_ATTR VkBool32 VkDebugMessengerCallback(
            VkDebugUtilsMessageSeverityFlagBitsEXT           messageSeverity,
            VkDebugUtilsMessageTypeFlagsEXT                  messageTypes,
            const VkDebugUtilsMessengerCallbackDataEXT*      pCallbackData,
            void*                                            pUserData );
        

        namespace Map
        {
            VkFormat Format( uint32_t format )
            {
                return VKE::RenderSystem::g_aFormats[ format ];
            }

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

            VkImageUsageFlags ImageUsage( RenderSystem::TEXTURE_USAGE usage )
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

            VkImageLayout ImageLayout( RenderSystem::TEXTURE_STATE layout )
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

            VkMemoryPropertyFlags MemoryPropertyFlags( RenderSystem::MEMORY_USAGE usages )
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
                    VK_FRONT_FACE_CLOCKWISE,
                    VK_FRONT_FACE_COUNTER_CLOCKWISE
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
                if( stages & RenderSystem::PipelineStages::TS_DOMAIN )
                {
                    vkFlags |= VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT;
                }
                if( stages & RenderSystem::PipelineStages::TS_HULL )
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

            VkImageTiling ImageUsageToTiling( const RenderSystem::TEXTURE_USAGE& usage )
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

            VkMemoryPropertyFlags MemoryUsagesToVkMemoryPropertyFlags( const RenderSystem::MEMORY_USAGE& usages )
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
                if( type & MemoryAccessTypes::VS_SHADER_READ ||
                    type & MemoryAccessTypes::PS_SHADER_READ || 
                    type & MemoryAccessTypes::GS_SHADER_READ || 
                    type & MemoryAccessTypes::CS_SHADER_READ || 
                    type & MemoryAccessTypes::TS_SHADER_READ || 
                    type & MemoryAccessTypes::MS_SHADER_READ || 
                    type & MemoryAccessTypes::RS_SHADER_READ )
                {
                    vkFlags |= VK_ACCESS_SHADER_READ_BIT;
                }
                if( type & MemoryAccessTypes::VS_SHADER_WRITE ||
                    type & MemoryAccessTypes::PS_SHADER_WRITE ||
                    type & MemoryAccessTypes::GS_SHADER_WRITE ||
                    type & MemoryAccessTypes::CS_SHADER_WRITE ||
                    type & MemoryAccessTypes::TS_SHADER_WRITE ||
                    type & MemoryAccessTypes::MS_SHADER_WRITE ||
                    type & MemoryAccessTypes::RS_SHADER_WRITE )
                {
                    vkFlags |= VK_ACCESS_SHADER_WRITE_BIT;
                }
                if( type & MemoryAccessTypes::VS_UNIFORM_READ ||
                    type & MemoryAccessTypes::PS_UNIFORM_READ ||
                    type & MemoryAccessTypes::GS_UNIFORM_READ ||
                    type & MemoryAccessTypes::CS_UNIFORM_READ ||
                    type & MemoryAccessTypes::TS_UNIFORM_READ ||
                    type & MemoryAccessTypes::MS_UNIFORM_READ ||
                    type & MemoryAccessTypes::RT_UNIFORM_READ )
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
                pOut->image = Info.hDDITexture;
                pOut->oldLayout = Map::ImageLayout( Info.currentState );
                pOut->newLayout = Map::ImageLayout( Info.newState );
                Convert::TextureSubresourceRange( &pOut->subresourceRange, Info.SubresourceRange );
            }

            void Barrier( VkBufferMemoryBarrier* pOut, const SBufferBarrierInfo& Info )
            {
                pOut->srcAccessMask = Convert::AccessMask( Info.srcMemoryAccess );
                pOut->dstAccessMask = Convert::AccessMask( Info.dstMemoryAccess );
                pOut->buffer = Info.hDDIBuffer;
                pOut->offset = Info.offset;
                pOut->size = Info.size;
            }

            VkPipelineStageFlags AccessMaskToPipelineStage( const MEMORY_ACCESS_TYPE& flags )
            {
                VkPipelineStageFlags ret = 0;
                if( flags & MemoryAccessTypes::INDIRECT_BUFFER_READ )
                {
                    ret |= VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
                }
                if( flags & MemoryAccessTypes::INDEX_READ )
                {
                    ret |= VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
                }
                if( flags & MemoryAccessTypes::VERTEX_ATTRIBUTE_READ )
                {
                    ret |= VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
                }
                if( flags & MemoryAccessTypes::VS_UNIFORM_READ ||
                    flags & MemoryAccessTypes::VS_SHADER_READ || 
                    flags & MemoryAccessTypes::VS_SHADER_WRITE )
                {
                    ret |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
                }
                if( flags & MemoryAccessTypes::PS_UNIFORM_READ ||
                    flags & MemoryAccessTypes::PS_SHADER_READ ||
                    flags & MemoryAccessTypes::PS_SHADER_WRITE )
                {
                    ret |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                }
                if( flags & MemoryAccessTypes::GS_UNIFORM_READ ||
                    flags & MemoryAccessTypes::GS_SHADER_READ ||
                    flags & MemoryAccessTypes::GS_SHADER_WRITE )
                {
                    ret |= VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT;
                }
                if( flags & MemoryAccessTypes::TS_UNIFORM_READ ||
                    flags & MemoryAccessTypes::TS_SHADER_READ ||
                    flags & MemoryAccessTypes::TS_SHADER_WRITE )
                {
                    ret |= VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT | VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT;
                }
                if( flags & MemoryAccessTypes::CS_UNIFORM_READ ||
                    flags & MemoryAccessTypes::CS_SHADER_READ ||
                    flags & MemoryAccessTypes::CS_SHADER_WRITE )
                {
                    ret |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
                }
                if( flags & MemoryAccessTypes::MS_UNIFORM_READ ||
                    flags & MemoryAccessTypes::MS_SHADER_READ ||
                    flags & MemoryAccessTypes::MS_SHADER_WRITE )
                {
                    ret |= VK_PIPELINE_STAGE_TASK_SHADER_BIT_NV | VK_PIPELINE_STAGE_MESH_SHADER_BIT_NV;
                }
                if( flags & MemoryAccessTypes::RT_UNIFORM_READ ||
                    flags & MemoryAccessTypes::RS_SHADER_READ ||
                    flags & MemoryAccessTypes::RS_SHADER_WRITE )
                {
                    ret |= VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_NV;
                }
                if( flags & MemoryAccessTypes::INPUT_ATTACHMENT_READ )
                {
                    ret |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                }
                if( flags & MemoryAccessTypes::COLOR_ATTACHMENT_READ ||
                    flags & MemoryAccessTypes::COLOR_ATTACHMENT_WRITE )
                {
                    ret |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                }
                if( flags & MemoryAccessTypes::DEPTH_STENCIL_ATTACHMENT_READ ||
                    flags & MemoryAccessTypes::DEPTH_STENCIL_ATTACHMENT_WRITE )
                {
                    ret |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
                }
                if( flags & MemoryAccessTypes::DATA_TRANSFER_READ ||
                    flags & MemoryAccessTypes::DATA_TRANSFER_WRITE )
                {
                    ret |= VK_PIPELINE_STAGE_TRANSFER_BIT;
                }

                if( flags & MemoryAccessTypes::CPU_MEMORY_READ ||
                    flags & MemoryAccessTypes::CPU_MEMORY_WRITE )
                {
                    ret |= VK_PIPELINE_STAGE_HOST_BIT;
                }

                if( flags & MemoryAccessTypes::GPU_MEMORY_READ ||
                    flags & MemoryAccessTypes::GPU_MEMORY_WRITE )
                {
                    ret |= VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                }

                return ret;
            }

            VkShaderStageFlags ShaderStages( const RenderSystem::PIPELINE_STAGES& stages )
            {
                VkShaderStageFlags ret = 0;
                if( stages & PipelineStages::COMPUTE )
                {
                    ret |= VK_SHADER_STAGE_COMPUTE_BIT;
                }
                if( stages & PipelineStages::GEOMETRY )
                {
                    ret |= VK_SHADER_STAGE_GEOMETRY_BIT;
                }
                if( stages & PipelineStages::PIXEL )
                {
                    ret |= VK_SHADER_STAGE_FRAGMENT_BIT;
                }
                if( stages & PipelineStages::TS_DOMAIN )
                {
                    ret |= VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
                }
                if( stages & PipelineStages::TS_HULL )
                {
                    ret |= VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
                }
                if( stages & PipelineStages::VERTEX )
                {
                    ret |= VK_SHADER_STAGE_VERTEX_BIT;
                }
                if( stages & PipelineStages::RT_ANY_HIT )
                {
                    ret |= VK_SHADER_STAGE_ANY_HIT_BIT_NV;
                }
                if( stages & PipelineStages::RT_CLOSEST_HIT )
                {
                    ret |= VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV;
                }
                if( stages & PipelineStages::RT_CALLABLE )
                {
                    ret |= VK_SHADER_STAGE_CALLABLE_BIT_NV;
                }
                if( stages & PipelineStages::RT_INTERSECTION )
                {
                    ret |= VK_SHADER_STAGE_INTERSECTION_BIT_NV;
                }
                if( stages & PipelineStages::RT_MISS_HIT )
                {
                    ret |= VK_SHADER_STAGE_MISS_BIT_NV;
                }
                if( stages & PipelineStages::RT_RAYGEN )
                {
                    ret |= VK_SHADER_STAGE_RAYGEN_BIT_NV;
                }
                if( stages & PipelineStages::MS_TASK )
                {
                    ret |= VK_SHADER_STAGE_TASK_BIT_NV;
                }
                if( stages & PipelineStages::MS_MESH )
                {
                    ret |= VK_SHADER_STAGE_MESH_BIT_NV;
                }
                return ret;
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

            struct SAllocData
            {
                size_t size = 0;
                size_t alignment;
                void* pPreviousAlloc;
                VkSystemAllocationScope vkScope;
                VkInternalAllocationType vkAllocationType;
            };

            void* DummyAllocCallback( void* pUserData, size_t size, size_t alignment, VkSystemAllocationScope vkScope )
            {
                SAllocData* pData = reinterpret_cast<SAllocData*>(pUserData);
                pData->size += size;
                pData->alignment = alignment;
                pData->vkScope = vkScope;
                void* pRet = VKE_MALLOC( size );
                return pRet;
            }

            void* DummyReallocCallback( void* pUserData, void* pOriginal, size_t size, size_t alignment, VkSystemAllocationScope vkScope )
            {
                SAllocData* pData = reinterpret_cast<SAllocData*>(pUserData);
                pData->size = size;
                pData->alignment = alignment;
                pData->vkScope = vkScope;
                pData->pPreviousAlloc = pOriginal;
                return VKE_REALLOC( pOriginal, size );
            }

            void DummyInternalAllocCallback( void* pUserData, size_t size, VkInternalAllocationType vkAllocationType,
                VkSystemAllocationScope vkAllocationScope )
            {
                SAllocData* pData = reinterpret_cast<SAllocData*>(pUserData);
                pData->size += size;
                pData->vkScope = vkAllocationScope;
                pData->vkAllocationType = vkAllocationType;
            }

            void DummyFreeCallback( void* pUserData, void* pMemory )
            {
                SAllocData* pData = reinterpret_cast<SAllocData*>(pUserData);
                VKE_FREE( pMemory );
            }

            void DummyInternalFreeCallback( void* pUserData, size_t size, VkInternalAllocationType vkType,
                VkSystemAllocationScope vkScope )
            {
                SAllocData* pData = reinterpret_cast<SAllocData*>(pUserData);
            }

            struct SSwapChainAllocator
            {
                uint8_t*    pMemory;
                uint32_t    currentChunkOffset = 0; // offset in current chunk
                uint32_t    memorySize;
                uint32_t    chunkSize; // == memorySize / elementCount
                uint32_t    ddiElementSize; // total memory returned from callbacks after all swapchain is created
                uint8_t     currentElement = 0;
                uint8_t     elementCount;

                VkAllocationCallbacks   VkCallbacks;

                Result Create( uint32_t elSize, uint8_t elCount )
                {
                    Result ret = VKE_ENOMEMORY;
                    VKE_ASSERT( pMemory != nullptr, "" );
                    chunkSize = elSize;
                    elementCount = elCount;
                    memorySize = chunkSize * elementCount;
                    pMemory = reinterpret_cast<uint8_t*>( VKE_MALLOC( memorySize ) );
                    if( pMemory != nullptr )
                    {
                        VkCallbacks.pUserData = this;
                        VkCallbacks.pfnAllocation = AllocCallback;
                        VkCallbacks.pfnFree = FreeCallback;
                        VkCallbacks.pfnReallocation = ReallocCallback;
                        VkCallbacks.pfnInternalFree = InternalFreeCallback;
                        VkCallbacks.pfnInternalAllocation = InternalAllocCallback;
                        ret = VKE_OK;
                    }
                    return ret;
                }

                void Destroy()
                {
                    if( pMemory != nullptr )
                    {
                        VKE_FREE( pMemory );
                        pMemory = nullptr;
                    }
                }

                void Reset()
                {
                    currentChunkOffset = 0;
                    currentElement = 0;
                }

                void FreeCurrentChunk()
                {
                    currentChunkOffset = 0;
                    currentElement = (currentElement + 1) % elementCount;
                }

                uint8_t* GetMemory(uint32_t size, uint32_t alignment)
                {
                    VKE_ASSERT( currentChunkOffset + size <= chunkSize, "" );
                    
                    uint8_t* pChunkMem = pMemory + (currentElement * chunkSize);
                    uint8_t* pPtr = pChunkMem + currentChunkOffset;

                    const auto alignedSize = Memory::CalcAlignedSize( size, alignment );
                    currentChunkOffset += alignedSize;

                    return pPtr;
                }

                static void* AllocCallback( void* pUserData, size_t size, size_t alignment, VkSystemAllocationScope vkScope )
                {
                    void* pRet;
                    {
                        SSwapChainAllocator* pAllocator = reinterpret_cast<SSwapChainAllocator*>(pUserData);
                        uint8_t* pPtr = pAllocator->GetMemory( size, alignment );
                        pRet = pPtr;
                    }
                    return pRet;
                }

                static void FreeCallback( void* pUserData, void* pMemory )
                {
                    SSwapChainAllocator* pAllocator = reinterpret_cast<SSwapChainAllocator*>(pUserData);
                    uint8_t* pMemEnd = pAllocator->pMemory + pAllocator->memorySize;
                    // Free allocations only out of memory block
                    if( pMemory < pAllocator->pMemory ||
                        pMemory >= pMemEnd )
                    {
                        VKE_FREE( pMemory );
                    }
                }

                static void* ReallocCallback( void* pUserData, void* pOriginal, size_t size, size_t alignment, VkSystemAllocationScope vkScope )
                {
                    VKE_ASSERT( 0, "This is not suppoerted for SwapChain." );
                    return VKE_REALLOC( pOriginal, size );
                }

                static void InternalFreeCallback( void* pUserData, size_t size, VkInternalAllocationType vkType,
                    VkSystemAllocationScope vkScope )
                {
                    size = size;
                }

                static void InternalAllocCallback( void* pUserData, size_t size, VkInternalAllocationType vkAllocationType,
                    VkSystemAllocationScope vkAllocationScope )
                {
                    size = size;
                }
            };

        } // Helper

        Result CheckRequiredExtensions( DDIExtMap* pmExtensionsInOut, DDIExtArray* pvRequiredInOut, CStrVec* pvNamesOut )
        {
            Result ret = VKE_OK;
            for( auto& ReqExt : *pvRequiredInOut )
            {
                bool found = false;
                for( auto& Pair : *pmExtensionsInOut )
                {
                    auto& Ext = Pair.second;

                    if( Ext.name == ReqExt.name )
                    {
                        found = true;
                        ReqExt.supported = true;
                        ReqExt.enabled = true;
                        Ext.enabled = true;
                        Ext.required = true;

                        pvNamesOut->PushBack( ReqExt.name.c_str() );
                        VKE_LOG( "Enable Vulkan required extension/layer: " << ReqExt.name.c_str() );
                        break;
                    }
                }
                if( !found )
                {
                    if( ReqExt.required )
                    {
                        VKE_LOG_ERR( "Vulkan EXT: " << ReqExt.name << " is not supported by this Device." );
                        ret = VKE_ENOTFOUND;
                    }
                    else
                    {
                        VKE_LOG_WARN( "Vulkan EXT: " << ReqExt.name << " is not supported by this Device." );
                    }
                }
            }
            return ret;
        }

        Result GetInstanceValidationLayers( VkICD::Global& Global,
            DDIExtMap* pmLayersInOut, DDIExtArray* pvRequiredInOut, CStrVec* pvNames )
        {
            static const char* apNames[] =
            {
                "VK_LAYER_LUNARG_standard_validation",
                "VK_LAYER_LUNARG_core_validation",
                "VK_LAYER_LUNARG_parameter_validation",
                /*VK_LAYER_GOOGLE_threading
                VK_LAYER_LUNARG_parameter_validation
                VK_LAYER_LUNARG_device_limits
                VK_LAYER_LUNARG_object_tracker
                VK_LAYER_LUNARG_image
                VK_LAYER_LUNARG_core_validation
                VK_LAYER_LUNARG_swapchain
                VK_LAYER_GOOGLE_unique_objects*/

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
            Utils::TCDynamicArray< VkLayerProperties, 64 > vProps;
            VK_ERR( Global.vkEnumerateInstanceLayerProperties( &count, nullptr ) );
            vProps.Resize( count );
            VK_ERR( Global.vkEnumerateInstanceLayerProperties( &count, &vProps[0] ) );

            pmLayersInOut->reserve( count );
            vke_string tmpName;
            tmpName.reserve( 128 );
            VKE_LOG( "SUPPORTED VULKAN INSTANCE LAYERS:" );
            for( uint32_t i = 0; i < count; ++i )
            {
                tmpName = vProps[i].layerName;
                pmLayersInOut->insert( DDIExtMap::value_type( tmpName, { tmpName, false, true, false } ) );
                VKE_LOG( tmpName.c_str() );
            }

            return CheckRequiredExtensions( pmLayersInOut, pvRequiredInOut, pvNames );
        }

        Result CheckInstanceExtensionNames( VkICD::Global& Global,
            DDIExtMap* pmExtensionsInOut, DDIExtArray* pvRequired, CStrVec* pvOut )
        {
            VKE_LOG_PROG( "VKEngine Checking instance extensions" );
            vke_vector< VkExtensionProperties > vProps;
            uint32_t count = 0;
            VK_ERR( Global.vkEnumerateInstanceExtensionProperties( nullptr, &count, nullptr ) );
            VKE_LOG_PROG( "VKEngine count: " << count );
            vProps.resize( count );
            VK_ERR( Global.vkEnumerateInstanceExtensionProperties( nullptr, &count, &vProps[0] ) );
            VKE_LOG_PROG( "VKEngine extensions queried" );
            
            pvOut->Reserve( count );
            VKE_LOG_PROG( "VKEngine reserve output" );
            pmExtensionsInOut->reserve( count );
            VKE_LOG_PROG( "VKEngine reserve map output" );
            vke_string tmpName;
            tmpName.reserve( 128 );
            VKE_LOG_PROG( "VKEngine reserve tmp string" );

            VKE_LOG( "SUPPORTED VULKAN INSTANCE EXTENSIONS:" );
            for( uint32_t i = 0; i < count; ++i )
            {
                tmpName = vProps[i].extensionName;
                pmExtensionsInOut->insert( DDIExtMap::value_type( tmpName, { tmpName, false, true, false } ) );
                VKE_LOG( tmpName.c_str() );
            }
            
            return CheckRequiredExtensions( pmExtensionsInOut, pvRequired, pvOut );
        }

        Result QueryAdapterProperties( const DDIAdapter& hAdapter, const DDIExtMap& mExts, SDeviceProperties* pOut )
        {
            auto& sInstanceICD = CDDI::GetInstantceICD();

            pOut->Properties.Memory = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2 };

            pOut->Properties.Device = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
            pOut->Features.Device = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };

            if( mExts.find( VK_NV_MESH_SHADER_EXTENSION_NAME ) != mExts.end() )
            {
                pOut->Properties.Device.pNext = &pOut->Properties.MeshShaderNV;
                pOut->Properties.MeshShaderNV = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_PROPERTIES_NV };

                pOut->Features.Device.pNext = &pOut->Features.MeshShaderNV;
                pOut->Features.MeshShaderNV = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_NV };
            }

#if VKE_VULKAN_1_1
            sInstanceICD.vkGetPhysicalDeviceFeatures2( m_hAdapter, &m_DeviceInfo.Features );
            sInstanceICD.vkGetPhysicalDeviceMemoryProperties2( m_hAdapter, &m_DeviceInfo.Properties.Memory );
            sInstanceICD.vkGetPhysicalDeviceProperties2( m_hAdapter, &m_DeviceInfo.Properties.Device );
#else
            if( sInstanceICD.vkGetPhysicalDeviceFeatures2 )
            {
                sInstanceICD.vkGetPhysicalDeviceFeatures2( hAdapter, &pOut->Features.Device );
            }
            else
            {
                sInstanceICD.vkGetPhysicalDeviceFeatures( hAdapter, &pOut->Features.Device.features );
            }
            if( sInstanceICD.vkGetPhysicalDeviceMemoryProperties2 )
            {
                sInstanceICD.vkGetPhysicalDeviceMemoryProperties2( hAdapter, &pOut->Properties.Memory );
            }
            else
            {
                sInstanceICD.vkGetPhysicalDeviceMemoryProperties( hAdapter, &pOut->Properties.Memory.memoryProperties );
            }
            if( sInstanceICD.vkGetPhysicalDeviceProperties2 )
            {
                sInstanceICD.vkGetPhysicalDeviceProperties2( hAdapter, &pOut->Properties.Device );
            }
            else
            {
                sInstanceICD.vkGetPhysicalDeviceProperties( hAdapter, &pOut->Properties.Device.properties );
            }
#endif // VKE_VULKAN_1_1
            {
                //ICD.Instance.vkGetPhysicalDeviceFormatProperties( vkPhysicalDevice, &m_DeviceInfo.FormatProperties );
            }

            uint32_t propCount = 0;
            sInstanceICD.vkGetPhysicalDeviceQueueFamilyProperties( hAdapter, &propCount, nullptr );
            if( propCount == 0 )
            {
                VKE_LOG_ERR( "No device queue family properties" );
                return VKE_FAIL;
            }

            pOut->vQueueFamilyProperties.Resize( propCount );
            auto& aProperties = pOut->vQueueFamilyProperties;
            auto& vQueueFamilies = pOut->vQueueFamilies;

            sInstanceICD.vkGetPhysicalDeviceQueueFamilyProperties( hAdapter, &propCount, &aProperties[0] );
            // Choose a family index
            for( uint32_t i = 0; i < propCount; ++i )
            {
                auto& VkProp = aProperties[i];
                uint32_t isCompute = VkProp.queueFlags & VK_QUEUE_COMPUTE_BIT;
                uint32_t isTransfer = VkProp.queueFlags & VK_QUEUE_TRANSFER_BIT;
                uint32_t isSparse = VkProp.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT;
                uint32_t isGraphics = VkProp.queueFlags & VK_QUEUE_GRAPHICS_BIT;
                VkBool32 isPresent = VK_FALSE;
#if VKE_USE_VULKAN_WINDOWS
                isPresent = sInstanceICD.vkGetPhysicalDeviceWin32PresentationSupportKHR( hAdapter, i );
#elif VKE_USE_VULKAN_LINUX
                isPresent = sInstanceICD.vkGetPhysicalDeviceXcbPresentationSupportKHR( hAdapter, i,
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


            for( uint32_t i = 0; i < RenderSystem::Formats::_MAX_COUNT; ++i )
            {
                const auto& fmt = RenderSystem::g_aFormats[i];
                sInstanceICD.vkGetPhysicalDeviceFormatProperties( hAdapter, fmt,
                    &pOut->Properties.aFormatProperties[i] );
            }

            return VKE_OK;
        }

        
        using DDIExtNameArray = Utils::TCDynamicArray< cstr_t >;

        
        Result CheckDeviceExtensions( VkPhysicalDevice vkPhysicalDevice,
            DDIExtArray* pvRequiredExtensions, DDIExtMap* pmAllExtensionsOut, DDIExtNameArray* pOut )
        {
            auto& InstanceICD = CDDI::GetInstantceICD();
            uint32_t count = 0;
            VK_ERR( InstanceICD.vkEnumerateDeviceExtensionProperties( vkPhysicalDevice, nullptr, &count, nullptr ) );

            Utils::TCDynamicArray< VkExtensionProperties > vProperties( count );
            pmAllExtensionsOut->reserve( count );

            VK_ERR( InstanceICD.vkEnumerateDeviceExtensionProperties( vkPhysicalDevice, nullptr, &count,
                &vProperties[0] ) );

            std::string ext;
            Result err = VKE_OK;

            vke_string tmpName;
            tmpName.reserve( 128 );
            VKE_LOG( "SUPPORTED VULKAN DEVICE EXTENSIONS:" );
            for( uint32_t p = 0; p < count; ++p )
            {
                tmpName = vProperties[ p ].extensionName;
                VKE_LOG( tmpName );
                pmAllExtensionsOut->insert( DDIExtMap::value_type( tmpName, { tmpName, false, true, false } ) );
            }

            return CheckRequiredExtensions( pmAllExtensionsOut, pvRequiredExtensions, pOut );
        }

        Result CDDI::LoadICD( const SDDILoadInfo& Info )
        {
            Result ret = VKE_OK;
            VKE_LOG_PROG( "VKEngine loading vulkan-1.dll" );
            shICD = Platform::DynamicLibrary::Load( "vulkan-1.dll" );
            if( shICD != 0 )
            {
                VKE_LOG_PROG( "vulkan-1.dll loaded" );

                ret = Vulkan::LoadGlobalFunctions( shICD, &sGlobalICD );
                if( VKE_SUCCEEDED( ret ) )
                {
                    VKE_LOG_PROG( "Vulkan global functions loaded" );
                    DDIExtArray vRequiredExts =
                    {
                        // name, required, supported, enabled
                        { VK_KHR_SURFACE_EXTENSION_NAME , true, false },
#if VKE_WINDOWS
                        { VK_KHR_WIN32_SURFACE_EXTENSION_NAME , true, false },
#elif VKE_LINUX
                        { VK_KHR_XCB_SURFACE_EXTENSION_NAME , true, false },
#elif VKE_ANDROID
                        { VK_KHR_ANDROID_SURFACE_EXTENSION_NAME , true, false },
#endif
                        { VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME, VKE_VULKAN_1_1, false },
#if VKE_RENDERER_DEBUG
                        { VK_EXT_DEBUG_UTILS_EXTENSION_NAME, false, false },
                        { VK_EXT_DEBUG_MARKER_EXTENSION_NAME, false, false },
                        { VK_EXT_DEBUG_REPORT_EXTENSION_NAME, true, false }
#endif // RENDERER_DEBUG
                    };

                    DDIExtArray vRequiredLayers =
                    {
#if VKE_RENDERER_DEBUG
                        // name, required, supported, enabled
                        { "VK_LAYER_LUNARG_standard_validation", false, false, false },
                        { "VK_LAYER_LUNARG_parameter_validation", false, false, false }
#endif // RENDERER_DEBUG
                    };

                    CStrVec vExtNames;
                    DDIExtMap mExtensions;
                    ret = CheckInstanceExtensionNames( sGlobalICD, &mExtensions, &vRequiredExts, &vExtNames );
                    if( VKE_FAILED( ret ) )
                    {
                        return ret;
                    }
                    VKE_LOG_PROG( "Vulkan ext checked" );

                    CStrVec vLayerNames;
                    DDIExtMap mLayers;
                    ret = GetInstanceValidationLayers( sGlobalICD, &mLayers, &vRequiredLayers, &vLayerNames );
                    if( VKE_SUCCEEDED( ret ) )
                    {
                        VKE_LOG_PROG( "Vulkan validation layers" );
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
                        InstInfo.enabledExtensionCount = static_cast<uint32_t>(vExtNames.GetCount());
                        InstInfo.enabledLayerCount = static_cast<uint32_t>(vLayerNames.GetCount());
                        InstInfo.flags = 0;
                        InstInfo.pApplicationInfo = &vkAppInfo;
                        InstInfo.ppEnabledExtensionNames = vExtNames.GetData();
                        InstInfo.ppEnabledLayerNames = vLayerNames.GetData();

                        VkResult vkRes = sGlobalICD.vkCreateInstance( &InstInfo, nullptr, &sVkInstance );
                        VK_ERR( vkRes );
                        if( vkRes == VK_SUCCESS )
                        {
                            VKE_LOG_PROG( "vk instance created" );
                            ret = Vulkan::LoadInstanceFunctions( sVkInstance, sGlobalICD, &sInstanceICD );
                            if( ret == VKE_OK )
                            {
                                VKE_LOG_PROG( "Vk instance functions loaded" );
                                if( sInstanceICD.vkCreateDebugReportCallbackEXT )
                                {
                                    VkDebugReportCallbackCreateInfoEXT ci = { VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT };
                                    ci.pfnCallback = VkDebugCallback;
                                    ci.pUserData = nullptr;
                                    vkRes = sInstanceICD.vkCreateDebugReportCallbackEXT( sVkInstance, &ci, nullptr, &sVkDebugReportCallback );
                                    VK_ERR( vkRes );
                                }
                                if( sInstanceICD.vkCreateDebugUtilsMessengerEXT )
                                {
                                    VkDebugUtilsMessengerCreateInfoEXT ci = { VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
                                    ci.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
                                        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
                                    ci.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
                                    ci.pfnUserCallback = VkDebugMessengerCallback;
                                    vkRes = sInstanceICD.vkCreateDebugUtilsMessengerEXT( sVkInstance, &ci, nullptr, &sVkDebugMessengerCallback );
                                }
                            }
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
                    VKE_LOG_ERR( "Unable to load Vulkan global function pointers." );
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

        const SDDIExtension& CDDI::GetExtensionInfo( cstr_t pName ) const
        {
            static const SDDIExtension sDummy;
            auto Itr = m_mExtensions.find( pName );
            if( Itr != m_mExtensions.end() )
            {
                return Itr->second;
            }
            return sDummy;
        }

        Result CDDI::CreateDevice( CDeviceContext* pCtx )
        {
            m_pCtx = pCtx;
            DDIExtArray vRequiredExtensions = 
            {
                // name, required, supported
                { VK_KHR_SWAPCHAIN_EXTENSION_NAME, true, false },
                { VK_KHR_MAINTENANCE1_EXTENSION_NAME, true, false },
                { VK_KHR_MAINTENANCE2_EXTENSION_NAME, true, false },
                { VK_KHR_MAINTENANCE3_EXTENSION_NAME, true, false },
                { VK_NV_MESH_SHADER_EXTENSION_NAME, false, false }
            };

            auto hAdapter = m_pCtx->m_Desc.pAdapterInfo->hDDIAdapter;
            VKE_ASSERT( hAdapter != NULL_HANDLE, "" );
            m_hAdapter = reinterpret_cast< VkPhysicalDevice >( hAdapter );
            //VkInstance vkInstance = reinterpret_cast<VkInstance>(Desc.hAPIInstance);
            
            DDIExtNameArray vDDIExtNames;
            VKE_RETURN_IF_FAILED( CheckDeviceExtensions( m_hAdapter, &vRequiredExtensions, &m_mExtensions, &vDDIExtNames ) );
            VKE_RETURN_IF_FAILED( QueryAdapterProperties( m_hAdapter, m_mExtensions, &m_DeviceProperties ) );
            
            for( uint32_t i = 0; i < m_DeviceProperties.Properties.Memory.memoryProperties.memoryHeapCount; ++i )
            {
                m_aHeapSizes[ i ] = m_DeviceProperties.Properties.Memory.memoryProperties.memoryHeaps[ i ].size;
            }

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
            di.enabledExtensionCount = vDDIExtNames.GetCount();
            di.enabledLayerCount = 0;
            di.pEnabledFeatures = nullptr;
            di.ppEnabledExtensionNames = vDDIExtNames.GetData();
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

        uint32_t CalcAlignedSize( uint32_t size, uint32_t alignment )
        {
            uint32_t ret = size;
            uint32_t remainder = size % alignment;
            if( remainder > 0 )
            {
                ret = size + alignment - remainder;
            }

            return ret;
        }

        void CDDI::UpdateDesc( SBufferDesc* pInOut )
        {
            if( pInOut->usage & BufferUsages::UNIFORM_BUFFER ||
                pInOut->usage & BufferUsages::UNIFORM_TEXEL_BUFFER )
            {
                pInOut->size = CalcAlignedSize( pInOut->size, m_DeviceProperties.Limits.minUniformBufferOffsetAlignment );
            }
        }

        void CDDI::UpdateDesc( STextureDesc* pInOut )
        {
            /*VkMemoryRequirements VkReq;
            m_ICD.vkGetImageMemoryRequirements( m_hDevice, hTexture, &VkReq );
            pInOut->memoryRequirements = VkReq.*/
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
                ci.usage = Convert::BufferUsage( Desc.usage );
                if( Desc.memoryUsage & MemoryUsages::GPU_ACCESS &&
                    (Desc.usage & BufferUsages::VERTEX_BUFFER || Desc.usage & BufferUsages::INDEX_BUFFER ||
                        Desc.usage & BufferUsages::UNIFORM_BUFFER) )
                {
                    ci.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
                }
                
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
                ci.format = Map::Format( Desc.format );
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
                ci.format = Map::Format( Desc.format );
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
                ci.format = Map::Format( Desc.format );
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
            const uint32_t attachmentCount = Desc.vDDIAttachments.GetCount();

            VkFramebufferCreateInfo ci;
            ci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            ci.pNext = nullptr;
            ci.flags = 0;
            ci.width = Desc.Size.width;
            ci.height = Desc.Size.height;
            ci.layers = 1;
            ci.attachmentCount = Desc.vDDIAttachments.GetCount();
            ci.pAttachments = Desc.vDDIAttachments.GetData();
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

            for( uint32_t a = 0; a < Desc.vRenderTargets.GetCount(); ++a )
            {
                const SRenderPassAttachmentDesc& AttachmentDesc = Desc.vRenderTargets[a];
                //const VkImageCreateInfo& vkImgInfo = ResMgr.GetTextureDesc( AttachmentDesc.hTextureView );
                VkAttachmentDescription vkAttachmentDesc;
                vkAttachmentDesc.finalLayout = Vulkan::Map::ImageLayout( AttachmentDesc.endLayout );
                vkAttachmentDesc.flags = 0;
                vkAttachmentDesc.format = Map::Format( AttachmentDesc.format );
                vkAttachmentDesc.initialLayout = Vulkan::Map::ImageLayout( AttachmentDesc.beginLayout );
                vkAttachmentDesc.loadOp = Vulkan::Convert::UsageToLoadOp( AttachmentDesc.usage );
                vkAttachmentDesc.storeOp = Vulkan::Convert::UsageToStoreOp( AttachmentDesc.usage );
                vkAttachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                vkAttachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                vkAttachmentDesc.samples = Vulkan::Map::SampleCount( AttachmentDesc.sampleCount );
                vVkAttachmentDescriptions.PushBack( vkAttachmentDesc );
            }

            VkAttachmentReference* pVkDepthStencilRef = nullptr;
            VkAttachmentReference VkDepthStencilRef;

            if( Desc.vSubpasses.IsEmpty() )
            {
                SRenderPassDesc::SSubpassDesc PassDesc;
                SSubpassDesc SubDesc;
                VkSubpassDescription VkSubpassDesc;

                for( uint32_t i = 0; i < Desc.vRenderTargets.GetCount(); ++i )
                {
                    auto& Curr = Desc.vRenderTargets[i];
                    bool isDepthBuffer = Curr.format == Formats::D24_UNORM_S8_UINT ||
                        Curr.format == Formats::X8_D24_UNORM_PACK32 ||
                        Curr.format == Formats::D32_SFLOAT_S8_UINT ||
                        Curr.format == Formats::D32_SFLOAT;
                    if( isDepthBuffer )
                    {
                        VkDepthStencilRef.attachment = 0;
                        VkDepthStencilRef.layout = Map::ImageLayout( Curr.beginLayout );
                        pVkDepthStencilRef = &VkDepthStencilRef;
                    }
                    else
                    {
                        // Find attachment
                        VkAttachmentReference vkRef;
                        vkRef.attachment = i;
                        vkRef.layout = Vulkan::Map::ImageLayout( Curr.beginLayout );
                        SubDesc.vColorAttachmentRefs.PushBack( vkRef );
                    }
                }

                VkSubpassDesc.colorAttachmentCount = SubDesc.vColorAttachmentRefs.GetCount();
                VkSubpassDesc.inputAttachmentCount = 0;
                VkSubpassDesc.pColorAttachments = SubDesc.vColorAttachmentRefs.GetData();
                VkSubpassDesc.pDepthStencilAttachment = pVkDepthStencilRef;
                VkSubpassDesc.pInputAttachments = nullptr;
                VkSubpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
                VkSubpassDesc.pPreserveAttachments = nullptr;
                VkSubpassDesc.pResolveAttachments = nullptr;
                VkSubpassDesc.preserveAttachmentCount = 0;
                VkSubpassDesc.flags = 0;

                vVkSubpassDescs.PushBack( VkSubpassDesc );
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
                    if( MakeAttachmentRef( Desc.vRenderTargets, RenderTargetDesc, &vkRef ) )
                    {
                        SubDesc.vColorAttachmentRefs.PushBack( vkRef );
                    }
                }

                for( uint32_t t = 0; t < SubpassDesc.vTextures.GetCount(); ++t )
                {
                    const auto& TexDesc = SubpassDesc.vTextures[t];
                    // Find attachment
                    VkAttachmentReference vkRef;
                    if( MakeAttachmentRef( Desc.vRenderTargets, TexDesc, &vkRef ) )
                    {
                        SubDesc.vInputAttachmentRefs.PushBack( vkRef );
                    }
                }

                // Find attachment
                VkAttachmentReference* pVkDepthStencilRef = nullptr;
                if( SubpassDesc.DepthBuffer.hTextureView != NULL_HANDLE )
                {
                    VkAttachmentReference vkRef;
                    if( MakeAttachmentRef( Desc.vRenderTargets, SubpassDesc.DepthBuffer, &vkRef ) )
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
            ci.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
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

                VkPipelineColorBlendStateCreateInfo VkColorBlendState = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
                {
                    auto& State = VkColorBlendState;
                    const auto& vBlendStates = Desc.Blending.vBlendStates;
                    VKE_ASSERT( vBlendStates.IsEmpty() == false && Desc.Shaders.apShaders[ShaderTypes::PIXEL].IsValid(),
                        "At least one blend state must be set for color attachment." );
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
                            State.logicOp = Map::LogicOperation( Desc.Blending.logicOperation );
                            State.logicOpEnable = Desc.Blending.enable;
                            memset( State.blendConstants, 0, sizeof( float ) * 4 );
                        }
                    }
                }
                ci.pColorBlendState = &VkColorBlendState;

                VkPipelineDepthStencilStateCreateInfo VkDepthStencil = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
                {
                    auto& State = VkDepthStencil;

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

                VkPipelineDynamicStateCreateInfo VkDynState = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
                {
                    static const VkDynamicState aVkStates[] =
                    {
                        VK_DYNAMIC_STATE_VIEWPORT,
                        VK_DYNAMIC_STATE_SCISSOR
                    };
                    auto& State = VkDynState;
                    State.dynamicStateCount = 2;
                    State.pDynamicStates = aVkStates;
                    ci.pDynamicState = &State;
                }

                VkPipelineMultisampleStateCreateInfo VkMultisampling = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
                {
                    auto& State = VkMultisampling;
                    //if( Desc.Multisampling.enable )
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

                VkPipelineRasterizationStateCreateInfo VkRasterization = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
                {
                    auto& State = VkRasterization;

                    State.cullMode = Map::CullMode( Desc.Rasterization.Polygon.cullMode );
                    State.depthBiasClamp = Desc.Rasterization.Depth.biasClampFactor;
                    State.depthBiasConstantFactor = Desc.Rasterization.Depth.biasConstantFactor;
                    State.depthBiasEnable = Desc.Rasterization.Depth.biasConstantFactor != 0.0f;
                    State.depthBiasSlopeFactor = Desc.Rasterization.Depth.biasSlopeFactor;
                    State.depthClampEnable = Desc.Rasterization.Depth.enableClamp;
                    State.frontFace = Map::FrontFace( Desc.Rasterization.Polygon.frontFace );
                    State.lineWidth = 1;
                    State.polygonMode = Map::PolygonMode( Desc.Rasterization.Polygon.mode );
                    State.rasterizerDiscardEnable = VK_FALSE;

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
                            VkPipelineShaderStageCreateInfo State = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
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
                            //vVkBindings.Resize( vAttribs.GetCount() );
                            SDescriptorSetLayoutDesc::BindingArray vBindings;
                            //vBindings.Resize( vAttribs.GetCount() );
                            uint32_t currIndex = 0;
                            uint32_t currLocation = 0;
                            uint32_t currOffset = 0;
                            uint32_t currVertexBufferBinding = UNDEFINED_U32;

                            for( uint32_t i = 0; i < vAttribs.GetCount(); ++i )
                            {
                                auto& vkAttrib = vVkAttribs[i];
                                vkAttrib.binding = vAttribs[i].binding;
                                vkAttrib.format = Map::Format( vAttribs[i].format );
                                vkAttrib.location = vAttribs[i].location;
                                vkAttrib.offset = vAttribs[i].offset;

                                if( currVertexBufferBinding != vAttribs[i].binding )
                                {
                                    currVertexBufferBinding = vAttribs[ i ].binding;
                                    VkVertexInputBindingDescription VkBinding;
                                    VkBinding.binding = currVertexBufferBinding;
                                    VkBinding.inputRate = Vulkan::Map::InputRate( vAttribs[i].inputRate );
                                    VkBinding.stride = vAttribs[i].stride;
                                    vVkBindings.PushBack( VkBinding );
                                }
                            }

                            State.pVertexAttributeDescriptions = &vVkAttribs[0];
                            State.pVertexBindingDescriptions = &vVkBindings[0];
                            State.vertexAttributeDescriptionCount = vVkAttribs.GetCount();
                            State.vertexBindingDescriptionCount = vVkBindings.GetCount();
                        }
                    }
                    /*else
                    {
                        VKE_LOG_ERR( "GraphicsPipeline has no InputLayout.VertexAttribute." );
                    }*/
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
                        VKE_ASSERT( vVkViewports.GetCount() == vVkScissors.GetCount(), "" );
                        State.pViewports = vVkViewports.GetData();
                        State.viewportCount = std::max( 1u, vVkViewports.GetCount() ); // at least one viewport
                        State.pScissors = vVkScissors.GetData();
                        State.scissorCount = State.viewportCount;
                    }
                    ci.pViewportState = &VkViewportState;
                }

                //VkGraphicsInfo.layout = reinterpret_cast< VkPipelineLayout >( Desc.hLayout.handle );
                //VkGraphicsInfo.renderPass = reinterpret_cast< VkRenderPass >( Desc.hRenderPass.handle );
                if( Desc.hDDILayout )
                {
                    VkGraphicsInfo.layout = Desc.hDDILayout;
                }
                else
                {
                    VkGraphicsInfo.layout = m_pCtx->GetPipelineLayout( Desc.hLayout )->GetDDIObject();
                }
                if( Desc.hDDIRenderPass != DDI_NULL_HANDLE )
                {
                    VkGraphicsInfo.renderPass = Desc.hDDIRenderPass;
                }
                else
                {
                    VkGraphicsInfo.renderPass = m_pCtx->GetRenderPass( Desc.hRenderPass )->GetDDIObject();
                }
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
                    VkBinding.stageFlags = Convert::ShaderStages( Binding.stages );

                    vVkBindings.PushBack( VkBinding );
                }
                ci.pBindings = vVkBindings.GetData();

                VK_ERR( DDI_CREATE_OBJECT( DescriptorSetLayout, ci, pAllocator, &hLayout ) );
            }

            return hLayout;
        }

        void CDDI::Update( const SUpdateBufferDescriptorSetInfo& Info )
        {
            VkWriteDescriptorSet VkWrite = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };

            VkWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            VkWrite.dstBinding = Info.binding;
            VkWrite.descriptorCount = Info.count;
            VkWrite.dstSet = Info.hDDISet;
            VkWrite.dstArrayElement = 0;
            const auto pVkBufferInfos = reinterpret_cast<const VkDescriptorBufferInfo*>(Info.vBufferInfos.GetData());
            VkWrite.pBufferInfo = pVkBufferInfos;
            m_ICD.vkUpdateDescriptorSets( m_hDevice, 1, &VkWrite, 0, nullptr );
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
                //DDIDescriptorSetLayout hDDIObj = m_pCtx->GetDescriptorSetLayout( Desc.vDescriptorSetLayouts[i] )->GetDDIObject();
                DDIDescriptorSetLayout hDDIObj = m_pCtx->GetDescriptorSetLayout( Desc.vDescriptorSetLayouts[i] );
                vVkDescLayouts.PushBack( hDDIObj );
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

        DDISampler CDDI::CreateObject( const SSamplerDesc& Desc, const void* pAllocator)
        {
            DDISampler hSampler = DDI_NULL_HANDLE;
            VkSamplerCreateInfo ci;
            ci.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            ci.pNext = nullptr;
            ci.flags = 0;
            ci.addressModeU = Map::AddressMode( Desc.assressModeU );
            ci.addressModeV = Map::AddressMode( Desc.addressModeV );
            ci.addressModeW = Map::AddressMode( Desc.addressModeW );
            ci.anisotropyEnable = Desc.enableAnisotropy;
            Convert::BorderColor( &ci.borderColor, Desc.BorderColor );
            ci.compareEnable = Desc.enableCompare;
            ci.compareOp = Map::CompareOperation( Desc.compareOperation );
            ci.magFilter = Map::MagFilter( Desc.magFilter );
            ci.maxAnisotropy = Desc.maxAnisotropy;
            ci.maxLod = Desc.LOD.max;
            ci.minFilter = Map::MinFilter( Desc.minFilter );
            ci.minLod = Desc.LOD.min;
            ci.mipLodBias = Desc.mipLODBias;
            ci.mipmapMode = Map::MipmapMode( Desc.mipmapMode );
            ci.unnormalizedCoordinates = Desc.unnormalizedCoordinates;
            VK_ERR( DDI_CREATE_OBJECT( Sampler, ci, pAllocator, &hSampler ) );
            return hSampler;
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

        Result CDDI::Allocate( const SAllocateMemoryDesc& Desc, SAllocateMemoryData* pOut )
        {
            Result ret = VKE_FAIL;
            VkMemoryPropertyFlags vkPropertyFlags = Convert::MemoryUsagesToVkMemoryPropertyFlags( Desc.usage );

            const auto& VkMemProps = m_DeviceProperties.Properties.Memory.memoryProperties;
            const int32_t idx = FindMemoryTypeIndex( &VkMemProps, UINT32_MAX, vkPropertyFlags );
            DDIMemory hMemory;
            if( idx >= 0 )
            {
                const auto heapIdx = VkMemProps.memoryTypes[ idx ].heapIndex;
                VKE_ASSERT( m_aHeapSizes[ heapIdx ] >= Desc.size, "" );
                VkMemoryAllocateInfo ai = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
                ai.allocationSize = Desc.size;
                ai.memoryTypeIndex = idx;
                VkResult res = m_ICD.vkAllocateMemory( m_hDevice, &ai, nullptr, &hMemory );
                VK_ERR( res );
                if( res == VK_SUCCESS )
                {
                    m_aHeapSizes[ heapIdx ] -= ai.allocationSize;

                    pOut->hDDIMemory = hMemory;
                    pOut->sizeLeft = m_aHeapSizes[ heapIdx ];
                }
                ret = res == VK_SUCCESS ? VKE_OK : VKE_ENOMEMORY;
            }
            else
            {
                VKE_LOG_ERR( "Required memory usage: " << Desc.usage << " is not suitable for this GPU." );
            }
            return ret;
        }

        Result CDDI::GetMemoryRequirements( const DDITexture& hTexture, SAllocationMemoryRequirements* pOut )
        {
            VkMemoryRequirements VkReq;
            m_ICD.vkGetImageMemoryRequirements( m_hDevice, hTexture, &VkReq );
            pOut->alignment = VkReq.alignment;
            pOut->size = VkReq.size;
            return VKE_OK;
        }

        Result CDDI::GetMemoryRequirements( const DDIBuffer& hBuffer, SAllocationMemoryRequirements* pOut )
        {
            VkMemoryRequirements VkReq;
            m_ICD.vkGetBufferMemoryRequirements( m_hDevice, hBuffer, &VkReq );
            pOut->alignment = VkReq.alignment;
            pOut->size = VkReq.size;
            return VKE_OK;
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
            //return WaitForFences( hFence, 0 ) == VKE_OK;
            VkResult res = m_ICD.vkGetFenceStatus( m_hDevice, hFence );
            return res == VK_SUCCESS;
        }

        void CDDI::Reset( DDIFence* phFence )
        {
            VK_ERR( m_ICD.vkResetFences( m_hDevice, 1, phFence ) );
        }

        Result CDDI::WaitForFences( const DDIFence& hFence, uint64_t timeout )
        {
            VkResult res = m_ICD.vkWaitForFences( m_hDevice, 1, &hFence, VK_TRUE, timeout );
            
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
                    VK_ERR( res );
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

        void* CDDI::MapMemory(const SMapMemoryInfo& Info)
        {
            Result ret;
            void* pData;
            VkResult res = m_ICD.vkMapMemory( m_hDevice, Info.hMemory, Info.offset, Info.size, 0, &pData );
            if( res != VK_SUCCESS )
            {
                pData = nullptr;
            }
            VK_ERR( res );
            return pData;
        }

        void CDDI::UnmapMemory( const DDIMemory& hDDIMemory )
        {
            m_ICD.vkUnmapMemory( m_hDevice, hDDIMemory );
        }

        void CDDI::Draw( const DDICommandBuffer& hCommandBuffer, const uint32_t& vertexCount,
            const uint32_t& instanceCount, const uint32_t& firstVertex, const uint32_t& firstInstance )
        {
            m_ICD.vkCmdDraw( hCommandBuffer, vertexCount, instanceCount, firstVertex, firstInstance );
        }

        void CDDI::Copy( const DDICommandBuffer& hDDICmdBuffer, const SCopyBufferToTextureInfo& Info )
        {

        }

        void CDDI::Copy( const DDICommandBuffer& hDDICmdBuffer, const SCopyBufferInfo& Info )
        {
            VkBufferCopy VkCopy;
            VkCopy.srcOffset = Info.Region.srcBufferOffset;
            VkCopy.dstOffset = Info.Region.dstBufferOffset;
            VkCopy.size = Info.Region.size;
            
            m_ICD.vkCmdCopyBuffer( hDDICmdBuffer, Info.hDDISrcBuffer, Info.hDDIDstBuffer,
                                   1, &VkCopy );
        }

        void CDDI::Copy( const DDICommandBuffer& hDDICmdBuffer, const SCopyTextureInfoEx& Info )
        {
            VkImageLayout vkSrcLayout = Map::ImageLayout( Info.srcTextureState );
            VkImageLayout vkDstLayout = Map::ImageLayout( Info.dstTextureState );

            VkImageCopy VkCopy;

            VkCopy.extent = { Info.pBaseInfo->Size.width, Info.pBaseInfo->Size.height, Info.pBaseInfo->depth };
            VkCopy.srcOffset = { Info.pBaseInfo->SrcOffset.x, Info.pBaseInfo->SrcOffset.y };
            VkCopy.dstOffset = { Info.pBaseInfo->DstOffset.x, Info.pBaseInfo->DstOffset.y };

            VkCopy.dstSubresource.aspectMask = Map::ImageAspect( Info.DstSubresource.aspect );
            VkCopy.dstSubresource.baseArrayLayer = Info.DstSubresource.beginArrayLayer;
            VkCopy.dstSubresource.layerCount = Info.DstSubresource.layerCount;
            VkCopy.dstSubresource.mipLevel = Info.DstSubresource.mipmapLevelCount;

            VkCopy.srcSubresource.aspectMask = Map::ImageAspect( Info.SrcSubresource.aspect );
            VkCopy.srcSubresource.baseArrayLayer = Info.SrcSubresource.beginArrayLayer;
            VkCopy.srcSubresource.layerCount = Info.SrcSubresource.layerCount;
            VkCopy.srcSubresource.mipLevel = Info.SrcSubresource.mipmapLevelCount;

            m_ICD.vkCmdCopyImage( hDDICmdBuffer, Info.pBaseInfo->hDDISrcTexture, vkSrcLayout,
                Info.pBaseInfo->hDDIDstTexture, vkDstLayout, 1, &VkCopy );
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
            Result ret = VKE_OK;
            //VK_ERR( res );
            //return res == VK_SUCCESS ? VKE_OK : VKE_FAIL;
            if( res != VK_SUCCESS )
            {
                switch( res )
                {
                    case VK_ERROR_OUT_OF_DATE_KHR:
                    case VK_ERROR_SURFACE_LOST_KHR:
                    {
                        ret = VKE_EOUTOFDATE;
                    }
                    break;
                    default:
                    {
                        ret = VKE_FAIL;
                        VK_ERR( res );
                    }
                    break;
                }
            }
            return ret;
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
            DDIPresentSurface hSurface = pOut->hSurface;
            uint16_t elementCount = Desc.elementCount;

            ExtentU16 Size = Desc.Size;
            if( Desc.pWindow.IsValid() )
            {
                Size = Desc.pWindow->GetDesc().Size;
            }

            Helper::SAllocData AllocData;
            VkAllocationCallbacks VkDummyCallbacks;
            VkDummyCallbacks.pUserData = &AllocData;
            VkDummyCallbacks.pfnAllocation = Helper::DummyAllocCallback;
            VkDummyCallbacks.pfnInternalAllocation = Helper::DummyInternalAllocCallback;
            VkDummyCallbacks.pfnFree = Helper::DummyFreeCallback;
            VkDummyCallbacks.pfnInternalFree = Helper::DummyInternalFreeCallback;
            VkDummyCallbacks.pfnReallocation = Helper::DummyReallocCallback;

            VkAllocationCallbacks* pVkCallbacks = nullptr;
            Helper::SSwapChainAllocator* pInternalAllocator = reinterpret_cast<Helper::SSwapChainAllocator*>(pOut->pInternalAllocator);
            if( pOut->pInternalAllocator == nullptr )
            {
                pInternalAllocator = VKE_NEW Helper::SSwapChainAllocator;
                if( VKE_SUCCEEDED( pInternalAllocator->Create( VKE_MEGABYTES( 1 ), 2 ) ) )
                {
                    pOut->pInternalAllocator = pInternalAllocator;
                    
                }
            }
            {
                pVkCallbacks = &pInternalAllocator->VkCallbacks;
            }
            if( pVkCallbacks == nullptr )
            {
                return VKE_ENOMEMORY;
            }

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
                vkRes = sInstanceICD.vkCreateWin32SurfaceKHR( sVkInstance, &SurfaceCI, pVkCallbacks, &hSurface );
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
                        sInstanceICD.vkDestroySurfaceKHR( sVkInstance, hSurface, pVkCallbacks );
                    }
                }
            }

            SPresentSurfaceCaps& Caps = pOut->Caps;
            ret = QueryPresentSurfaceCaps( hSurface, &Caps );
            Size = Caps.CurrentSize;
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
                VKE_LOG_ERR( "Requested format: " << Desc.format << " / " << Desc.colorSpace << " is not supported for present surface." );
                goto ERR;
            }

            found = false;
            if( Desc.enableVSync )
            {
                pOut->mode = PresentModes::FIFO;
                found = Caps.vModes.Find( pOut->mode ) != Caps.vModes.Npos();
            }
            else
            {
                pOut->mode = PresentModes::MAILBOX;
                found = Caps.vModes.Find( pOut->mode ) != Caps.vModes.Npos();
            }
            if( !found )
            {
                if( Caps.vModes.IsEmpty() )
                {
                    VKE_LOG_WARN( "The device doesn't support presentation mode." );
                    goto ERR;
                }
                // Get any supported
                pOut->mode = Caps.vModes[ 0 ];
                VKE_LOG_WARN( "Requested presentation mode is not supported for presentation surface." );
                found = true;
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
                ci.imageExtent.width = Size.width;
                ci.imageExtent.height = Size.height;
                ci.imageFormat = Map::Format( pOut->Format.format );
                ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
                ci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
                ci.minImageCount = elementCount;
                ci.oldSwapchain = pOut->hSwapChain;
                ci.pQueueFamilyIndices = &familyIndex;
                ci.queueFamilyIndexCount = 1;
                ci.presentMode = aVkModes[ pOut->mode ];
                ci.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
                ci.surface = pOut->hSurface;

                res = m_ICD.vkCreateSwapchainKHR( m_hDevice, &ci, pVkCallbacks, &hSwapChain );
                
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
                        pOut->vFramebuffers.Resize( imgCount );

                        res = m_ICD.vkGetSwapchainImagesKHR( m_hDevice, hSwapChain, &imgCount, &pOut->vImages[0] );
                        VK_ERR( res );
                        if( res == VK_SUCCESS )
                        {
                            Utils::TCDynamicArray< VkImageMemoryBarrier > vVkBarriers;

                            // Create renderpass
                            {
                                VkAttachmentReference ColorAttachmentRef;
                                ColorAttachmentRef.attachment = 0;
                                ColorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

                                VkSubpassDescription SubPassDesc = {};
                                SubPassDesc.colorAttachmentCount = 1;
                                SubPassDesc.pColorAttachments = &ColorAttachmentRef;
                                SubPassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

                                VkAttachmentDescription AtDesc = {};
                                //AtDesc.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                                //AtDesc.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                                AtDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                                AtDesc.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
                                AtDesc.format = SwapChainCI.imageFormat;
                                AtDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                                AtDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                                AtDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                                AtDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                                AtDesc.samples = VK_SAMPLE_COUNT_1_BIT;

                                if( pOut->hRenderPass == DDI_NULL_HANDLE )
                                {
                                    VkRenderPassCreateInfo ci = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
                                    ci.flags = 0;
                                    ci.attachmentCount = 1;
                                    ci.pAttachments = &AtDesc;
                                    ci.pDependencies = nullptr;
                                    ci.pSubpasses = &SubPassDesc;
                                    ci.subpassCount = 1;
                                    ci.dependencyCount = 0;
                                    res = m_ICD.vkCreateRenderPass( m_hDevice, &ci, pVkCallbacks, &pOut->hRenderPass );
                                    VK_ERR( res );
                                    if( res != VK_SUCCESS )
                                    {
                                        VKE_LOG_ERR( "Unable to create SwapChain RenderPass" );
                                        goto ERR;
                                    }
                                    _CreateDebugInfo<VK_OBJECT_TYPE_RENDER_PASS>(
                                        pOut->hRenderPass, "Swapchain RenderPass" );
                                }
                            }

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
                                res = m_ICD.vkCreateImageView( m_hDevice, &ci, pVkCallbacks, &hView );
                                VK_ERR( res );
                                if( res != VK_SUCCESS )
                                {
                                    VKE_LOG_ERR( "Unable to create ImageView for SwapChain image." );
                                    goto ERR;
                                }
                                pOut->vImageViews[i] = hView;

                                // Do a barrier for image
                                {
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

                                // Create framebuffers for render pass
                                {
                                    VkFramebufferCreateInfo ci = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
                                    ci.attachmentCount = 1;
                                    ci.pAttachments = &pOut->vImageViews[ i ];
                                    ci.width = Size.width;
                                    ci.height = Size.height;
                                    ci.renderPass = pOut->hRenderPass;
                                    ci.layers = 1;
                                    res = m_ICD.vkCreateFramebuffer( m_hDevice, &ci, pVkCallbacks, &pOut->vFramebuffers[ i ] );
                                    VK_ERR( res );
                                    if( res != VK_SUCCESS )
                                    {
                                        VKE_LOG_ERR( "Unable to create Framebuffer for SwapChain" );
                                        goto ERR;
                                    }

                                    _CreateDebugInfo<VK_OBJECT_TYPE_IMAGE>(
                                        pOut->vImages[ i ], "Swapchain Image" );
                                    _CreateDebugInfo<VK_OBJECT_TYPE_IMAGE_VIEW>(
                                        pOut->vImageViews[ i ], "Swapchain ImageView" );
                                    _CreateDebugInfo<VK_OBJECT_TYPE_FRAMEBUFFER>(
                                        pOut->vFramebuffers[ i ], "Swapchain Framebuffer" );

                                }
                                {
                                    STextureBarrierInfo Info;
                                    Info.hDDITexture = pOut->vImages[ i ];
                                    Info.currentState = TextureStates::UNDEFINED;
                                    Info.newState = TextureStates::PRESENT;
                                    Info.srcMemoryAccess = MemoryAccessTypes::GPU_MEMORY_WRITE;
                                    Info.dstMemoryAccess = MemoryAccessTypes::GPU_MEMORY_READ;
                                    Info.SubresourceRange.aspect = TextureAspects::COLOR;
                                    Info.SubresourceRange.beginArrayLayer = 0;
                                    Info.SubresourceRange.beginMipmapLevel = 0;
                                    Info.SubresourceRange.layerCount = 1;
                                    Info.SubresourceRange.mipmapLevelCount = 1;
                                    Desc.pCtx->Barrier( Info );
                                }
                            }
                            {
                                // Change image layout UNDEFINED -> PRESENT
                                VKE_ASSERT( Desc.pCtx != nullptr, "GraphicsContext must be set." );
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
                DestroyObject( &pOut->vImageViews[i], pVkCallbacks );
            }
            if( hSwapChain != DDI_NULL_HANDLE )
            {
                m_ICD.vkDestroySwapchainKHR( m_hDevice, hSwapChain, pVkCallbacks );
            }
            if( hSurface != DDI_NULL_HANDLE )
            {
                sInstanceICD.vkDestroySurfaceKHR( sVkInstance, hSurface, pVkCallbacks );
            }
            pInternalAllocator->Reset();
            return ret;
        }

        Result CDDI::ReCreateSwapChain( const SSwapChainDesc& Desc, SDDISwapChain* pOut )
        {
            Result ret = VKE_FAIL;
            auto pInternalAllocator = reinterpret_cast<Helper::SSwapChainAllocator*>(pOut->pInternalAllocator);
            VkAllocationCallbacks* pVkAllocator = &pInternalAllocator->VkCallbacks;

            for( uint32_t i = 0; i < pOut->vImageViews.GetCount(); ++i )
            {
                DestroyObject( &pOut->vImageViews[i], pVkAllocator );
                DestroyObject( &pOut->vFramebuffers[i], pVkAllocator );
            }
            if( pOut->hSwapChain != DDI_NULL_HANDLE )
            {
                m_ICD.vkDestroySwapchainKHR( m_hDevice, pOut->hSwapChain, pVkAllocator );
                pOut->hSwapChain = DDI_NULL_HANDLE;
            }
            if( pOut->hSurface != DDI_NULL_HANDLE )
            {
                sInstanceICD.vkDestroySurfaceKHR( sVkInstance, pOut->hSurface, pVkAllocator );
                pOut->hSurface = DDI_NULL_HANDLE;
            }
            if( pOut->hRenderPass != DDI_NULL_HANDLE )
            {
                m_ICD.vkDestroyRenderPass( m_hDevice, pOut->hRenderPass, pVkAllocator );
                pOut->hRenderPass = DDI_NULL_HANDLE;
            }
            pOut->vFramebuffers.Clear();
            pOut->vImages.Clear();
            pOut->vImageViews.Clear();
            pOut->hSwapChain = DDI_NULL_HANDLE;
            pInternalAllocator->FreeCurrentChunk();
            //DestroySwapChain( pOut, nullptr );
            ret = CreateSwapChain( Desc, nullptr, pOut );
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
            Helper::SSwapChainAllocator* pInternalAllocator = reinterpret_cast<Helper::SSwapChainAllocator*>(pInOut->pInternalAllocator);
            const VkAllocationCallbacks* pVkAllocator = &pInternalAllocator->VkCallbacks;
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
            pInternalAllocator->Destroy();
        }

        Result CDDI::GetCurrentBackBufferIndex( const SDDISwapChain& SwapChain, const SDDIGetBackBufferInfo& Info,
            uint32_t* pOut )
        {
            Result ret = VKE_FAIL;

            VkResult res = m_ICD.vkAcquireNextImageKHR( m_hDevice, SwapChain.hSwapChain, Info.waitTimeout,
                Info.hAcquireSemaphore, Info.hFence, pOut );
            switch( res )
            {
                case VK_SUCCESS:
                {
                    ret = VKE_OK;
                }
                break;
                case VK_TIMEOUT:
                case VK_NOT_READY:
                case VK_SUBOPTIMAL_KHR:
                case VK_ERROR_VALIDATION_FAILED_EXT:
                {
                    ret = VKE_ENOTREADY;
                }
                break;
                case VK_ERROR_DEVICE_LOST:
                {
                    ret = VKE_EDEVICELOST;
                }
                break;
                case VK_ERROR_OUT_OF_DATE_KHR:
                case VK_ERROR_SURFACE_LOST_KHR:
                {
                    ret = VKE_EOUTOFDATE;
                }
                break;
                default:
                {
                    VK_ERR( res );
                }
                break;
            }
            return ret;
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

        void CDDI::Bind( const SBindPipelineInfo& Info )
        {
            VKE_ASSERT( Info.pCmdBuffer != nullptr && Info.pCmdBuffer->GetDDIObject() != DDI_NULL_HANDLE &&
                Info.pPipeline != nullptr && Info.pPipeline->GetDDIObject() != DDI_NULL_HANDLE,
                "Invalid parameter");
            m_ICD.vkCmdBindPipeline( Info.pCmdBuffer->GetDDIObject(),
                Convert::PipelineTypeToBindPoint( Info.pPipeline->GetType() ), Info.pPipeline->GetDDIObject() );
        }

        void CDDI::Unbind( const DDICommandBuffer&, const DDIPipeline& )
        {

        }

        void CDDI::Bind( const SBindRenderPassInfo& Info )
        {
            VKE_ASSERT( Info.pBeginInfo != nullptr, "" );
            {
                VkRenderPassBeginInfo bi;
                bi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                bi.pNext = nullptr;
                bi.clearValueCount = Info.pBeginInfo->vDDIClearValues.GetCount();
                bi.pClearValues = Info.pBeginInfo->vDDIClearValues.GetData();
                bi.renderArea.extent.width = Info.pBeginInfo->RenderArea.Size.width;
                bi.renderArea.extent.height = Info.pBeginInfo->RenderArea.Size.height;
                bi.renderArea.offset = { Info.pBeginInfo->RenderArea.Offset.x, Info.pBeginInfo->RenderArea.Offset.y };

                //bi.renderPass = reinterpret_cast<DDIRenderPass>(Info.hPass.handle);
                //bi.framebuffer = reinterpret_cast<DDIFramebuffer>(Info.hFramebuffer.handle);
                bi.renderPass = Info.pBeginInfo->hDDIRenderPass;
                bi.framebuffer = Info.pBeginInfo->hDDIFramebuffer;
                m_ICD.vkCmdBeginRenderPass( Info.hDDICommandBuffer, &bi, VK_SUBPASS_CONTENTS_INLINE );
            }
        }

        void CDDI::Unbind( const DDICommandBuffer& hCb, const DDIRenderPass& )
        {
            m_ICD.vkCmdEndRenderPass( hCb );
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

        void CDDI::SetState( const DDICommandBuffer& hCommandBuffer, const SViewportDesc& Desc )
        {
            VkViewport Viewport;
            Viewport.width = Desc.Size.width;
            Viewport.height = Desc.Size.height;
            Viewport.x = Desc.Position.x;
            Viewport.y = Desc.Position.y;
            Viewport.minDepth = Desc.MinMaxDepth.min;
            Viewport.maxDepth = Desc.MinMaxDepth.max;
            m_ICD.vkCmdSetViewport( hCommandBuffer, 0, 1, &Viewport );
        }

        void CDDI::SetState( const DDICommandBuffer& hCommandBuffer, const SScissorDesc& Desc )
        {
            VkRect2D Scissor;
            Scissor.extent.width = Desc.Size.width;
            Scissor.extent.height = Desc.Size.height;
            Scissor.offset.x = Desc.Position.x;
            Scissor.offset.y = Desc.Position.y;
            m_ICD.vkCmdSetScissor( hCommandBuffer, 0, 1, &Scissor );
        }

        void CDDI::Barrier( const DDICommandBuffer& hCommandBuffer, const SBarrierInfo& Info )
        {
            VkMemoryBarrier* pVkMemBarriers = nullptr;
            VkImageMemoryBarrier* pVkImgBarriers = nullptr;
            VkBufferMemoryBarrier* pVkBuffBarrier = nullptr;
            VkPipelineStageFlags srcStage = 0;
            VkPipelineStageFlags dstStage = 0;

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
                        dstStage |= Convert::AccessMaskToPipelineStage( Barriers[i].dstMemoryAccess );
                        srcStage |= Convert::AccessMaskToPipelineStage( Barriers[i].srcMemoryAccess );
                    }
                    pVkMemBarriers = vVkMemBarriers.GetData();
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
                        dstStage |= Convert::AccessMaskToPipelineStage( Barriers[i].dstMemoryAccess );
                        srcStage |= Convert::AccessMaskToPipelineStage( Barriers[i].srcMemoryAccess );
                    }
                    pVkImgBarriers = vVkImgBarriers.GetData();
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
                        const VkAccessFlags flags = vVkBufferBarriers[i].dstAccessMask;
                        dstStage |= Convert::AccessMaskToPipelineStage( Barriers[i].dstMemoryAccess );
                        srcStage |= Convert::AccessMaskToPipelineStage( Barriers[i].srcMemoryAccess );
                    }
                    pVkBuffBarrier = vVkBufferBarriers.GetData();
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
            Memory::Copy( pOut, sizeof( DDIClearValue ), &In, sizeof( SClearValue ) );
        }


        VKAPI_ATTR VkBool32 VKAPI_CALL VkDebugCallback( VkDebugReportFlagsEXT msgFlags,
                                                        VkDebugReportObjectTypeEXT objType,
                                                        uint64_t srcObject, size_t location,
                                                        int32_t msgCode, const char *pLayerPrefix,
                                                        const char *pMsg, void *pUserData )
        {
            std::ostringstream message;

            if( msgFlags & VK_DEBUG_REPORT_ERROR_BIT_EXT )
            {
                message << "ERROR: ";
            }
            else if( msgFlags & VK_DEBUG_REPORT_WARNING_BIT_EXT )
            {
                message << "WARNING: ";
            }
            else if( msgFlags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT )
            {
                message << "PERFORMANCE WARNING: ";
            }
            else if( msgFlags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT )
            {
                message << "INFO: ";
            }
            else if( msgFlags & VK_DEBUG_REPORT_DEBUG_BIT_EXT )
            {
                message << "DEBUG: ";
            }
            message << "[" << pLayerPrefix << "] Code " << msgCode << " : " << pMsg;

#ifdef _WIN32
            MessageBox( NULL, message.str().c_str(), "Alert", MB_OK );
#else
            std::cout << message.str() << std::endl;
#endif

            /*
            * false indicates that layer should not bail-out of an
            * API call that had validation failures. This may mean that the
            * app dies inside the driver due to invalid parameter(s).
            * That's what would happen without validation layers, so we'll
            * keep that behavior here.
            */
            return false;
        }

        VKAPI_ATTR VkBool32 VkDebugMessengerCallback(
            VkDebugUtilsMessageSeverityFlagBitsEXT           messageSeverity,
            VkDebugUtilsMessageTypeFlagsEXT                  messageTypes,
            const VkDebugUtilsMessengerCallbackDataEXT*      pCallbackData,
            void*                                            pUserData )
        {
#define MSG pCallbackData->pMessageIdName << ": " << pCallbackData->pMessage
            if( pCallbackData && pCallbackData->pMessageIdName )
            {
                switch( messageSeverity )
                {
                case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
                    VKE_LOG_ERR( MSG );
                    break;
                case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
                    VKE_LOG_WARN( MSG );
                    break;
                case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
                    VKE_LOG_WARN( MSG );
                    break;
                default:
                    VKE_LOG( MSG );
                    break;
                }
            }
            VKE_ASSERT( messageSeverity != VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
                pCallbackData->pMessageIdName );
            return VK_FALSE;
        }

    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER