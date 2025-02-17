#include "RenderSystem/CDDI.h"
#if VKE_VULKAN_RENDER_SYSTEM
#include "RenderSystem/CDeviceContext.h"
#include "RenderSystem/CContextBase.h"
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

        VKAPI_ATTR VkBool32 VKAPI_CALL VkDebugMessengerCallback(
            VkDebugUtilsMessageSeverityFlagBitsEXT           messageSeverity,
            VkDebugUtilsMessageTypeFlagsEXT                  messageTypes,
            const VkDebugUtilsMessengerCallbackDataEXT*      pCallbackData,
            void*                                            pUserData );


        DDIExtArray GetRequiredInstanceExtensions( bool debug )
        {
            DDIExtArray Ret = {
                // name, required, supported, enabled
                { VK_KHR_SURFACE_EXTENSION_NAME, true, false },
#if VKE_WINDOWS
                { VK_KHR_WIN32_SURFACE_EXTENSION_NAME, true, false },
#elif VKE_LINUX
                { VK_KHR_XCB_SURFACE_EXTENSION_NAME, true, false },
#elif VKE_ANDROID
                { VK_KHR_ANDROID_SURFACE_EXTENSION_NAME, true, false },
#endif
                { VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME, true, false },
                
            };
            if( debug )
            {
                //                        name,                          required,   supported,  enabled
                Ret.PushBack( { VK_EXT_DEBUG_UTILS_EXTENSION_NAME, false, false } );
                Ret.PushBack( { VK_EXT_DEBUG_MARKER_EXTENSION_NAME, false, false } );
                Ret.PushBack( { VK_EXT_DEBUG_REPORT_EXTENSION_NAME, true, false } );
            }
            return Ret;
        }

        const DDIExtArray GetRequiredDeviceExtensions(bool debug)
        {
            const DDIExtArray Ret =
            {         
                // name, required, supported
                { VK_KHR_SWAPCHAIN_EXTENSION_NAME, true, false },
                { VK_KHR_MAINTENANCE1_EXTENSION_NAME, true, false },
                { VK_KHR_MAINTENANCE2_EXTENSION_NAME, true, false },
                { VK_KHR_MAINTENANCE3_EXTENSION_NAME, true, false },
                { VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME, true, false },
                { VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME, true, false },
                { VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME, true, false },
                { VK_KHR_COPY_COMMANDS_2_EXTENSION_NAME, true, false }
            };
            return Ret;
        }

        namespace Map
        {
            Result NativeResult( VkResult native )
            {
                Result ret = VKE_FAIL;
                switch (native)
                {
                    case VK_SUCCESS: ret = VKE_OK; break;
                    case VK_NOT_READY: ret = VKE_ENOTREADY; break;
                    case VK_TIMEOUT: ret = VKE_TIMEOUT; break;
                    case VK_ERROR_OUT_OF_HOST_MEMORY:
                    case VK_ERROR_OUT_OF_DEVICE_MEMORY:
                    case VK_ERROR_OUT_OF_POOL_MEMORY: ret = Results::NO_MEMORY; break;
                    case VK_ERROR_DEVICE_LOST: ret = Results::DEVICE_LOST; break;
                }
                return ret;
            }

            VkFormat Format( uint32_t format )
            {
                return VKE::RenderSystem::g_aFormats[ format ];
            }

            auto Formats(const FORMAT* pFormats, uint32_t count)
            {
                Utils::TCDynamicArray<VkFormat> vRet;
                for( uint32_t i = 0; i < count; ++i )
                {
                    vRet.PushBack( Format( pFormats[ i ] ) );
                }
                return vRet;
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
                static const VkImageLayout aVkLayouts[TextureStates::_MAX_COUNT] =
                {
                    VK_IMAGE_LAYOUT_UNDEFINED, // undefined
                    VK_IMAGE_LAYOUT_GENERAL, // general
                    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, // color rt
                    VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, // depth rt
                    VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL, // stencil rt
                    VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, // depth stencil rt
                    VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL, // depth buffer
                    VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL, // stencil buffer
                    VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, // deptn stencil buffer
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
                };
                return aVkLayouts[layout];
            }

            VkImageAspectFlags ImageAspect( RenderSystem::TEXTURE_ASPECT aspect )
            {
                VKE_ASSERT( aspect != 0 );
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

            VkFilter Filter(RenderSystem::TEXTURE_FILTER filter)
            {
                static const VkFilter aNativeFilters[RenderSystem::TextureFilters::_MAX_COUNT] =
                {
                    VK_FILTER_NEAREST,
                    VK_FILTER_LINEAR,
                    VK_FILTER_CUBIC_IMG
                };
                return aNativeFilters[ filter ];
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

            VkStencilOp StencilOperation( const RenderSystem::STENCIL_FUNCTION& op )
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
                static const VkShaderStageFlagBits aVkBits[ RenderSystem::ShaderTypes::_MAX_COUNT ] =
                {
                    VK_SHADER_STAGE_VERTEX_BIT,
                    VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT,
                    VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT,
                    VK_SHADER_STAGE_GEOMETRY_BIT,
                    VK_SHADER_STAGE_FRAGMENT_BIT,
                    VK_SHADER_STAGE_COMPUTE_BIT,
                    VK_SHADER_STAGE_TASK_BIT_NV,
                    VK_SHADER_STAGE_MESH_BIT_NV,
                    VK_SHADER_STAGE_RAYGEN_BIT_KHR,
                    VK_SHADER_STAGE_ANY_HIT_BIT_KHR,
                    VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
                    VK_SHADER_STAGE_MISS_BIT_KHR,
                    VK_SHADER_STAGE_CALLABLE_BIT_KHR,
                    VK_SHADER_STAGE_INTERSECTION_BIT_KHR
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

            VkSamplerAddressMode AddressMode( const ADDRESS_MODE& mode )
            {
                static const VkSamplerAddressMode aModes[] =
                {
                    VK_SAMPLER_ADDRESS_MODE_REPEAT,
                    VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
                    VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                    VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
                    VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE
                };
                return aModes[ mode ];
            }

            VkSamplerMipmapMode MipmapMode( const MIPMAP_MODE& mode )
            {
                static const VkSamplerMipmapMode aModes[] =
                {
                    VK_SAMPLER_MIPMAP_MODE_LINEAR,
                    VK_SAMPLER_MIPMAP_MODE_NEAREST
                };
                return aModes[ mode ];
            }

            VkTessellationDomainOrigin TessellationDomainOrigin( RenderSystem::TESSELLATION_DOMAIN_ORIGIN origin )
            {
                static const VkTessellationDomainOrigin saValues[ TessellationDomainOrigins::_MAX_COUNT ] =
                {
                    VK_TESSELLATION_DOMAIN_ORIGIN_UPPER_LEFT,
                    VK_TESSELLATION_DOMAIN_ORIGIN_LOWER_LEFT
                };
                return saValues[origin];
            }

            MEMORY_HEAP_TYPE VkMemPropertyFlagsToHeapType( VkMemoryPropertyFlags propertyFlags )
            {
                if ((propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
                {
                    if( ( propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT ) ==
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT )
                    {
                        return MemoryHeapTypes::UPLOAD;
                    }
                    return MemoryHeapTypes::GPU;
                }
                if( ( propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT ) ==
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT )
                {
                    return MemoryHeapTypes::CPU;
                }
                return MemoryHeapTypes::OTHER;
            }

        } // Map

        namespace Convert
        {
            VkBorderColor BorderColor( const BORDER_COLOR& color )
            {
                static const VkBorderColor aColors[] =
                {
                    VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK,
                    VK_BORDER_COLOR_INT_TRANSPARENT_BLACK,
                    VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK,
                    VK_BORDER_COLOR_INT_OPAQUE_BLACK,
                    VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
                    VK_BORDER_COLOR_INT_OPAQUE_WHITE
                };
                return aColors[ color ];
            }

            VkFilter Filter( const SAMPLER_FILTER& filter )
            {
                static const VkFilter aFilters[] =
                {
                    VK_FILTER_NEAREST,
                    VK_FILTER_LINEAR,
                    VK_FILTER_CUBIC_IMG
                };
                return aFilters[ filter ];
            }

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

            VkAttachmentLoadOp UsageToLoadOp( RenderSystem::RENDER_TARGET_RENDER_PASS_OP usage )
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


            VkAttachmentStoreOp UsageToStoreOp( RenderSystem::RENDER_TARGET_RENDER_PASS_OP usage )
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
                    case VK_FORMAT_A2B10G10R10_UNORM_PACK32: return RenderSystem::Formats::A2B10G10R10_UNORM_PACK32;
                }
                char buff[128];
                sprintf_s( buff, "Cannot convert VkFormat: %d to Engine format.", vkFormat );
                VKE_ASSERT2( 0, buff );
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

            VkBufferUsageFlags BufferUsage( const RenderSystem::BUFFER_USAGE usage )
            {
                VkBufferUsageFlags vkFlags = 0;
                if( usage & RenderSystem::BufferUsages::INDEX_BUFFER )
                {
                    vkFlags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
                }
                if( usage & RenderSystem::BufferUsages::VERTEX_BUFFER )
                {
                    vkFlags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
                }
                if( usage & RenderSystem::BufferUsages::CONSTANT_BUFFER )
                {
                    if( usage & RenderSystem::BufferUsages::TEXEL_BUFFER )
                    {
                        vkFlags |= VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT;
                    }
                    else
                    {
                        vkFlags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
                    }
                }
                if( usage & RenderSystem::BufferUsages::TRANSFER_DST )
                {
                    vkFlags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
                }
                if( usage & RenderSystem::BufferUsages::TRANSFER_SRC )
                {
                    vkFlags |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
                }
                if( usage & RenderSystem::BufferUsages::INDIRECT_BUFFER )
                {
                    vkFlags |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
                }
                if( usage & RenderSystem::BufferUsages::STORAGE_BUFFER )
                {
                    if( usage & RenderSystem::BufferUsages::TEXEL_BUFFER )
                    {
                        vkFlags |= VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT;
                    }
                    else
                    {
                        vkFlags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
                    }
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
                    flags |= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
                }
                /// TODO: upload heap is not currently supported
                if( usages & RenderSystem::MemoryUsages::CPU_ACCESS )
                {
                    flags |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
                    if( usages & RenderSystem::MemoryUsages::CPU_NO_FLUSH )
                    {
                        flags |= VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
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
                if( type & MemoryAccessTypes::COLOR_RENDER_TARGET_READ )
                {
                    vkFlags |= VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
                }
                if( type & MemoryAccessTypes::COLOR_RENDER_TARGET_WRITE )
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
                if( type & MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ )
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
                    ret |= VK_PIPELINE_STAGE_TASK_SHADER_BIT_EXT | VK_PIPELINE_STAGE_MESH_SHADER_BIT_EXT;
                }
                if( flags & MemoryAccessTypes::RT_UNIFORM_READ ||
                    flags & MemoryAccessTypes::RS_SHADER_READ ||
                    flags & MemoryAccessTypes::RS_SHADER_WRITE )
                {
                    ret |= VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR;
                }
                if( flags & MemoryAccessTypes::INPUT_ATTACHMENT_READ )
                {
                    ret |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                }
                if( flags & MemoryAccessTypes::COLOR_RENDER_TARGET_READ ||
                    flags & MemoryAccessTypes::COLOR_RENDER_TARGET_WRITE )
                {
                    ret |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                }
                if( flags & MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ ||
                    flags & MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_WRITE )
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
                    ret |= VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
                }
                if( stages & PipelineStages::RT_CLOSEST_HIT )
                {
                    ret |= VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
                }
                if( stages & PipelineStages::RT_CALLABLE )
                {
                    ret |= VK_SHADER_STAGE_CALLABLE_BIT_KHR;
                }
                if( stages & PipelineStages::RT_INTERSECTION )
                {
                    ret |= VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
                }
                if( stages & PipelineStages::RT_MISS_HIT )
                {
                    ret |= VK_SHADER_STAGE_MISS_BIT_KHR;
                }
                if( stages & PipelineStages::RT_RAYGEN )
                {
                    ret |= VK_SHADER_STAGE_RAYGEN_BIT_KHR;
                }
                if( stages & PipelineStages::MS_TASK )
                {
                    ret |= VK_SHADER_STAGE_TASK_BIT_EXT;
                }
                if( stages & PipelineStages::MS_MESH )
                {
                    ret |= VK_SHADER_STAGE_MESH_BIT_EXT;
                }
                return ret;
            }

            void RenderSystemToVkRect2D( const VKE::Rect2D& Rect, VkRect2D* pOut )
            {
                pOut->offset = { Rect.Position.x, Rect.Position.y };
                pOut->extent = { Rect.Size.width, Rect.Size.height };
            }

        } // Convert

        namespace Helper
        {


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

            void* VKAPI_PTR DummyAllocCallback( void* pUserData, size_t size, size_t alignment, VkSystemAllocationScope vkScope )
            {
                SAllocData* pData = reinterpret_cast<SAllocData*>(pUserData);
                pData->size += size;
                pData->alignment = alignment;
                pData->vkScope = vkScope;
                void* pRet = VKE_MALLOC( size );
                return pRet;
            }

            void* VKAPI_PTR DummyReallocCallback( void* pUserData, void* pOriginal, size_t size, size_t alignment, VkSystemAllocationScope vkScope )
            {
                SAllocData* pData = reinterpret_cast<SAllocData*>(pUserData);
                pData->size = size;
                pData->alignment = alignment;
                pData->vkScope = vkScope;
                pData->pPreviousAlloc = pOriginal;
                return VKE_REALLOC( pOriginal, size );
            }

            void VKAPI_PTR DummyInternalAllocCallback( void* pUserData, size_t size, VkInternalAllocationType vkAllocationType,
                VkSystemAllocationScope vkAllocationScope )
            {
                SAllocData* pData = reinterpret_cast<SAllocData*>(pUserData);
                pData->size += size;
                pData->vkScope = vkAllocationScope;
                pData->vkAllocationType = vkAllocationType;
            }

            void VKAPI_PTR DummyFreeCallback( void* pUserData, void* pMemory )
            {
                //SAllocData* pData = reinterpret_cast<SAllocData*>(pUserData);
                VKE_FREE( pMemory );
            }

            void VKAPI_PTR DummyInternalFreeCallback( void*, size_t, VkInternalAllocationType,
                VkSystemAllocationScope )
            {
                //SAllocData* pData = reinterpret_cast<SAllocData*>(pUserData);
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
                    VKE_ASSERT2( pMemory == nullptr, "" );
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
                    VKE_ASSERT2( currentChunkOffset + size <= chunkSize, "" );

                    uint8_t* pChunkMem = pMemory + (currentElement * chunkSize);
                    uint8_t* pPtr = pChunkMem + currentChunkOffset;

                    const auto alignedSize = Memory::CalcAlignedSize( size, alignment );
                    currentChunkOffset += alignedSize;

                    return pPtr;
                }

                static void* VKAPI_PTR AllocCallback( void* pUserData, size_t size, size_t alignment,
                                            VkSystemAllocationScope )
                {
                    void* pRet;
                    {
                        SSwapChainAllocator* pAllocator = reinterpret_cast<SSwapChainAllocator*>(pUserData);
                        uint8_t* pPtr = pAllocator->GetMemory( static_cast< uint32_t >( size ), static_cast< uint32_t >( alignment ) );
                        pRet = pPtr;
                    }
                    return pRet;
                }

                static void VKAPI_PTR FreeCallback( void* pUserData, void* pMemory )
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

                static void* VKAPI_PTR ReallocCallback( void* pUserData, void* pOriginal, size_t size,
                                              size_t alignment, VkSystemAllocationScope )
                {
                    ( void )alignment;
                    ( void )pUserData;
                    VKE_ASSERT2( 0, "This is not suppoerted for SwapChain." );
                    return VKE_REALLOC( pOriginal, size );
                }

                static void VKAPI_PTR InternalFreeCallback( void* pUserData, size_t size,
                                                  VkInternalAllocationType, VkSystemAllocationScope )
                {
                    ( void )pUserData;
                    ( void )size;
                }

                static void VKAPI_PTR InternalAllocCallback( void*, size_t,
                                                   VkInternalAllocationType, VkSystemAllocationScope )
                {
                }
            };

            template<typename HandleT, class DescT>
            vke_force_inline void SetObjectDebugName( const CDDI* pDDI, HandleT hObj, VkObjectType objType,
                                                         const DescT& Desc )
            {
#if VKE_RENDER_SYSTEM_DEBUG
                pDDI->SetObjectDebugName( ( uint64_t )hObj, objType, Desc.GetDebugName() );
#endif
            }

        } // Helper

        vke_force_inline int32_t FindMemoryTypeIndex( const VkPhysicalDeviceMemoryProperties* pMemProps,
                                                      uint32_t requiredMemBits,
                                                      VkMemoryPropertyFlags requiredProperties );

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
            //static const char* apNames[] =
            //{
            //    "VK_LAYER_KHRONOS_validation",
            //    //"VK_LAYER_LUNARG_core_validation",
            //    //"VK_LAYER_LUNARG_parameter_validation",
            //    /*VK_LAYER_GOOGLE_threading
            //    VK_LAYER_LUNARG_parameter_validation
            //    VK_LAYER_LUNARG_device_limits
            //    VK_LAYER_LUNARG_object_tracker
            //    VK_LAYER_LUNARG_image
            //    VK_LAYER_LUNARG_core_validation
            //    VK_LAYER_LUNARG_swapchain
            //    VK_LAYER_GOOGLE_unique_objects*/

            //};
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
            if( count > 0 )
            {
                vProps.Resize(count);
                VK_ERR(Global.vkEnumerateInstanceLayerProperties(&count, &vProps[0]));

                pmLayersInOut->reserve(count);
                vke_string tmpName;
                tmpName.reserve(128);
                VKE_LOG("SUPPORTED VULKAN INSTANCE LAYERS:");
                for (uint32_t i = 0; i < count; ++i)
                {
                    tmpName = vProps[i].layerName;
                    pmLayersInOut->insert(DDIExtMap::value_type(tmpName, { tmpName, false, true, false }));
                    VKE_LOG(tmpName.c_str());
                }
            }
            else
            {
                VKE_LOG_WARN("Vulkan instance layers are not supported on this machine.");
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

        template<class T> void AddVulkanNext( T& Struct, void*** pppNext )
        {
            void** ppNext = *pppNext;
            *ppNext = &Struct;
            ppNext = &Struct.pNext;
        }

        template<class T> void InitVulkanNext( T& Struct, void*** pppNext )
        {
            *pppNext = &Struct.pNext;
        }

        struct SVulkanNext
        {
            void** ppNext;

            ~SVulkanNext()
            {
                if( ppNext )
                {
                    *ppNext = nullptr;
                }
            }

            template<class T> SVulkanNext( T& Struct ) : ppNext((void**)&Struct.pNext)
            {
            }
             
            template<class T> SVulkanNext& Add( T* pStruct, VkStructureType type )
            {
                auto& Struct = *pStruct;
                Struct = { type };
                *ppNext = &Struct;
                ppNext = &Struct.pNext;
                return *this;
            }

            template<class T> SVulkanNext& Add( T* pStruct )
            {
                auto& Struct = *pStruct;
                *ppNext = &Struct;
                ppNext = (void**) &Struct.pNext;
                return *this;
            }
        };

        Result QueryAdapterProperties( const DDIAdapter& hAdapter, const DDIExtMap& mExts,
                                       SDeviceProperties* pOut )
        {
            auto& sInstanceICD = CDDI::GetInstantceICD();

            Memory::Zero( &pOut->Features );
            Memory::Zero( &pOut->Limits );
            Memory::Zero( &pOut->Properties );

            pOut->Properties.Memory = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2 };

            pOut->Properties.Device = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
            pOut->Features.Device = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };

            auto& Features = pOut->Features;
            auto& Properties = pOut->Properties;

            SVulkanNext NextFeatures( pOut->Features.Device );
            NextFeatures.Add( &pOut->Features.Device11, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES )
                .Add( &pOut->Features.DynamicRendering,
                      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR )
                .Add( &pOut->Features.Device12, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES );

            SVulkanNext NextProperties( pOut->Properties.Device );
            NextProperties.Add( &pOut->Properties.Device11, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_PROPERTIES )
                .Add( &pOut->Properties.Device12, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES )
                .Add( &pOut->Properties.DescriptorIndexing,
                      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_PROPERTIES );

            if( mExts.find( VK_EXT_MESH_SHADER_EXTENSION_NAME ) != mExts.end() )
            {
                NextFeatures.Add( &Features.MeshShaderNV, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_NV );
                NextProperties.Add( &Properties.MeshShaderNV,
                                    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_PROPERTIES_NV );

                NextFeatures.Add( &Features.MeshShaderEXT, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT );
                NextProperties.Add( &Properties.MeshShaderEXT,
                                    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_PROPERTIES_EXT );
            }

            if(mExts.find(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME) != mExts.end())
            {
                NextFeatures
                    .Add( &Features.Raytracing10, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR )
                    .Add( &Features.Raytracing11, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR )
                    .Add( &Features.Raytracing12,
                          VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_MOTION_BLUR_FEATURES_NV );

                NextProperties.Add( &Properties.Raytracing10,
                                    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR );
            }



            sInstanceICD.vkGetPhysicalDeviceFeatures2( hAdapter, &pOut->Features.Device );
            sInstanceICD.vkGetPhysicalDeviceMemoryProperties2( hAdapter, &pOut->Properties.Memory );
            sInstanceICD.vkGetPhysicalDeviceProperties2( hAdapter, &pOut->Properties.Device );

#if 0
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
                Family.type = QueueTypes::GENERAL;

                if( isSparse )
                {
                    Family.type = QueueTypeBits::SPARSE;
                }
                if( isPresent )
                {
                    Family.type = QueueTypeBits::PRESENT;
                }
                if( isTransfer )
                {
                    Family.type = QueueTypeBits::TRANSFER;
                }
                if( isCompute )
                {
                    Family.type = QueueTypeBits::COMPUTE;
                }
                if( isGraphics )
                {
                    Family.type = QueueTypeBits::GENERAL;
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

        void CDDI::GetFormatFeatures(FORMAT fmt, STextureFormatFeatures* pOut) const
        {
            Memory::Zero(pOut);
            const auto& Props = m_DeviceProperties.Properties.aFormatProperties[ fmt ];
            Utils::TCBitset<VkFormatFeatureFlags> Bits( Props.optimalTilingFeatures );
            
            pOut->sampled = Bits == VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;
            pOut->colorRenderTarget = Bits == VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT;
            pOut->storage = Bits == VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT;
            pOut->storageAtomic = Bits == VK_FORMAT_FEATURE_STORAGE_IMAGE_ATOMIC_BIT;
            pOut->uniformTexelBuffer = Bits == VK_FORMAT_FEATURE_UNIFORM_TEXEL_BUFFER_BIT;
            pOut->storageTexelBuffer = Bits == VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_BIT;
            pOut->storageTexelBufferAtomic = Bits == VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_ATOMIC_BIT;
            pOut->depthStencilRenderTarget = Bits == VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
            pOut->blitSrc = Bits == VK_FORMAT_FEATURE_BLIT_SRC_BIT;
            pOut->blitDst = Bits == VK_FORMAT_FEATURE_BLIT_DST_BIT;
            pOut->linearFilter = Bits == VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT;
            pOut->transferSrc = Bits == VK_FORMAT_FEATURE_TRANSFER_SRC_BIT;
            pOut->transferDst = Bits == VK_FORMAT_FEATURE_TRANSFER_DST_BIT;
        }

        using DDIExtNameArray = Utils::TCDynamicArray< cstr_t >;


        Result GetDeviceExtensions( VkPhysicalDevice vkPhysicalDevice,
            DDIExtMap* pmAllExtensionsOut )
        {
            auto& InstanceICD = CDDI::GetInstantceICD();
            uint32_t count = 0;
            VK_ERR( InstanceICD.vkEnumerateDeviceExtensionProperties( vkPhysicalDevice, nullptr, &count, nullptr ) );

            Utils::TCDynamicArray< VkExtensionProperties > vProperties( count );
            pmAllExtensionsOut->reserve( count );

            VK_ERR( InstanceICD.vkEnumerateDeviceExtensionProperties( vkPhysicalDevice, nullptr, &count,
                &vProperties[0] ) );

            std::string ext;

            vke_string tmpName;
            tmpName.reserve( 128 );
            VKE_LOG( "SUPPORTED VULKAN DEVICE EXTENSIONS:" );
            for( uint32_t p = 0; p < count; ++p )
            {
                tmpName = vProperties[ p ].extensionName;
                VKE_LOG( tmpName );
                pmAllExtensionsOut->insert( DDIExtMap::value_type( tmpName, { tmpName, false, true, false } ) );
            }

            return VKE_OK;
        }

        Result CheckDeviceExtensions( const DDIExtMap& mAllExtensions,
            const DDIExtNameArray& vRequestedExtensions )
        {
            DDIExtNameArray vNotSupported;
            for( uint32_t i = 0; i < vRequestedExtensions.GetCount(); ++i )
            {
                cstr_t pName = vRequestedExtensions[ i ];
                if( mAllExtensions.find(pName) == mAllExtensions.end() )
                {
                    vNotSupported.PushBack( pName );
                }
            }
            if( !vNotSupported.IsEmpty() )
            {
                VKE_LOG_ERR( "Some requested extensions are not supported:" );
                for( uint32_t i = 0; i < vNotSupported.GetCount(); ++i )
                {
                    VKE_LOG_ERR( vNotSupported[ i ] );
                }
                return VKE_FAIL;
            }
            return VKE_OK;
        }

        FEATURE_LEVEL ConvertVulkanAPIToFeatureLevel(uint32_t apiVer)
        {
            FEATURE_LEVEL ret = FeatureLevels::LEVEL_1_0;
            if(VK_API_VERSION_MAJOR(apiVer) == 1)
            {
                auto minor = VK_API_VERSION_MINOR( apiVer );
                switch(minor)
                {
                    case 1: ret = FeatureLevels::LEVEL_1_1; break;
                    case 2: ret = FeatureLevels::LEVEL_1_2; break;
                    case 3: ret = FeatureLevels::LEVEL_1_3; break;
                    case 4: ret = FeatureLevels::LEVEL_1_4; break;
                }
            }
            return ret;
        }

        uint32_t ConvertFeatureSetToVulkanAPIVersion(FEATURE_LEVEL set)
        {
            uint32_t ret = VK_MAKE_API_VERSION(0, 1, 0, 0);
            switch(set)
            {
                case FeatureLevels::LEVEL_1_1: ret = VK_MAKE_API_VERSION( 0, 1, 1, 0 ); break;
                case FeatureLevels::LEVEL_1_2: ret = VK_MAKE_API_VERSION( 0, 1, 2, 0 ); break;
                case FeatureLevels::LEVEL_1_3: ret = VK_MAKE_API_VERSION( 0, 1, 3, 0 ); break;
                case FeatureLevels::LEVEL_1_4: ret = VK_MAKE_API_VERSION( 0, 1, 4, 0 ); break;
            }
            return ret;
        }

        Result CDDI::LoadICD( const SDDILoadInfo& Info, SDriverInfo* pOut )
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
                    DDIExtArray vRequiredInstanceExts = GetRequiredInstanceExtensions( Info.enableDebugMode );
                    DDIExtArray vRequiredDeviceExts = GetRequiredDeviceExtensions( Info.enableDebugMode );

                    DDIExtArray vRequiredLayers;
                    if( Info.enableDebugMode )
                    {
                        //                          name,                          required,   supported,  enabled
                        vRequiredLayers.PushBack( { "VK_LAYER_KHRONOS_validation", true, false, false } );
                    }
                    

                    CStrVec vExtNames;
                    DDIExtMap mExtensions;
                    ret = CheckInstanceExtensionNames( sGlobalICD, &mExtensions, &vRequiredInstanceExts, &vExtNames );
                    VKE_ASSERT2( VKE_SUCCEEDED( ret ), "Required extension is not supported." );
                    if( VKE_FAILED( ret ) )
                    {
                        return ret;
                    }
                    VKE_LOG_PROG( "Vulkan ext checked" );

                    CStrVec vLayerNames;
                    DDIExtMap mLayers;
                    ret = GetInstanceValidationLayers( sGlobalICD, &mLayers, &vRequiredLayers, &vLayerNames );
                    VKE_ASSERT2( VKE_SUCCEEDED( ret ), "Required validation layer is not supported." );

                    // Vulkan 1.1 not supported
                    uint32_t apiVersion;
                    if(sGlobalICD.vkEnumerateInstanceVersion == nullptr)
                    {
                        apiVersion = VK_MAKE_API_VERSION( 0, 1, 0, 0 );
                    }
                    else
                    {
                        sGlobalICD.vkEnumerateInstanceVersion( &apiVersion );
                    }

                    pOut->featureLevel = ConvertVulkanAPIToFeatureLevel( apiVersion );

                    if( VKE_SUCCEEDED( ret ) )
                    {
                        VKE_LOG_PROG( "Vulkan validation layers" );
                        VkApplicationInfo vkAppInfo;
                        vkAppInfo.apiVersion = apiVersion;
                        vkAppInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
                        vkAppInfo.pNext = nullptr;
                        vkAppInfo.applicationVersion = Info.AppInfo.applicationVersion;
                        vkAppInfo.engineVersion = Info.AppInfo.engineVersion;
                        vkAppInfo.pApplicationName = Info.AppInfo.pApplicationName;
                        vkAppInfo.pEngineName = Info.AppInfo.pEngineName;

                        VkInstanceCreateInfo InstInfo;
                        Vulkan::InitInfo( &InstInfo, VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO );

                        
                        
                        Utils::TCDynamicArray<VkValidationFeatureEnableEXT> vEnableValFeatures =
                        {
                            VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT
                        };
                        VkValidationFeaturesEXT ValidationFeatures = { VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT };
                        ValidationFeatures.enabledValidationFeatureCount = vEnableValFeatures.GetCount();
                        ValidationFeatures.pEnabledValidationFeatures = vEnableValFeatures.GetData();

                        VkDebugReportCallbackCreateInfoEXT DbgReport = { VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT };
                        DbgReport.pfnCallback = VkDebugCallback;
                        DbgReport.pUserData = nullptr;
                        DbgReport.flags = VK_DEBUG_REPORT_DEBUG_BIT_EXT | VK_DEBUG_REPORT_ERROR_BIT_EXT |
                                   VK_DEBUG_REPORT_INFORMATION_BIT_EXT;

                        VkDebugUtilsMessengerCreateInfoEXT DbgUtils = {
                            VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT
                        };
                        DbgUtils.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
                                             VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                             VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
                        DbgUtils.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                         VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                         VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
                        DbgUtils.pfnUserCallback = VkDebugMessengerCallback;

                        SVulkanNext FeaturesNext( InstInfo );
                        if( Info.enableDebugMode )
                        {
                            FeaturesNext.Add( &ValidationFeatures );
                        }

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
                            VKE_LOG_PROG( "Vulkan instance created with API ver: "
                                << VK_API_VERSION_MAJOR(apiVersion) << "."
                                << VK_API_VERSION_MINOR(apiVersion) );
                            ret = Vulkan::LoadInstanceFunctions( sVkInstance, sGlobalICD, &sInstanceICD );
                            if( ret == VKE_OK )
                            {
                                VKE_LOG_PROG( "Vk instance functions loaded" );
                                if( Info.enableDebugMode )
                                {
                                    if( sInstanceICD.vkCreateDebugReportCallbackEXT )
                                    {
                                        vkRes = sInstanceICD.vkCreateDebugReportCallbackEXT(
                                            sVkInstance, &DbgReport, nullptr, &sVkDebugReportCallback );
                                        VK_ERR( vkRes );
                                    }
                                    else if( sInstanceICD.vkCreateDebugUtilsMessengerEXT )
                                    {
                                        vkRes = sInstanceICD.vkCreateDebugUtilsMessengerEXT(
                                            sVkInstance, &DbgUtils, nullptr, &sVkDebugMessengerCallback );
                                        VK_ERR( vkRes );
                                    }
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


        Result LoadDeviceExtensions( VkPhysicalDevice vkPhysicalDevice, DDIExtMap* pmAllExtensionsOut )
        {
            auto& InstanceICD = CDDI::GetInstantceICD();
            uint32_t count = 0;
            VK_ERR( InstanceICD.vkEnumerateDeviceExtensionProperties( vkPhysicalDevice, nullptr, &count, nullptr ) );
            Utils::TCDynamicArray<VkExtensionProperties> vProperties( count );
            pmAllExtensionsOut->reserve( count );
            VK_ERR( InstanceICD.vkEnumerateDeviceExtensionProperties( vkPhysicalDevice, nullptr, &count,
                                                                      &vProperties[ 0 ] ) );
            std::string ext;
            vke_string tmpName;
            tmpName.reserve( 128 );
            VKE_LOG( "SUPPORTED VULKAN DEVICE EXTENSIONS:" );
            for( uint32_t p = 0; p < count; ++p )
            {
                tmpName = vProperties[ p ].extensionName;
                VKE_LOG( tmpName );
                pmAllExtensionsOut->insert( DDIExtMap::value_type( tmpName, { tmpName, false, true, false } ) );
            }
            return VKE_OK;
        }

        Result EnableDeviceFeatures( VkPhysicalDevice vkPhysicalDevice,
            SDeviceProperties* pProps, DDIExtMap* pmExts, SSettings* pSettingsOut,
            SVulkanDeviceFeatures* pEnableOut, VkPhysicalDeviceFeatures* pEnabledFeaturesOut,
            VkDeviceCreateInfo* pOut, DDIExtNameArray* pExtOut )
        {
            // Required extensions
            *pExtOut =
            {
                VK_KHR_SWAPCHAIN_EXTENSION_NAME,
                VK_KHR_MAINTENANCE1_EXTENSION_NAME,
                VK_KHR_MAINTENANCE2_EXTENSION_NAME,
                VK_KHR_MAINTENANCE3_EXTENSION_NAME,
                VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME,
                VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME,
                VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME,
                VK_KHR_COPY_COMMANDS_2_EXTENSION_NAME
            };
            
            Result ret = GetDeviceExtensions( vkPhysicalDevice, pmExts );
            if( VKE_FAILED( ret ) )
            {
                return ret;
            }
            ret = QueryAdapterProperties( vkPhysicalDevice, *pmExts, pProps );
            if( VKE_FAILED( ret ) )
            {
                return ret;
            }

            auto& Props = *pProps;
            auto& Device = Props.Properties.Device;
            auto& Features = Props.Features;
            auto& Device11 = Props.Features.Device11;
            auto& Device12 = Props.Features.Device12;
            auto& DeviceFeatures = Props.Features.Device.features;
            auto& Settings = *pSettingsOut;

            auto deviceFeatureLevel = ConvertVulkanAPIToFeatureLevel( Device.properties.apiVersion );
            auto requestedLevel = Settings.featureLevel;
            if (requestedLevel == FeatureLevels::LEVEL_DEFAULT || requestedLevel > deviceFeatureLevel)
            {
                requestedLevel = deviceFeatureLevel;
                pSettingsOut->featureLevel = deviceFeatureLevel;
            }

            pEnabledFeaturesOut->robustBufferAccess = VKE_RENDER_SYSTEM_DEBUG && DeviceFeatures.robustBufferAccess;
            
            
            VkDeviceCreateInfo& Info = *pOut;
            SVulkanNext NextFeatures( Info );

            if( requestedLevel >= FeatureLevels::LEVEL_1_0  )
            {
                pEnabledFeaturesOut->geometryShader = DeviceFeatures.geometryShader;
                pEnabledFeaturesOut->tessellationShader = DeviceFeatures.tessellationShader;
                pEnabledFeaturesOut->fillModeNonSolid = DeviceFeatures.fillModeNonSolid;
            }
            if( requestedLevel >= FeatureLevels::LEVEL_1_1 )
            {
                pEnabledFeaturesOut->sparseBinding = DeviceFeatures.sparseBinding;
                pEnabledFeaturesOut->sparseResidencyBuffer = DeviceFeatures.sparseResidencyBuffer;
                pEnabledFeaturesOut->sparseResidencyAliased = DeviceFeatures.sparseResidencyAliased;
                pEnabledFeaturesOut->sparseResidencyImage2D = DeviceFeatures.sparseResidencyImage2D;
                pEnabledFeaturesOut->sparseResidencyImage3D = DeviceFeatures.sparseResidencyImage3D;

                if( !Device11.shaderDrawParameters )
                {
                    VKE_LOG_ERR( "Required device feature: 'Shader Draw Parameters' is not supported." );
                    ret = VKE_FAIL;
                }
                pEnableOut->Device11.sType = Device11.sType;
                pEnableOut->Device11.shaderDrawParameters = Device11.shaderDrawParameters;
                NextFeatures.Add( &pEnableOut->Device11 );
            }
            if( requestedLevel >= FeatureLevels::LEVEL_1_2 )
            {
                if( !Features.DynamicRendering.dynamicRendering )
                {
                    VKE_LOG_ERR( "Required device feature: 'Dynamic Rendering' is not supported." );
                    ret = VKE_FAIL;
                }

                pEnableOut->DynamicRendering = Features.DynamicRendering;
                NextFeatures.Add( &pEnableOut->DynamicRendering );

                pExtOut->PushBack( VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME );

                if( !Device12.descriptorIndexing )
                {
                    VKE_LOG_ERR( "Required device feature: 'Descriptor Indexing' is not supported." );
                    ret = VKE_FAIL;
                    if( !Device12.runtimeDescriptorArray )
                    {
                        VKE_LOG_ERR( "Required device feature: 'Runtime Descriptor Array' is not supported." );
                        ret = VKE_FAIL;
                    }
                }

                pEnableOut->Device12.sType = Device12.sType;
                pEnableOut->Device12.descriptorIndexing = Device12.descriptorIndexing;
                pEnableOut->Device12.runtimeDescriptorArray = Device12.runtimeDescriptorArray;
                NextFeatures.Add( &pEnableOut->Device12 );

                pSettingsOut->Features.bindlessResourceAccess = ( FEATURE_ENABLE_MODE )Device12.descriptorIndexing;
                pSettingsOut->Features.dynamicRenderPass = ( FEATURE_ENABLE_MODE )Features.DynamicRendering.dynamicRendering;
            }
            if( requestedLevel >= FeatureLevels::LEVEL_1_3 )
            {
                if( true )
                {
                    if( !Features.Raytracing10.rayTracingPipeline )
                    {
                        VKE_LOG_ERR( "Required device feature: 'Raytracing 1.0' is not supported." );
                        ret = VKE_FAIL;
                    }
                    if( !Features.Raytracing11.rayQuery )
                    {
                        VKE_LOG_ERR( "Required device feature: 'Raytracing 1.1' is not supported." );
                        ret = VKE_FAIL;
                    }
                    if( !Features.MeshShaderEXT.meshShader )
                    {
                        VKE_LOG_ERR( "Required device feature: 'Mesh Shaders' is not supported." );
                        ret = VKE_FAIL;
                    }
                    pEnableOut->Raytracing10 = Features.Raytracing10;
                    pEnableOut->Raytracing11 = Features.Raytracing11;
                    pEnableOut->Raytracing12 = Features.Raytracing12;
                    NextFeatures.Add( &pEnableOut->Raytracing10 )
                        .Add( &pEnableOut->Raytracing11 )
                        .Add( &pEnableOut->Raytracing12 );
                    pExtOut->PushBack( VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME );
                    pExtOut->PushBack( VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME );
                    pExtOut->PushBack( VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME );
                    pExtOut->PushBack( VK_KHR_RAY_QUERY_EXTENSION_NAME );
                    pExtOut->PushBack( VK_NV_RAY_TRACING_MOTION_BLUR_EXTENSION_NAME );
                }
                if( true )
                {
                    if( Features.MeshShaderNV.meshShader && Features.MeshShaderNV.taskShader )
                    {
                        pEnableOut->MeshShaderNV = Features.MeshShaderNV;
                        pExtOut->PushBack( VK_NV_MESH_SHADER_EXTENSION_NAME );
                    }
                    
                    if( Features.MeshShaderEXT.meshShader && Features.MeshShaderEXT.taskShader )
                    {
                        pEnableOut->MeshShaderEXT = Features.MeshShaderEXT;
                        pEnableOut->MeshShaderEXT.multiviewMeshShader = VK_FALSE;
                        pEnableOut->MeshShaderEXT.primitiveFragmentShadingRateMeshShader = VK_FALSE;
                        NextFeatures.Add( &pEnableOut->MeshShaderEXT );
                        pExtOut->PushBack( VK_EXT_MESH_SHADER_EXTENSION_NAME );
                    }
                }
            }
            if( requestedLevel >= FeatureLevels::LEVEL_ULTIMATE )
            {
                
            }

            ret = CheckDeviceExtensions( *pmExts, *pExtOut );
            return ret;
        }

        Result CDDI::CreateDevice( const SCreateDeviceDesc& Desc, CDeviceContext* pCtx )
        {
            m_pCtx = pCtx;
            m_pCtx->m_Features = Desc.Settings;

            auto hAdapter = m_pCtx->m_Desc.pAdapterInfo->hDDIAdapter;
            VKE_ASSERT2( hAdapter != INVALID_HANDLE, "" );
            m_hAdapter = reinterpret_cast<VkPhysicalDevice>( hAdapter );
            // VkInstance vkInstance = reinterpret_cast<VkInstance>(Desc.hAPIInstance);

            DDIExtNameArray vDDIExtNames;
            /*VKE_RETURN_IF_FAILED( LoadDeviceExtensions( m_hAdapter, &m_mExtensions ) );
            DDIExtArray vRequiredExtensions = GetRequiredDeviceExtensions( false );
            VKE_RETURN_IF_FAILED(
                CheckDeviceExtensions( m_hAdapter, &vRequiredExtensions,
                    &m_mExtensions, &vDDIExtNames ) );
            VKE_RETURN_IF_FAILED( EnableDeviceExtensions(
                Desc.Settings.Features, m_mExtensions, &m_DeviceInfo.Features,
                &vRequiredExtensions ) );
            VKE_RETURN_IF_FAILED( QueryAdapterProperties( m_hAdapter,
                m_mExtensions, &m_DeviceProperties ) );*/

            

            //auto featureLevel = CheckRequestedFeatureLevel(m_DeviceInfo, Desc.Settings.featureLevel );

            VkDeviceCreateInfo di;
            Vulkan::InitInfo( &di, VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO );

            VkPhysicalDeviceFeatures VkEnabledFeatures = {};
            SVulkanDeviceFeatures EnableFeatures = {};
            if (VKE_FAILED(EnableDeviceFeatures( m_hAdapter, &m_DeviceProperties,
                &m_mExtensions, &m_pCtx->m_Features, &EnableFeatures,
                &VkEnabledFeatures, &di, &vDDIExtNames)))
            {
                return VKE_FAIL;
            }

            for( uint32_t i = 0; i < m_DeviceProperties.Properties.Memory.memoryProperties.memoryHeapCount; ++i )
            {
                m_aHeapSizes[ i ] = m_DeviceProperties.Properties.Memory.memoryProperties.memoryHeaps[ i ].size;
            }

            Utils::TCDynamicArray<VkDeviceQueueCreateInfo> vQis;
            for( auto& Family: m_DeviceProperties.vQueueFamilies )
            {
                if( !Family.vQueues.IsEmpty() )
                {
                    VkDeviceQueueCreateInfo qi;
                    Vulkan::InitInfo( &qi, VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO );
                    qi.flags = 0;
                    qi.pQueuePriorities = &Family.vPriorities[ 0 ];
                    qi.queueFamilyIndex = Family.index;
                    qi.queueCount = static_cast<uint32_t>( Family.vQueues.GetCount() );
                    vQis.PushBack( qi );
                }
            }
            m_DeviceProperties.Features.Device.features.fillModeNonSolid = true;

            di.enabledExtensionCount = vDDIExtNames.GetCount();
            di.enabledLayerCount = 0;
            di.pEnabledFeatures = &VkEnabledFeatures;
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

        void CDDI::QueryDeviceInfo( SDeviceInfo* pOut )
        {
            auto& Limits = pOut->Limits;

            auto& Alignment = Limits.Alignment;
            Alignment.constantBufferOffset = static_cast<uint32_t>(m_DeviceProperties.Limits.minUniformBufferOffsetAlignment);
            Alignment.bufferCopyOffset =
                static_cast<uint32_t>( m_DeviceProperties.Limits.optimalBufferCopyOffsetAlignment );
            Alignment.bufferCopyRowPitch = ( uint32_t )m_DeviceProperties.Limits.optimalBufferCopyRowPitchAlignment;
            Alignment.memoryMap = ( uint32_t )m_DeviceProperties.Limits.minMemoryMapAlignment;
            Alignment.texelBufferOffset = ( uint32_t )m_DeviceProperties.Limits.minTexelBufferOffsetAlignment;
            Alignment.storageBufferOffset = ( uint32_t )m_DeviceProperties.Limits.minStorageBufferOffsetAlignment;

            auto& Binding = Limits.Binding;
            Binding.maxConstantBufferRange = m_DeviceProperties.Limits.maxUniformBufferRange;
            Binding.maxPushConstantsSize = m_DeviceProperties.Limits.maxPushConstantsSize;
            Binding.Stage.maxConstantBufferCount = m_DeviceProperties.Limits.maxPerStageDescriptorUniformBuffers;
            Binding.Stage.maxSamplerCount = m_DeviceProperties.Limits.maxPerStageDescriptorSamplers;
            Binding.Stage.maxStorageBufferCount = m_DeviceProperties.Limits.maxPerStageDescriptorStorageBuffers;
            Binding.Stage.maxStorageTextureCount = m_DeviceProperties.Limits.maxPerStageDescriptorStorageImages;
            Binding.Stage.maxResourceCount = m_DeviceProperties.Limits.maxPerStageResources;
            Binding.Stage.maxTextureCount = m_DeviceProperties.Limits.maxPerStageDescriptorSampledImages;

            auto& Memory = Limits.Memory;
            Memory.maxAllocationCount = m_DeviceProperties.Limits.maxMemoryAllocationCount;
            Memory.minMapAlignment = (uint32_t)m_DeviceProperties.Limits.minMemoryMapAlignment;
            Memory.minTexelBufferOffsetAlignment = ( uint32_t )m_DeviceProperties.Limits.minTexelBufferOffsetAlignment;
            Memory.minConstantBufferOffsetAlignment =
                ( uint32_t )m_DeviceProperties.Limits.minUniformBufferOffsetAlignment;
            Memory.minStorageBufferOffsetAlignment =
                ( uint32_t )m_DeviceProperties.Limits.minStorageBufferOffsetAlignment;

            // Get heaps for GPU, CPU and Upload
            
            for( uint32_t i = 0; i < VK_MAX_MEMORY_HEAPS; ++i )
            {
                m_aHeapIndexToHeapTypeMap[ i ] = MemoryHeapTypes::OTHER;
            }
            for (uint32_t i = 0; i < MemoryHeapTypes::_MAX_COUNT; ++i)
            {
                m_aHeapTypeToHeapIndexMap[ i ] = VK_MAX_MEMORY_HEAPS - 1;
            }
            /// TODO: enable support other heap types
            {
                VkMemoryPropertyFlags vkPropertyFlags =
                    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
                // Convert::MemoryUsagesToVkMemoryPropertyFlags( MemoryUsages::GPU_ACCESS | MemoryUsages::CPU_ACCESS );
                const auto& VkMemProps = m_DeviceProperties.Properties.Memory.memoryProperties;
                const int32_t idx = FindMemoryTypeIndex( &VkMemProps, UINT32_MAX, vkPropertyFlags );
                //Memory.aHeapSizes[ MemoryHeapTypes::CPU_COHERENT ] = 0;
                m_aHeapTypeToHeapIndexMap[ MemoryHeapTypes::CPU_COHERENT ] = INVALID_POSITION;
                if( idx >= 0 )
                {
                    const auto heapIdx = VkMemProps.memoryTypes[ idx ].heapIndex;
                    m_aHeapTypeToHeapIndexMap[ MemoryHeapTypes::CPU_COHERENT ] = heapIdx;
                    m_aHeapIndexToHeapTypeMap[ heapIdx ] = MemoryHeapTypes::CPU_COHERENT;
                }
            }
            {
                VkMemoryPropertyFlags vkPropertyFlags =
                    VK_MEMORY_PROPERTY_HOST_CACHED_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
                // Convert::MemoryUsagesToVkMemoryPropertyFlags( MemoryUsages::GPU_ACCESS | MemoryUsages::CPU_ACCESS );
                const auto& VkMemProps = m_DeviceProperties.Properties.Memory.memoryProperties;
                const int32_t idx = FindMemoryTypeIndex( &VkMemProps, UINT32_MAX, vkPropertyFlags );
                //Memory.aHeapSizes[ MemoryHeapTypes::CPU_CACHED ] = 0;
                m_aHeapTypeToHeapIndexMap[ MemoryHeapTypes::CPU_CACHED ] = INVALID_POSITION;
                if( idx >= 0 )
                {
                    const auto heapIdx = VkMemProps.memoryTypes[ idx ].heapIndex;
                    m_aHeapTypeToHeapIndexMap[ MemoryHeapTypes::CPU_CACHED ] = heapIdx;
                    m_aHeapIndexToHeapTypeMap[ heapIdx ] = MemoryHeapTypes::CPU_CACHED;
                }
            }
            {
                VkMemoryPropertyFlags vkPropertyFlags =
                    VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
                // Convert::MemoryUsagesToVkMemoryPropertyFlags( MemoryUsages::GPU_ACCESS | MemoryUsages::CPU_ACCESS );
                const auto& VkMemProps = m_DeviceProperties.Properties.Memory.memoryProperties;
                const int32_t idx = FindMemoryTypeIndex( &VkMemProps, UINT32_MAX, vkPropertyFlags );
                //Memory.aHeapSizes[ MemoryHeapTypes::OTHER ] = 0;
                //m_aHeapTypeToHeapIndexMap[ MemoryHeapTypes::OTHER ] = idx;
                if( idx >= 0 )
                {
                    const auto heapIdx = VkMemProps.memoryTypes[ idx ].heapIndex;
                    m_aHeapIndexToHeapTypeMap[ heapIdx ] = MemoryHeapTypes::OTHER;
                }
            }
            // Note that order of these queries matters as there can be the same heapIndex
            // for the same heap type. In that case we need to override with more generic ones like CPU or GPU.
            {
                VkMemoryPropertyFlags vkPropertyFlags =
                    Convert::MemoryUsagesToVkMemoryPropertyFlags( MemoryUsages::GPU_ACCESS );
                const auto& VkMemProps = m_DeviceProperties.Properties.Memory.memoryProperties;
                const int32_t idx = FindMemoryTypeIndex( &VkMemProps, UINT32_MAX, vkPropertyFlags );
                //Memory.aHeapSizes[ MemoryHeapTypes::GPU ] = 0;
                m_aHeapTypeToHeapIndexMap[ MemoryHeapTypes::GPU ] = INVALID_POSITION;
                if( idx >= 0 )
                {
                    const auto heapIdx = VkMemProps.memoryTypes[ idx ].heapIndex;
                    m_aHeapTypeToHeapIndexMap[ MemoryHeapTypes::GPU ] = heapIdx;
                    m_aHeapIndexToHeapTypeMap[ heapIdx ] = MemoryHeapTypes::GPU;
                }
            }
            {
                VkMemoryPropertyFlags vkPropertyFlags =
                    Convert::MemoryUsagesToVkMemoryPropertyFlags( MemoryUsages::CPU_ACCESS );
                const auto& VkMemProps = m_DeviceProperties.Properties.Memory.memoryProperties;
                const int32_t idx = FindMemoryTypeIndex( &VkMemProps, UINT32_MAX, vkPropertyFlags );
                //Memory.aHeapSizes[ MemoryHeapTypes::CPU ] = 0;
                m_aHeapTypeToHeapIndexMap[ MemoryHeapTypes::CPU ] = INVALID_POSITION;
                if( idx >= 0 )
                {
                    const auto heapIdx = VkMemProps.memoryTypes[ idx ].heapIndex;
                    m_aHeapTypeToHeapIndexMap[ MemoryHeapTypes::CPU ] = heapIdx;
                    m_aHeapIndexToHeapTypeMap[ heapIdx ] = MemoryHeapTypes::CPU;
                }
            }
            {
                VkMemoryPropertyFlags vkPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
                                                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                                        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
                // Convert::MemoryUsagesToVkMemoryPropertyFlags( MemoryUsages::GPU_ACCESS | MemoryUsages::CPU_ACCESS );
                const auto& VkMemProps = m_DeviceProperties.Properties.Memory.memoryProperties;
                const int32_t idx = FindMemoryTypeIndex( &VkMemProps, UINT32_MAX, vkPropertyFlags );
                m_aHeapTypeToHeapIndexMap[ MemoryHeapTypes::UPLOAD ] = INVALID_POSITION;
                if( idx >= 0 )
                {
                    const auto heapIdx = VkMemProps.memoryTypes[ idx ].heapIndex;
                    m_aHeapTypeToHeapIndexMap[ MemoryHeapTypes::UPLOAD ] = heapIdx;
                    m_aHeapIndexToHeapTypeMap[ heapIdx ] = MemoryHeapTypes::UPLOAD;
                }
            }

            auto& RenderPass = Limits.RenderPass;
            RenderPass.maxColorRenderTargetCount = m_DeviceProperties.Limits.maxColorAttachments;

            auto& Query = Limits.Query;
            Query.timestampPeriod = m_DeviceProperties.Limits.timestampPeriod;
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

        /*void CDDI::UpdateDesc( SBufferDesc* pInOut )
        {
            if( pInOut->usage & BufferUsages::CONSTANT_BUFFER ||
                pInOut->usage & BufferUsages::UNIFORM_TEXEL_BUFFER )
            {
                pInOut->size = CalcAlignedSize( pInOut->size, static_cast<uint32_t>( m_DeviceProperties.Limits.minUniformBufferOffsetAlignment ) );
            }
        }*/

        DDIBuffer   CDDI::CreateBuffer( const SBufferDesc& Desc, const void* pAllocator )
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
                if( Desc.memoryUsage & MemoryUsages::GPU_ACCESS )
                {
                    ci.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
                }

                VkResult vkRes = DDI_CREATE_OBJECT( Buffer, ci, pAllocator, &hBuffer );
                VK_ERR( vkRes );
                VKE_ASSERT2(strlen(Desc.GetDebugName()) > 0, "Debug name must be set in Debug mode");
                SetObjectDebugName( ( uint64_t )hBuffer, VK_OBJECT_TYPE_BUFFER, Desc.GetDebugName() );
            }
            return hBuffer;
        }

        void CDDI::DestroyBuffer( DDIBuffer* phBuffer, const void* pAllocator )
        {
            DDI_DESTROY_OBJECT( Buffer, phBuffer, pAllocator );
        }

        DDIBufferView CDDI::CreateBufferView( const SBufferViewDesc& Desc, const void* pAllocator )
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
            VKE_ASSERT2( strlen( Desc.GetDebugName() ) > 0, "Debug name must be set in Debug mode" );
            SetObjectDebugName( ( uint64_t )hView, VK_OBJECT_TYPE_BUFFER_VIEW, Desc.GetDebugName() );

            return hView;
        }

        void CDDI::DestroyBufferView( DDIBufferView* phBufferView, const void* pAllocator )
        {
            DDI_DESTROY_OBJECT( BufferView, phBufferView, pAllocator );
        }

        DDITexture CDDI::CreateTexture( const STextureDesc& Desc, const void* pAllocator )
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
                ci.mipLevels = Desc.mipmapCount;
                ci.samples = Vulkan::Map::SampleCount( Desc.multisampling );
                ci.pQueueFamilyIndices = nullptr;
                ci.queueFamilyIndexCount = 0;
                ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                ci.tiling = Vulkan::Convert::ImageUsageToTiling( Desc.usage );
                ci.arrayLayers = Desc.arrayElementCount;
                ci.extent.width = Desc.Size.width;
                ci.extent.height = Desc.Size.height;
                ci.extent.depth = 1;
                ci.usage = Vulkan::Map::ImageUsage( Desc.usage );
            }
            VkResult vkRes = DDI_CREATE_OBJECT( Image, ci, pAllocator, &hImage );
            VK_ERR( vkRes );
#if VKE_RENDER_SYSTEM_DEBUG
            //VKE_ASSERT2( strlen( Desc.GetDebugName() ) > 0, "Debug name must be set in Debug mode" );
            cstr_t pName;
            if( strlen( Desc.GetDebugName() ) > 0 )
            {
                pName = Desc.GetDebugName();
            }
            else
            {
                VKE_ASSERT2( !Desc.Name.IsEmpty(), "Name must not be empty" );
                pName = Desc.Name.GetData();
            }
            SetObjectDebugName( ( uint64_t )hImage, VK_OBJECT_TYPE_IMAGE, pName );
#endif
            return hImage;
        }

        Result CDDI::GetTextureFormatProperties( const STextureDesc& Desc, STextureFormatProperties* pOut )
        {
            Result ret = VKE_OK;
            VkPhysicalDeviceImageFormatInfo2 NativeFormatInfo
                = { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_FORMAT_INFO_2,
                    .pNext = nullptr,
                    .format = Map::Format(Desc.format),
                    .type = Map::ImageType(Desc.type),
                    .tiling = Vulkan::Convert::ImageUsageToTiling( Desc.usage ),
                    .usage = Map::ImageUsage(Desc.usage),
                    .flags = 0
            };

            VkImageFormatProperties2 NativeProperties =
            {
                .sType = VK_STRUCTURE_TYPE_IMAGE_FORMAT_PROPERTIES_2,
                .pNext = nullptr
            };
            auto nativeResult = sInstanceICD.vkGetPhysicalDeviceImageFormatProperties2( m_hAdapter, &NativeFormatInfo, &NativeProperties );
            VK_ERR( nativeResult );
            pOut->MaxSize = { NativeProperties.imageFormatProperties.maxExtent.width };
            pOut->maxDepth = NativeProperties.imageFormatProperties.maxExtent.depth;
            pOut->maxMipLevelCount = NativeProperties.imageFormatProperties.maxMipLevels;
            pOut->maxArrayLayerCount = NativeProperties.imageFormatProperties.maxArrayLayers;
            pOut->maxResourceSize = (uint32_t)NativeProperties.imageFormatProperties.maxResourceSize;
            ret = Map::NativeResult( nativeResult );
            return ret;
        }

        void CDDI::DestroyTexture( DDITexture* phImage, const void* pAllocator )
        {
            DDI_DESTROY_OBJECT( Image, phImage, pAllocator );
        }

        DDITextureView CDDI::CreateTextureView( const STextureViewDesc& Desc, const void* pAllocator )
        {
            static const VkComponentMapping DefaultMapping = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
            DDITextureView hView = DDI_NULL_HANDLE;
            VkImageViewCreateInfo ci;
            {
                Convert::TextureSubresourceRange( &ci.subresourceRange, Desc.SubresourceRange );
                VKE_ASSERT2( Desc.hTexture != INVALID_HANDLE, "" );
                TextureRefPtr pTex = m_pCtx->GetTexture( Desc.hTexture );
                VKE_ASSERT2( pTex.IsValid(), "" );
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

#if VKE_RENDER_SYSTEM_DEBUG
            VKE_ASSERT2( strlen( Desc.GetDebugName() ) > 0, "Debug name must be set in Debug mode" );
            SetObjectDebugName( ( uint64_t )hView, VK_OBJECT_TYPE_IMAGE_VIEW, Desc.GetDebugName() );
#endif

            return hView;
        }

        void CDDI::DestroyTextureView( DDITextureView* phImageView, const void* pAllocator )
        {
            DDI_DESTROY_OBJECT( ImageView, phImageView, pAllocator );
        }

        DDIFramebuffer CDDI::CreateFramebuffer( const SFramebufferDesc& Desc, const void* pAllocator )
        {
            //const uint32_t attachmentCount = Desc.vDDIAttachments.GetCount();

            VkFramebufferCreateInfo ci;
            ci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            ci.pNext = nullptr;
            ci.flags = 0;
            ci.width = Desc.Size.width;
            ci.height = Desc.Size.height;
            ci.layers = 1;
            ci.attachmentCount = Desc.vDDIAttachments.GetCount();
            ci.pAttachments = Desc.vDDIAttachments.GetData();
            ci.renderPass = (DDIRenderPass)Desc.hRenderPass.handle;
            //ci.renderPass = m_pCtx->GetRenderPass( Desc.hRenderPass )->GetDDIObject();

            DDIFramebuffer hFramebuffer = DDI_NULL_HANDLE;
            VkResult vkRes = DDI_CREATE_OBJECT( Framebuffer, ci, pAllocator, &hFramebuffer );
            VK_ERR( vkRes );

            VKE_ASSERT2( strlen( Desc.GetDebugName() ) > 0, "Debug name must be set in Debug mode" );
            SetObjectDebugName( ( uint64_t )hFramebuffer, VK_OBJECT_TYPE_FRAMEBUFFER, Desc.GetDebugName() );

            return hFramebuffer;

        }

        void CDDI::DestroyFramebuffer( DDIFramebuffer* phFramebuffer, const void* pAllocator )
        {
            DDI_DESTROY_OBJECT( Framebuffer, phFramebuffer, pAllocator );
        }

        DDIFence CDDI::CreateFence( const SFenceDesc& Desc, const void* pAllocator )
        {
            VkFenceCreateInfo ci;
            ci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            ci.pNext = nullptr;
            ci.flags = Desc.isSignaled;
            DDIFence hObj = DDI_NULL_HANDLE;
            VkResult res = DDI_CREATE_OBJECT( Fence, ci, pAllocator, &hObj );
            VK_ERR( res );
            Helper::SetObjectDebugName( this, hObj, VK_OBJECT_TYPE_FENCE, Desc );
            return hObj;
        }

        void CDDI::DestroyFence( DDIFence* phFence, const void* pAllocator )
        {
            DDI_DESTROY_OBJECT( Fence, phFence, pAllocator );
        }

        DDISemaphore CDDI::CreateSemaphore( const SSemaphoreDesc& Desc, const void* pAllocator )
        {
            DDISemaphore hSemaphore = DDI_NULL_HANDLE;
            VkSemaphoreCreateInfo ci;
            ci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            ci.pNext = nullptr;
            ci.flags = 0;
            VK_ERR( DDI_CREATE_OBJECT( Semaphore, ci, pAllocator, &hSemaphore ) );
            Helper::SetObjectDebugName( this, hSemaphore, VK_OBJECT_TYPE_SEMAPHORE, Desc );
            return hSemaphore;
        }

        void CDDI::DestroySemaphore( DDISemaphore* phSemaphore, const void* pAllocator )
        {
            DDI_DESTROY_OBJECT( Semaphore, phSemaphore, pAllocator );
        }

        DDICommandBufferPool CDDI::CreateCommandBufferPool( const SCommandBufferPoolDesc& Desc, const void* pAllocator )
        {
            DDICommandBufferPool hPool = DDI_NULL_HANDLE;
            VkCommandPoolCreateInfo ci;
            ci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            ci.pNext = nullptr;
            ci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            ci.queueFamilyIndex = Desc.pContext->m_pQueue->GetFamilyIndex();
            VkResult res = DDI_CREATE_OBJECT( CommandPool, ci, pAllocator, &hPool );
            VK_ERR( res );
            return hPool;
        }

        void CDDI::DestroyCommandBufferPool( DDICommandBufferPool* phPool, const void* pAllocator )
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
                pRefOut->layout = Vulkan::Map::ImageLayout( SubpassAttachmentDesc.state );
                res = true;
            }
            return res;
        }

        DDIRenderPass CDDI::CreateRenderPass( const SRenderPassDesc& Desc, const void* )
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
                vkAttachmentDesc.finalLayout = Vulkan::Map::ImageLayout( AttachmentDesc.endState );
                vkAttachmentDesc.flags = 0;
                vkAttachmentDesc.format = Map::Format( AttachmentDesc.format );
                vkAttachmentDesc.initialLayout = Vulkan::Map::ImageLayout( AttachmentDesc.beginState );
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
                        VkDepthStencilRef.attachment = i;
                        VkDepthStencilRef.layout = Map::ImageLayout( Curr.beginState );
                        pVkDepthStencilRef = &VkDepthStencilRef;
                    }
                    else
                    {
                        // Find attachment
                        VkAttachmentReference vkRef;
                        vkRef.attachment = i;
                        vkRef.layout = Vulkan::Map::ImageLayout( Curr.beginState );
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
                pVkDepthStencilRef = nullptr;
                if( SubpassDesc.DepthBuffer.hTextureView != INVALID_HANDLE )
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
                SetObjectDebugName( ( uint64_t )hPass, VK_OBJECT_TYPE_RENDER_PASS, Desc.GetDebugName() );
            }
            return hPass;
        }

        void CDDI::DestroyRenderPass( DDIRenderPass* phRenderPass, const void* pAllocator )
        {
            DDI_DESTROY_OBJECT( RenderPass, phRenderPass, pAllocator );
        }

        DDIDescriptorPool CDDI::CreateDescriptorPool( const SDescriptorPoolDesc& Desc, const void* pAllocator )
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
            SetObjectDebugName( ( uint64_t )hPool, VK_OBJECT_TYPE_DESCRIPTOR_POOL, Desc.GetDebugName() );
            return hPool;
        }

        void CDDI::DestroyDescriptorPool( DDIDescriptorPool* phPool, const void* pAllocator )
        {
            DDI_DESTROY_OBJECT( DescriptorPool, phPool, pAllocator );
        }

        DDIPipeline CDDI::CreatePipeline( const SPipelineDesc& Desc, const void* pAllocator)
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

                    Utils::TCDynamicArray< VkPipelineColorBlendAttachmentState > vVkBlendStates;
                    if( vBlendStates.IsEmpty() )
                    {
                        VKE_LOG_WARN( "No blend states specified for pipeline: " << Desc.GetDebugName() );
                        VkPipelineColorBlendAttachmentState VkState;
                        VkState.alphaBlendOp = VK_BLEND_OP_ADD;
                        VkState.blendEnable = VK_FALSE;
                        VkState.colorBlendOp = VK_BLEND_OP_ADD;
                        VkState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
                        VkState.srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
                        VkState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
                        VkState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
                        VkState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;

                        vVkBlendStates.PushBack( VkState );
                    }
                    else
                    {
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
                        }
                    }
                    State.pAttachments = &vVkBlendStates[0];
                    State.attachmentCount = vVkBlendStates.GetCount();
                    State.logicOp = Map::LogicOperation( Desc.Blending.logicOperation );
                    State.logicOpEnable = Desc.Blending.enable;
                    memset( State.blendConstants, 0, sizeof( float ) * 4 );
                }
                ci.pColorBlendState = &VkColorBlendState;

                VkPipelineDepthStencilStateCreateInfo VkDepthStencil = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
                {
                    auto& State = VkDepthStencil;

                    if( Desc.DepthStencil.Depth.enable )
                    {

                        State.depthBoundsTestEnable = Desc.DepthStencil.Depth.Bounds.enable;
                        State.depthCompareOp = Map::CompareOperation( Desc.DepthStencil.Depth.compareFunc );
                        State.depthTestEnable = Desc.DepthStencil.Depth.test;
                        State.depthWriteEnable = Desc.DepthStencil.Depth.write;
                        State.maxDepthBounds = Desc.DepthStencil.Depth.Bounds.max;
                        State.minDepthBounds = Desc.DepthStencil.Depth.Bounds.min;
                    }
                    if( Desc.DepthStencil.Stencil.enable )
                    {
                        {
                            VkStencilOpState VkFace;
                            const auto& Face = Desc.DepthStencil.Stencil.BackFace;

                            VkFace.compareMask = Face.compareMask;
                            VkFace.compareOp = Map::CompareOperation( Face.compareFunc );
                            VkFace.depthFailOp = Map::StencilOperation( Face.depthFailFunc );
                            VkFace.failOp = Map::StencilOperation( Face.failFunc );
                            VkFace.passOp = Map::StencilOperation( Face.passFunc );
                            VkFace.reference = Face.reference;
                            VkFace.writeMask = Face.writeMask;
                        }
                        {
                            VkStencilOpState VkFace;
                            const auto& Face = Desc.DepthStencil.Stencil.FrontFace;

                            VkFace.compareMask = Desc.DepthStencil.Stencil.BackFace.compareMask;
                            VkFace.compareOp = Map::CompareOperation( Face.compareFunc );
                            VkFace.depthFailOp = Map::StencilOperation( Face.depthFailFunc );
                            VkFace.failOp = Map::StencilOperation( Face.failFunc );
                            VkFace.passOp = Map::StencilOperation( Face.passFunc );
                            VkFace.reference = Face.reference;
                            VkFace.writeMask = Face.writeMask;
                        }
                    }
                    State.stencilTestEnable = Desc.DepthStencil.Stencil.enable;
                    if( Desc.DepthStencil.Depth.enable || Desc.DepthStencil.Stencil.enable )
                    {
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
                                State.pName = pShader->GetDesc().EntryPoint.GetData();
                                State.stage = Map::ShaderStage( static_cast<SHADER_TYPE>(i) );
                                State.pSpecializationInfo = nullptr;
                                vkShaderStages |= State.stage;
                                stageCount++;
                                vVkStages.PushBack( State );
                                VKE_LOG( "Stage: " << State.stage << ": " << State.pName );
                            }
                        }
                    }
                }

                ci.stageCount = stageCount;
                ci.pStages = &vVkStages[0];

                VkPipelineTessellationStateCreateInfo VkTesselation = { VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO };
                VkPipelineTessellationDomainOriginStateCreateInfo VkDomainOrigin;
                if( Desc.Tesselation.enable )
                {
                    auto& State = VkTesselation;
                    {
                        State.patchControlPoints = Desc.Tesselation.patchControlPoints;
                    }
                    
                    VkDomainOrigin.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_DOMAIN_ORIGIN_STATE_CREATE_INFO;
                    VkDomainOrigin.pNext = nullptr;
                    VkDomainOrigin.domainOrigin = Map::TessellationDomainOrigin( Desc.Tesselation.domainOrigin );
                    State.pNext = &VkDomainOrigin;

                    ci.pTessellationState = &VkTesselation;
                }

                VkPipelineInputAssemblyStateCreateInfo VkInputAssembly = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
                ci.pInputAssemblyState = nullptr;
                if( Desc.InputLayout.enable )
                {
                    auto& State = VkInputAssembly;
                    {
                        State.primitiveRestartEnable = Desc.InputLayout.enablePrimitiveRestart;
                        State.topology = Map::PrimitiveTopology( Desc.InputLayout.topology );
                    }
                    ci.pInputAssemblyState = &VkInputAssembly;
                }

                VkPipelineVertexInputStateCreateInfo VkVertexInput = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO  };
                ci.pVertexInputState = nullptr;
                if( Desc.InputLayout.enable )
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
                            uint32_t currVertexBufferBinding = UNDEFINED_U32;
                            uint32_t vertexSize = 0;

                            for( uint32_t i = 0; i < vAttribs.GetCount(); ++i )
                            {
                                auto& vkAttrib = vVkAttribs[i];
                                vkAttrib.binding = vAttribs[i].vertexBufferBindingIndex;
                                vkAttrib.format = Map::Format( vAttribs[i].format );
                                vkAttrib.location = vAttribs[i].location;
                                vkAttrib.offset = vAttribs[i].offset;
                                vertexSize += vAttribs[i].stride;
                            }
                            for( uint32_t i = 0; i < vAttribs.GetCount(); ++i )
                            {
                                if( currVertexBufferBinding != vAttribs[i].vertexBufferBindingIndex )
                                {
                                    currVertexBufferBinding = vAttribs[i].vertexBufferBindingIndex;
                                    VkVertexInputBindingDescription VkBinding;
                                    VkBinding.binding = currVertexBufferBinding;
                                    VkBinding.inputRate = Vulkan::Map::InputRate( vAttribs[i].inputRate );
                                    VkBinding.stride = vertexSize; // todo this will cause corruption if more than 1 vertex buffer is used
                                    vVkBindings.PushBack( VkBinding );
                                }
                            }

                            State.pVertexAttributeDescriptions = &vVkAttribs[0];
                            State.pVertexBindingDescriptions = &vVkBindings[0];
                            State.vertexAttributeDescriptionCount = vVkAttribs.GetCount();
                            State.vertexBindingDescriptionCount = vVkBindings.GetCount();
                        }
                    }
                    
                    ci.pVertexInputState = &VkVertexInput;
                }
                

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
                        VKE_ASSERT2( vVkViewports.GetCount() == vVkScissors.GetCount(), "" );
                        State.pViewports = vVkViewports.GetData();
                        State.viewportCount = std::max( 1u, vVkViewports.GetCount() ); // at least one viewport
                        State.pScissors = vVkScissors.GetData();
                        State.scissorCount = State.viewportCount;
                    }
                    ci.pViewportState = &VkViewportState;
                }

                bool create = true;
                if( Desc.hDDILayout )
                {
                    VkGraphicsInfo.layout = Desc.hDDILayout;
                }
                else
                {
                    create = Desc.hLayout != INVALID_HANDLE;
                    if( create )
                    {
                        VkGraphicsInfo.layout = m_pCtx->GetPipelineLayout( Desc.hLayout )->GetDDIObject();
                    }
                    else
                    {
                        VKE_LOG_WARN( "No valid pipeline layout handle provided. Pipeline will not be created." );
                    }
                }
                if( Desc.hDDIRenderPass != DDI_NULL_HANDLE )
                {
                    VkGraphicsInfo.renderPass = Desc.hDDIRenderPass;
                }
                else if( Desc.hRenderPass != INVALID_HANDLE )
                {
                    VkGraphicsInfo.renderPass = m_pCtx->GetRenderPass( Desc.hRenderPass )->GetDDIObject();
                }
                if( create )
                {
                    VkPipelineRenderingCreateInfoKHR VkDynamicRenderingInfo;
                    const auto vFormats = Map::Formats( Desc.vColorRenderTargetFormats.GetData(), Desc.vColorRenderTargetFormats.GetCount() );
                    if (VkGraphicsInfo.renderPass == DDI_NULL_HANDLE)
                    {
                        VkDynamicRenderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
                        VkDynamicRenderingInfo.pNext = nullptr;
                        VkDynamicRenderingInfo.colorAttachmentCount = vFormats.GetCount();
                        VkDynamicRenderingInfo.pColorAttachmentFormats = vFormats.GetDataOrNull();
                        VkDynamicRenderingInfo.depthAttachmentFormat = Map::Format( Desc.depthRenderTargetFormat );
                        VkDynamicRenderingInfo.stencilAttachmentFormat = Map::Format( Desc.stencilRenderTargetFormat );
                        VkDynamicRenderingInfo.viewMask = 0;
                        VkGraphicsInfo.pNext = &VkDynamicRenderingInfo;
                    }
                    vkRes = m_ICD.vkCreateGraphicsPipelines( m_hDevice, VK_NULL_HANDLE, 1, &VkGraphicsInfo, nullptr, &hPipeline );
                }
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
                    ci.stage.pName = pShader->GetDesc().EntryPoint.GetData();
                    ci.stage.stage = Map::ShaderStage( static_cast<SHADER_TYPE>(ShaderTypes::COMPUTE) );
                    ci.stage.pSpecializationInfo = nullptr;
                }

                VkComputeInfo.layout = (VkPipelineLayout)(Desc.hLayout.handle);
                vkRes = m_ICD.vkCreateComputePipelines( m_hDevice, VK_NULL_HANDLE, 1, &VkComputeInfo, pVkCallbacks, &hPipeline );
            }

            VK_ERR( vkRes );
            SetObjectDebugName( ( uint64_t )hPipeline, VK_OBJECT_TYPE_PIPELINE, Desc.GetDebugName() );
            return hPipeline;
        }

        void CDDI::DestroyPipeline( DDIPipeline* phPipeline, const void* pAllocator )
        {
            DDI_DESTROY_OBJECT( Pipeline, phPipeline, pAllocator );
        }

        DDIDescriptorSetLayout CDDI::CreateDescriptorSetLayout( const SDescriptorSetLayoutDesc& Desc, const void* pAllocator )
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

                    vVkBindings[ i ] = ( VkBinding );
                }
                ci.pBindings = vVkBindings.GetData();

                VK_ERR( DDI_CREATE_OBJECT( DescriptorSetLayout, ci, pAllocator, &hLayout ) );
                VKE_ASSERT2( strlen( Desc.GetDebugName() ) > 0, "" );
                SetObjectDebugName( ( uint64_t )hLayout, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, Desc.GetDebugName() );
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

        void CDDI::Update( const SUpdateTextureDescriptorSetInfo& Info )
        {
            Utils::TCDynamicArray< VkDescriptorImageInfo, 8 > vVkInfos;
            for( uint32_t i = 0; i < Info.vTextureInfos.GetCount(); ++i )
            {
                const auto& Curr = Info.vTextureInfos[i];
                VkDescriptorImageInfo VkInfo;
                VkInfo.imageLayout = Map::ImageLayout( Curr.textureState );
                VkInfo.imageView = Curr.hDDITextureView;
                VkInfo.sampler = Curr.hDDISampler;
                vVkInfos.PushBack( VkInfo );
            }

            VkWriteDescriptorSet VkWrite = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
            VkWrite.descriptorCount = Info.count;
            VkWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            VkWrite.dstArrayElement = 0;
            VkWrite.dstBinding = Info.binding;
            VkWrite.dstSet = Info.hDDISet;
            VkWrite.pImageInfo = vVkInfos.GetData();

            m_ICD.vkUpdateDescriptorSets( m_hDevice, 1, &VkWrite, 0, nullptr );
        }

        void CDDI::Update( const DDIDescriptorSet& hDDISet, const SUpdateBindingsHelper& Info )
        {
            Utils::TCDynamicArray <  VkWriteDescriptorSet > vVkWrites;
            VkWriteDescriptorSet VkWrite = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
            
            using ImageViewInfosArray = Utils::TCDynamicArray< VkDescriptorImageInfo, 128 >;
            using ImageViewInfosArrays = Utils::TCDynamicArray< ImageViewInfosArray, 32 >;
            using RenderTargetInfosArray = Utils::TCDynamicArray< VkDescriptorImageInfo, 8 >;
            using RenderTargetInfosArrays = Utils::TCDynamicArray< RenderTargetInfosArray, 8 >;
            using SamplerInfoArray = Utils::TCDynamicArray< VkDescriptorImageInfo, 128 >;
            using SamplerInfosArrays = Utils::TCDynamicArray< SamplerInfoArray, 32 >;

            VKE_ASSERT2( Info.vRTs.GetCount() < 8, "Too many render targets to bind" );
            VKE_ASSERT2( Info.vTexViews.GetCount() < 32, "Too many texture views to bind" );
            VKE_ASSERT2( Info.vSamplers.GetCount() < 32, "Too many samplers to bind." );
            VKE_ASSERT2( Info.vSamplerAndTextures.GetCount() < 32, "Too many samplers to bind." );

            RenderTargetInfosArrays vvVkRenderTargetInfos( Info.vRTs.GetCount() );
            ImageViewInfosArrays vvVkImageViewsInfos( Info.vTexViews.GetCount() );
            SamplerInfosArrays vvVkSamplerInfos( Info.vSamplers.GetCount() );
            ImageViewInfosArrays vvVkImageSamplerInfosArrays( Info.vSamplerAndTextures.GetCount() );

            for( uint32_t i = 0; i < Info.vRTs.GetCount(); ++i )
            {
                const auto& Curr = Info.vRTs[i];
                for( uint32_t j = 0; j < Curr.count; ++j )
                {
                    VkDescriptorImageInfo VkInfo;
                    VkInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    VkInfo.imageView = m_pCtx->GetTextureView( Curr.ahHandles[j] )->GetDDIObject();
                    VkInfo.sampler = DDI_NULL_HANDLE;
                    vvVkRenderTargetInfos[ i ].PushBack( VkInfo );
                }

                VkWrite.descriptorCount = Curr.count;
                VkWrite.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
                VkWrite.dstArrayElement = 0;
                VkWrite.dstBinding = Curr.binding;
                VkWrite.pImageInfo = vvVkRenderTargetInfos[ i ].GetData();
                VkWrite.dstSet = hDDISet;
                vVkWrites.PushBack( VkWrite );
            }

            /*for( uint32_t i = 0; i < Info.vTexs.GetCount(); ++i )
            {
                vVkImgInfos[1].Clear();
                const auto& Curr = Info.vTexs[i];
                for( uint32_t j = 0; j < Curr.count; ++j )
                {
                    VkDescriptorImageInfo VkInfo;
                    VkInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    VkInfo.imageView = m_pCtx->GetTextureView( Curr.ahHandles[j] )->GetDDIObject();
                    VkInfo.sampler = DDI_NULL_HANDLE;
                    vVkImgInfos[1].PushBack( VkInfo );
                }

                VkWrite.descriptorCount = Curr.count;
                VkWrite.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
                VkWrite.dstArrayElement = 0;
                VkWrite.dstBinding = Curr.binding;
                VkWrite.pImageInfo = vVkImgInfos[1].GetData();
                VkWrite.dstSet = hDDISet;
                vVkWrites.PushBack( VkWrite );
            }*/

            for (uint32_t i = 0; i < Info.vTexViews.GetCount(); ++i)
            {
                const auto& Curr = Info.vTexViews[i];
                VkDescriptorImageInfo VkInfo;
                for (uint32_t j = 0; j < Curr.count; ++j)
                {
                    VkInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    VkInfo.imageView = m_pCtx->GetTextureView(Curr.ahHandles[j])->GetDDIObject();
                    VkInfo.sampler = DDI_NULL_HANDLE;
                    vvVkImageViewsInfos[ i ].PushBack( VkInfo );
                    /*VKE_LOG("Update desc set: " << hDDISet << ", " << (uint32_t)Curr.binding << ", " <<
                             j << ": " << VkInfo.imageView << ": " << Curr.ahHandles[ j ].handle );*/
                }

                VkWrite.descriptorCount = Curr.count;
                VkWrite.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
                VkWrite.dstArrayElement = 0;
                VkWrite.dstBinding = Curr.binding;
                VkWrite.pImageInfo = vvVkImageViewsInfos[ i ].GetData();
                VkWrite.dstSet = hDDISet;
                vVkWrites.PushBack(VkWrite);
            }

            for( uint32_t i = 0; i < Info.vSamplers.GetCount(); ++i )
            {
                const auto& Curr = Info.vSamplers[i];
                for( uint32_t j = 0; j < Curr.count; ++j )
                {
                    VkDescriptorImageInfo VkInfo;
                    VkInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    VkInfo.imageView = DDI_NULL_HANDLE;
                    VkInfo.sampler = m_pCtx->GetSampler( Curr.ahHandles[j] )->GetDDIObject();
                    vvVkSamplerInfos[i].PushBack( VkInfo );
                }

                VkWrite.descriptorCount = Curr.count;
                VkWrite.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
                VkWrite.dstArrayElement = 0;
                VkWrite.dstBinding = Curr.binding;
                VkWrite.pImageInfo = vvVkSamplerInfos[ i ].GetData();
                VkWrite.dstSet = hDDISet;
                vVkWrites.PushBack( VkWrite );
            }

            for (uint32_t i = 0; i < Info.vSamplerAndTextures.GetCount(); ++i)
            {
                const auto& Curr = Info.vSamplerAndTextures[i];
                for (uint32_t j = 0; j < Curr.count; ++j)
                {
                    VkDescriptorImageInfo VkInfo;
                    VkInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    VkInfo.imageView = m_pCtx->GetTextureView(Curr.ahTexViews[j])->GetDDIObject();
                    VkInfo.sampler = m_pCtx->GetSampler(Curr.ahSamplers[j])->GetDDIObject();
                    vvVkImageSamplerInfosArrays[ i ].PushBack( VkInfo );
                }

                VkWrite.descriptorCount = Curr.count;
                VkWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                VkWrite.dstArrayElement = 0;
                VkWrite.dstBinding = Curr.binding;
                VkWrite.pImageInfo = vvVkImageSamplerInfosArrays[i].GetData();
                VkWrite.dstSet = hDDISet;
                vVkWrites.PushBack(VkWrite);
            }

            using VkBufferInfoArray = Utils::TCDynamicArray<VkDescriptorBufferInfo>;
            Utils::TCDynamicArray< VkBufferInfoArray > vvVkBuffInfos;
            vvVkBuffInfos.Resize( Info.vBuffers.GetCount() );

            for( uint32_t i = 0; i < Info.vBuffers.GetCount(); ++i )
            {
                auto& vVkBuffInfos = vvVkBuffInfos[i];

                const auto& Curr = Info.vBuffers[i];
                for( uint32_t j = 0; j < Curr.count; ++j )
                {
                    VkDescriptorBufferInfo VkInfo;
                    VkInfo.buffer = m_pCtx->GetBuffer( Curr.ahHandles[j] )->GetDDIObject();
                    VkInfo.offset = Curr.offset;
                    VkInfo.range = Curr.range;
                    vVkBuffInfos.PushBack( VkInfo );
                }

                VkWrite.descriptorCount = vVkBuffInfos.GetCount();
                VkWrite.descriptorType = Map::DescriptorType( Curr.type );
                VkWrite.dstArrayElement = 0;
                VkWrite.dstBinding = Curr.binding;
                VkWrite.pBufferInfo = vVkBuffInfos.GetData();
                VkWrite.dstSet = hDDISet;
                vVkWrites.PushBack( VkWrite );
            }

            m_ICD.vkUpdateDescriptorSets( m_hDevice, vVkWrites.GetCount(), vVkWrites.GetData(), 0, nullptr );
        }

        void CDDI::Update( const DDIDescriptorSet& hDDISrcSet, DDIDescriptorSet* phDDIDstOut )
        {
            VkCopyDescriptorSet vkCopy;
            vkCopy.sType = VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET;
            vkCopy.pNext = nullptr;
            vkCopy.descriptorCount = 1;
            vkCopy.dstArrayElement = 0;
            vkCopy.dstBinding = 0;
            vkCopy.srcArrayElement = 0;
            vkCopy.srcBinding = 1;
            vkCopy.srcSet = hDDISrcSet;
            vkCopy.dstSet = *phDDIDstOut;
            m_ICD.vkUpdateDescriptorSets( m_hDevice, 0, 0, 1, &vkCopy );
        }

        void CDDI::DestroyDescriptorSetLayout( DDIDescriptorSetLayout* phLayout, const void* pAllocator )
        {
            DDI_DESTROY_OBJECT( DescriptorSetLayout, phLayout, pAllocator );
        }

        DDIPipelineLayout CDDI::CreatePipelineLayout( const SPipelineLayoutDesc& Desc, const void* pAllocator )
        {
            DDIPipelineLayout hLayout = DDI_NULL_HANDLE;
            VkPipelineLayoutCreateInfo ci;
            ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            ci.pNext = nullptr;
            ci.flags = 0;

            //VKE_ASSERT2( !Desc.vDescriptorSetLayouts.IsEmpty(), "There should be at least one DescriptorSetLayout." );
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

        void CDDI::DestroyPipelineLayout( DDIPipelineLayout* phLayout, const void* pAllocator )
        {
            DDI_DESTROY_OBJECT( PipelineLayout, phLayout, pAllocator );
        }

        DDIShader CDDI::CreateShader( const SShaderData& Data, const void* pAllocator )
        {
            VKE_ASSERT2( Data.stage == ShaderCompilationStages::COMPILED_IR_BINARY &&
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

        void CDDI::DestroyShader( DDIShader* phShader, const void* pAllocator )
        {
            DDI_DESTROY_OBJECT( ShaderModule, phShader, pAllocator );
        }

        DDISampler CDDI::CreateSampler( const SSamplerDesc& Desc, const void* pAllocator)
        {
            DDISampler hSampler = DDI_NULL_HANDLE;
            VkSamplerCreateInfo ci;
            ci.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            ci.pNext = nullptr;
            ci.flags = 0;
            ci.addressModeU = Map::AddressMode( Desc.AddressMode.U );
            ci.addressModeV = Map::AddressMode( Desc.AddressMode.V );
            ci.addressModeW = Map::AddressMode( Desc.AddressMode.W );
            ci.anisotropyEnable = Desc.enableAnisotropy;
            ci.borderColor = Convert::BorderColor( Desc.borderColor );
            ci.compareEnable = Desc.enableCompare;
            ci.compareOp = Map::CompareOperation( Desc.compareFunc );
            ci.magFilter = Convert::Filter( Desc.Filter.mag );
            ci.maxAnisotropy = Desc.maxAnisotropy;
            ci.maxLod = Desc.LOD.max;
            ci.minFilter = Convert::Filter( Desc.Filter.min );
            ci.minLod = Desc.LOD.min;
            ci.mipLodBias = Desc.mipLODBias;
            ci.mipmapMode = Map::MipmapMode( Desc.mipmapMode );
            ci.unnormalizedCoordinates = Desc.unnormalizedCoordinates;
            VK_ERR( DDI_CREATE_OBJECT( Sampler, ci, pAllocator, &hSampler ) );
            return hSampler;
        }

        void CDDI::DestroySampler( DDISampler* phSampler, const void* pAllocator )
        {
            DDI_DESTROY_OBJECT( Sampler, phSampler, pAllocator );
        }

        DDIEvent CDDI::CreateEvent( const SEventDesc&, const void* pAllocator )
        {
            static const VkEventCreateInfo ci = { VK_STRUCTURE_TYPE_EVENT_CREATE_INFO };
            DDIEvent hRet;
            VK_ERR( DDI_CREATE_OBJECT( Event, ci, pAllocator, &hRet ) );
            return hRet;
        }

        void CDDI::DestroyEvent( DDIEvent* phEvent, const void* pAllocator )
        {
            DDI_DESTROY_OBJECT( Event, phEvent, pAllocator );
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
            
            switch (res)
            {
                case VK_SUCCESS: ret = VKE_OK; break;
                case VK_ERROR_OUT_OF_POOL_MEMORY: ret = VKE_ENOMEMORY; break;
                default: VK_ERR(res);
            }

            VKE_ASSERT2( strlen( Info.GetDebugName() ) > 0, "Debug name must be set in Debug mode" );
#if VKE_RENDER_SYSTEM_DEBUG
            if( VKE_SUCCEEDED( ret ) )
            {
                for( uint32_t i = 0; i < ai.descriptorSetCount; ++i )
                {
                    SetObjectDebugName( ( uint64_t )pSets[ i ], VK_OBJECT_TYPE_DESCRIPTOR_SET, Info.GetDebugName() );
                }
            }
#endif
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

        size_t CDDI::GetMemoryHeapTotalSize( MEMORY_HEAP_TYPE type ) const
        {
            const auto idx = m_aHeapTypeToHeapIndexMap[ type ];
            return m_DeviceProperties.Properties.Memory.memoryProperties.memoryHeaps[ idx ].size;
        }

        size_t CDDI::GetMemoryHeapCurrentSize( MEMORY_HEAP_TYPE type ) const
        {
            const auto idx = m_aHeapTypeToHeapIndexMap[ type ];
            return m_aHeapSizes[ idx ];
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

        MEMORY_HEAP_TYPE CDDI::GetMemoryHeapType( MEMORY_USAGE usage ) const
        {
            MEMORY_HEAP_TYPE ret = MemoryHeapTypes::OTHER;
            VkMemoryPropertyFlags vkPropertyFlags = Convert::MemoryUsagesToVkMemoryPropertyFlags( usage );
            const auto& VkMemProps = m_DeviceProperties.Properties.Memory.memoryProperties;
            const int32_t idx = FindMemoryTypeIndex( &VkMemProps, UINT32_MAX, vkPropertyFlags );
            if (idx >= 0)
            {
                //const auto heapIdx = VkMemProps.memoryTypes[ idx ].heapIndex;
                const auto memFlags = VkMemProps.memoryTypes[ idx ].propertyFlags;
                if( ( memFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT ) == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT )
                {
                    ret = MemoryHeapTypes::GPU;
                    if( ( memFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT ) == VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT )
                    {
                        ret = MemoryHeapTypes::UPLOAD;
                    }
                }
                else if( ( memFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT ) == VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT )
                {
                    ret = MemoryHeapTypes::CPU;
                }
            }
            return ret;
        }

        Result CDDI::Allocate( const SAllocateMemoryDesc& Desc, SAllocateMemoryData* pOut )
        {
            Result ret = VKE_FAIL;
            VkMemoryPropertyFlags vkPropertyFlags = Convert::MemoryUsagesToVkMemoryPropertyFlags( Desc.usage );

            const auto& VkMemProps = m_DeviceProperties.Properties.Memory.memoryProperties;
            int32_t idx = FindMemoryTypeIndex( &VkMemProps, UINT32_MAX, vkPropertyFlags );
            //const uint32_t idx = m_aHeapTypeToHeapIndexMap[  ];
            DDIMemory hMemory;
            if( idx >= 0 )
            {
                auto heapIdx = VkMemProps.memoryTypes[ idx ].heapIndex;
                auto memFlags = VkMemProps.memoryTypes[ idx ].propertyFlags;
                // If there is no space left in upload heap try to allocate on CPU
                if( ( Desc.usage & MemoryUsages::UPLOAD ) == MemoryUsages::UPLOAD &&
                    m_aHeapSizes[ heapIdx ] < Desc.size )
                {
                    VKE_LOG_WARN( "No free space left on UPLOAD heap: " << VKE_LOG_MEM_SIZE( m_aHeapSizes[ heapIdx ] )
                                                                        << ", requested allocation size: "
                    << VKE_LOG_MEM_SIZE(Desc.size) << ". Trying to allocate on a CPU heap instead." );
                    MEMORY_USAGE newUsages = MemoryUsages::STAGING_BUFFER;
                    vkPropertyFlags = Convert::MemoryUsagesToVkMemoryPropertyFlags( newUsages );
                    idx = FindMemoryTypeIndex( &VkMemProps, UINT32_MAX, vkPropertyFlags );
                    VKE_ASSERT2( idx >= 0, "" );
                    heapIdx = VkMemProps.memoryTypes[ idx ].heapIndex;
                    memFlags = VkMemProps.memoryTypes[ idx ].propertyFlags;
                }
                VKE_ASSERT2( m_aHeapSizes[ heapIdx ] >= Desc.size, "" );
                VkMemoryAllocateInfo ai = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
                ai.allocationSize = Desc.size;
                ai.memoryTypeIndex = idx;
                VkResult res = m_ICD.vkAllocateMemory( m_hDevice, &ai, nullptr, &hMemory );
                VK_ERR( res );
                if( res == VK_SUCCESS )
                {
                    m_aHeapSizes[ heapIdx ] -= ai.allocationSize;

                    pOut->hDDIMemory = hMemory;
                    pOut->sizeLeft = static_cast< uint32_t >( m_aHeapSizes[ heapIdx ] );
                    pOut->heapType = Map::VkMemPropertyFlagsToHeapType( memFlags );
                }
                ret = res == VK_SUCCESS ? VKE_OK : VKE_ENOMEMORY;
            }
            else
            {
                VKE_LOG_ERR( "Required memory usage: " << Desc.usage << " is not suitable for this GPU." );
            }
            return ret;
        }

        Result CDDI::GetTextureMemoryRequirements( const DDITexture& hTexture, SAllocationMemoryRequirementInfo* pOut )
        {
            VkMemoryRequirements VkReq;
            m_ICD.vkGetImageMemoryRequirements( m_hDevice, hTexture, &VkReq );
            pOut->alignment = static_cast< uint32_t >( VkReq.alignment );
            pOut->size = static_cast< uint32_t >( VkReq.size );
            return VKE_OK;
        }

        Result CDDI::GetBufferMemoryRequirements( const DDIBuffer& hBuffer, SAllocationMemoryRequirementInfo* pOut )
        {
            VkMemoryRequirements VkReq;
            m_ICD.vkGetBufferMemoryRequirements( m_hDevice, hBuffer, &VkReq );
            pOut->alignment = static_cast< uint32_t >( VkReq.alignment );
            pOut->size = static_cast< uint32_t >( VkReq.size );
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

        bool CDDI::IsSignaled( const DDIFence& hFence ) const
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

        void CDDI::DrawIndexed( const DDICommandBuffer& hCommandBuffer, const SDrawParams& Params )
        {
            m_ICD.vkCmdDrawIndexed( hCommandBuffer,
                                    Params.Indexed.indexCount, Params.Indexed.instanceCount,
                                    Params.Indexed.startIndex, Params.Indexed.vertexOffset,
                                    Params.Indexed.startInstance );
        }

        void CDDI::DrawMesh(const DDICommandBuffer& hCommandBuffer, uint32_t width, uint32_t height, uint32_t depth)
        {
            m_ICD.vkCmdDrawMeshTasksEXT( hCommandBuffer, width, height, depth );
        }

        void CDDI::BeginRenderPass( DDICommandBuffer hCommandBuffer, const SBeginRenderPassInfo2& Info )
        {
            Utils::TCDynamicArray<VkRenderingAttachmentInfoKHR, CRenderPass::MAX_RT_COUNT> vVkAttachments;

            VkRenderingInfoKHR vkInfo;
            vkInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
            vkInfo.pNext = nullptr;
            vkInfo.flags = 0;
            vkInfo.colorAttachmentCount = Info.vColorRenderTargetInfos.GetCount();
            Convert::RenderSystemToVkRect2D( Info.RenderArea, &vkInfo.renderArea );
            vkInfo.layerCount = Info.renderTargetLayerCount;
            vkInfo.viewMask = Info.renderTargetLayerIndex;

            VkRenderingAttachmentInfoKHR vkRTInfo;
            vkRTInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
            vkRTInfo.pNext = nullptr;

            for( uint32_t i = 0; i < Info.vColorRenderTargetInfos.GetCount(); ++i )
            {
                const auto& RTInfo = Info.vColorRenderTargetInfos[ i ];
                Convert::ClearValues( &RTInfo.ClearColor, 1, &vkRTInfo.clearValue );
                vkRTInfo.imageLayout = Map::ImageLayout( RTInfo.state );
                vkRTInfo.imageView = RTInfo.hDDIView;
                vkRTInfo.loadOp = Convert::UsageToLoadOp( RTInfo.renderPassOp );
                vkRTInfo.storeOp = Convert::UsageToStoreOp( RTInfo.renderPassOp );
                vkRTInfo.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                vkRTInfo.resolveImageView = VK_NULL_HANDLE;
                vkRTInfo.resolveMode = VK_RESOLVE_MODE_NONE;
                vVkAttachments.PushBack( vkRTInfo );
            }

            VkRenderingAttachmentInfoKHR vkDepthAttachment = { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR };
            VkRenderingAttachmentInfoKHR vkStencilAttachment = { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR };

            vkInfo.pDepthAttachment = nullptr;
            vkInfo.pStencilAttachment = nullptr;

            if( Info.DepthRenderTargetInfo.hDDIView != DDI_NULL_HANDLE )
            {
                const auto& RT = Info.DepthRenderTargetInfo;
                auto& vkAttachment = vkDepthAttachment;
                Convert::ClearValues( &RT.ClearColor, 1, &vkAttachment.clearValue );
                vkAttachment.imageLayout = Map::ImageLayout( RT.state );
                vkAttachment.imageView = RT.hDDIView;
                vkAttachment.loadOp = Convert::UsageToLoadOp( RT.renderPassOp );
                vkAttachment.storeOp = Convert::UsageToStoreOp( RT.renderPassOp );
                vkAttachment.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                vkAttachment.resolveImageView = VK_NULL_HANDLE;
                vkAttachment.resolveMode = VK_RESOLVE_MODE_NONE;

                vkInfo.pDepthAttachment = &vkAttachment;
            }

            if( Info.StencilRenderTargetInfo.hDDIView != DDI_NULL_HANDLE )
            {
                const auto& RT = Info.StencilRenderTargetInfo;
                auto& vkAttachment = vkStencilAttachment;
                Convert::ClearValues( &RT.ClearColor, 1, &vkAttachment.clearValue );
                vkAttachment.imageLayout = Map::ImageLayout( RT.state );
                vkAttachment.imageView = RT.hDDIView;
                vkAttachment.loadOp = Convert::UsageToLoadOp( RT.renderPassOp );
                vkAttachment.storeOp = Convert::UsageToStoreOp( RT.renderPassOp );
                vkAttachment.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                vkAttachment.resolveImageView = VK_NULL_HANDLE;
                vkAttachment.resolveMode = VK_RESOLVE_MODE_NONE;

                vkInfo.pStencilAttachment = &vkAttachment;
            }

            vkInfo.pColorAttachments = vVkAttachments.GetDataOrNull();

            m_ICD.vkCmdBeginRenderingKHR( hCommandBuffer, &vkInfo );
        }

        void CDDI::EndRenderPass(DDICommandBuffer hDDICommandBuffer)
        {
            m_ICD.vkCmdEndRenderingKHR( hDDICommandBuffer );
        }

        void CDDI::Copy( const DDICommandBuffer& hCmdBuffer, const SCopyBufferToTextureInfo& Info )
        {
            Utils::TCDynamicArray< VkBufferImageCopy > vRegions( Info.vRegions.GetCount() );
            for (uint32_t i = 0; i < vRegions.GetCount(); ++i)
            {
                const auto& Region = Info.vRegions[i];
                auto& VkRegion = vRegions[i];
                VkRegion.bufferImageHeight = Region.bufferTextureHeight;
                VkRegion.bufferRowLength = Region.bufferRowLength;
                VkRegion.bufferOffset = Region.bufferOffset;
                VkRegion.imageExtent = {Region.textureWidth, Region.textureHeight, Region.textureDepth};
                VkRegion.imageOffset = { (int32_t)Region.textureOffsetX, (int32_t)Region.textureOffsetY, (int32_t)Region.textureOffsetZ};
                VkRegion.imageSubresource.aspectMask = Map::ImageAspect(Region.TextureSubresource.aspect);
                VkRegion.imageSubresource.baseArrayLayer = Region.TextureSubresource.beginArrayLayer;
                VkRegion.imageSubresource.layerCount = Region.TextureSubresource.layerCount;
                VkRegion.imageSubresource.mipLevel = Region.TextureSubresource.beginMipmapLevel;
            }
            VkImageLayout vkLayout = Map::ImageLayout( Info.textureState );
            m_ICD.vkCmdCopyBufferToImage(hCmdBuffer, Info.hDDISrcBuffer, Info.hDDIDstTexture, vkLayout,
                vRegions.GetCount(), &vRegions[ 0 ] );
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

        void TextureSubresourceToNativeSubresource( const STextureSubresourceRange& Subres,
                                                    VkImageSubresourceLayers* pOut )
        {
            pOut->aspectMask = Map::ImageAspect( Subres.aspect );
            pOut->baseArrayLayer = Subres.beginArrayLayer;
            pOut->layerCount = Subres.layerCount;
            pOut->mipLevel = Subres.beginMipmapLevel;
        }

        void CDDI::Copy( const DDICommandBuffer& hDDICmdBuffer, const SCopyTextureInfoEx& Info )
        {
            VkImageLayout vkSrcLayout = Map::ImageLayout( Info.srcTextureState );
            VkImageLayout vkDstLayout = Map::ImageLayout( Info.dstTextureState );

            VkImageCopy VkCopy;

            VkCopy.extent = { Info.pBaseInfo->Size.width, Info.pBaseInfo->Size.height, Math::Max( 1u, Info.pBaseInfo->depth ) };
            VkCopy.srcOffset = { Info.pBaseInfo->SrcOffset.x, Info.pBaseInfo->SrcOffset.y };
            VkCopy.dstOffset = { Info.pBaseInfo->DstOffset.x, Info.pBaseInfo->DstOffset.y };

            TextureSubresourceToNativeSubresource( Info.DstSubresource, &VkCopy.dstSubresource );
            TextureSubresourceToNativeSubresource( Info.SrcSubresource, &VkCopy.srcSubresource );
            

            m_ICD.vkCmdCopyImage( hDDICmdBuffer, Info.pBaseInfo->hDDISrcTexture, vkSrcLayout,
                Info.pBaseInfo->hDDIDstTexture, vkDstLayout, 1, &VkCopy );
        }

        void CDDI::Blit( const DDICommandBuffer& hAPICmdBuffer, const SBlitTextureInfo& Info )
        {
            Utils::TCDynamicArray<VkImageBlit2KHR> vNativeRegions( Info.vRegions.GetCount() );
            for( uint32_t i = 0; i < Info.vRegions.GetCount(); ++i )
            {
                const auto& Region = Info.vRegions[ i ];
                auto& Native = vNativeRegions[ i ];
                Native.sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2_KHR;
                Native.pNext = nullptr;
                TextureSubresourceToNativeSubresource( Region.SrcSubresource, &Native.srcSubresource );
                TextureSubresourceToNativeSubresource( Region.DstSubresource, &Native.dstSubresource );
               
                for( uint32_t o = 0; o < 2; ++o )
                {
                    Native.srcOffsets[ o ].x = Region.srcOffsets[ o ].x;
                    Native.srcOffsets[ o ].y = Region.srcOffsets[ o ].y;
                    Native.srcOffsets[ o ].z = Region.srcOffsets[ o ].z;
                    Native.dstOffsets[ o ].x = Region.dstOffsets[ o ].x;
                    Native.dstOffsets[ o ].y = Region.dstOffsets[ o ].y;
                    Native.dstOffsets[ o ].z = Region.dstOffsets[ o ].z;
                }
            }

            VkBlitImageInfo2KHR NativeInfo =
            {
                .sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2_KHR,
                .pNext = nullptr,
                .srcImage = Info.hAPISrcTexture,
                .srcImageLayout = Map::ImageLayout(Info.srcTextureState),
                .dstImage = Info.hAPIDstTexture,
                .dstImageLayout = Map::ImageLayout(Info.dstTextureState),
                .regionCount = Info.vRegions.GetCount(),
                .pRegions = vNativeRegions.GetData(),
                .filter = Map::Filter( Info.filter )
            };

            m_ICD.vkCmdBlitImage2KHR( hAPICmdBuffer, &NativeInfo );
        }

        void CDDI::SetEvent( const DDIEvent& hDDIEvent )
        {
            m_ICD.vkSetEvent( m_hDevice, hDDIEvent );
        }

        void CDDI::SetEvent( const DDICommandBuffer& hDDICmdBuffer, const DDIEvent& hDDIEvent, const PIPELINE_STAGES& stages )
        {
            m_ICD.vkCmdSetEvent( hDDICmdBuffer, hDDIEvent, Convert::PipelineStages( stages ) );
        }

        void CDDI::Reset( const DDIEvent& hDDIInOut )
        {
            m_ICD.vkResetEvent( m_hDevice, hDDIInOut );
        }

        void CDDI::Reset( const DDICommandBuffer& hDDICmdBuffer, const DDIEvent& hDDIEvent, const PIPELINE_STAGES& stages )
        {
            m_ICD.vkCmdResetEvent( hDDICmdBuffer, hDDIEvent, Convert::PipelineStages( stages ) );
        }

        bool CDDI::IsSet( const DDIEvent& hDDIEvent )
        {
            VkResult res = m_ICD.vkGetEventStatus( m_hDevice, hDDIEvent );
            return res == VK_EVENT_SET;
        }

        Result CDDI::Submit( const SSubmitInfo& Info )
        {
            Result ret = VKE_FAIL;

            static VkPipelineStageFlags vkWaitMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            Utils::TCDynamicArray< VkPipelineStageFlags > vWaitMask( Info.waitSemaphoreCount, vkWaitMask );

            VkSubmitInfo si;
            si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            si.pNext = nullptr;
            si.pSignalSemaphores = Info.pDDISignalSemaphores;
            si.signalSemaphoreCount = Info.signalSemaphoreCount;
            si.pWaitSemaphores = Info.pDDIWaitSemaphores;
            si.waitSemaphoreCount = Info.waitSemaphoreCount;
            si.pWaitDstStageMask = vWaitMask.GetData();
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
            VKE_ASSERT2( ret != VKE_FAIL, "TDR" );
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

        Result CDDI::CreateSwapChain( const SSwapChainDesc& Desc, const void*, SDDISwapChain* pOut )
        {
            Result ret = VKE_FAIL;
            VkResult vkRes;
            DDIPresentSurface hSurface = pOut->hSurface;
            uint16_t elementCount = Desc.backBufferCount;
            VkSwapchainKHR hSwapChain = DDI_NULL_HANDLE;

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
                //pInternalAllocator = VKE_NEW Helper::SSwapChainAllocator;
                if( VKE_SUCCEEDED( Memory::CreateObject( &HeapAllocator, &pInternalAllocator ) ) )
                {
                    if( VKE_SUCCEEDED( pInternalAllocator->Create( VKE_MEGABYTES( 1 ), 2 ) ) )
                    {
                        pOut->pInternalAllocator = pInternalAllocator;
                    }
                    else
                    {
                        VKE_LOG_ERR( "Unable to create CSwapChain internal allocator." );
                        goto ERR;
                    }
                }
                else
                {
                    VKE_LOG_ERR( "Unable to create memory for CSwapChain internal allocator." );
                    goto ERR;
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
            {
                SPresentSurfaceCaps& Caps = pOut->Caps;
                ret = QueryPresentSurfaceCaps( hSurface, &Caps );
                Size = Caps.CurrentSize;
                if( !Caps.canBeUsedAsRenderTarget )
                {
                    VKE_LOG_ERR( "Created present surface can't be used as render target." );
                    goto ERR;
                }
                bool found = false;
                for( auto& format: Caps.vFormats )
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
                    VKE_LOG_ERR( "Requested format: " << Desc.format << " / " << Desc.colorSpace
                                                      << " is not supported for present surface." );
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
                    elementCount = std::min<uint16_t>( static_cast<uint16_t>( Caps.minImageCount ), 2u );
                }
                else
                {
                    elementCount = std::min<uint16_t>( elementCount, static_cast<uint16_t>( Caps.maxImageCount ) );
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

            {
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
                    ci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
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
                        if( imgCount <= Desc.backBufferCount )
                        {
                            pOut->vImages.Resize( imgCount );
                            pOut->vImageViews.Resize( imgCount );
                            pOut->vFramebuffers.Resize( imgCount );
                            res =
                                m_ICD.vkGetSwapchainImagesKHR( m_hDevice, hSwapChain, &imgCount, &pOut->vImages[ 0 ] );
                            VK_ERR( res );
                            if( res == VK_SUCCESS )
                            {
                                Utils::TCDynamicArray<VkImageMemoryBarrier> vVkBarriers;
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
                                    // AtDesc.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                                    // AtDesc.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                                    AtDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                                    AtDesc.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
                                    AtDesc.format = SwapChainCI.imageFormat;
                                    AtDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                                    AtDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                                    AtDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                                    AtDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                                    AtDesc.samples = VK_SAMPLE_COUNT_1_BIT;
                                    if( pOut->hDDIRenderPass == DDI_NULL_HANDLE )
                                    {
                                        VkRenderPassCreateInfo ci = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
                                        ci.flags = 0;
                                        ci.attachmentCount = 1;
                                        ci.pAttachments = &AtDesc;
                                        ci.pDependencies = nullptr;
                                        ci.pSubpasses = &SubPassDesc;
                                        ci.subpassCount = 1;
                                        ci.dependencyCount = 0;
                                        res = m_ICD.vkCreateRenderPass( m_hDevice, &ci, pVkCallbacks,
                                                                        &pOut->hDDIRenderPass );
                                        VK_ERR( res );
                                        if( res != VK_SUCCESS )
                                        {
                                            VKE_LOG_ERR( "Unable to create SwapChain RenderPass" );
                                            goto ERR;
                                        }
                                        _CreateDebugInfo<VK_OBJECT_TYPE_RENDER_PASS>( pOut->hDDIRenderPass,
                                                                                      "Swapchain RenderPass" );
                                    }
                                }
                                for( uint32_t i = 0; i < imgCount; ++i )
                                {
                                    VkImageViewCreateInfo ci;
                                    ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                                    ci.pNext = nullptr;
                                    ci.flags = 0;
                                    ci.format = SwapChainCI.imageFormat;
                                    ci.image = pOut->vImages[ i ];
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
                                    pOut->vImageViews[ i ] = hView;
                                    // Do a barrier for image
                                    {
                                        VkImageMemoryBarrier vkBarrier;
                                        vkBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                                        vkBarrier.pNext = nullptr;
                                        vkBarrier.image = pOut->vImages[ i ];
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
                                        VkFramebufferCreateInfo fbci = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
                                        fbci.attachmentCount = 1;
                                        fbci.pAttachments = &pOut->vImageViews[ i ];
                                        fbci.width = Size.width;
                                        fbci.height = Size.height;
                                        fbci.renderPass = pOut->hDDIRenderPass;
                                        fbci.layers = 1;
                                        res = m_ICD.vkCreateFramebuffer( m_hDevice, &fbci, pVkCallbacks,
                                                                         &pOut->vFramebuffers[ i ] );
                                        VK_ERR( res );
                                        if( res != VK_SUCCESS )
                                        {
                                            VKE_LOG_ERR( "Unable to create Framebuffer for SwapChain" );
                                            goto ERR;
                                        }
                                        _CreateDebugInfo<VK_OBJECT_TYPE_IMAGE>( pOut->vImages[ i ], "Swapchain Image" );
                                        _CreateDebugInfo<VK_OBJECT_TYPE_IMAGE_VIEW>( pOut->vImageViews[ i ],
                                                                                     "Swapchain ImageView" );
                                        _CreateDebugInfo<VK_OBJECT_TYPE_FRAMEBUFFER>( pOut->vFramebuffers[ i ],
                                                                                      "Swapchain Framebuffer" );
                                    }
                                    {
                                        /*STextureBarrierInfo Info;
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
                                        Desc.pCtx->GetCommandBuffer()->Barrier( Info );*/
                                    }
                                }
                                {
                                    // Change image layout UNDEFINED -> PRESENT
                                    //VKE_ASSERT2( Desc.pCtx != nullptr, "GraphicsContext must be set." );
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
            }
            pOut->hSwapChain = hSwapChain;

            ret = VKE_OK;
            return ret;

        ERR:
            for( uint32_t i = 0; i < pOut->vImageViews.GetCount(); ++i )
            {
                DestroyTextureView( &pOut->vImageViews[i], pVkCallbacks );
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

            //Desc.pCtx->GetCommandBuffer()->ExecuteBarriers();

            for( uint32_t i = 0; i < pOut->vImageViews.GetCount(); ++i )
            {
                DestroyTextureView( &pOut->vImageViews[i], pVkAllocator );
                DestroyFramebuffer( &pOut->vFramebuffers[i], pVkAllocator );
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
            if( pOut->hDDIRenderPass != DDI_NULL_HANDLE )
            {
                m_ICD.vkDestroyRenderPass( m_hDevice, pOut->hDDIRenderPass, pVkAllocator );
                pOut->hDDIRenderPass = DDI_NULL_HANDLE;
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

                pOut->MinSize.width = static_cast< uint16_t >( vkSurfaceCaps.minImageExtent.width );
                pOut->MinSize.height = static_cast< uint16_t >( vkSurfaceCaps.minImageExtent.height );
                pOut->MaxSize.width = static_cast< uint16_t >( vkSurfaceCaps.maxImageExtent.width );
                pOut->MaxSize.height = static_cast< uint16_t >( vkSurfaceCaps.maxImageExtent.height );
                pOut->CurrentSize.width = static_cast< uint16_t >( vkSurfaceCaps.currentExtent.width );
                pOut->CurrentSize.height = static_cast< uint16_t >( vkSurfaceCaps.currentExtent.height );
                pOut->minImageCount = static_cast< uint16_t >( vkSurfaceCaps.minImageCount );
                pOut->maxImageCount = static_cast< uint16_t >( vkSurfaceCaps.maxImageCount );
                pOut->canBeUsedAsRenderTarget = hasColorAttachment;
                //pOut->transform = vkSurfaceCaps.currentTransform
            }
            return ret;
        }

        void CDDI::DestroySwapChain( SDDISwapChain* pInOut, const void* )
        {
            Helper::SSwapChainAllocator* pInternalAllocator = reinterpret_cast<Helper::SSwapChainAllocator*>(pInOut->pInternalAllocator);
            const VkAllocationCallbacks* pVkAllocator = &pInternalAllocator->VkCallbacks;
            for( uint32_t i = 0; i < pInOut->vImageViews.GetCount(); ++i )
            {
                DestroyTextureView( &pInOut->vImageViews[i], pVkAllocator );
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
            if( pInternalAllocator != nullptr )
            {
                pInternalAllocator->Destroy();
                Memory::DestroyObject( &HeapAllocator, &pInternalAllocator );
                pInOut->pInternalAllocator = nullptr;
            }
        }

        Result CDDI::GetCurrentBackBufferIndex( const SDDISwapChain& SwapChain, const SDDIGetBackBufferInfo& Info,
            uint32_t* pOut )
        {
            Result ret = VKE_FAIL;

            VkResult res = m_ICD.vkAcquireNextImageKHR( m_hDevice, SwapChain.hSwapChain, Info.waitTimeout,
                Info.hSignalGPUFence, Info.hSignalCPUFence, pOut );
            switch( res )
            {
                case VK_SUCCESS:
                {
                    ret = VKE_OK;
                }
                break;
                case VK_TIMEOUT:
                {
                    ret = VKE_TIMEOUT;
                    break;
                }
                case VK_NOT_READY:
                case VK_SUBOPTIMAL_KHR:
                {
                    ret = VKE_ENOTREADY;
                    break;
                }
                case VK_ERROR_VALIDATION_FAILED_EXT:
                {
                    
                    VKE_LOG( res );
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
            VKE_ASSERT2( Info.pCmdBuffer != nullptr && Info.pCmdBuffer->GetDDIObject() != DDI_NULL_HANDLE &&
                Info.pPipeline != nullptr && Info.pPipeline->GetDDIObject() != DDI_NULL_HANDLE,
                "Invalid parameter");
            m_ICD.vkCmdBindPipeline( Info.pCmdBuffer->GetDDIObject(),
                Convert::PipelineTypeToBindPoint( Info.pPipeline->GetType() ), Info.pPipeline->GetDDIObject() );
        }

        void CDDI::UnbindPipeline( const DDICommandBuffer&, const DDIPipeline& )
        {

        }

        void CDDI::Bind( const SBindRenderPassInfo& Info )
        {
            VKE_ASSERT2( Info.pBeginInfo != nullptr, "" );
            {
                VkRenderPassBeginInfo bi;
                bi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                bi.pNext = nullptr;
                bi.clearValueCount = Info.pBeginInfo->vDDIClearValues.GetCount();
                bi.pClearValues = Info.pBeginInfo->vDDIClearValues.GetData();
                bi.renderArea.extent.width = Info.pBeginInfo->RenderArea.Size.width;
                bi.renderArea.extent.height = Info.pBeginInfo->RenderArea.Size.height;
                bi.renderArea.offset = { Info.pBeginInfo->RenderArea.Position.x, Info.pBeginInfo->RenderArea.Position.y };

                //bi.renderPass = reinterpret_cast<DDIRenderPass>(Info.hPass.handle);
                //bi.framebuffer = reinterpret_cast<DDIFramebuffer>(Info.hFramebuffer.handle);
                bi.renderPass = Info.pBeginInfo->hDDIRenderPass;
                bi.framebuffer = Info.pBeginInfo->hDDIFramebuffer;
                m_ICD.vkCmdBeginRenderPass( Info.hDDICommandBuffer, &bi, VK_SUBPASS_CONTENTS_INLINE );
            }
        }

        void CDDI::UnbindRenderPass( const DDICommandBuffer& hCb, const DDIRenderPass& )
        {
            m_ICD.vkCmdEndRenderPass( hCb );
        }

        void CDDI::Bind( const SBindDDIDescriptorSetsInfo& Info )
        {
            m_ICD.vkCmdBindDescriptorSets( Info.hDDICommandBuffer,
                Convert::PipelineTypeToBindPoint( Info.pipelineType ), Info.hDDIPipelineLayout,
                Info.firstSet, Info.setCount, Info.aDDISetHandles, Info.dynamicOffsetCount,
                Info.aDynamicOffsets );
        }

        void CDDI::Bind( const DDICommandBuffer& hDDICmdBuffer, const DDIBuffer& hDDIBuffer, const uint32_t offset )
        {
            VkDeviceSize ddiOffset = offset;
            m_ICD.vkCmdBindVertexBuffers( hDDICmdBuffer, 0, 1, &hDDIBuffer, &ddiOffset );
        }

        void CDDI::Bind( const DDICommandBuffer& hDDICmdBuffer, const DDIBuffer& hDDIBuffer, const uint32_t offset,
            const INDEX_TYPE& type )
        {
            m_ICD.vkCmdBindIndexBuffer( hDDICmdBuffer, hDDIBuffer, offset, Map::IndexType( type ) );
        }

        void CDDI::SetState( const DDICommandBuffer& hCommandBuffer, const SViewportDesc& Desc )
        {
            VkViewport Viewport;
            Viewport.width = Desc.Size.width;
            Viewport.x = Desc.Position.x;
#if VKE_VULKAN_NEGATIVE_VIEWPORT_HEIGT
            Viewport.y = Desc.Size.height + Desc.Position.y;
            Viewport.height = -Viewport.y;
#else
            Viewport.height = Desc.Size.height;
            Viewport.y = Desc.Position.y;
#endif
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
            Utils::TCDynamicArray< VkImageMemoryBarrier, SBarrierInfo::MAX_BARRIER_COUNT > vVkImgBarriers/*( Info.vTextureBarriers.GetCount() )*/;
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
                        //vVkImgBarriers[i] = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
                        vVkImgBarriers.PushBack( { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER } );
                        auto& VkBarrier = vVkImgBarriers.Back();
                        Convert::Barrier( &VkBarrier, Barriers[ i ] );
                        VkBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                        VkBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
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
                        //const VkAccessFlags flags = vVkBufferBarriers[i].dstAccessMask;
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


        void CDDI::Convert( const SClearValue& In, DDIClearValue* pOut )
        {
            Memory::Copy( pOut, sizeof( DDIClearValue ), &In, sizeof( SClearValue ) );
        }

        void CDDI::BeginDebugInfo( const DDICommandBuffer& hDDICmdBuff, const SDebugInfo* pInfo )
        {
            if( sInstanceICD.vkCmdBeginDebugUtilsLabelEXT && pInfo )
            {
                VkDebugUtilsLabelEXT li = {};
                li.color[ 0 ] = pInfo->Color.r;
                li.color[ 1 ] = pInfo->Color.g;
                li.color[ 2 ] = pInfo->Color.b;
                li.color[ 3 ] = pInfo->Color.a;
                li.pLabelName = pInfo->pText;
                li.pNext = nullptr;
                li.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
                sInstanceICD.vkCmdBeginDebugUtilsLabelEXT( hDDICmdBuff, &li );
            }
        }

        void CDDI::EndDebugInfo( const DDICommandBuffer& hDDICmdBuff )
        {
            if( sInstanceICD.vkCmdEndDebugUtilsLabelEXT )
            {
                sInstanceICD.vkCmdEndDebugUtilsLabelEXT( hDDICmdBuff );
            }
        }

        void CDDI::SetObjectDebugName( const uint64_t& handle, const uint32_t& objType, cstr_t pName ) const
        {
#if VKE_RENDER_SYSTEM_DEBUG
            if( sInstanceICD.vkSetDebugUtilsObjectNameEXT && pName )
            {
                VKE_ASSERT2( strlen( pName ) > 0, "VKE_RENDER_SYSTEM_DEBUG requires debug names for all objects." );
                VKE_ASSERT2( m_hDevice != DDI_NULL_HANDLE, "Device must be created first!" );
                VkDebugUtilsObjectNameInfoEXT ni;
                ni.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
                ni.pNext = nullptr;
                ni.objectHandle = handle;
                ni.objectType = ( VkObjectType )objType;
                ni.pObjectName = pName;
                sInstanceICD.vkSetDebugUtilsObjectNameEXT( m_hDevice, &ni );
            }
#endif
        }

        void CDDI::SetQueueDebugName( uint64_t handle, cstr_t pName ) const
        {
            SetObjectDebugName( handle, VK_OBJECT_TYPE_QUEUE, pName );
        }

        VKAPI_ATTR VkBool32 VKAPI_CALL VkDebugCallback( VkDebugReportFlagsEXT msgFlags,
                                                        VkDebugReportObjectTypeEXT objType, uint64_t srcObject,
                                                        size_t location, int32_t msgCode, const char* pLayerPrefix,
                                                        const char* pMsg, void* )
        {
            std::ostringstream message;
            ( void )location;
            ( void )srcObject;
            ( void )objType;
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
            auto str = std::regex_replace( message.str(), std::regex( " : " ), "\n" );
            str = std::regex_replace( str, std::regex( ";" ), "\n" );
            VKE_LOG( str );
            VKE_ASSERT2( (msgFlags & VK_DEBUG_REPORT_ERROR_BIT_EXT) == 0, message.str().c_str() );
#ifdef _WIN32
            if( msgFlags == VK_DEBUG_REPORT_ERROR_BIT_EXT )
            {
                MessageBox( NULL, message.str().c_str(), "VULKAN API ERROR", MB_OK | MB_ICONERROR );
            }
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

        VKAPI_ATTR VkBool32 VKAPI_CALL VkDebugMessengerCallback(
            VkDebugUtilsMessageSeverityFlagBitsEXT           messageSeverity,
            VkDebugUtilsMessageTypeFlagsEXT                  messageTypes,
            const VkDebugUtilsMessengerCallbackDataEXT*      pCallbackData,
            void*                                            /*pUserData*/ )
        {
#if VKE_LOG_RENDER_API_ERRORS
            ( void )messageTypes;
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
            VKE_ASSERT2( messageSeverity != VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
                pCallbackData->pMessageIdName );
#endif
            return VK_FALSE;
        }

    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDER_SYSTEM