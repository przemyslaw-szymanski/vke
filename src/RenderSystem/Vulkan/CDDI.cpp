#include "RenderSystem/CDDI.h"

#if VKE_RENDER_SYSTEM_VULKAN

#include "Core/Managers/CFileManager.h"
#include "Core/Platform/CWindow.h"
#include "RenderSystem/CContextBase.h"
#include "RenderSystem/CDeviceContext.h"
#include "RenderSystem/CGraphicsContext.h"
#include "RenderSystem/Resources/CBuffer.h"
#include "RenderSystem/Resources/CTexture.h"

#include "CCommandLineArgs.h"
#include <glslang/SPIRV/GlslangToSpv.h>
#include <glslang/Public/ShaderLang.h>

namespace VKE
{

#define DDI_CREATE_OBJECT( _name, _CreateInfo, _pAllocator, _phObj )                                                   \
    m_Implementation.m_ICD.vkCreate##_name(                                                                            \
        m_hDevice, &( _CreateInfo ), static_cast< const VkAllocationCallbacks* >( _pAllocator ), ( _phObj ) );

#define DDI_DESTROY_OBJECT( _name, _phObj, _pAllocator )                                                               \
    if( ( _phObj ) && ( *_phObj ) != NativeAPI::Null )                                                                 \
    {                                                                                                                  \
        m_Implementation.m_ICD.vkDestroy##_name(                                                                       \
            m_hDevice, ( *_phObj ), static_cast< const VkAllocationCallbacks* >( _pAllocator ) );                      \
        ( *_phObj ) = NativeAPI::Null;                                                                                 \
    }

    namespace RenderSystem
    {
        template< VkObjectType ObjectType, typename DDIObjectT >
        VkResult _CreateDebugInfo( CDDI* rhi, const DDIObjectT& hDDIObject, cstr_t pName )
        {
            VkResult ret = VK_SUCCESS;
#if VKE_RENDER_SYSTEM_DEBUG
            if( NativeAPI::SImplementation::sInstanceICD.vkSetDebugUtilsObjectNameEXT )
            {
                VkDebugUtilsObjectNameInfoEXT ni = { VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT };
                ni.objectHandle                  = (uint64_t)( hDDIObject );
                ni.objectType                    = ObjectType;
                ni.pObjectName                   = pName;
                ret = NativeAPI::SImplementation::sInstanceICD.vkSetDebugUtilsObjectNameEXT( rhi->GetDevice(), &ni );
            }
#endif // VKE_RENDER_SYSTEM_DEBUG
            VK_ERR( ret );
            return ret;
        }

        VkICD::Global            NativeAPI::SImplementation::sGlobalICD;
        VkICD::Instance          NativeAPI::SImplementation::sInstanceICD;
        handle_t                 NativeAPI::SImplementation::shICD                     = 0;
        VkInstance               NativeAPI::SImplementation::sVkInstance               = VK_NULL_HANDLE;
        VkDebugReportCallbackEXT NativeAPI::SImplementation::sVkDebugReportCallback    = VK_NULL_HANDLE;
        VkDebugUtilsMessengerEXT NativeAPI::SImplementation::sVkDebugMessengerCallback = VK_NULL_HANDLE;
        CDDI::AdapterArray       CDDI::svAdapters;

        VKAPI_ATTR VkBool32 VKAPI_CALL VkDebugCallback( VkDebugReportFlagsEXT      msgFlags,
                                                        VkDebugReportObjectTypeEXT objType, uint64_t srcObject,
                                                        size_t location, int32_t msgCode, const char* pLayerPrefix,
                                                        const char* pMsg, void* pUserData );

        VKAPI_ATTR VkBool32 VKAPI_CALL VkDebugMessengerCallback(
            VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes,
            const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData );

        NativeAPI::DDIExtArray GetRequiredInstanceExtensions( bool debug )
        {
            NativeAPI::DDIExtArray Ret = {
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

        const NativeAPI::DDIExtArray GetRequiredDeviceExtensions( bool debug )
        {
            const NativeAPI::DDIExtArray Ret = {
                // name, required, supported
                { VK_KHR_SWAPCHAIN_EXTENSION_NAME, true, false },
                { VK_KHR_MAINTENANCE1_EXTENSION_NAME, true, false },
                { VK_KHR_MAINTENANCE2_EXTENSION_NAME, true, false },
                { VK_KHR_MAINTENANCE3_EXTENSION_NAME, true, false },
                { VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME, true, false },
                { VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME, true, false },
                { VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME, true, false },
                { VK_KHR_COPY_COMMANDS_2_EXTENSION_NAME, true, false },
            };
            return Ret;
        }

        namespace Map
        {
            Result NativeResult( VkResult native )
            {
                Result ret = VKE_FAIL;
                switch( native )
                {
                    case VK_SUCCESS:
                        ret = VKE_OK;
                        break;
                    case VK_NOT_READY:
                        ret = VKE_ENOTREADY;
                        break;
                    case VK_TIMEOUT:
                        ret = VKE_TIMEOUT;
                        break;
                    case VK_ERROR_OUT_OF_HOST_MEMORY:
                    case VK_ERROR_OUT_OF_DEVICE_MEMORY:
                    case VK_ERROR_OUT_OF_POOL_MEMORY:
                        ret = Results::NO_MEMORY;
                        break;
                    case VK_ERROR_DEVICE_LOST:
                        ret = Results::DEVICE_LOST;
                        break;
                }
                return ret;
            }

            VkFormat Format( uint32_t format )
            {
                return VKE::RenderSystem::g_aFormats[ format ];
            }

            auto Formats( const FORMAT* pFormats, uint32_t count )
            {
                Utils::TCDynamicArray< VkFormat > vRet;
                for( uint32_t i = 0; i < count; ++i )
                {
                    vRet.PushBack( Format( pFormats[ i ] ) );
                }
                return vRet;
            }

            VkImageType ImageType( RenderSystem::TEXTURE_TYPE type )
            {
                static const VkImageType aVkImageTypes[] = {
                    VK_IMAGE_TYPE_1D,
                    VK_IMAGE_TYPE_2D,
                    VK_IMAGE_TYPE_3D,

                    // TODO(szymansk): From vulkan.cpp -> keep or remove?
                    VK_IMAGE_TYPE_2D, // cube
                };
                return aVkImageTypes[ type ];
            }

            VkImageViewType ImageViewType( RenderSystem::TEXTURE_VIEW_TYPE type )
            {
                static const VkImageViewType aVkTypes[] = {
                    VK_IMAGE_VIEW_TYPE_1D,         VK_IMAGE_VIEW_TYPE_2D,       VK_IMAGE_VIEW_TYPE_3D,
                    VK_IMAGE_VIEW_TYPE_1D_ARRAY,   VK_IMAGE_VIEW_TYPE_2D_ARRAY, VK_IMAGE_VIEW_TYPE_CUBE,
                    VK_IMAGE_VIEW_TYPE_CUBE_ARRAY,
                };
                return aVkTypes[ type ];
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
                    flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT |
                             VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
                }
                else if( usage & TextureUsages::DEPTH_STENCIL_RENDER_TARGET )
                {
                    flags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT |
                             VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
                }
                if( usage & TextureUsages::STORAGE )
                {
                    flags |=
                        VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
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
                static const VkImageLayout aVkLayouts[ TextureStates::_MAX_COUNT ] = {
                    VK_IMAGE_LAYOUT_UNDEFINED,                        // undefined
                    VK_IMAGE_LAYOUT_GENERAL,                          // general
                    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,         // color rt
                    VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,         // depth rt
                    VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL,       // stencil rt
                    VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, // depth stencil rt
                    VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL,          // depth buffer
                    VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL,        // stencil buffer
                    VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,  // deptn stencil buffer
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
                };
                return aVkLayouts[ layout ];
            }

            VkImageAspectFlags ImageAspect( RenderSystem::TEXTURE_ASPECT aspect )
            {
                VKE_ASSERT( aspect != 0 );
                static const VkImageAspectFlags aVkAspects[] = {
                    // UNKNOWN
                    0,
                    // COLOR
                    VK_IMAGE_ASPECT_COLOR_BIT,
                    // DEPTH
                    VK_IMAGE_ASPECT_DEPTH_BIT,
                    // STENCIL
                    VK_IMAGE_ASPECT_STENCIL_BIT,
                    // DEPTH_STENCIL
                    VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT,
                };
                return aVkAspects[ aspect ];
            }

            VkFilter Filter( RenderSystem::TEXTURE_FILTER filter )
            {
                static const VkFilter aNativeFilters[ RenderSystem::TextureFilters::_MAX_COUNT ] = {
                    VK_FILTER_NEAREST, VK_FILTER_LINEAR, VK_FILTER_CUBIC_IMG
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
                static const VkBlendOp aVkOps[] = { VK_BLEND_OP_ADD,
                                                    VK_BLEND_OP_SUBTRACT,
                                                    VK_BLEND_OP_REVERSE_SUBTRACT,
                                                    VK_BLEND_OP_MIN,
                                                    VK_BLEND_OP_MAX };
                return aVkOps[ op ];
            }

            VkColorComponentFlags ColorComponent( const RenderSystem::ColorComponent& component )
            {
                VkColorComponentFlags vkComponent = 0;
                if( component & RenderSystem::ColorComponents::ALL )
                {
                    vkComponent = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
                                  VK_COLOR_COMPONENT_A_BIT;
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
                static const VkBlendFactor aVkFactors[] = { VK_BLEND_FACTOR_ZERO,
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
                                                            VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA };
                return aVkFactors[ factor ];
            }

            VkLogicOp LogicOperation( const RenderSystem::LOGIC_OPERATION& op )
            {
                static const VkLogicOp aVkOps[] = {
                    VK_LOGIC_OP_CLEAR,         VK_LOGIC_OP_AND,         VK_LOGIC_OP_AND_REVERSE, VK_LOGIC_OP_COPY,
                    VK_LOGIC_OP_AND_INVERTED,  VK_LOGIC_OP_NO_OP,       VK_LOGIC_OP_XOR,         VK_LOGIC_OP_OR,
                    VK_LOGIC_OP_NOR,           VK_LOGIC_OP_EQUIVALENT,  VK_LOGIC_OP_INVERT,      VK_LOGIC_OP_OR_REVERSE,
                    VK_LOGIC_OP_COPY_INVERTED, VK_LOGIC_OP_OR_INVERTED, VK_LOGIC_OP_NAND,        VK_LOGIC_OP_SET
                };
                return aVkOps[ op ];
            }

            VkStencilOp StencilOperation( const RenderSystem::STENCIL_FUNCTION& op )
            {
                static const VkStencilOp aVkOps[] = { VK_STENCIL_OP_KEEP,
                                                      VK_STENCIL_OP_ZERO,
                                                      VK_STENCIL_OP_REPLACE,
                                                      VK_STENCIL_OP_INCREMENT_AND_CLAMP,
                                                      VK_STENCIL_OP_DECREMENT_AND_CLAMP,
                                                      VK_STENCIL_OP_INVERT,
                                                      VK_STENCIL_OP_INCREMENT_AND_WRAP,
                                                      VK_STENCIL_OP_DECREMENT_AND_WRAP };
                return aVkOps[ op ];
            }

            VkCompareOp CompareOperation( const RenderSystem::COMPARE_FUNCTION& op )
            {
                static const VkCompareOp aVkOps[] = { VK_COMPARE_OP_NEVER,
                                                      VK_COMPARE_OP_LESS,
                                                      VK_COMPARE_OP_EQUAL,
                                                      VK_COMPARE_OP_LESS_OR_EQUAL,
                                                      VK_COMPARE_OP_GREATER,
                                                      VK_COMPARE_OP_NOT_EQUAL,
                                                      VK_COMPARE_OP_GREATER_OR_EQUAL,
                                                      VK_COMPARE_OP_ALWAYS };
                return aVkOps[ op ];
            }

            VkPrimitiveTopology PrimitiveTopology( const RenderSystem::PRIMITIVE_TOPOLOGY& topology )
            {
                static const VkPrimitiveTopology aVkTopologies[] = {
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
                return aVkTopologies[ topology ];
            }

            VkSampleCountFlagBits SampleCount( const RenderSystem::SAMPLE_COUNT& count )
            {
                static const VkSampleCountFlagBits aVkSamples[] = { VK_SAMPLE_COUNT_1_BIT,  VK_SAMPLE_COUNT_2_BIT,
                                                                    VK_SAMPLE_COUNT_4_BIT,  VK_SAMPLE_COUNT_8_BIT,
                                                                    VK_SAMPLE_COUNT_16_BIT, VK_SAMPLE_COUNT_32_BIT,
                                                                    VK_SAMPLE_COUNT_64_BIT };
                return aVkSamples[ count ];
            }

            VkCullModeFlags CullMode( const RenderSystem::CULL_MODE& mode )
            {
                static const VkCullModeFlagBits aVkModes[] = {
                    VK_CULL_MODE_NONE, VK_CULL_MODE_FRONT_BIT, VK_CULL_MODE_BACK_BIT, VK_CULL_MODE_FRONT_AND_BACK
                };
                return aVkModes[ mode ];
            }

            VkFrontFace FrontFace( const RenderSystem::FRONT_FACE& face )
            {
                static const VkFrontFace aVkFaces[] = { VK_FRONT_FACE_CLOCKWISE, VK_FRONT_FACE_COUNTER_CLOCKWISE };
                return aVkFaces[ face ];
            }

            VkPolygonMode PolygonMode( const RenderSystem::POLYGON_MODE& mode )
            {
                static const VkPolygonMode aVkModes[] = { VK_POLYGON_MODE_FILL,
                                                          VK_POLYGON_MODE_LINE,
                                                          VK_POLYGON_MODE_POINT };
                return aVkModes[ mode ];
            }

            VkShaderStageFlagBits ShaderStage( const RenderSystem::SHADER_TYPE& type )
            {
                static const VkShaderStageFlagBits aVkBits[ RenderSystem::ShaderTypes::_MAX_COUNT ] = {
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
                return aVkBits[ type ];
            }

            VkVertexInputRate InputRate( const RenderSystem::VERTEX_INPUT_RATE& rate )
            {
                static const VkVertexInputRate aVkRates[] = { VK_VERTEX_INPUT_RATE_VERTEX,
                                                              VK_VERTEX_INPUT_RATE_INSTANCE };
                return aVkRates[ rate ];
            }

            VkDescriptorType DescriptorType( const RenderSystem::DESCRIPTOR_SET_TYPE& type )
            {
                /*
                struct BindingTypes
                {
                    enum TYPE : uint8_t
                    {
                        SAMPLER,             // only sampler
                        TEXTURE,             // only texture without sampler
                        STORAGE_TEXTURE,
                        READ_ONLY_TEXEL_BUFFER,
                        READ_WRITE_TEXEL_BUFFER,
                        CONSTANT_BUFFER,
                        BUFFER,
                        DYNAMIC_CONSTANT_BUFFER,
                        DYNAMIC_BUFFER,
                        RENDER_TARGET,
                        DEPTH_STENCIL,
                        _MAX_COUNT,
                        UNKNOWN = _MAX_COUNT
                    };
                };
                */
                static const VkDescriptorType aVkDescriptorType[BindingTypes::_MAX_COUNT] =
                {
                    VK_DESCRIPTOR_TYPE_SAMPLER,
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
                return aVkDescriptorType[ type ];
            }

            VkIndexType IndexType( const INDEX_TYPE& type )
            {
                static const VkIndexType aVkTypes[] = { VK_INDEX_TYPE_UINT16, VK_INDEX_TYPE_UINT32 };
                return aVkTypes[ type ];
            }

            VkCommandBufferLevel CommandBufferLevel( const RenderSystem::COMMAND_BUFFER_LEVEL& level )
            {
                static const VkCommandBufferLevel aVkLevels[] = { VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                                                                  VK_COMMAND_BUFFER_LEVEL_SECONDARY };
                return aVkLevels[ level ];
            }

            VkSamplerAddressMode AddressMode( const ADDRESS_MODE& mode )
            {
                static const VkSamplerAddressMode aModes[] = { VK_SAMPLER_ADDRESS_MODE_REPEAT,
                                                               VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
                                                               VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                                                               VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
                                                               VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE };
                return aModes[ mode ];
            }

            VkSamplerMipmapMode MipmapMode( const MIPMAP_MODE& mode )
            {
                static const VkSamplerMipmapMode aModes[] = { VK_SAMPLER_MIPMAP_MODE_LINEAR,
                                                              VK_SAMPLER_MIPMAP_MODE_NEAREST };
                return aModes[ mode ];
            }

            VkTessellationDomainOrigin TessellationDomainOrigin( RenderSystem::TESSELLATION_DOMAIN_ORIGIN origin )
            {
                static const VkTessellationDomainOrigin saValues[ TessellationDomainOrigins::_MAX_COUNT ] = {
                    VK_TESSELLATION_DOMAIN_ORIGIN_UPPER_LEFT, VK_TESSELLATION_DOMAIN_ORIGIN_LOWER_LEFT
                };
                return saValues[ origin ];
            }

            MEMORY_HEAP_TYPE VkMemPropertyFlagsToHeapType( VkMemoryPropertyFlags propertyFlags )
            {
                if( ( propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT ) == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT )
                {
                    if( ( propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT ) == VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT )
                    {
                        return MemoryHeapTypes::UPLOAD;
                    }
                    return MemoryHeapTypes::GPU;
                }
                if( ( propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT ) == VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT )
                {
                    return MemoryHeapTypes::CPU;
                }
                return MemoryHeapTypes::OTHER;
            }

        } // namespace Map

        namespace Convert
        {
            VkBorderColor BorderColor( const BORDER_COLOR& color )
            {
                static const VkBorderColor aColors[] = {
                    VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK, VK_BORDER_COLOR_INT_TRANSPARENT_BLACK,
                    VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK,      VK_BORDER_COLOR_INT_OPAQUE_BLACK,
                    VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,      VK_BORDER_COLOR_INT_OPAQUE_WHITE
                };
                return aColors[ color ];
            }

            VkFilter Filter( const SAMPLER_FILTER& filter )
            {
                static const VkFilter aFilters[] = { VK_FILTER_NEAREST, VK_FILTER_LINEAR, VK_FILTER_CUBIC_IMG };
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
                static const VkImageViewType aTypes[] = { VK_IMAGE_VIEW_TYPE_1D,
                                                          VK_IMAGE_VIEW_TYPE_2D,
                                                          VK_IMAGE_VIEW_TYPE_3D };
                assert( type <= VK_IMAGE_TYPE_3D && "Invalid image type for image view type" );
                return aTypes[ type ];
            }

            VkAttachmentLoadOp UsageToLoadOp( RenderSystem::RENDER_TARGET_RENDER_PASS_OP usage )
            {
                static const VkAttachmentLoadOp aLoads[] = {
                    VK_ATTACHMENT_LOAD_OP_DONT_CARE, // undefined
                    VK_ATTACHMENT_LOAD_OP_LOAD,      // color
                    VK_ATTACHMENT_LOAD_OP_CLEAR,     // color clear
                    VK_ATTACHMENT_LOAD_OP_LOAD,      // color store
                    VK_ATTACHMENT_LOAD_OP_CLEAR,     // color clear store
                    VK_ATTACHMENT_LOAD_OP_LOAD,      // depth
                    VK_ATTACHMENT_LOAD_OP_CLEAR,     // depth clear
                    VK_ATTACHMENT_LOAD_OP_LOAD,      // depth store
                    VK_ATTACHMENT_LOAD_OP_CLEAR,     // depth clear store
                };
                return aLoads[ usage ];
            }

            VkAttachmentStoreOp UsageToStoreOp( RenderSystem::RENDER_TARGET_RENDER_PASS_OP usage )
            {
                static const VkAttachmentStoreOp aStores[] = {
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
                return aStores[ usage ];
            }

            VkImageLayout ImageUsageToLayout( VkImageUsageFlags vkFlags )
            {
                const auto imgSampled      = vkFlags & VK_IMAGE_USAGE_SAMPLED_BIT;
                const auto inputAttachment = vkFlags & VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
                const auto isReadOnly      = imgSampled || inputAttachment;

                if( vkFlags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT )
                {
                    return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                }
                else if( vkFlags & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT )
                {
                    if( isReadOnly )
                    {
                        return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
                    }
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
                const auto imgSampled      = vkFlags & VK_IMAGE_USAGE_SAMPLED_BIT;
                const auto inputAttachment = vkFlags & VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
                bool       isReadOnly      = imgSampled || inputAttachment;

                if( vkFlags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT )
                {
                    if( isReadOnly )
                    {
                        return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    }
                    return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                }
                else if( vkFlags & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT )
                {
                    if( isReadOnly )
                    {
                        return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
                    }
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
                // TEXTURE_FORMAT is based on VkFormat, therefore we can just static_cast the enum with
                // boundaries check of supported formats instead of doing switch/case.
                // When we'll have unit tests, we'll make sure that all formats are mapped correctly.
                // TODO(blturkot): Unit test to verify that all formats are mapped correctly.

                unsigned int vkFormatValue = static_cast< unsigned int >( vkFormat );
                if( vkFormatValue < static_cast< unsigned int >( RenderSystem::Formats::_MAX_COUNT ) )
                {
                    return static_cast< RenderSystem::TEXTURE_FORMAT >( vkFormatValue );
                }

                char buff[ 128 ];
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
                if( usage & RenderSystem::BufferUsages::BUFFER )
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

            static const VkMemoryPropertyFlags g_aRequiredMemoryFlags[] = {
                0,                                                                          // unknown
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,                                        // gpu
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, // cpu access
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
                    VK_MEMORY_PROPERTY_HOST_CACHED_BIT // cpu access optimal
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
                static const VkPipelineBindPoint aVkBindPoints[] = { VK_PIPELINE_BIND_POINT_GRAPHICS,
                                                                     VK_PIPELINE_BIND_POINT_COMPUTE };
                return aVkBindPoints[ type ];
            }

            void ClearValues( const SClearValue* pSrc, const uint32_t count, VkClearValue* pDst )
            {
                for( uint32_t i = 0; i < count; ++i )
                {
                    const SClearValue& Src = pSrc[ i ];
                    VkClearValue&      Dst = pDst[ i ];

                    Dst.color.float32[ 0 ]   = Src.Color.r;
                    Dst.color.float32[ 1 ]   = Src.Color.g;
                    Dst.color.float32[ 2 ]   = Src.Color.b;
                    Dst.color.float32[ 3 ]   = Src.Color.a;
                    Dst.depthStencil.depth   = Src.DepthStencil.depth;
                    Dst.depthStencil.stencil = Src.DepthStencil.stencil;
                }
            }

            void TextureSubresourceRange( VkImageSubresourceRange* pVkDst, const STextureSubresourceRange& Src )
            {
                pVkDst->aspectMask     = Map::ImageAspect( Src.aspect );
                pVkDst->baseArrayLayer = Src.beginArrayLayer;
                pVkDst->baseMipLevel   = Src.beginMipmapLevel;
                pVkDst->layerCount     = Src.layerCount;
                pVkDst->levelCount     = Src.mipmapLevelCount;
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
                if( type & MemoryAccessTypes::VS_SHADER_READ || type & MemoryAccessTypes::PS_SHADER_READ ||
                    type & MemoryAccessTypes::GS_SHADER_READ || type & MemoryAccessTypes::CS_SHADER_READ ||
                    type & MemoryAccessTypes::TS_SHADER_READ || type & MemoryAccessTypes::MS_SHADER_READ ||
                    type & MemoryAccessTypes::RS_SHADER_READ )
                {
                    vkFlags |= VK_ACCESS_SHADER_READ_BIT;
                }
                if( type & MemoryAccessTypes::VS_SHADER_WRITE || type & MemoryAccessTypes::PS_SHADER_WRITE ||
                    type & MemoryAccessTypes::GS_SHADER_WRITE || type & MemoryAccessTypes::CS_SHADER_WRITE ||
                    type & MemoryAccessTypes::TS_SHADER_WRITE || type & MemoryAccessTypes::MS_SHADER_WRITE ||
                    type & MemoryAccessTypes::RS_SHADER_WRITE )
                {
                    vkFlags |= VK_ACCESS_SHADER_WRITE_BIT;
                }
                if( type & MemoryAccessTypes::VS_UNIFORM_READ || type & MemoryAccessTypes::PS_UNIFORM_READ ||
                    type & MemoryAccessTypes::GS_UNIFORM_READ || type & MemoryAccessTypes::CS_UNIFORM_READ ||
                    type & MemoryAccessTypes::TS_UNIFORM_READ || type & MemoryAccessTypes::MS_UNIFORM_READ ||
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
                pOut->image         = Info.hDDITexture;
                pOut->oldLayout     = Map::ImageLayout( Info.currentState );
                pOut->newLayout     = Map::ImageLayout( Info.newState );
                Convert::TextureSubresourceRange( &pOut->subresourceRange, Info.SubresourceRange );
            }

            void Barrier( VkBufferMemoryBarrier* pOut, const SBufferBarrierInfo& Info )
            {
                pOut->srcAccessMask = Convert::AccessMask( Info.srcMemoryAccess );
                pOut->dstAccessMask = Convert::AccessMask( Info.dstMemoryAccess );
                pOut->buffer        = Info.hDDIBuffer;
                pOut->offset        = Info.offset;
                pOut->size          = Info.size;
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
                if( flags & MemoryAccessTypes::VS_UNIFORM_READ || flags & MemoryAccessTypes::VS_SHADER_READ ||
                    flags & MemoryAccessTypes::VS_SHADER_WRITE )
                {
                    ret |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
                }
                if( flags & MemoryAccessTypes::PS_UNIFORM_READ || flags & MemoryAccessTypes::PS_SHADER_READ ||
                    flags & MemoryAccessTypes::PS_SHADER_WRITE )
                {
                    ret |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                }
                if( flags & MemoryAccessTypes::GS_UNIFORM_READ || flags & MemoryAccessTypes::GS_SHADER_READ ||
                    flags & MemoryAccessTypes::GS_SHADER_WRITE )
                {
                    ret |= VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT;
                }
                if( flags & MemoryAccessTypes::TS_UNIFORM_READ || flags & MemoryAccessTypes::TS_SHADER_READ ||
                    flags & MemoryAccessTypes::TS_SHADER_WRITE )
                {
                    ret |= VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT |
                           VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT;
                }
                if( flags & MemoryAccessTypes::CS_UNIFORM_READ || flags & MemoryAccessTypes::CS_SHADER_READ ||
                    flags & MemoryAccessTypes::CS_SHADER_WRITE )
                {
                    ret |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
                }
                if( flags & MemoryAccessTypes::MS_UNIFORM_READ || flags & MemoryAccessTypes::MS_SHADER_READ ||
                    flags & MemoryAccessTypes::MS_SHADER_WRITE )
                {
                    ret |= VK_PIPELINE_STAGE_TASK_SHADER_BIT_EXT | VK_PIPELINE_STAGE_MESH_SHADER_BIT_EXT;
                }
                if( flags & MemoryAccessTypes::RT_UNIFORM_READ || flags & MemoryAccessTypes::RS_SHADER_READ ||
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
                if( flags & MemoryAccessTypes::DATA_TRANSFER_READ || flags & MemoryAccessTypes::DATA_TRANSFER_WRITE )
                {
                    ret |= VK_PIPELINE_STAGE_TRANSFER_BIT;
                }

                if( flags & MemoryAccessTypes::CPU_MEMORY_READ || flags & MemoryAccessTypes::CPU_MEMORY_WRITE )
                {
                    ret |= VK_PIPELINE_STAGE_HOST_BIT;
                }

                if( flags & MemoryAccessTypes::GPU_MEMORY_READ || flags & MemoryAccessTypes::GPU_MEMORY_WRITE )
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

            void RenderSystemToVkRect2D( const VKE::ExtentI32& Position, const VKE::RenderSystem::TextureSize& Size, VkRect2D* pOut )
            {
                pOut->offset = { Position.x, Position.y };
                pOut->extent = { Size.width, Size.height };
            }

        } // namespace Convert

        namespace NativeAPI
        {
            struct SFence
            {
                VKE_RENDER_SYSTEM_DEBUG_NAME;
                std::atomic<FenceValue> counter;
                FenceValue                lastSignaledValue = 0;
                bool                      isNativeMonitored = false;
                bool                      isBinary          = false;
                struct SFences
                {
                    CPUFence hFence = Null;
                    GPUFence hSemaphore = Null;
                };

                Utils::TCDynamicArray< FenceValue > vValues;
                Utils::TCDynamicArray< SFences >  vFences;

                VKE::Result Create( const CDDI* pApi, const SFenceDesc& Desc, bool nativeMonitored )
                {
                    isBinary = Desc.startValue == UNDEFINED_U64;
                    isNativeMonitored = nativeMonitored;
                    this->counter     = Desc.startValue; // increase current counter
                    if( isNativeMonitored || isBinary )
                    {  
                        if( vFences.IsEmpty() )
                        {
                            SFences        Fences;
                            SSemaphoreDesc SemDesc;
                            SemDesc.SetDebugName( Desc.GetDebugName() );
                            SemDesc.startValue = Desc.startValue;
                            Fences.hSemaphore  = pApi->CreateSemaphore( SemDesc, nullptr );
                            if( isBinary )
                            {
                                SFenceDesc FenceDesc;
                                FenceDesc.SetDebugName( Desc.GetDebugName() );
                                Fences.hFence = pApi->CreateFence( FenceDesc, nullptr );
                            }
                            vFences.PushBack( Fences );
                            vValues.PushBack( Desc.startValue );
                        }
                    }
                    else
                    {
                        this->SetDebugName( Desc.GetDebugName() );
                    }

                    return VKE_OK;
                }

                /// <summary>
                /// Increases counter value and returns Fence associated to requested one.
                /// </summary>
                /// <param name="pApi"></param>
                /// <param name="value">New value for which fence will wait</param>
                /// <returns></returns>
                SFences* Signal( CDDI* pApi, FenceValue value )
                {
                    if( !isBinary && !isNativeMonitored )
                    {
                        VKE_ASSERT( this->counter.load() < value );
                        this->counter = value; // increase current counter
                        // Check if there is any fence completed
                        // mark it as 0 and reuse it
                        Recycle( pApi );
                        // Find first free index
                        // Index is free when its value is set to 0
                        // Index is freed when fence is signaled
                        auto idx = vValues.Find( 0 );

                        if( idx == INVALID_POSITION )
                        {
                            idx = vValues.PushBack( value );
                            if( vFences.GetCount() <= idx )
                            {
                                auto idx2 = vFences.PushBack( {} );
                                VKE_ASSERT( idx == idx2 );
                                VKE_ASSERT( vValues.GetCount() == vFences.GetCount() );
                                SFences&   Fences = vFences.Back();
                                SFenceDesc FenDesc;
                                FenDesc.startValue = 0;
                                FenDesc.SetDebugName( "%s_%d", GetDebugName(), idx2 );
                                Fences.hFence = pApi->CreateFence( FenDesc, nullptr );
                                SSemaphoreDesc SemDesc;
                                SemDesc.SetDebugName( FenDesc.GetDebugName() );
                                Fences.hSemaphore = pApi->CreateSemaphore( SemDesc, nullptr );
                                return &Fences;
                            }
                        }
                        // Fence must be signaled if it is recycled
                        const bool signaled = pApi->IsSignaled( vFences[ idx ].hFence );
                        VKE_ASSERT( signaled );
                        VKE_ASSERT( vValues.GetCount() == vFences.GetCount() );
                        pApi->Reset( &vFences[ idx ].hFence );
                        vValues[ idx ] = value;
                        return &vFences[ idx ];
                    }
                    else if( isBinary )
                    {
                        pApi->WaitForFence( this, 0 );
                        pApi->Reset( &vFences[ 0 ].hFence );
                    }
                    return &vFences[ 0 ];
                }

                void Recycle( CDDI* pApi )
                {
                    GetLastSignaledValue( pApi );
                }

                SFences* GetFences( FenceValue value )
                {
                    if( isNativeMonitored || isBinary )
                    {
                        return &vFences[0];
                    }
                    auto idx = vValues.Find( value );
                    return &vFences[ idx ];
                }

                void Reset(CDDI* pApi, FenceValue value)
                {
                    VKE_ASSERT( vValues.GetCount() == vFences.GetCount() );
                    for( uint32_t i = 0; i < vFences.GetCount(); ++i )
                    {
                        //pApi->Reset( &vFences[ i ].hFence );
                        vValues[ i ] = 0;
                    }
                    counter = value;
                    lastSignaledValue = 0;
                }

                FenceValue GetLastSignaledValue( const CDDI* pApi )
                {
                    for( uint32_t i = 0; i < vValues.GetCount(); ++i )
                    {
                        auto value = vValues[ i ];
                        if( value > 0 )
                        {
                            if( pApi->IsSignaled( vFences[ i ].hFence ) )
                            {
                                vValues[ i ] = 0; // reset this fence as it is no longer valid
                                lastSignaledValue          = Math::Max( lastSignaledValue, value );
                            }
                        }
                    }
                    // if lastSignaledValue == 0 that means fence was not signaled yet
                    //VKE_ASSERT( lastSignaledValue == 0 || lastSignaledValue >= counter.load() );
                    return lastSignaledValue;
                }
            };
        } // namespace NativeAPI

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
                size_t                   size = 0;
                size_t                   alignment;
                void*                    pPreviousAlloc;
                VkSystemAllocationScope  vkScope;
                VkInternalAllocationType vkAllocationType;
            };

            void* VKAPI_PTR DummyAllocCallback( void* pUserData, size_t size, size_t alignment,
                                                VkSystemAllocationScope vkScope )
            {
                SAllocData* pData  = reinterpret_cast< SAllocData* >( pUserData );
                pData->size       += size;
                pData->alignment   = alignment;
                pData->vkScope     = vkScope;
                void* pRet         = VKE_MALLOC( size );
                return pRet;
            }

            void* VKAPI_PTR DummyReallocCallback( void* pUserData, void* pOriginal, size_t size, size_t alignment,
                                                  VkSystemAllocationScope vkScope )
            {
                SAllocData* pData     = reinterpret_cast< SAllocData* >( pUserData );
                pData->size           = size;
                pData->alignment      = alignment;
                pData->vkScope        = vkScope;
                pData->pPreviousAlloc = pOriginal;
                return VKE_REALLOC( pOriginal, size );
            }

            void VKAPI_PTR DummyInternalAllocCallback( void* pUserData, size_t size,
                                                       VkInternalAllocationType vkAllocationType,
                                                       VkSystemAllocationScope  vkAllocationScope )
            {
                SAllocData* pData        = reinterpret_cast< SAllocData* >( pUserData );
                pData->size             += size;
                pData->vkScope           = vkAllocationScope;
                pData->vkAllocationType  = vkAllocationType;
            }

            void VKAPI_PTR DummyFreeCallback( void* pUserData, void* pMemory )
            {
                // SAllocData* pData = reinterpret_cast<SAllocData*>(pUserData);
                VKE_FREE( pMemory );
            }

            void VKAPI_PTR DummyInternalFreeCallback( void*, size_t, VkInternalAllocationType, VkSystemAllocationScope )
            {
                // SAllocData* pData = reinterpret_cast<SAllocData*>(pUserData);
            }

            struct SSwapChainAllocator
            {
                uint8_t* pMemory;
                uint32_t currentChunkOffset = 0; // offset in current chunk
                uint32_t memorySize;
                uint32_t chunkSize;      // == memorySize / elementCount
                uint32_t ddiElementSize; // total memory returned from callbacks after all swapchain is created
                uint8_t  currentElement = 0;
                uint8_t  elementCount;

                VkAllocationCallbacks VkCallbacks;

                Result Create( uint32_t elSize, uint8_t elCount )
                {
                    Result ret = VKE_ENOMEMORY;
                    VKE_ASSERT2( pMemory == nullptr, "" );
                    chunkSize    = elSize;
                    elementCount = elCount;
                    memorySize   = chunkSize * elementCount;
                    pMemory      = reinterpret_cast< uint8_t* >( VKE_MALLOC( memorySize ) );
                    if( pMemory != nullptr )
                    {
                        VkCallbacks.pUserData             = this;
                        VkCallbacks.pfnAllocation         = AllocCallback;
                        VkCallbacks.pfnFree               = FreeCallback;
                        VkCallbacks.pfnReallocation       = ReallocCallback;
                        VkCallbacks.pfnInternalFree       = InternalFreeCallback;
                        VkCallbacks.pfnInternalAllocation = InternalAllocCallback;
                        ret                               = VKE_OK;
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
                    currentElement     = 0;
                }

                void FreeCurrentChunk()
                {
                    currentChunkOffset = 0;
                    currentElement     = ( currentElement + 1 ) % elementCount;
                }

                uint8_t* GetMemory( uint32_t size, uint32_t alignment )
                {
                    VKE_ASSERT2( currentChunkOffset + size <= chunkSize, "" );

                    uint8_t* pChunkMem = pMemory + ( currentElement * chunkSize );
                    uint8_t* pPtr      = pChunkMem + currentChunkOffset;

                    const auto alignedSize  = Memory::CalcAlignedSize( size, alignment );
                    currentChunkOffset     += alignedSize;

                    return pPtr;
                }

                static void* VKAPI_PTR AllocCallback( void* pUserData, size_t size, size_t alignment,
                                                      VkSystemAllocationScope )
                {
                    void* pRet;
                    {
                        SSwapChainAllocator* pAllocator = reinterpret_cast< SSwapChainAllocator* >( pUserData );
                        uint8_t*             pPtr       = pAllocator->GetMemory( static_cast< uint32_t >( size ),
                                                               static_cast< uint32_t >( alignment ) );
                        pRet                            = pPtr;
                    }
                    return pRet;
                }

                static void VKAPI_PTR FreeCallback( void* pUserData, void* pMemory )
                {
                    SSwapChainAllocator* pAllocator = reinterpret_cast< SSwapChainAllocator* >( pUserData );
                    uint8_t*             pMemEnd    = pAllocator->pMemory + pAllocator->memorySize;
                    // Free allocations only out of memory block
                    if( pMemory < pAllocator->pMemory || pMemory >= pMemEnd )
                    {
                        VKE_FREE( pMemory );
                    }
                }

                static void* VKAPI_PTR ReallocCallback( void* pUserData, void* pOriginal, size_t size, size_t alignment,
                                                        VkSystemAllocationScope )
                {
                    (void)alignment;
                    (void)pUserData;
                    VKE_ASSERT2( 0, "This is not suppoerted for SwapChain." );
                    return VKE_REALLOC( pOriginal, size );
                }

                static void VKAPI_PTR InternalFreeCallback( void* pUserData, size_t size, VkInternalAllocationType,
                                                            VkSystemAllocationScope )
                {
                    (void)pUserData;
                    (void)size;
                }

                static void VKAPI_PTR InternalAllocCallback( void*, size_t, VkInternalAllocationType,
                                                             VkSystemAllocationScope )
                {
                }
            };

            template< typename HandleT, class DescT >
            vke_force_inline void SetObjectDebugName( const CDDI* pDDI, HandleT hObj, VkObjectType objType,
                                                      const DescT& Desc )
            {
#if VKE_RENDER_SYSTEM_DEBUG
                pDDI->SetObjectDebugName( (uint64_t)hObj, objType, Desc.GetDebugName() );
#endif
            }

        } // namespace Helper

        namespace NativeAPI
        {
            struct SRenderPass
            {
                using ColorRenderTargetArray = Utils::TCDynamicArray<VkRenderingAttachmentInfo, 8 >;
                using ClearColorArray = Utils::TCDynamicArray<VkClearValue, 8>;
                using FormatArray = Utils::TCDynamicArray<VkFormat, 8>;
                VKE_RENDER_SYSTEM_DEBUG_NAME;
                /// <summary>
                /// Legacy render pass
                /// </summary>
                VkRenderPass hNativeRenderPass = NativeAPI::Null;
                VkFramebuffer             hNativeFramebuffer = NativeAPI::Null;
                VkRenderPassBeginInfo     NativeBeginInfo;
                ClearColorArray           vNativeClearColors;
                ColorRenderTargetArray vColorRenderTargets;
                VkRenderingAttachmentInfo VkDepthRenderTarget;
                VkRenderingAttachmentInfo VkStencilRenderTarget;
                /// <summary>
                /// Renderpass less
                /// </summary>
                VkRenderingInfo VkInfo;
                FormatArray     vColorRenderTargetFormats;
                VkFormat        VkDepthRenderTargetFormat;
                VkFormat        VkStencilRenderTargetFormat;
            };
        } // namespace NativeAPI

        vke_force_inline int32_t FindMemoryTypeIndex( const VkPhysicalDeviceMemoryProperties* pMemProps,
                                                      uint32_t                                requiredMemBits,
                                                      VkMemoryPropertyFlags                   requiredProperties );

        Result CheckRequiredExtensions( NativeAPI::DDIExtMap*   pmExtensionsInOut,
                                        NativeAPI::DDIExtArray* pvRequiredInOut, CStrVec* pvNamesOut )
        {
            Result ret = VKE_OK;
            for( auto& ReqExt: *pvRequiredInOut )
            {
                bool found = false;
                for( auto& Pair: *pmExtensionsInOut )
                {
                    auto& Ext = Pair.second;

                    if( Ext.name == ReqExt.name )
                    {
                        found            = true;
                        ReqExt.supported = true;
                        ReqExt.enabled   = true;
                        Ext.enabled      = true;
                        Ext.required     = true;

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

        Result GetInstanceValidationLayers( VkICD::Global& Global, NativeAPI::DDIExtMap* pmLayersInOut,
                                            NativeAPI::DDIExtArray* pvRequiredInOut, CStrVec* pvNames )
        {
            // static const char* apNames[] =
            //{
            //     "VK_LAYER_KHRONOS_validation",
            //     //"VK_LAYER_LUNARG_core_validation",
            //     //"VK_LAYER_LUNARG_parameter_validation",
            //     /*VK_LAYER_GOOGLE_threading
            //     VK_LAYER_LUNARG_parameter_validation
            //     VK_LAYER_LUNARG_device_limits
            //     VK_LAYER_LUNARG_object_tracker
            //     VK_LAYER_LUNARG_image
            //     VK_LAYER_LUNARG_core_validation
            //     VK_LAYER_LUNARG_swapchain
            //     VK_LAYER_GOOGLE_unique_objects*/

            //};
            /*vNames.push_back("VK_LAYER_GOOGLE_threading");
            vNames.push_back("VK_LAYER_LUNARG_parameter_validation");
            vNames.push_back("VK_LAYER_LUNARG_device_limits");
            vNames.push_back("VK_LAYER_LUNARG_object_tracker");
            vNames.push_back("VK_LAYER_LUNARG_image");
            vNames.push_back("VK_LAYER_LUNARG_core_validation");
            vNames.push_back("VK_LAYER_LUNARG_swapchain");
            vNames.push_back("VK_LAYER_GOOGLE_unique_objects");*/

            uint32_t                                       count = 0;
            Utils::TCDynamicArray< VkLayerProperties, 64 > vProps;
            VK_ERR( Global.vkEnumerateInstanceLayerProperties( &count, nullptr ) );
            if( count > 0 )
            {
                vProps.Resize( count );
                VK_ERR( Global.vkEnumerateInstanceLayerProperties( &count, &vProps[ 0 ] ) );

                pmLayersInOut->reserve( count );
                vke_string tmpName;
                tmpName.reserve( 128 );
                VKE_LOG( "SUPPORTED VULKAN INSTANCE LAYERS:" );
                for( uint32_t i = 0; i < count; ++i )
                {
                    tmpName = vProps[ i ].layerName;
                    pmLayersInOut->insert(
                        NativeAPI::DDIExtMap::value_type( tmpName, { tmpName, false, true, false } ) );
                    VKE_LOG( tmpName.c_str() );
                }
            }
            else
            {
                VKE_LOG_WARN( "Vulkan instance layers are not supported on this machine." );
            }
            return CheckRequiredExtensions( pmLayersInOut, pvRequiredInOut, pvNames );
        }

        Result CheckInstanceExtensionNames( VkICD::Global& Global, NativeAPI::DDIExtMap* pmExtensionsInOut,
                                            NativeAPI::DDIExtArray* pvRequired, CStrVec* pvOut )
        {
            VKE_LOG_PROG( "VKEngine Checking instance extensions" );
            vke_vector< VkExtensionProperties > vProps;
            uint32_t                            count = 0;
            VK_ERR( Global.vkEnumerateInstanceExtensionProperties( nullptr, &count, nullptr ) );
            VKE_LOG_PROG( "VKEngine count: " << count );
            vProps.resize( count );
            VK_ERR( Global.vkEnumerateInstanceExtensionProperties( nullptr, &count, &vProps[ 0 ] ) );
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
                tmpName = vProps[ i ].extensionName;
                pmExtensionsInOut->insert(
                    NativeAPI::DDIExtMap::value_type( tmpName, { tmpName, false, true, false } ) );
                VKE_LOG( tmpName.c_str() );
            }

            return CheckRequiredExtensions( pmExtensionsInOut, pvRequired, pvOut );
        }

        template< class T >
        void AddVulkanNext( T& Struct, void*** pppNext )
        {
            void** ppNext = *pppNext;
            *ppNext       = &Struct;
            ppNext        = &Struct.pNext;
        }

        template< class T >
        void InitVulkanNext( T& Struct, void*** pppNext )
        {
            *pppNext = &Struct.pNext;
        }

        struct SVulkanNext
        {
            const void** ppNext;

            ~SVulkanNext()
            {
                if( ppNext )
                {
                    *ppNext = nullptr;
                }
            }

            template< class T >
            SVulkanNext( T& Struct ) //: ppNext( (void**)&Struct.pNext )
            {
                ppNext = (const void**) & Struct.pNext;
                while( ppNext && *ppNext )
                {
                    ppNext = (const void**) & ( (VkBaseInStructure*)*ppNext )->pNext;
                }
            }

            template< class T >
            SVulkanNext& Add( T* pStruct, VkStructureType type )
            {
                auto& Struct = *pStruct;
                Struct       = { type };
                *ppNext      = &Struct;
                ppNext       = (const void**) & Struct.pNext;
                return *this;
            }

            template< class T >
            SVulkanNext& Add( T* pStruct )
            {
                auto& Struct = *pStruct;
                *ppNext      = &Struct;
                ppNext       = (const void**)&Struct.pNext;
                return *this;
            }
        };

        Result QueryAdapterProperties( const NativeAPI::Adapter& hAdapter, const NativeAPI::DDIExtMap& mExts,
                                       SDeviceProperties* pOut )
        {
            Memory::Zero( &pOut->Features );
            Memory::Zero( &pOut->Limits );
            Memory::Zero( &pOut->Properties );

            pOut->Properties.Memory = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2 };

            pOut->Properties.Device = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
            pOut->Features.Device   = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };

            auto& Features   = pOut->Features;
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

            if( mExts.find( VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME ) != mExts.end() )
            {
                NextFeatures
                    .Add( &Features.Raytracing10, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR )
                    .Add( &Features.Raytracing11, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR )
                    .Add( &Features.Raytracing12,
                          VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_MOTION_BLUR_FEATURES_NV );

                NextProperties.Add( &Properties.Raytracing10,
                                    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR );
            }

            NativeAPI::SImplementation::sInstanceICD.vkGetPhysicalDeviceFeatures2( hAdapter, &pOut->Features.Device );
            NativeAPI::SImplementation::sInstanceICD.vkGetPhysicalDeviceMemoryProperties2( hAdapter,
                                                                                           &pOut->Properties.Memory );
            NativeAPI::SImplementation::sInstanceICD.vkGetPhysicalDeviceProperties2( hAdapter,
                                                                                     &pOut->Properties.Device );

#if 0
            if( NativeAPI::SImplementation::sInstanceICD.vkGetPhysicalDeviceFeatures2 )
            {
                NativeAPI::SImplementation::sInstanceICD.vkGetPhysicalDeviceFeatures2( hAdapter, &pOut->Features.Device );
            }
            else
            {
                NativeAPI::SImplementation::sInstanceICD.vkGetPhysicalDeviceFeatures( hAdapter, &pOut->Features.Device.features );
            }
            if( NativeAPI::SImplementation::sInstanceICD.vkGetPhysicalDeviceMemoryProperties2 )
            {
                NativeAPI::SImplementation::sInstanceICD.vkGetPhysicalDeviceMemoryProperties2( hAdapter, &pOut->Properties.Memory );
            }
            else
            {
                NativeAPI::SImplementation::sInstanceICD.vkGetPhysicalDeviceMemoryProperties( hAdapter, &pOut->Properties.Memory.memoryProperties );
            }
            if( NativeAPI::SImplementation::sInstanceICD.vkGetPhysicalDeviceProperties2 )
            {
                NativeAPI::SImplementation::sInstanceICD.vkGetPhysicalDeviceProperties2( hAdapter, &pOut->Properties.Device );
            }
            else
            {
                NativeAPI::SImplementation::sInstanceICD.vkGetPhysicalDeviceProperties( hAdapter, &pOut->Properties.Device.properties );
            }
#endif // VKE_VULKAN_1_1
            {
                // ICD.Instance.vkGetPhysicalDeviceFormatProperties( vkPhysicalDevice, &m_DeviceInfo.FormatProperties );
            }

            uint32_t propCount = 0;
            NativeAPI::SImplementation::sInstanceICD.vkGetPhysicalDeviceQueueFamilyProperties(
                hAdapter, &propCount, nullptr );
            if( propCount == 0 )
            {
                VKE_LOG_ERR( "No device queue family properties" );
                return VKE_FAIL;
            }

            pOut->vQueueFamilyProperties.Resize( propCount );
            auto& aProperties    = pOut->vQueueFamilyProperties;
            auto& vQueueFamilies = pOut->vQueueFamilies;

            NativeAPI::SImplementation::sInstanceICD.vkGetPhysicalDeviceQueueFamilyProperties(
                hAdapter, &propCount, &aProperties[ 0 ] );
            // Choose a family index
            for( uint32_t i = 0; i < propCount; ++i )
            {
                auto&    VkProp     = aProperties[ i ];
                uint32_t isCompute  = VkProp.queueFlags & VK_QUEUE_COMPUTE_BIT;
                uint32_t isTransfer = VkProp.queueFlags & VK_QUEUE_TRANSFER_BIT;
                uint32_t isSparse   = VkProp.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT;
                uint32_t isGraphics = VkProp.queueFlags & VK_QUEUE_GRAPHICS_BIT;
                VkBool32 isPresent  = VK_FALSE;
#if VKE_USE_VULKAN_WINDOWS
                isPresent = NativeAPI::SImplementation::sInstanceICD.vkGetPhysicalDeviceWin32PresentationSupportKHR(
                    hAdapter, i );
#elif VKE_USE_VULKAN_LINUX
                isPresent = NativeAPI::SImplementation::sInstanceICD.vkGetPhysicalDeviceXcbPresentationSupportKHR(
                    hAdapter, i, xcb_connection, visual_id );
#elif VKE_USE_VULKAN_ANDROID
#error "implement"
#endif

                SQueueFamilyInfo Family;
                Family.vQueues.Resize( aProperties[ i ].queueCount );
                Family.vPriorities.Resize( aProperties[ i ].queueCount, 1.0f );
                Family.index = i;
                Family.type  = QueueTypes::GENERAL;

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
                const auto& fmt = RenderSystem::g_aFormats[ i ];
                NativeAPI::SImplementation::sInstanceICD.vkGetPhysicalDeviceFormatProperties(
                    hAdapter, fmt, &pOut->Properties.aFormatProperties[ i ] );
            }

            return VKE_OK;
        }

        void CDDI::GetFormatFeatures( FORMAT fmt, STextureFormatFeatures* pOut ) const
        {
            Memory::Zero( pOut );
            const auto&                             Props = m_DeviceProperties.Properties.aFormatProperties[ fmt ];
            Utils::TCBitset< VkFormatFeatureFlags > Bits( Props.optimalTilingFeatures );

            pOut->sampled                  = Bits == VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;
            pOut->colorRenderTarget        = Bits == VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT;
            pOut->storage                  = Bits == VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT;
            pOut->storageAtomic            = Bits == VK_FORMAT_FEATURE_STORAGE_IMAGE_ATOMIC_BIT;
            pOut->uniformTexelBuffer       = Bits == VK_FORMAT_FEATURE_UNIFORM_TEXEL_BUFFER_BIT;
            pOut->storageTexelBuffer       = Bits == VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_BIT;
            pOut->storageTexelBufferAtomic = Bits == VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_ATOMIC_BIT;
            pOut->depthStencilRenderTarget = Bits == VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
            pOut->blitSrc                  = Bits == VK_FORMAT_FEATURE_BLIT_SRC_BIT;
            pOut->blitDst                  = Bits == VK_FORMAT_FEATURE_BLIT_DST_BIT;
            pOut->linearFilter             = Bits == VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT;
            pOut->transferSrc              = Bits == VK_FORMAT_FEATURE_TRANSFER_SRC_BIT;
            pOut->transferDst              = Bits == VK_FORMAT_FEATURE_TRANSFER_DST_BIT;
        }

        using DDIExtNameArray = Utils::TCDynamicArray< cstr_t >;

        Result GetDeviceExtensions( VkPhysicalDevice vkPhysicalDevice, NativeAPI::DDIExtMap* pmAllExtensionsOut )
        {
            auto& sInstanceICD = NativeAPI::SImplementation::sInstanceICD;

            uint32_t count = 0;
            VK_ERR( sInstanceICD.vkEnumerateDeviceExtensionProperties( vkPhysicalDevice, nullptr, &count, nullptr ) );

            Utils::TCDynamicArray< VkExtensionProperties > vProperties( count );
            pmAllExtensionsOut->reserve( count );

            VK_ERR( NativeAPI::SImplementation::sInstanceICD.vkEnumerateDeviceExtensionProperties(
                vkPhysicalDevice, nullptr, &count, &vProperties[ 0 ] ) );

            std::string ext;

            vke_string tmpName;
            tmpName.reserve( 128 );
            VKE_LOG( "SUPPORTED VULKAN DEVICE EXTENSIONS:" );
            for( uint32_t p = 0; p < count; ++p )
            {
                tmpName = vProperties[ p ].extensionName;
                VKE_LOG( tmpName );
                pmAllExtensionsOut->insert(
                    NativeAPI::DDIExtMap::value_type( tmpName, { tmpName, false, true, false } ) );
            }

            return VKE_OK;
        }

        Result CheckDeviceExtensions( const NativeAPI::DDIExtMap& mAllExtensions,
                                      const DDIExtNameArray&      vRequestedExtensions )
        {
            DDIExtNameArray vNotSupported;
            for( uint32_t i = 0; i < vRequestedExtensions.GetCount(); ++i )
            {
                cstr_t pName = vRequestedExtensions[ i ];
                if( mAllExtensions.find( pName ) == mAllExtensions.end() )
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

        Result EnableDeviceExtensions( const NativeAPI::DDIExtMap& mAllExtensions, NativeAPI::SImplementation::SDeviceFeatures* pFeatures,
                                       DDIExtNameArray*                             pExtToEnable )
        {
            if( !pFeatures->Device12.timelineSemaphore )
            {
                auto Itr = mAllExtensions.find( VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME );
                if( Itr != mAllExtensions.end() )
                {
                    const auto& Ext = Itr->second;
                    if( Ext.supported )
                    {
                        pExtToEnable->PushBack( VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME );
                        pFeatures->Device12.timelineSemaphore = true;
                    }
                }
            }
            return VKE_OK;
        }

        FEATURE_LEVEL ConvertVulkanAPIToFeatureLevel( uint32_t apiVer )
        {
            FEATURE_LEVEL ret = FeatureLevels::LEVEL_1_0;
            if( VK_API_VERSION_MAJOR( apiVer ) == 1 )
            {
                auto minor = VK_API_VERSION_MINOR( apiVer );
                switch( minor )
                {
                    case 1:
                        ret = FeatureLevels::LEVEL_1_1;
                        break;
                    case 2:
                        ret = FeatureLevels::LEVEL_1_2;
                        break;
                    case 3:
                        ret = FeatureLevels::LEVEL_1_3;
                        break;
                    case 4:
                        ret = FeatureLevels::LEVEL_1_4;
                        break;
                }
            }
            return ret;
        }

        uint32_t ConvertFeatureSetToVulkanAPIVersion( FEATURE_LEVEL set )
        {
            uint32_t ret = VK_MAKE_API_VERSION( 0, 1, 0, 0 );
            switch( set )
            {
                case FeatureLevels::LEVEL_1_1:
                    ret = VK_MAKE_API_VERSION( 0, 1, 1, 0 );
                    break;
                case FeatureLevels::LEVEL_1_2:
                    ret = VK_MAKE_API_VERSION( 0, 1, 2, 0 );
                    break;
                case FeatureLevels::LEVEL_1_3:
                    ret = VK_MAKE_API_VERSION( 0, 1, 3, 0 );
                    break;
                case FeatureLevels::LEVEL_1_4:
                    ret = VK_MAKE_API_VERSION( 0, 1, 4, 0 );
                    break;
            }
            return ret;
        }

        Result CDDI::Load( const SDDILoadInfo& Info, SDriverInfo* pOut )
        {
            Result ret = VKE_OK;
            VKE_LOG_PROG( "VKEngine loading vulkan-1.dll" );

            auto& sGlobalICD = NativeAPI::SImplementation::sGlobalICD;
            auto& shICD      = NativeAPI::SImplementation::shICD;

            shICD = Platform::DynamicLibrary::Load( "vulkan-1.dll" );
            if( shICD != 0 )
            {
                VKE_LOG_PROG( "vulkan-1.dll loaded" );

                ret = Vulkan::LoadGlobalFunctions( shICD, &sGlobalICD );
                if( VKE_SUCCEEDED( ret ) )
                {
                    VKE_LOG_PROG( "Vulkan global functions loaded" );
                    NativeAPI::DDIExtArray vRequiredInstanceExts =
                        GetRequiredInstanceExtensions( Info.enableDebugMode );
                    NativeAPI::DDIExtArray vRequiredDeviceExts = GetRequiredDeviceExtensions( Info.enableDebugMode );
                    const bool enableValidationKnob        = GetCommandLineParam< bool >( "rs.vk_validation", true );
                    const bool enableRenderSystemDebugKnob = GetCommandLineParam< bool >( "rs.debug", true );

                    NativeAPI::DDIExtArray vRequiredLayers;
                    if( Info.enableDebugMode && enableValidationKnob && enableRenderSystemDebugKnob )
                    {
                        //                          name,                          required,   supported,  enabled
                        vRequiredLayers.PushBack( { "VK_LAYER_KHRONOS_validation", true, false, false } );
                    }

                    CStrVec              vExtNames;
                    NativeAPI::DDIExtMap mExtensions;
                    ret = CheckInstanceExtensionNames( sGlobalICD, &mExtensions, &vRequiredInstanceExts, &vExtNames );
                    VKE_ASSERT2( VKE_SUCCEEDED( ret ), "Required extension is not supported." );
                    if( VKE_FAILED( ret ) )
                    {
                        return ret;
                    }
                    VKE_LOG_PROG( "Vulkan ext checked" );

                    CStrVec              vLayerNames;
                    NativeAPI::DDIExtMap mLayers;
                    ret = GetInstanceValidationLayers( sGlobalICD, &mLayers, &vRequiredLayers, &vLayerNames );
                    VKE_ASSERT2( VKE_SUCCEEDED( ret ), "Required validation layer is not supported." );

                    // Vulkan 1.1 not supported
                    uint32_t apiVersion;
                    if( sGlobalICD.vkEnumerateInstanceVersion == nullptr )
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
                        vkAppInfo.apiVersion         = apiVersion;
                        vkAppInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
                        vkAppInfo.pNext              = nullptr;
                        vkAppInfo.applicationVersion = Info.AppInfo.applicationVersion;
                        vkAppInfo.engineVersion      = Info.AppInfo.engineVersion;
                        vkAppInfo.pApplicationName   = Info.AppInfo.pApplicationName;
                        vkAppInfo.pEngineName        = Info.AppInfo.pEngineName;

                        VkInstanceCreateInfo InstInfo;
                        Vulkan::InitInfo( &InstInfo, VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO );

                        Utils::TCDynamicArray< VkValidationFeatureEnableEXT > vEnableValFeatures = {
                            // Disable this one due to nsight restriction
                            // VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT
                        };
                        Utils::TCDynamicArray< VkValidationFeatureDisableEXT > vDisabledValFeatures = {
                            // Disable this one due to nsignt restriction
                            VK_VALIDATION_FEATURE_DISABLE_UNIQUE_HANDLES_EXT
                        };
                        VkValidationFeaturesEXT ValidationFeatures = { VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT };
                        ValidationFeatures.enabledValidationFeatureCount  = vEnableValFeatures.GetCount();
                        ValidationFeatures.pEnabledValidationFeatures     = vEnableValFeatures.GetData();
                        ValidationFeatures.pDisabledValidationFeatures    = vDisabledValFeatures.GetDataOrNull();
                        ValidationFeatures.disabledValidationFeatureCount = vDisabledValFeatures.GetCount();

                        VkDebugReportCallbackCreateInfoEXT DbgReport = {
                            VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT
                        };
                        DbgReport.pfnCallback = VkDebugCallback;
                        DbgReport.pUserData   = nullptr;
                        DbgReport.flags       = VK_DEBUG_REPORT_DEBUG_BIT_EXT | VK_DEBUG_REPORT_ERROR_BIT_EXT |
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

                        InstInfo.enabledExtensionCount   = static_cast< uint32_t >( vExtNames.GetCount() );
                        InstInfo.enabledLayerCount       = static_cast< uint32_t >( vLayerNames.GetCount() );
                        InstInfo.flags                   = 0;
                        InstInfo.pApplicationInfo        = &vkAppInfo;
                        InstInfo.ppEnabledExtensionNames = vExtNames.GetData();
                        InstInfo.ppEnabledLayerNames     = vLayerNames.GetData();

                        VkResult vkRes =
                            sGlobalICD.vkCreateInstance( &InstInfo, nullptr, &NativeAPI::SImplementation::sVkInstance );
                        VK_ERR( vkRes );
                        if( vkRes == VK_SUCCESS )
                        {
                            VKE_LOG_PROG( "Vulkan instance created with API ver: "
                                          << VK_API_VERSION_MAJOR( apiVersion ) << "."
                                          << VK_API_VERSION_MINOR( apiVersion ) );
                            ret = Vulkan::LoadInstanceFunctions( NativeAPI::SImplementation::sVkInstance,
                                                                 sGlobalICD,
                                                                 &NativeAPI::SImplementation::sInstanceICD );
                            if( ret == VKE_OK )
                            {
                                VKE_LOG_PROG( "Vk instance functions loaded" );
                                if( Info.enableDebugMode )
                                {
                                    if( NativeAPI::SImplementation::sInstanceICD.vkCreateDebugReportCallbackEXT )
                                    {
                                        vkRes = NativeAPI::SImplementation::sInstanceICD.vkCreateDebugReportCallbackEXT(
                                            NativeAPI::SImplementation::sVkInstance,
                                            &DbgReport,
                                            nullptr,
                                            &NativeAPI::SImplementation::sVkDebugReportCallback );
                                        VK_ERR( vkRes );
                                    }
                                    else if( NativeAPI::SImplementation::sInstanceICD.vkCreateDebugUtilsMessengerEXT )
                                    {
                                        vkRes = NativeAPI::SImplementation::sInstanceICD.vkCreateDebugUtilsMessengerEXT(
                                            NativeAPI::SImplementation::sVkInstance,
                                            &DbgUtils,
                                            nullptr,
                                            &NativeAPI::SImplementation::sVkDebugMessengerCallback );
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

        void CloseICD()
        {
            // sGlobalICD.vkDestroyInstance( NativeAPI::SImplementation::sVkInstance, nullptr );
            NativeAPI::SImplementation::sInstanceICD.vkDestroyInstance( NativeAPI::SImplementation::sVkInstance,
                                                                        nullptr );
            NativeAPI::SImplementation::sVkInstance = VK_NULL_HANDLE;
            Platform::DynamicLibrary::Close( NativeAPI::SImplementation::shICD );
        }

        const NativeAPI::SDDIExtension& NativeAPI::SImplementation::GetExtensionInfo( cstr_t pName ) const
        {
            static const NativeAPI::SDDIExtension sDummy;

            auto Itr = m_mExtensions.find( pName );
            if( Itr != m_mExtensions.end() )
            {
                return Itr->second;
            }
            return sDummy;
        }

        Result LoadDeviceExtensions( VkPhysicalDevice vkPhysicalDevice, NativeAPI::DDIExtMap* pmAllExtensionsOut )
        {
            auto& sInstanceICD = NativeAPI::SImplementation::sInstanceICD;

            uint32_t count = 0;
            VK_ERR( sInstanceICD.vkEnumerateDeviceExtensionProperties( vkPhysicalDevice, nullptr, &count, nullptr ) );
            Utils::TCDynamicArray< VkExtensionProperties > vProperties( count );
            pmAllExtensionsOut->reserve( count );
            VK_ERR( NativeAPI::SImplementation::sInstanceICD.vkEnumerateDeviceExtensionProperties(
                vkPhysicalDevice, nullptr, &count, &vProperties[ 0 ] ) );
            std::string ext;
            vke_string  tmpName;
            tmpName.reserve( 128 );
            VKE_LOG( "SUPPORTED VULKAN DEVICE EXTENSIONS:" );
            for( uint32_t p = 0; p < count; ++p )
            {
                tmpName = vProperties[ p ].extensionName;
                VKE_LOG( tmpName );
                pmAllExtensionsOut->insert(
                    NativeAPI::DDIExtMap::value_type( tmpName, { tmpName, false, true, false } ) );
            }
            return VKE_OK;
        }

        Result EnableDeviceFeatures( VkPhysicalDevice vkPhysicalDevice, SDeviceProperties* pProps,
                                     NativeAPI::DDIExtMap* pmExts, SSettings* pSettingsOut,
                                     NativeAPI::SImplementation::SDeviceFeatures* pEnableOut,
                                     VkDeviceCreateInfo* pOut,
                                     DDIExtNameArray* pExtOut )
        {
            // Required extensions
            *pExtOut = { VK_KHR_SWAPCHAIN_EXTENSION_NAME,
                         VK_KHR_MAINTENANCE1_EXTENSION_NAME,
                         VK_KHR_MAINTENANCE2_EXTENSION_NAME,
                         VK_KHR_MAINTENANCE3_EXTENSION_NAME,
                         VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME,
                         VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME,
                         VK_KHR_COPY_COMMANDS_2_EXTENSION_NAME };

            Memory::Zero( pEnableOut );

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

            auto& Props          = *pProps;
            auto& Device         = Props.Properties.Device;
            auto& Features       = Props.Features;
            auto& Device11       = Props.Features.Device11;
            auto& Device12       = Props.Features.Device12;
            auto& DeviceFeatures = Props.Features.Device.features;
            auto& Settings       = *pSettingsOut;

            Features.DynamicRendering.dynamicRendering = GetCommandLineParam< int >(
                "renderer.vk.dynamicRendering", Features.DynamicRendering.dynamicRendering );

            const auto& featureLevelKnob = GetCommandLineParam< int >( "renderer.featureLevel", Settings.featureLevel );

            auto deviceFeatureLevel = ConvertVulkanAPIToFeatureLevel( Device.properties.apiVersion );
            auto requestedLevel     = featureLevelKnob;
            if( requestedLevel == FeatureLevels::LEVEL_DEFAULT || requestedLevel > deviceFeatureLevel )
            {
                requestedLevel             = deviceFeatureLevel;
                pSettingsOut->featureLevel = deviceFeatureLevel;
            }

            pEnableOut->Device.features.robustBufferAccess = VKE_RENDER_SYSTEM_DEBUG && DeviceFeatures.robustBufferAccess;

            VkDeviceCreateInfo& Info = *pOut;
            SVulkanNext         NextFeatures( Info );

            if( requestedLevel >= FeatureLevels::LEVEL_1_0 )
            {
                pEnableOut->Device.features.geometryShader = DeviceFeatures.geometryShader;
                pEnableOut->Device.features.tessellationShader = DeviceFeatures.tessellationShader;
                pEnableOut->Device.features.fillModeNonSolid   = DeviceFeatures.fillModeNonSolid;
            }
            if( requestedLevel >= FeatureLevels::LEVEL_1_1 )
            {
                pEnableOut->Device.features.sparseBinding   = DeviceFeatures.sparseBinding;
                pEnableOut->Device.features.sparseResidencyBuffer = DeviceFeatures.sparseResidencyBuffer;
                pEnableOut->Device.features.sparseResidencyAliased = DeviceFeatures.sparseResidencyAliased;
                pEnableOut->Device.features.sparseResidencyImage2D = DeviceFeatures.sparseResidencyImage2D;
                pEnableOut->Device.features.sparseResidencyImage3D = DeviceFeatures.sparseResidencyImage3D;

                if( !Device11.shaderDrawParameters )
                {
                    VKE_LOG_ERR( "Required device feature: 'Shader Draw Parameters' is not supported." );
                    ret = VKE_FAIL;
                }
                pEnableOut->Device11.sType                = Device11.sType;
                pEnableOut->Device11.shaderDrawParameters = Device11.shaderDrawParameters;
                NextFeatures.Add( &pEnableOut->Device11 );
            }
            if( requestedLevel >= FeatureLevels::LEVEL_1_2 )
            {
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

                pEnableOut->Device12.sType                  = Device12.sType;
                pEnableOut->Device12.descriptorIndexing     = Device12.descriptorIndexing;
                pEnableOut->Device12.runtimeDescriptorArray = Device12.runtimeDescriptorArray;
                pEnableOut->Device12.timelineSemaphore      = Device12.timelineSemaphore;
                NextFeatures.Add( &pEnableOut->Device12 );

                pSettingsOut->Features.bindlessResourceAccess = (FEATURE_ENABLE_MODE)Device12.descriptorIndexing;
                pSettingsOut->Features.dynamicRenderPass =
                    (FEATURE_ENABLE_MODE)Features.DynamicRendering.dynamicRendering;
            }
            if( requestedLevel >= FeatureLevels::LEVEL_1_3 )
            {
                if( GetCommandLineParam< bool >( "rs.raytracing", true ) )
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
                const auto v = GetCommandLineParam< bool >( "rs.mesh_shaders", true );
                if( v )
                {
                    if( Features.MeshShaderNV.meshShader && Features.MeshShaderNV.taskShader )
                    {
                        pEnableOut->MeshShaderNV = Features.MeshShaderNV;
                        pExtOut->PushBack( VK_NV_MESH_SHADER_EXTENSION_NAME );
                    }

                    if( Features.MeshShaderEXT.meshShader && Features.MeshShaderEXT.taskShader )
                    {
                        pEnableOut->MeshShaderEXT                                        = Features.MeshShaderEXT;
                        pEnableOut->MeshShaderEXT.multiviewMeshShader                    = VK_FALSE;
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
            if( VKE_SUCCEEDED( ret ) )
            {
                ret = EnableDeviceExtensions( *pmExts, pEnableOut, pExtOut );
            }
            return ret;
        }

        Result CDDI::CreateDevice( const SCreateDeviceDesc& Desc, CDeviceContext* pCtx )
        {
            m_pCtx             = pCtx;
            m_pCtx->m_Features = Desc.Settings;

            auto hAdapter = m_pCtx->m_Desc.pAdapterInfo->hDDIAdapter;
            VKE_ASSERT2( hAdapter != INVALID_HANDLE, "" );
            m_hAdapter = reinterpret_cast< VkPhysicalDevice >( hAdapter );
            // VkInstance vkInstance = reinterpret_cast<VkInstance>(Desc.hAPIInstance);

            DDIExtNameArray vDDIExtNames;
            /*VKE_RETURN_IF_FAILED( LoadDeviceExtensions( m_hAdapter, &m_mExtensions ) );
            NativeAPI::DDIExtArray vRequiredExtensions = GetRequiredDeviceExtensions( false );
            VKE_RETURN_IF_FAILED(
                CheckDeviceExtensions( m_hAdapter, &vRequiredExtensions,
                    &m_mExtensions, &vDDIExtNames ) );
            VKE_RETURN_IF_FAILED( EnableDeviceExtensions(
                Desc.Settings.Features, m_mExtensions, &m_DeviceInfo.Features,
                &vRequiredExtensions ) );
            VKE_RETURN_IF_FAILED( QueryAdapterProperties( m_hAdapter,
                m_mExtensions, &m_DeviceProperties ) );*/

            // auto featureLevel = CheckRequestedFeatureLevel(m_DeviceInfo, Desc.Settings.featureLevel );

            VkDeviceCreateInfo di;
            Vulkan::InitInfo( &di, VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO );

            if( VKE_FAILED( EnableDeviceFeatures( m_hAdapter,
                                                  &m_DeviceProperties,
                                                  &m_Implementation.m_mExtensions,
                                                  &m_pCtx->m_Features,
                                                  &m_Implementation.Features,
                                                  &di,
                                                  &vDDIExtNames ) ) )
            {
                return VKE_FAIL;
            }

            for( uint32_t i = 0; i < m_DeviceProperties.Properties.Memory.memoryProperties.memoryHeapCount; ++i )
            {
                m_Implementation.m_aHeapSizes[ i ] =
                    m_DeviceProperties.Properties.Memory.memoryProperties.memoryHeaps[ i ].size;
            }

            Utils::TCDynamicArray< VkDeviceQueueCreateInfo > vQis;
            for( auto& Family: m_DeviceProperties.vQueueFamilies )
            {
                if( !Family.vQueues.IsEmpty() )
                {
                    VkDeviceQueueCreateInfo qi;
                    Vulkan::InitInfo( &qi, VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO );
                    qi.flags            = 0;
                    qi.pQueuePriorities = &Family.vPriorities[ 0 ];
                    qi.queueFamilyIndex = Family.index;
                    qi.queueCount       = static_cast< uint32_t >( Family.vQueues.GetCount() );
                    vQis.PushBack( qi );
                }
            }
            m_DeviceProperties.Features.Device.features.fillModeNonSolid = true;

            di.enabledExtensionCount   = vDDIExtNames.GetCount();
            di.enabledLayerCount       = 0;
            di.pEnabledFeatures        = &m_Implementation.Features.Device.features;
            di.ppEnabledExtensionNames = vDDIExtNames.GetData();
            di.ppEnabledLayerNames     = nullptr;
            di.pQueueCreateInfos       = &vQis[ 0 ];
            di.queueCreateInfoCount    = static_cast< uint32_t >( vQis.GetCount() );
            di.flags                   = 0;

            VK_ERR( NativeAPI::SImplementation::sInstanceICD.vkCreateDevice( m_hAdapter, &di, nullptr, &m_hDevice ) );

            VKE_RETURN_IF_FAILED( Vulkan::LoadDeviceFunctions(
                m_hDevice, NativeAPI::SImplementation::sInstanceICD, &m_Implementation.m_ICD ) );

            for( SQueueFamilyInfo& Family: m_DeviceProperties.vQueueFamilies )
            {
                for( uint32_t q = 0; q < Family.vQueues.GetCount(); ++q )
                {
                    VkQueue vkQueue;
                    m_Implementation.m_ICD.vkGetDeviceQueue( m_hDevice, Family.index, q, &vkQueue );
                    Family.vQueues[ q ] = vkQueue;
                }
            }

            return VKE_OK;
        }

        void CDDI::DestroyDevice()
        {
            if( m_hDevice != NativeAPI::Null )
            {
                NativeAPI::SImplementation::sInstanceICD.vkDestroyDevice( m_hDevice, nullptr );
            }
            m_hDevice = NativeAPI::Null;
            m_pCtx    = nullptr;
        }

        Result CDDI::QueryAdapters( AdapterInfoArray* pOut )
        {
            Result   ret   = VKE_FAIL;
            uint32_t count = 0;
            VkResult vkRes = NativeAPI::SImplementation::sInstanceICD.vkEnumeratePhysicalDevices(
                NativeAPI::SImplementation::sVkInstance, &count, nullptr );
            VK_ERR( vkRes );
            if( vkRes == VK_SUCCESS )
            {
                if( count > 0 )
                {
                    svAdapters.Resize( count );
                    vkRes = NativeAPI::SImplementation::sInstanceICD.vkEnumeratePhysicalDevices(
                        NativeAPI::SImplementation::sVkInstance, &count, &svAdapters[ 0 ] );
                    VK_ERR( vkRes );
                    if( vkRes == VK_SUCCESS )
                    {
                        const uint32_t nameLen = Min( VK_MAX_PHYSICAL_DEVICE_NAME_SIZE, Constants::MAX_NAME_LENGTH );

                        for( size_t i = 0; i < svAdapters.GetCount(); ++i )
                        {
                            const auto& vkPhysicalDevice = svAdapters[ i ];

                            VkPhysicalDeviceProperties Props;
                            NativeAPI::SImplementation::sInstanceICD.vkGetPhysicalDeviceProperties( vkPhysicalDevice,
                                                                                                    &Props );
                            RenderSystem::SAdapterInfo Info = {};
                            Info.apiVersion                 = Props.apiVersion;
                            Info.deviceID                   = Props.deviceID;
                            Info.driverVersion              = Props.driverVersion;
                            Info.type        = static_cast< RenderSystem::ADAPTER_TYPE >( Props.deviceType );
                            Info.vendorID    = Props.vendorID;
                            Info.hDDIAdapter = reinterpret_cast< handle_t >( vkPhysicalDevice );
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
            Alignment.constantBufferOffset =
                static_cast< uint32_t >( m_DeviceProperties.Limits.minUniformBufferOffsetAlignment );
            Alignment.bufferCopyOffset =
                static_cast< uint32_t >( m_DeviceProperties.Limits.optimalBufferCopyOffsetAlignment );
            Alignment.bufferCopyRowPitch  = (uint32_t)m_DeviceProperties.Limits.optimalBufferCopyRowPitchAlignment;
            Alignment.memoryMap           = (uint32_t)m_DeviceProperties.Limits.minMemoryMapAlignment;
            Alignment.texelBufferOffset   = (uint32_t)m_DeviceProperties.Limits.minTexelBufferOffsetAlignment;
            Alignment.storageBufferOffset = (uint32_t)m_DeviceProperties.Limits.minStorageBufferOffsetAlignment;

            auto& Binding                        = Limits.Binding;
            Binding.maxConstantBufferRange       = m_DeviceProperties.Limits.maxUniformBufferRange;
            Binding.maxPushConstantsSize         = m_DeviceProperties.Limits.maxPushConstantsSize;
            Binding.Stage.maxConstantBufferCount = m_DeviceProperties.Limits.maxPerStageDescriptorUniformBuffers;
            Binding.Stage.maxSamplerCount        = m_DeviceProperties.Limits.maxPerStageDescriptorSamplers;
            Binding.Stage.maxStorageBufferCount  = m_DeviceProperties.Limits.maxPerStageDescriptorStorageBuffers;
            Binding.Stage.maxStorageTextureCount = m_DeviceProperties.Limits.maxPerStageDescriptorStorageImages;
            Binding.Stage.maxResourceCount       = m_DeviceProperties.Limits.maxPerStageResources;
            Binding.Stage.maxTextureCount        = m_DeviceProperties.Limits.maxPerStageDescriptorSampledImages;

            auto& Memory                         = Limits.Memory;
            Memory.maxAllocationCount            = m_DeviceProperties.Limits.maxMemoryAllocationCount;
            Memory.minMapAlignment               = (uint32_t)m_DeviceProperties.Limits.minMemoryMapAlignment;
            Memory.minTexelBufferOffsetAlignment = (uint32_t)m_DeviceProperties.Limits.minTexelBufferOffsetAlignment;
            Memory.minConstantBufferOffsetAlignment =
                (uint32_t)m_DeviceProperties.Limits.minUniformBufferOffsetAlignment;
            Memory.minStorageBufferOffsetAlignment =
                (uint32_t)m_DeviceProperties.Limits.minStorageBufferOffsetAlignment;

            // Get heaps for GPU, CPU and Upload

            for( uint32_t i = 0; i < VK_MAX_MEMORY_HEAPS; ++i )
            {
                HeapMap.IndexToType[ i ] = MemoryHeapTypes::OTHER;
            }
            for( uint32_t i = 0; i < MemoryHeapTypes::_MAX_COUNT; ++i )
            {
                HeapMap.TypeToIndex[ i ] = VK_MAX_MEMORY_HEAPS - 1;
            }
            /// TODO: enable support other heap types
            {
                VkMemoryPropertyFlags vkPropertyFlags =
                    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
                // Convert::MemoryUsagesToVkMemoryPropertyFlags( MemoryUsages::GPU_ACCESS | MemoryUsages::CPU_ACCESS );
                const auto&   VkMemProps = m_DeviceProperties.Properties.Memory.memoryProperties;
                const int32_t idx        = FindMemoryTypeIndex( &VkMemProps, UINT32_MAX, vkPropertyFlags );
                // Memory.aHeapSizes[ MemoryHeapTypes::CPU_COHERENT ] = 0;
                HeapMap.TypeToIndex[ MemoryHeapTypes::CPU_COHERENT ] = INVALID_POSITION;
                if( idx >= 0 )
                {
                    const auto heapIdx                                   = VkMemProps.memoryTypes[ idx ].heapIndex;
                    HeapMap.TypeToIndex[ MemoryHeapTypes::CPU_COHERENT ] = heapIdx;
                    HeapMap.IndexToType[ heapIdx ]                       = MemoryHeapTypes::CPU_COHERENT;
                }
            }
            {
                VkMemoryPropertyFlags vkPropertyFlags =
                    VK_MEMORY_PROPERTY_HOST_CACHED_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
                // Convert::MemoryUsagesToVkMemoryPropertyFlags( MemoryUsages::GPU_ACCESS | MemoryUsages::CPU_ACCESS );
                const auto&   VkMemProps = m_DeviceProperties.Properties.Memory.memoryProperties;
                const int32_t idx        = FindMemoryTypeIndex( &VkMemProps, UINT32_MAX, vkPropertyFlags );
                // Memory.aHeapSizes[ MemoryHeapTypes::CPU_CACHED ] = 0;
                HeapMap.TypeToIndex[ MemoryHeapTypes::CPU_CACHED ] = INVALID_POSITION;
                if( idx >= 0 )
                {
                    const auto heapIdx                                 = VkMemProps.memoryTypes[ idx ].heapIndex;
                    HeapMap.TypeToIndex[ MemoryHeapTypes::CPU_CACHED ] = heapIdx;
                    HeapMap.IndexToType[ heapIdx ]                     = MemoryHeapTypes::CPU_CACHED;
                }
            }
            {
                VkMemoryPropertyFlags vkPropertyFlags =
                    VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
                // Convert::MemoryUsagesToVkMemoryPropertyFlags( MemoryUsages::GPU_ACCESS | MemoryUsages::CPU_ACCESS );
                const auto&   VkMemProps = m_DeviceProperties.Properties.Memory.memoryProperties;
                const int32_t idx        = FindMemoryTypeIndex( &VkMemProps, UINT32_MAX, vkPropertyFlags );
                // Memory.aHeapSizes[ MemoryHeapTypes::OTHER ] = 0;
                // HeapMap.TypeToIndex[ MemoryHeapTypes::OTHER ] = idx;
                if( idx >= 0 )
                {
                    const auto heapIdx             = VkMemProps.memoryTypes[ idx ].heapIndex;
                    HeapMap.IndexToType[ heapIdx ] = MemoryHeapTypes::OTHER;
                }
            }
            // Note that order of these queries matters as there can be the same heapIndex
            // for the same heap type. In that case we need to override with more generic ones like CPU or GPU.
            {
                VkMemoryPropertyFlags vkPropertyFlags =
                    Convert::MemoryUsagesToVkMemoryPropertyFlags( MemoryUsages::GPU_ACCESS );
                const auto&   VkMemProps = m_DeviceProperties.Properties.Memory.memoryProperties;
                const int32_t idx        = FindMemoryTypeIndex( &VkMemProps, UINT32_MAX, vkPropertyFlags );
                // Memory.aHeapSizes[ MemoryHeapTypes::GPU ] = 0;
                HeapMap.TypeToIndex[ MemoryHeapTypes::GPU ] = INVALID_POSITION;
                if( idx >= 0 )
                {
                    const auto heapIdx                          = VkMemProps.memoryTypes[ idx ].heapIndex;
                    HeapMap.TypeToIndex[ MemoryHeapTypes::GPU ] = heapIdx;
                    HeapMap.IndexToType[ heapIdx ]              = MemoryHeapTypes::GPU;
                }
            }
            {
                VkMemoryPropertyFlags vkPropertyFlags =
                    Convert::MemoryUsagesToVkMemoryPropertyFlags( MemoryUsages::CPU_ACCESS );
                const auto&   VkMemProps = m_DeviceProperties.Properties.Memory.memoryProperties;
                const int32_t idx        = FindMemoryTypeIndex( &VkMemProps, UINT32_MAX, vkPropertyFlags );
                // Memory.aHeapSizes[ MemoryHeapTypes::CPU ] = 0;
                HeapMap.TypeToIndex[ MemoryHeapTypes::CPU ] = INVALID_POSITION;
                if( idx >= 0 )
                {
                    const auto heapIdx                          = VkMemProps.memoryTypes[ idx ].heapIndex;
                    HeapMap.TypeToIndex[ MemoryHeapTypes::CPU ] = heapIdx;
                    HeapMap.IndexToType[ heapIdx ]              = MemoryHeapTypes::CPU;
                }
            }
            {
                VkMemoryPropertyFlags vkPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
                                                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                                        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
                // Convert::MemoryUsagesToVkMemoryPropertyFlags( MemoryUsages::GPU_ACCESS | MemoryUsages::CPU_ACCESS );
                const auto&   VkMemProps = m_DeviceProperties.Properties.Memory.memoryProperties;
                const int32_t idx        = FindMemoryTypeIndex( &VkMemProps, UINT32_MAX, vkPropertyFlags );
                HeapMap.TypeToIndex[ MemoryHeapTypes::UPLOAD ] = INVALID_POSITION;
                if( idx >= 0 )
                {
                    const auto heapIdx                             = VkMemProps.memoryTypes[ idx ].heapIndex;
                    HeapMap.TypeToIndex[ MemoryHeapTypes::UPLOAD ] = heapIdx;
                    HeapMap.IndexToType[ heapIdx ]                 = MemoryHeapTypes::UPLOAD;
                }
            }

            auto& RenderPass                     = Limits.RenderPass;
            RenderPass.maxColorRenderTargetCount = m_DeviceProperties.Limits.maxColorAttachments;

            auto& Query           = Limits.Query;
            Query.timestampPeriod = m_DeviceProperties.Limits.timestampPeriod;
        }

        uint32_t CalcAlignedSize( uint32_t size, uint32_t alignment )
        {
            uint32_t ret       = size;
            uint32_t remainder = size % alignment;
            if( remainder > 0 )
            {
                ret = size + alignment - remainder;
            }

            return ret;
        }

        /*void CDDI::UpdateDesc( SBufferDesc* pInOut )
        {
            if( pInOut->usage & BufferUsages::READ_ONLY_BUFFER ||
                pInOut->usage & BufferUsages::UNIFORM_TEXEL_BUFFER )
            {
                pInOut->size = CalcAlignedSize( pInOut->size, static_cast<uint32_t>(
        m_DeviceProperties.Limits.minUniformBufferOffsetAlignment ) );
            }
        }*/

        NativeAPI::Buffer CDDI::CreateBuffer( const SBufferDesc& Desc, const SBindMemoryInfo& MemInfo )
        {
            VKE_ASSERT( MemInfo.hDDIMemory != NativeAPI::Null );
            VKE_ASSERT( MemInfo.reserved != INVALID_HANDLE );
            NativeAPI::Buffer hNativeBuffer = NativeAPI::Null;
            {
                hNativeBuffer = reinterpret_cast< NativeAPI::Buffer >( MemInfo.reserved );
                auto vkRes    = m_Implementation.m_ICD.vkBindBufferMemory(
                    m_hDevice, hNativeBuffer, MemInfo.hDDIMemory, MemInfo.offset );
                if( vkRes == VK_SUCCESS )
                {
                    VKE_ASSERT2( strlen( Desc.GetDebugName() ) > 0, "Debug name must be set in Debug mode" );
                    SetObjectDebugName( (uint64_t)MemInfo.reserved, VK_OBJECT_TYPE_BUFFER, Desc.GetDebugName() );
                }
                else
                {
                    m_Implementation.m_ICD.vkDestroyBuffer( m_hDevice, hNativeBuffer, nullptr );
                    hNativeBuffer = NativeAPI::Null;
                }
            }
            return hNativeBuffer;
        }

        void CDDI::DestroyBuffer( NativeAPI::Buffer* phBuffer, const void* pAllocator )
        {
            DDI_DESTROY_OBJECT( Buffer, phBuffer, pAllocator );
        }

        NativeAPI::BufferView CDDI::CreateBufferView( const SBufferViewDesc& Desc, const void* pAllocator )
        {
            NativeAPI::BufferView  hView = NativeAPI::Null;
            VkBufferViewCreateInfo ci;
            {
                ci.sType  = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
                ci.pNext  = nullptr;
                ci.flags  = 0;
                ci.format = Map::Format( Desc.format );
                ci.buffer = m_pCtx->GetBuffer( Desc.hBuffer )->GetDDIObject();
                ci.offset = Desc.offset;
            }
            VkResult vkRes = DDI_CREATE_OBJECT( BufferView, ci, pAllocator, &hView );
            VK_ERR( vkRes );
            VKE_ASSERT2( strlen( Desc.GetDebugName() ) > 0, "Debug name must be set in Debug mode" );
            SetObjectDebugName( (uint64_t)hView, VK_OBJECT_TYPE_BUFFER_VIEW, Desc.GetDebugName() );

            return hView;
        }

        void CDDI::DestroyBufferView( NativeAPI::BufferView* phBufferView, const void* pAllocator )
        {
            DDI_DESTROY_OBJECT( BufferView, phBufferView, pAllocator );
        }

        NativeAPI::Texture CDDI::CreateTexture( const STextureDesc& Desc, const SBindMemoryInfo& MemInfo )
        {
            VKE_ASSERT( MemInfo.hDDIMemory != NativeAPI::Null );
            VKE_ASSERT( MemInfo.reserved != INVALID_HANDLE );
            NativeAPI::Texture hNativeTexture = NativeAPI::Null;
            if( MemInfo.hDDIMemory != NativeAPI::Null && MemInfo.reserved != INVALID_HANDLE )
            {
                hNativeTexture = reinterpret_cast<NativeAPI::Texture>(MemInfo.reserved);
                auto               res            = m_Implementation.m_ICD.vkBindImageMemory(
                    m_hDevice, hNativeTexture, MemInfo.hDDIMemory, MemInfo.offset );
                VK_ERR( res );
                if( res == VK_SUCCESS )
                {

#if VKE_RENDER_SYSTEM_DEBUG
                    // VKE_ASSERT2( strlen( Desc.GetDebugName() ) > 0, "Debug name must be set in Debug mode" );
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
                    SetObjectDebugName( MemInfo.reserved, VK_OBJECT_TYPE_IMAGE, pName );
#endif
                }
                else
                {
                    m_Implementation.m_ICD.vkDestroyImage( m_hDevice, hNativeTexture, nullptr );
                }
            }
            return hNativeTexture;
        }

        Result CDDI::GetTextureFormatProperties( const STextureDesc& Desc, STextureFormatProperties* pOut )
        {
            Result                           ret              = VKE_OK;
            VkPhysicalDeviceImageFormatInfo2 NativeFormatInfo = {
                .sType  = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_FORMAT_INFO_2,
                .pNext  = nullptr,
                .format = Map::Format( Desc.format ),
                .type   = Map::ImageType( Desc.type ),
                .tiling = Convert::ImageUsageToTiling( Desc.usage ),
                .usage  = Map::ImageUsage( Desc.usage ),
                .flags  = 0
            };

            VkImageFormatProperties2 NativeProperties = { .sType = VK_STRUCTURE_TYPE_IMAGE_FORMAT_PROPERTIES_2,
                                                          .pNext = nullptr };
            auto nativeResult = NativeAPI::SImplementation::sInstanceICD.vkGetPhysicalDeviceImageFormatProperties2(
                m_hAdapter, &NativeFormatInfo, &NativeProperties );
            VK_ERR( nativeResult );
            pOut->MaxSize            = { NativeProperties.imageFormatProperties.maxExtent.width };
            pOut->maxDepth           = NativeProperties.imageFormatProperties.maxExtent.depth;
            pOut->maxMipLevelCount   = NativeProperties.imageFormatProperties.maxMipLevels;
            pOut->maxArrayLayerCount = NativeProperties.imageFormatProperties.maxArrayLayers;
            pOut->maxResourceSize    = (uint32_t)NativeProperties.imageFormatProperties.maxResourceSize;
            ret                      = Map::NativeResult( nativeResult );
            return ret;
        }

        void CDDI::DestroyTexture( NativeAPI::Texture* phImage, const void* pAllocator )
        {
            DDI_DESTROY_OBJECT( Image, phImage, pAllocator );
        }

        NativeAPI::TextureView CDDI::CreateTextureView( const STextureViewDesc& Desc, const void* pAllocator )
        {
            static const VkComponentMapping DefaultMapping = {
                VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A
            };
            NativeAPI::TextureView hView = NativeAPI::Null;
            VkImageViewCreateInfo  ci;
            {
                Convert::TextureSubresourceRange( &ci.subresourceRange, Desc.SubresourceRange );
                VKE_ASSERT2( Desc.hTexture != INVALID_HANDLE, "" );
                TextureRefPtr pTex = m_pCtx->GetTexture( Desc.hTexture );
                VKE_ASSERT2( pTex!= nullptr, "" );
                ci.components = DefaultMapping;
                ci.flags      = 0;
                ci.format     = Map::Format( Desc.format );
                ci.image      = pTex->GetDDIObject();
                ci.pNext      = nullptr;
                ci.sType      = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                ci.viewType   = Map::ImageViewType( Desc.type );
            }

            VkResult vkRes = DDI_CREATE_OBJECT( ImageView, ci, pAllocator, &hView );
            VK_ERR( vkRes );

#if VKE_RENDER_SYSTEM_DEBUG
            VKE_ASSERT2( strlen( Desc.GetDebugName() ) > 0, "Debug name must be set in Debug mode" );
            SetObjectDebugName( (uint64_t)hView, VK_OBJECT_TYPE_IMAGE_VIEW, Desc.GetDebugName() );
#endif

            return hView;
        }

        void CDDI::DestroyTextureView( NativeAPI::TextureView* phImageView, const void* pAllocator )
        {
            DDI_DESTROY_OBJECT( ImageView, phImageView, pAllocator );
        }

        NativeAPI::Framebuffer CDDI::CreateFramebuffer( const SFramebufferDesc& Desc, const void* pAllocator )
        {
            // const uint32_t attachmentCount = Desc.vDDIAttachments.GetCount();

            VkFramebufferCreateInfo ci;
            ci.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            ci.pNext           = nullptr;
            ci.flags           = 0;
            ci.width           = Desc.Size.width;
            ci.height          = Desc.Size.height;
            ci.layers          = 1;
            ci.attachmentCount = Desc.vDDIAttachments.GetCount();
            ci.pAttachments    = Desc.vDDIAttachments.GetData();
            ci.renderPass      = (VkRenderPass)Desc.hRenderPass->hNativeRenderPass;
            // ci.renderPass = m_pCtx->GetRenderPass( Desc.hRenderPass )->GetDDIObject();

            NativeAPI::Framebuffer hFramebuffer = NativeAPI::Null;
            VkResult               vkRes        = DDI_CREATE_OBJECT( Framebuffer, ci, pAllocator, &hFramebuffer );
            VK_ERR( vkRes );

            VKE_ASSERT2( strlen( Desc.GetDebugName() ) > 0, "Debug name must be set in Debug mode" );
            SetObjectDebugName( (uint64_t)hFramebuffer, VK_OBJECT_TYPE_FRAMEBUFFER, Desc.GetDebugName() );

            return hFramebuffer;
        }

        void CDDI::DestroyFramebuffer( NativeAPI::Framebuffer* phFramebuffer, const void* pAllocator )
        {
            DDI_DESTROY_OBJECT( Framebuffer, phFramebuffer, pAllocator );
        }

        NativeAPI::CPUFence CDDI::CreateFence( const SFenceDesc& Desc, const void* pAllocator ) const
        {
            VkFenceCreateInfo ci;
            ci.sType                 = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            ci.pNext                 = nullptr;
            ci.flags                 = Desc.startValue > 0;
            NativeAPI::CPUFence hObj = NativeAPI::Null;
            VkResult            res  = DDI_CREATE_OBJECT( Fence, ci, pAllocator, &hObj );
            VK_ERR( res );
            Helper::SetObjectDebugName( this, hObj, VK_OBJECT_TYPE_FENCE, Desc );
            return hObj;
        }

        NativeAPI::Fence CDDI::CreateFence2( const SFenceDesc& Desc ) const
        {
            VKE_ASSERT( Desc.IsDebugNameEmpty() == false );
            NativeAPI::SFence* pFence = nullptr;
            
            if( VKE_SUCCEEDED( Memory::CreateObject( &HeapAllocator, &pFence ) ) )
            {
                if( VKE_FAILED( pFence->Create( this, Desc, m_Implementation.Features.Device12.timelineSemaphore ) ) )
                {
                    Memory::DestroyObject( &HeapAllocator, &pFence );
                }
            }
            return pFence;
        }

        void CDDI::DestroyFence( NativeAPI::CPUFence* phFence, const void* pAllocator )
        {
            DDI_DESTROY_OBJECT( Fence, phFence, pAllocator );
        }

        void CDDI::DestroyFence( NativeAPI::Fence* phFence )
        {
            NativeAPI::Fence pFence = *phFence;
            auto&            vFences = pFence->vFences;
            for( uint32_t i = 0; i < vFences.GetCount(); ++i )
            {
                DestroyFence( &vFences[ i ].hFence, nullptr );
                DestroySemaphore( &vFences[ i ].hSemaphore, nullptr );
            }
            Memory::DestroyObject( &HeapAllocator, phFence );
        }

        NativeAPI::GPUFence CDDI::CreateSemaphore( const SSemaphoreDesc& Desc, const void* pAllocator ) const
        {
            NativeAPI::GPUFence   hSemaphore = NativeAPI::Null;
            
            VkSemaphoreCreateInfo ci;
            ci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            ci.pNext = nullptr;
            ci.flags = 0;
            
            SVulkanNext Next( ci );
            if( Desc.startValue != UNDEFINED_U64 )
            {
                VkSemaphoreTypeCreateInfo TypeCi;
                TypeCi.sType         = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
                TypeCi.pNext         = nullptr;
                TypeCi.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
                TypeCi.initialValue  = Desc.startValue;
                Next.Add( &TypeCi );
            }
            VK_ERR( DDI_CREATE_OBJECT( Semaphore, ci, pAllocator, &hSemaphore ) );
            Helper::SetObjectDebugName( this, hSemaphore, VK_OBJECT_TYPE_SEMAPHORE, Desc );
            return hSemaphore;
        }

        void CDDI::DestroySemaphore( NativeAPI::GPUFence* phSemaphore, const void* pAllocator )
        {
            DDI_DESTROY_OBJECT( Semaphore, phSemaphore, pAllocator );
        }

        NativeAPI::CommandBufferPool CDDI::CreateCommandBufferPool( const SCommandBufferPoolDesc& Desc,
                                                                    const void*                   pAllocator )
        {
            NativeAPI::CommandBufferPool hPool = NativeAPI::Null;
            VkCommandPoolCreateInfo      ci;
            ci.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            ci.pNext            = nullptr;
            ci.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            ci.queueFamilyIndex = Desc.pContext->m_pQueue->GetFamilyIndex();
            VkResult res        = DDI_CREATE_OBJECT( CommandPool, ci, pAllocator, &hPool );
            VK_ERR( res );
            return hPool;
        }

        void CDDI::DestroyCommandBufferPool( NativeAPI::CommandBufferPool* phPool, const void* pAllocator )
        {
            DDI_DESTROY_OBJECT( CommandPool, phPool, pAllocator );
        }

        static int32_t FindTextureHandle( const SRenderPassDesc::AttachmentDescArray& vAttachments,
                                          const TextureViewHandle&                    hTexView )
        {
            int32_t res = -1;
            for( uint32_t a = 0; a < vAttachments.GetCount(); ++a )
            {
                if( hTexView == vAttachments[ a ].hTextureView )
                {
                    res = a;
                    break;
                }
            }
            return res;
        }

        static bool MakeAttachmentRef( const SRenderPassDesc::AttachmentDescArray& vAttachments,
                                       const SSubpassAttachmentDesc&               SubpassAttachmentDesc,
                                       VkAttachmentReference*                      pRefOut )
        {
            int32_t idx = FindTextureHandle( vAttachments, SubpassAttachmentDesc.hTextureView );
            bool    res = false;
            if( idx >= 0 )
            {
                pRefOut->attachment = idx;
                pRefOut->layout     = Map::ImageLayout( SubpassAttachmentDesc.state );
                res                 = true;
            }
            return res;
        }

        NativeAPI::RenderPass CDDI::CreateRenderPass( const SRenderPassDesc& Desc, const void* )
        {
            NativeAPI::RenderPass pPass = NativeAPI::Null;
            if( VKE_FAILED( Memory::CreateObject( &HeapAllocator, &pPass ) ) )
            {
                return NativeAPI::Null;
            }
            
            if( m_Implementation.Features.DynamicRendering.dynamicRendering )
            {
                auto& BeginInfo = pPass->VkInfo;
                BeginInfo       = {};
                BeginInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
                BeginInfo.viewMask = 0;
                BeginInfo.layerCount = 1;
                Convert::RenderSystemToVkRect2D( Desc.PositionOffset, Desc.Size, &BeginInfo.renderArea );
                pPass->SetDebugName( Desc.GetDebugName() );

                for( uint32_t a = 0; a < Desc.vRenderTargets.GetCount(); ++a )
                {
                    const SRenderPassAttachmentDesc& AttachmentDesc = Desc.vRenderTargets[ a ];
                    VKE_ASSERT( AttachmentDesc.hNativeView != NativeAPI::Null );
                    

                    VkRenderingAttachmentInfo Info;
                    Info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
                    Info.pNext = nullptr;
                    Convert::ClearValues( &AttachmentDesc.ClearValue, 1, &Info.clearValue );
                    Info.imageLayout = Map::ImageLayout( AttachmentDesc.beginState );
                    Info.imageView   = AttachmentDesc.hNativeView;
                    Info.loadOp      = Convert::UsageToLoadOp( AttachmentDesc.usage );
                    Info.storeOp     = Convert::UsageToStoreOp( AttachmentDesc.usage );
                    Info.resolveMode = VK_RESOLVE_MODE_NONE;
                    Info.resolveImageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
                    Info.resolveImageView   = NativeAPI::Null;
                    /// TODO: handle resolves
                    Info.resolveImageView                           = NativeAPI::Null;

                    NativeAPI::Format nativeFormat = Map::Format( AttachmentDesc.format );

                    if( IsDepthFormat( AttachmentDesc.format ) )
                    {
                        pPass->VkDepthRenderTarget = Info;
                        pPass->VkInfo.pDepthAttachment = &pPass->VkDepthRenderTarget;
                        pPass->VkDepthRenderTargetFormat = nativeFormat;
                    }
                    else if( IsStencilFormat( AttachmentDesc.format ) )
                    {
                        pPass->VkStencilRenderTarget = Info;
                        pPass->VkInfo.pStencilAttachment = &pPass->VkStencilRenderTarget;
                        pPass->VkStencilRenderTargetFormat = nativeFormat;
                    }
                    else if( IsDepthStencilFormat( AttachmentDesc.format ) )
                    {
                        pPass->VkDepthRenderTarget = Info;
                        pPass->VkStencilRenderTarget = Info;
                        pPass->VkDepthRenderTargetFormat = nativeFormat;
                        pPass->VkStencilRenderTargetFormat = nativeFormat;
                    }
                    else
                    {    
                        BeginInfo.colorAttachmentCount++;
                        pPass->vColorRenderTargets.PushBack( Info );
                        pPass->vColorRenderTargetFormats.PushBack( nativeFormat );
                    }
                }
                pPass->VkInfo.pColorAttachments = pPass->vColorRenderTargets.GetData();
            }
            else
            {
                using VkAttachmentDescriptionArray = Utils::TCDynamicArray< VkAttachmentDescription, 8 >;
                using VkAttachmentRefArray         = Utils::TCDynamicArray< VkAttachmentReference >;

                struct SSubpassDesc
                {
                    VkAttachmentRefArray   vInputAttachmentRefs;
                    VkAttachmentRefArray   vColorAttachmentRefs;
                    VkAttachmentReference  vkDepthStencilRef;
                    VkAttachmentReference* pVkDepthStencilRef = nullptr;
                };

                using SubpassDescArray   = Utils::TCDynamicArray< SSubpassDesc >;
                using VkSubpassDescArray = Utils::TCDynamicArray< VkSubpassDescription >;
                using VkClearValueArray  = Utils::TCDynamicArray< VkClearValue >;
                using VkImageViewArray   = Utils::TCDynamicArray< VkImageView >;

                VkAttachmentDescriptionArray vVkAttachmentDescriptions;
                SubpassDescArray             vSubpassDescs;
                VkSubpassDescArray           vVkSubpassDescs;
                // VkClearValueArray vVkClearValues;

                for( uint32_t a = 0; a < Desc.vRenderTargets.GetCount(); ++a )
                {
                    const SRenderPassAttachmentDesc& AttachmentDesc = Desc.vRenderTargets[ a ];
                    // const VkImageCreateInfo& vkImgInfo = ResMgr.GetTextureDesc( AttachmentDesc.hTextureView );
                    VkAttachmentDescription vkAttachmentDesc;
                    vkAttachmentDesc.finalLayout    = Map::ImageLayout( AttachmentDesc.endState );
                    vkAttachmentDesc.flags          = 0;
                    vkAttachmentDesc.format         = Map::Format( AttachmentDesc.format );
                    vkAttachmentDesc.initialLayout  = Map::ImageLayout( AttachmentDesc.beginState );
                    vkAttachmentDesc.loadOp         = Convert::UsageToLoadOp( AttachmentDesc.usage );
                    vkAttachmentDesc.storeOp        = Convert::UsageToStoreOp( AttachmentDesc.usage );
                    vkAttachmentDesc.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                    vkAttachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                    vkAttachmentDesc.samples        = Map::SampleCount( AttachmentDesc.sampleCount );
                    vVkAttachmentDescriptions.PushBack( vkAttachmentDesc );
                }

                VkAttachmentReference* pVkDepthStencilRef = nullptr;
                VkAttachmentReference  VkDepthStencilRef;

                if( Desc.vSubpasses.IsEmpty() )
                {
                    SRenderPassDesc::SSubpassDesc PassDesc;
                    SSubpassDesc                  SubDesc;
                    VkSubpassDescription          VkSubpassDesc;

                    for( uint32_t i = 0; i < Desc.vRenderTargets.GetCount(); ++i )
                    {
                        auto& Curr = Desc.vRenderTargets[ i ];
                        bool  isDepthBuffer =
                            Curr.format == Formats::D24_UNORM_S8_UINT || Curr.format == Formats::X8_D24_UNORM_PACK32 ||
                            Curr.format == Formats::D32_SFLOAT_S8_UINT || Curr.format == Formats::D32_SFLOAT;
                        if( isDepthBuffer )
                        {
                            VkDepthStencilRef.attachment = i;
                            VkDepthStencilRef.layout     = Map::ImageLayout( Curr.beginState );
                            pVkDepthStencilRef           = &VkDepthStencilRef;
                        }
                        else
                        {
                            // Find attachment
                            VkAttachmentReference vkRef;
                            vkRef.attachment = i;
                            vkRef.layout     = Map::ImageLayout( Curr.beginState );
                            SubDesc.vColorAttachmentRefs.PushBack( vkRef );
                        }
                    }

                    VkSubpassDesc.colorAttachmentCount    = SubDesc.vColorAttachmentRefs.GetCount();
                    VkSubpassDesc.inputAttachmentCount    = 0;
                    VkSubpassDesc.pColorAttachments       = SubDesc.vColorAttachmentRefs.GetData();
                    VkSubpassDesc.pDepthStencilAttachment = pVkDepthStencilRef;
                    VkSubpassDesc.pInputAttachments       = nullptr;
                    VkSubpassDesc.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
                    VkSubpassDesc.pPreserveAttachments    = nullptr;
                    VkSubpassDesc.pResolveAttachments     = nullptr;
                    VkSubpassDesc.preserveAttachmentCount = 0;
                    VkSubpassDesc.flags                   = 0;

                    vVkSubpassDescs.PushBack( VkSubpassDesc );
                }

                for( uint32_t s = 0; s < Desc.vSubpasses.GetCount(); ++s )
                {
                    SSubpassDesc SubDesc;

                    const auto& SubpassDesc = Desc.vSubpasses[ s ];
                    for( uint32_t r = 0; r < SubpassDesc.vRenderTargets.GetCount(); ++r )
                    {
                        const auto& RenderTargetDesc = SubpassDesc.vRenderTargets[ r ];

                        // Find attachment
                        VkAttachmentReference vkRef;
                        if( MakeAttachmentRef( Desc.vRenderTargets, RenderTargetDesc, &vkRef ) )
                        {
                            SubDesc.vColorAttachmentRefs.PushBack( vkRef );
                        }
                    }

                    for( uint32_t t = 0; t < SubpassDesc.vTextures.GetCount(); ++t )
                    {
                        const auto& TexDesc = SubpassDesc.vTextures[ t ];
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
                            SubDesc.vkDepthStencilRef  = vkRef;
                            SubDesc.pVkDepthStencilRef = &SubDesc.vkDepthStencilRef;
                        }
                    }

                    VkSubpassDescription VkSubpassDesc;
                    const auto           colorCount = SubDesc.vColorAttachmentRefs.GetCount();
                    const auto           inputCount = SubDesc.vInputAttachmentRefs.GetCount();

                    VkSubpassDesc.colorAttachmentCount = colorCount;
                    VkSubpassDesc.pColorAttachments = ( colorCount > 0 ) ? &SubDesc.vColorAttachmentRefs[ 0 ] : nullptr;
                    VkSubpassDesc.inputAttachmentCount = inputCount;
                    VkSubpassDesc.pInputAttachments = ( inputCount > 0 ) ? &SubDesc.vInputAttachmentRefs[ 0 ] : nullptr;
                    VkSubpassDesc.pDepthStencilAttachment = pVkDepthStencilRef;
                    VkSubpassDesc.flags                   = 0;
                    VkSubpassDesc.pResolveAttachments     = nullptr;
                    VkSubpassDesc.preserveAttachmentCount = 0;
                    VkSubpassDesc.pPreserveAttachments    = nullptr;
                    VkSubpassDesc.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;

                    vSubpassDescs.PushBack( SubDesc );
                    vVkSubpassDescs.PushBack( VkSubpassDesc );
                }

                {
                    VkRenderPassCreateInfo ci;
                    ci.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
                    ci.pNext           = nullptr;
                    ci.attachmentCount = vVkAttachmentDescriptions.GetCount();
                    ci.pAttachments    = &vVkAttachmentDescriptions[ 0 ];
                    ci.dependencyCount = 0;
                    ci.pDependencies   = nullptr;
                    ci.subpassCount    = vVkSubpassDescs.GetCount();
                    ci.pSubpasses      = &vVkSubpassDescs[ 0 ];
                    ci.flags           = 0;
                    VkResult res       = m_Implementation.m_ICD.vkCreateRenderPass( m_hDevice, &ci, nullptr, &pPass->hNativeRenderPass );
                    VK_ERR( res );
                    SetObjectDebugName( (uint64_t)pPass->hNativeRenderPass, VK_OBJECT_TYPE_RENDER_PASS, Desc.GetDebugName() );
                }
                {
                    pPass->vNativeClearColors.Resize( Desc.vRenderTargets.GetCount() );
                    for( uint32_t i = 0; i < Desc.vRenderTargets.GetCount(); ++i )
                    {
                        Convert::ClearValues( &Desc.vRenderTargets[ i ].ClearValue, 1, &pPass->vNativeClearColors[ i ] );
                    }

                    Utils::TCDynamicArray< VkImageView, 8 > vNativeViews;
                    for( uint32_t i = 0; i < Desc.vRenderTargets.GetCount(); ++i )
                    {
                        vNativeViews.PushBack( Desc.vRenderTargets[ i ].hNativeView );
                    }
                    VkFramebufferCreateInfo FbCi;
                    FbCi.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                    FbCi.pNext                   = nullptr;
                    FbCi.attachmentCount         = vNativeViews.GetCount();
                    FbCi.pAttachments            = vNativeViews.GetData();
                    FbCi.flags                   = 0;
                    FbCi.layers                  = 1;
                    FbCi.width                   = Desc.Size.width;
                    FbCi.height                  = Desc.Size.height;
                    FbCi.renderPass              = pPass->hNativeRenderPass;
                    VkResult res                 = m_Implementation.m_ICD.vkCreateFramebuffer(
                        m_hDevice, &FbCi, nullptr, &pPass->hNativeFramebuffer );
                    if( res == VK_SUCCESS )
                    {
                        auto& bi             = pPass->NativeBeginInfo;
                        bi.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                        bi.pNext             = nullptr;
                        bi.clearValueCount   = pPass->vNativeClearColors.GetCount();
                        bi.pClearValues      = pPass->vNativeClearColors.GetData();
                        bi.framebuffer       = pPass->hNativeFramebuffer;
                        bi.renderArea.offset = { Desc.PositionOffset.x, Desc.PositionOffset.y };
                        bi.renderArea.extent = { Desc.Size.width, Desc.Size.height };
                        bi.renderPass        = pPass->hNativeRenderPass;
                    }
                    else
                    {
                        DestroyRenderPass( &pPass, nullptr );
                        pPass = nullptr;
                    }
                }
            }
            return pPass;
        }

        void CDDI::DestroyRenderPass( NativeAPI::RenderPass* phRenderPass, const void* pAllocator )
        {
            auto pPass = ( *phRenderPass );
            if( pPass != NativeAPI::Null && pPass->hNativeRenderPass != NativeAPI::Null )
            {
                m_Implementation.m_ICD.vkDestroyRenderPass( m_hDevice, pPass->hNativeRenderPass, nullptr );
                Memory::DestroyObject( &HeapAllocator, &pPass );
                *phRenderPass = NativeAPI::Null;
            }
        }

        NativeAPI::DescriptorPool CDDI::CreateDescriptorPool( const SDescriptorPoolDesc& Desc, const void* pAllocator )
        {
            NativeAPI::DescriptorPool  hPool = NativeAPI::Null;
            VkDescriptorPoolCreateInfo ci;
            ci.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            ci.pNext         = nullptr;
            ci.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
            ci.maxSets       = Desc.maxSetCount;
            ci.poolSizeCount = Desc.vPoolSizes.GetCount();

            Utils::TCDynamicArray< VkDescriptorPoolSize > vVkSizes;
            vVkSizes.Resize( ci.poolSizeCount );
            for( uint32_t i = 0; i < ci.poolSizeCount; ++i )
            {
                vVkSizes[ i ].descriptorCount = Desc.vPoolSizes[ i ].count;
                vVkSizes[ i ].type            = Map::DescriptorType( Desc.vPoolSizes[ i ].type );
            }
            ci.pPoolSizes = &vVkSizes[ 0 ];

            // VkResult res = m_Implementation.m_ICD.vkCreateDescriptorPool( m_hDevice, &ci, pVkAllocator,
            // &hPool );
            VkResult res = DDI_CREATE_OBJECT( DescriptorPool, ci, pAllocator, &hPool );
            VK_ERR( res );
            SetObjectDebugName( (uint64_t)hPool, VK_OBJECT_TYPE_DESCRIPTOR_POOL, Desc.GetDebugName() );
            return hPool;
        }

        void CDDI::DestroyDescriptorPool( NativeAPI::DescriptorPool* phPool, const void* pAllocator )
        {
            DDI_DESTROY_OBJECT( DescriptorPool, phPool, pAllocator );
        }

        NativeAPI::Pipeline CDDI::CreatePipeline( const SPipelineDesc& Desc, const void* pAllocator )
        {
            NativeAPI::Pipeline          hPipeline    = NativeAPI::Null;
            VkResult                     vkRes        = VK_ERROR_OUT_OF_HOST_MEMORY;
            const VkAllocationCallbacks* pVkCallbacks = reinterpret_cast< const VkAllocationCallbacks* >( pAllocator );
            VKE_ASSERT2( Desc.hDDIRenderPass != NativeAPI::Null, "RenderPass must be set" );

            // Utils::TCDynamicArray< VkPipelineColorBlendAttachmentState,
            // Config::RenderSystem::Pipeline::MAX_BLEND_STATE_COUNT > vVkBlendStates;
            const bool isGraphics = Desc.Shaders.apShaders[ ShaderTypes::COMPUTE ]== nullptr;

            VkGraphicsPipelineCreateInfo VkGraphicsInfo = {};
            VkComputePipelineCreateInfo  VkComputeInfo  = {};

            if( isGraphics )
            {
                VkGraphicsPipelineCreateInfo& ci = VkGraphicsInfo;
                ci.sType                         = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
                ci.pNext                         = nullptr;
                ci.flags                         = 0;

                VkPipelineColorBlendStateCreateInfo VkColorBlendState = {
                    VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO
                };
                {
                    auto&       State        = VkColorBlendState;
                    const auto& vBlendStates = Desc.Blending.vBlendStates;

                    Utils::TCDynamicArray< VkPipelineColorBlendAttachmentState > vVkBlendStates;
                    if( vBlendStates.IsEmpty() )
                    {
                        VKE_LOG_WARN( "No blend states specified for pipeline: " << Desc.GetDebugName() );
                        VkPipelineColorBlendAttachmentState VkState;
                        VkState.alphaBlendOp   = VK_BLEND_OP_ADD;
                        VkState.blendEnable    = VK_FALSE;
                        VkState.colorBlendOp   = VK_BLEND_OP_ADD;
                        VkState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                                 VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
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
                                auto& vkBlendState               = vVkBlendStates[ i ];
                                vkBlendState.alphaBlendOp        = Map::BlendOp( vBlendStates[ i ].Alpha.operation );
                                vkBlendState.blendEnable         = vBlendStates[ i ].enable;
                                vkBlendState.colorBlendOp        = Map::BlendOp( vBlendStates[ i ].Color.operation );
                                vkBlendState.colorWriteMask      = Map::ColorComponent( vBlendStates[ i ].writeMask );
                                vkBlendState.dstAlphaBlendFactor = Map::BlendFactor( vBlendStates[ i ].Alpha.dst );
                                vkBlendState.dstColorBlendFactor = Map::BlendFactor( vBlendStates[ i ].Color.dst );
                                vkBlendState.srcAlphaBlendFactor = Map::BlendFactor( vBlendStates[ i ].Alpha.src );
                                vkBlendState.srcColorBlendFactor = Map::BlendFactor( vBlendStates[ i ].Color.src );
                            }
                        }
                    }
                    State.pAttachments    = &vVkBlendStates[ 0 ];
                    State.attachmentCount = vVkBlendStates.GetCount();
                    State.logicOp         = Map::LogicOperation( Desc.Blending.logicOperation );
                    State.logicOpEnable   = Desc.Blending.enable;
                    memset( State.blendConstants, 0, sizeof( float ) * 4 );
                }
                ci.pColorBlendState = &VkColorBlendState;

                VkPipelineDepthStencilStateCreateInfo VkDepthStencil = {
                    VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO
                };
                {
                    auto& State = VkDepthStencil;

                    if( Desc.DepthStencil.Depth.enable )
                    {

                        State.depthBoundsTestEnable = Desc.DepthStencil.Depth.Bounds.enable;
                        State.depthCompareOp        = Map::CompareOperation( Desc.DepthStencil.Depth.compareFunc );
                        State.depthTestEnable       = Desc.DepthStencil.Depth.test;
                        State.depthWriteEnable      = Desc.DepthStencil.Depth.write;
                        State.maxDepthBounds        = Desc.DepthStencil.Depth.Bounds.max;
                        State.minDepthBounds        = Desc.DepthStencil.Depth.Bounds.min;
                    }
                    if( Desc.DepthStencil.Stencil.enable )
                    {
                        {
                            VkStencilOpState VkFace;
                            const auto&      Face = Desc.DepthStencil.Stencil.BackFace;

                            VkFace.compareMask = Face.compareMask;
                            VkFace.compareOp   = Map::CompareOperation( Face.compareFunc );
                            VkFace.depthFailOp = Map::StencilOperation( Face.depthFailFunc );
                            VkFace.failOp      = Map::StencilOperation( Face.failFunc );
                            VkFace.passOp      = Map::StencilOperation( Face.passFunc );
                            VkFace.reference   = Face.reference;
                            VkFace.writeMask   = Face.writeMask;
                        }
                        {
                            VkStencilOpState VkFace;
                            const auto&      Face = Desc.DepthStencil.Stencil.FrontFace;

                            VkFace.compareMask = Desc.DepthStencil.Stencil.BackFace.compareMask;
                            VkFace.compareOp   = Map::CompareOperation( Face.compareFunc );
                            VkFace.depthFailOp = Map::StencilOperation( Face.depthFailFunc );
                            VkFace.failOp      = Map::StencilOperation( Face.failFunc );
                            VkFace.passOp      = Map::StencilOperation( Face.passFunc );
                            VkFace.reference   = Face.reference;
                            VkFace.writeMask   = Face.writeMask;
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
                    static const VkDynamicState aVkStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
                    auto&                       State       = VkDynState;
                    State.dynamicStateCount                 = 2;
                    State.pDynamicStates                    = aVkStates;
                    ci.pDynamicState                        = &State;
                }

                VkPipelineMultisampleStateCreateInfo VkMultisampling = {
                    VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO
                };
                {
                    auto& State = VkMultisampling;
                    // if( Desc.Multisampling.enable )
                    {
                        State.alphaToCoverageEnable = false;
                        State.alphaToOneEnable      = false;
                        State.minSampleShading      = 0;
                        State.pSampleMask           = nullptr;
                        State.rasterizationSamples  = Map::SampleCount( Desc.Multisampling.sampleCount );
                        State.sampleShadingEnable   = false;

                        ci.pMultisampleState = &VkMultisampling;
                    }
                }

                VkPipelineRasterizationStateCreateInfo VkRasterization = {
                    VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO
                };
                {
                    auto& State = VkRasterization;

                    State.cullMode                = Map::CullMode( Desc.Rasterization.Polygon.cullMode );
                    State.depthBiasClamp          = Desc.Rasterization.Depth.biasClampFactor;
                    State.depthBiasConstantFactor = Desc.Rasterization.Depth.biasConstantFactor;
                    State.depthBiasEnable         = Desc.Rasterization.Depth.biasConstantFactor != 0.0f;
                    State.depthBiasSlopeFactor    = Desc.Rasterization.Depth.biasSlopeFactor;
                    State.depthClampEnable        = Desc.Rasterization.Depth.enableClamp;
                    State.frontFace               = Map::FrontFace( Desc.Rasterization.Polygon.frontFace );
                    State.lineWidth               = 1;
                    State.polygonMode             = Map::PolygonMode( Desc.Rasterization.Polygon.mode );
                    State.rasterizerDiscardEnable = VK_FALSE;

                    ci.pRasterizationState = &VkRasterization;
                }

                VkShaderStageFlags                                                                vkShaderStages = 0;
                Utils::TCDynamicArray< VkPipelineShaderStageCreateInfo, ShaderTypes::_MAX_COUNT > vVkStages;
                uint32_t                                                                          stageCount = 0;
                {
                    for( uint32_t i = 0; i < ShaderTypes::_MAX_COUNT; ++i )
                    {
                        if( Desc.Shaders.apShaders[ i ]!= nullptr )
                        {
                            auto pShader =
                                Desc.Shaders.apShaders[ i ]; // m_pCtx->GetShader( Desc.Shaders.apShaders[i] );
                            VkPipelineShaderStageCreateInfo State = {
                                VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO
                            };
                            {
                                if( VKE_FAILED( pShader->Compile() ) )
                                {
                                }
                                State.module               = pShader->GetDDIObject();
                                State.pName                = pShader->GetDesc().EntryPoint.GetData();
                                State.stage                = Map::ShaderStage( static_cast< SHADER_TYPE >( i ) );
                                State.pSpecializationInfo  = nullptr;
                                vkShaderStages            |= State.stage;
                                stageCount++;
                                vVkStages.PushBack( State );
                                VKE_LOG( "Stage: " << State.stage << ": " << State.pName );
                            }
                        }
                    }
                }

                ci.stageCount = stageCount;
                ci.pStages    = &vVkStages[ 0 ];

                VkPipelineTessellationStateCreateInfo VkTesselation = {
                    VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO
                };
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
                    State.pNext                 = &VkDomainOrigin;

                    ci.pTessellationState = &VkTesselation;
                }

                VkPipelineInputAssemblyStateCreateInfo VkInputAssembly = {
                    VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO
                };
                ci.pInputAssemblyState = nullptr;
                if( Desc.InputLayout.enable )
                {
                    auto& State = VkInputAssembly;
                    {
                        State.primitiveRestartEnable = Desc.InputLayout.enablePrimitiveRestart;
                        State.topology               = Map::PrimitiveTopology( Desc.InputLayout.topology );
                    }
                    ci.pInputAssemblyState = &VkInputAssembly;
                }

                VkPipelineVertexInputStateCreateInfo VkVertexInput = {
                    VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO
                };
                ci.pVertexInputState = nullptr;
                if( Desc.InputLayout.enable )
                {
                    auto&       State    = VkVertexInput;
                    const auto& vAttribs = Desc.InputLayout.vVertexAttributes;
                    if( !vAttribs.IsEmpty() )
                    {
                        {
                            Utils::TCDynamicArray< VkVertexInputAttributeDescription,
                                                   Config::RenderSystem::Pipeline::MAX_VERTEX_ATTRIBUTE_COUNT >
                                vVkAttribs;
                            Utils::TCDynamicArray< VkVertexInputBindingDescription,
                                                   Config::RenderSystem::Pipeline::MAX_VERTEX_INPUT_BINDING_COUNT >
                                vVkBindings;
                            vVkAttribs.Resize( vAttribs.GetCount() );
                            // vVkBindings.Resize( vAttribs.GetCount() );
                            SDescriptorSetLayoutDesc::BindingArray vBindings;
                            // vBindings.Resize( vAttribs.GetCount() );
                            uint32_t currVertexBufferBinding = UNDEFINED_U32;
                            uint32_t vertexSize              = 0;

                            for( uint32_t i = 0; i < vAttribs.GetCount(); ++i )
                            {
                                auto& vkAttrib     = vVkAttribs[ i ];
                                vkAttrib.binding   = vAttribs[ i ].vertexBufferBindingIndex;
                                vkAttrib.format    = Map::Format( vAttribs[ i ].format );
                                vkAttrib.location  = vAttribs[ i ].location;
                                vkAttrib.offset    = vAttribs[ i ].offset;
                                vertexSize        += vAttribs[ i ].stride;
                            }
                            for( uint32_t i = 0; i < vAttribs.GetCount(); ++i )
                            {
                                if( currVertexBufferBinding != vAttribs[ i ].vertexBufferBindingIndex )
                                {
                                    currVertexBufferBinding = vAttribs[ i ].vertexBufferBindingIndex;
                                    VkVertexInputBindingDescription VkBinding;
                                    VkBinding.binding   = currVertexBufferBinding;
                                    VkBinding.inputRate = Map::InputRate( vAttribs[ i ].inputRate );
                                    VkBinding.stride    = vertexSize; // todo this will cause corruption if more than 1
                                                                      // vertex buffer is used
                                    vVkBindings.PushBack( VkBinding );
                                }
                            }

                            State.pVertexAttributeDescriptions    = &vVkAttribs[ 0 ];
                            State.pVertexBindingDescriptions      = &vVkBindings[ 0 ];
                            State.vertexAttributeDescriptionCount = vVkAttribs.GetCount();
                            State.vertexBindingDescriptionCount   = vVkBindings.GetCount();
                        }
                    }

                    ci.pVertexInputState = &VkVertexInput;
                }

                VkPipelineViewportStateCreateInfo VkViewportState = {
                    VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO
                };
                if( Desc.Viewport.enable )
                {
                    auto& State = VkViewportState;
                    {
                        using VkViewportArray =
                            Utils::TCDynamicArray< VkViewport, Config::RenderSystem::Pipeline::MAX_VIEWPORT_COUNT >;
                        using VkScissorArray =
                            Utils::TCDynamicArray< VkRect2D, Config::RenderSystem::Pipeline::MAX_SCISSOR_COUNT >;
                        VkViewportArray vVkViewports;
                        VkScissorArray  vVkScissors;

                        for( uint32_t i = 0; i < Desc.Viewport.vViewports.GetCount(); ++i )
                        {
                            const auto& Viewport = Desc.Viewport.vViewports[ i ];
                            VkViewport  vkViewport;
                            vkViewport.x        = Viewport.Position.x;
                            vkViewport.y        = Viewport.Position.y;
                            vkViewport.width    = Viewport.Size.width;
                            vkViewport.height   = Viewport.Size.height;
                            vkViewport.minDepth = Viewport.MinMaxDepth.begin;
                            vkViewport.maxDepth = Viewport.MinMaxDepth.end;

                            vVkViewports.PushBack( vkViewport );
                        }

                        for( uint32_t i = 0; i < Desc.Viewport.vScissors.GetCount(); ++i )
                        {
                            const auto& Scissor = Desc.Viewport.vScissors[ i ];
                            VkRect2D    vkScissor;
                            vkScissor.extent.width  = Scissor.Size.width;
                            vkScissor.extent.height = Scissor.Size.height;
                            vkScissor.offset.x      = Scissor.Position.x;
                            vkScissor.offset.y      = Scissor.Position.y;

                            vVkScissors.PushBack( vkScissor );
                        }
                        VKE_ASSERT2( vVkViewports.GetCount() == vVkScissors.GetCount(), "" );
                        State.pViewports    = vVkViewports.GetData();
                        State.viewportCount = std::max( 1u, vVkViewports.GetCount() ); // at least one viewport
                        State.pScissors     = vVkScissors.GetData();
                        State.scissorCount  = State.viewportCount;
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
                if( Desc.hDDIRenderPass != NativeAPI::Null )
                {
                    VkGraphicsInfo.renderPass = Desc.hDDIRenderPass->hNativeRenderPass;
                }
                /*else if( Desc.hRenderPass != INVALID_HANDLE )
                {
                    VkGraphicsInfo.renderPass = m_pCtx->GetRenderPass( Desc.hRenderPass )->GetDDIObject()->VkNative;
                }*/
                if( create )
                {
                    VkPipelineRenderingCreateInfoKHR VkDynamicRenderingInfo;
                    Utils::TCDynamicArray< NativeAPI::Format > vFormats;
                    if( VkGraphicsInfo.renderPass == NativeAPI::Null )
                    {
                        VkDynamicRenderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
                        VkDynamicRenderingInfo.pNext = nullptr;
                        VkGraphicsInfo.pNext         = &VkDynamicRenderingInfo;
                        VkDynamicRenderingInfo.viewMask = 0;

                        if( Desc.hDDIRenderPass != NativeAPI::Null )
                        {
                            const auto                                    pPass = Desc.hDDIRenderPass;
                            
                            VkDynamicRenderingInfo.colorAttachmentCount = pPass->vColorRenderTargetFormats.GetCount();
                            VkDynamicRenderingInfo.pColorAttachmentFormats = pPass->vColorRenderTargetFormats.GetData();
                            VkDynamicRenderingInfo.depthAttachmentFormat   = pPass->VkDepthRenderTargetFormat;
                            VkDynamicRenderingInfo.stencilAttachmentFormat = pPass->VkStencilRenderTargetFormat;
                        }
                        else
                        {
                            vFormats = Map::Formats( Desc.vColorRenderTargetFormats.GetData(),
                                                                Desc.vColorRenderTargetFormats.GetCount() );
                            VkDynamicRenderingInfo.colorAttachmentCount    = vFormats.GetCount();
                            VkDynamicRenderingInfo.pColorAttachmentFormats = vFormats.GetDataOrNull();
                            VkDynamicRenderingInfo.depthAttachmentFormat = Map::Format( Desc.depthRenderTargetFormat );
                            VkDynamicRenderingInfo.stencilAttachmentFormat =
                                Map::Format( Desc.stencilRenderTargetFormat );
                        }
                    }
                    vkRes = m_Implementation.m_ICD.vkCreateGraphicsPipelines(
                        m_hDevice, VK_NULL_HANDLE, 1, &VkGraphicsInfo, nullptr, &hPipeline );
                }
            }
            else
            {
                VkComputePipelineCreateInfo& ci = VkComputeInfo;
                ci.sType                        = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
                ci.pNext                        = nullptr;
                ci.flags                        = 0;
                ci.basePipelineHandle           = VK_NULL_HANDLE;
                ci.basePipelineIndex            = -1;

                // auto pShader = m_pCtx->GetShader( Desc.Shaders.apShaders[ ShaderTypes::COMPUTE ] );
                auto pShader = Desc.Shaders.apShaders[ ShaderTypes::COMPUTE ];

                ci.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
                ci.stage.pName = nullptr;
                ci.stage.flags = 0;
                {
                    if( VKE_FAILED( pShader->Compile() ) )
                    {
                    }
                    ci.stage.module = pShader->GetDDIObject();
                    ci.stage.pName  = pShader->GetDesc().EntryPoint.GetData();
                    ci.stage.stage  = Map::ShaderStage( static_cast< SHADER_TYPE >( ShaderTypes::COMPUTE ) );
                    ci.stage.pSpecializationInfo = nullptr;
                }

                VkComputeInfo.layout = (VkPipelineLayout)( Desc.hLayout.handle );
                vkRes                = m_Implementation.m_ICD.vkCreateComputePipelines(
                    m_hDevice, VK_NULL_HANDLE, 1, &VkComputeInfo, pVkCallbacks, &hPipeline );
            }

            VK_ERR( vkRes );
            SetObjectDebugName( (uint64_t)hPipeline, VK_OBJECT_TYPE_PIPELINE, Desc.GetDebugName() );
            return hPipeline;
        }

        void CDDI::DestroyPipeline( NativeAPI::Pipeline* phPipeline, const void* pAllocator )
        {
            DDI_DESTROY_OBJECT( Pipeline, phPipeline, pAllocator );
        }

        NativeAPI::DescriptorSetLayout CDDI::CreateDescriptorSetLayout( const SDescriptorSetLayoutDesc& Desc,
                                                                        const void*                     pAllocator )
        {
            if( !Desc.IsValid() )
            {
                return NativeAPI::Null;
            }
            NativeAPI::DescriptorSetLayout hLayout = NativeAPI::Null;

            VkDescriptorSetLayoutCreateInfo ci;
            ci.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            ci.pNext        = nullptr;
            ci.flags        = 0;
            ci.bindingCount = Desc.vBindings.GetCount();

            using VkBindingArray =
                Utils::TCDynamicArray< VkDescriptorSetLayoutBinding,
                                       Config::RenderSystem::Pipeline::MAX_DESCRIPTOR_BINDING_COUNT >;
            VkBindingArray vVkBindings;
            if( vVkBindings.Resize( ci.bindingCount ) )
            {
                for( uint32_t i = 0; i < ci.bindingCount; ++i )
                {
                    auto&       VkBinding        = vVkBindings[ i ];
                    const auto& Binding          = Desc.vBindings[ i ];
                    VkBinding.binding            = Binding.idx;
                    VkBinding.descriptorCount    = Binding.count;
                    VkBinding.descriptorType     = Map::DescriptorType( Binding.type );
                    VkBinding.pImmutableSamplers = nullptr;
                    VkBinding.stageFlags         = Convert::ShaderStages( Binding.stages );

                    vVkBindings[ i ] = ( VkBinding );
                }
                ci.pBindings = vVkBindings.GetData();

                VK_ERR( DDI_CREATE_OBJECT( DescriptorSetLayout, ci, pAllocator, &hLayout ) );
                VKE_ASSERT2( strlen( Desc.GetDebugName() ) > 0, "" );
                SetObjectDebugName( (uint64_t)hLayout, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, Desc.GetDebugName() );
            }

            return hLayout;
        }

        void CDDI::Update( const SUpdateBufferDescriptorSetInfo& Info )
        {
            VkWriteDescriptorSet VkWrite = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };

            VkWrite.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            VkWrite.dstBinding      = Info.binding;
            VkWrite.descriptorCount = Info.count;
            VkWrite.dstSet          = Info.hDDISet;
            VkWrite.dstArrayElement = 0;
            const auto pVkBufferInfos =
                reinterpret_cast< const VkDescriptorBufferInfo* >( Info.vBufferInfos.GetData() );
            VkWrite.pBufferInfo = pVkBufferInfos;
            m_Implementation.m_ICD.vkUpdateDescriptorSets( m_hDevice, 1, &VkWrite, 0, nullptr );
        }

        void CDDI::Update( const SUpdateTextureDescriptorSetInfo& Info )
        {
            Utils::TCDynamicArray< VkDescriptorImageInfo, 8 > vVkInfos;
            for( uint32_t i = 0; i < Info.vTextureInfos.GetCount(); ++i )
            {
                const auto&           Curr = Info.vTextureInfos[ i ];
                VkDescriptorImageInfo VkInfo;
                VkInfo.imageLayout = Map::ImageLayout( Curr.textureState );
                VkInfo.imageView   = Curr.hDDITextureView;
                VkInfo.sampler     = Curr.hDDISampler;
                vVkInfos.PushBack( VkInfo );
            }

            VkWriteDescriptorSet VkWrite = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
            VkWrite.descriptorCount      = Info.count;
            VkWrite.descriptorType       = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            VkWrite.dstArrayElement      = 0;
            VkWrite.dstBinding           = Info.binding;
            VkWrite.dstSet               = Info.hDDISet;
            VkWrite.pImageInfo           = vVkInfos.GetData();

            m_Implementation.m_ICD.vkUpdateDescriptorSets( m_hDevice, 1, &VkWrite, 0, nullptr );
        }

        void CDDI::Update( const NativeAPI::DescriptorSet& hDDISet, const SUpdateBindingsHelper& Info )
        {
            Utils::TCDynamicArray< VkWriteDescriptorSet > vVkWrites;
            VkWriteDescriptorSet                          VkWrite = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };

            using ImageViewInfosArray     = Utils::TCDynamicArray< VkDescriptorImageInfo, 128 >;
            using ImageViewInfosArrays    = Utils::TCDynamicArray< ImageViewInfosArray, 32 >;
            using RenderTargetInfosArray  = Utils::TCDynamicArray< VkDescriptorImageInfo, 8 >;
            using RenderTargetInfosArrays = Utils::TCDynamicArray< RenderTargetInfosArray, 8 >;
            using SamplerInfoArray        = Utils::TCDynamicArray< VkDescriptorImageInfo, 128 >;
            using SamplerInfosArrays      = Utils::TCDynamicArray< SamplerInfoArray, 32 >;

            VKE_ASSERT2( Info.vRTs.GetCount() < 8, "Too many render targets to bind" );
            VKE_ASSERT2( Info.vTexViews.GetCount() < 32, "Too many texture views to bind" );
            VKE_ASSERT2( Info.vSamplers.GetCount() < 32, "Too many samplers to bind." );
            VKE_ASSERT2( Info.vSamplerAndTextures.GetCount() < 32, "Too many samplers to bind." );

            RenderTargetInfosArrays vvVkRenderTargetInfos( Info.vRTs.GetCount() );
            ImageViewInfosArrays    vvVkImageViewsInfos( Info.vTexViews.GetCount() );
            SamplerInfosArrays      vvVkSamplerInfos( Info.vSamplers.GetCount() );
            ImageViewInfosArrays    vvVkImageSamplerInfosArrays( Info.vSamplerAndTextures.GetCount() );

            for( uint32_t i = 0; i < Info.vRTs.GetCount(); ++i )
            {
                const auto& Curr = Info.vRTs[ i ];
                for( uint32_t j = 0; j < Curr.count; ++j )
                {
                    VkDescriptorImageInfo VkInfo;
                    VkInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    VkInfo.imageView   = m_pCtx->GetTextureView( Curr.ahHandles[ j ] )->GetDDIObject();
                    VkInfo.sampler     = NativeAPI::Null;
                    vvVkRenderTargetInfos[ i ].PushBack( VkInfo );
                }

                VkWrite.descriptorCount = Curr.count;
                VkWrite.descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
                VkWrite.dstArrayElement = 0;
                VkWrite.dstBinding      = Curr.binding;
                VkWrite.pImageInfo      = vvVkRenderTargetInfos[ i ].GetData();
                VkWrite.dstSet          = hDDISet;
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
                    VkInfo.sampler = NativeAPI::Null;
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

            for( uint32_t i = 0; i < Info.vTexViews.GetCount(); ++i )
            {
                const auto&           Curr = Info.vTexViews[ i ];
                VkDescriptorImageInfo VkInfo;
                for( uint32_t j = 0; j < Curr.count; ++j )
                {
                    VkInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    VkInfo.imageView   = m_pCtx->GetTextureView( Curr.ahHandles[ j ] )->GetDDIObject();
                    VkInfo.sampler     = NativeAPI::Null;
                    vvVkImageViewsInfos[ i ].PushBack( VkInfo );
                    /*VKE_LOG("Update desc set: " << hDDISet << ", " << (uint32_t)Curr.binding << ", " <<
                             j << ": " << VkInfo.imageView << ": " << Curr.ahHandles[ j ].handle );*/
                }

                VkWrite.descriptorCount = Curr.count;
                VkWrite.descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
                VkWrite.dstArrayElement = 0;
                VkWrite.dstBinding      = Curr.binding;
                VkWrite.pImageInfo      = vvVkImageViewsInfos[ i ].GetData();
                VkWrite.dstSet          = hDDISet;
                vVkWrites.PushBack( VkWrite );
            }

            for( uint32_t i = 0; i < Info.vSamplers.GetCount(); ++i )
            {
                const auto& Curr = Info.vSamplers[ i ];
                for( uint32_t j = 0; j < Curr.count; ++j )
                {
                    VkDescriptorImageInfo VkInfo;
                    VkInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    VkInfo.imageView   = NativeAPI::Null;
                    VkInfo.sampler     = m_pCtx->GetSampler( Curr.ahHandles[ j ] )->GetDDIObject();
                    vvVkSamplerInfos[ i ].PushBack( VkInfo );
                }

                VkWrite.descriptorCount = Curr.count;
                VkWrite.descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLER;
                VkWrite.dstArrayElement = 0;
                VkWrite.dstBinding      = Curr.binding;
                VkWrite.pImageInfo      = vvVkSamplerInfos[ i ].GetData();
                VkWrite.dstSet          = hDDISet;
                vVkWrites.PushBack( VkWrite );
            }

            for( uint32_t i = 0; i < Info.vSamplerAndTextures.GetCount(); ++i )
            {
                const auto& Curr = Info.vSamplerAndTextures[ i ];
                for( uint32_t j = 0; j < Curr.count; ++j )
                {
                    VkDescriptorImageInfo VkInfo;
                    VkInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    VkInfo.imageView   = m_pCtx->GetTextureView( Curr.ahTexViews[ j ] )->GetDDIObject();
                    VkInfo.sampler     = m_pCtx->GetSampler( Curr.ahSamplers[ j ] )->GetDDIObject();
                    vvVkImageSamplerInfosArrays[ i ].PushBack( VkInfo );
                }

                VkWrite.descriptorCount = Curr.count;
                VkWrite.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                VkWrite.dstArrayElement = 0;
                VkWrite.dstBinding      = Curr.binding;
                VkWrite.pImageInfo      = vvVkImageSamplerInfosArrays[ i ].GetData();
                VkWrite.dstSet          = hDDISet;
                vVkWrites.PushBack( VkWrite );
            }

            using VkBufferInfoArray = Utils::TCDynamicArray< VkDescriptorBufferInfo >;
            Utils::TCDynamicArray< VkBufferInfoArray > vvVkBuffInfos;
            vvVkBuffInfos.Resize( Info.vBuffers.GetCount() );

            for( uint32_t i = 0; i < Info.vBuffers.GetCount(); ++i )
            {
                auto& vVkBuffInfos = vvVkBuffInfos[ i ];

                const auto& Curr = Info.vBuffers[ i ];
                for( uint32_t j = 0; j < Curr.count; ++j )
                {
                    VkDescriptorBufferInfo VkInfo;
                    VkInfo.buffer = m_pCtx->GetBuffer( Curr.ahHandles[ j ] )->GetDDIObject();
                    VkInfo.offset = Curr.offset;
                    VkInfo.range  = Curr.elementSize * Curr.elementCount;
                    vVkBuffInfos.PushBack( VkInfo );
                }

                VkWrite.descriptorCount = vVkBuffInfos.GetCount();
                VkWrite.descriptorType  = Map::DescriptorType( Curr.type );
                VkWrite.dstArrayElement = 0;
                VkWrite.dstBinding      = Curr.binding;
                VkWrite.pBufferInfo     = vVkBuffInfos.GetData();
                VkWrite.dstSet          = hDDISet;
                vVkWrites.PushBack( VkWrite );
            }

            m_Implementation.m_ICD.vkUpdateDescriptorSets(
                m_hDevice, vVkWrites.GetCount(), vVkWrites.GetData(), 0, nullptr );
        }

        void CDDI::Update( const NativeAPI::DescriptorSet& hDDISrcSet, NativeAPI::DescriptorSet* phDDIDstOut )
        {
            VkCopyDescriptorSet vkCopy;
            vkCopy.sType           = VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET;
            vkCopy.pNext           = nullptr;
            vkCopy.descriptorCount = 1;
            vkCopy.dstArrayElement = 0;
            vkCopy.dstBinding      = 0;
            vkCopy.srcArrayElement = 0;
            vkCopy.srcBinding      = 1;
            vkCopy.srcSet          = hDDISrcSet;
            vkCopy.dstSet          = *phDDIDstOut;
            m_Implementation.m_ICD.vkUpdateDescriptorSets( m_hDevice, 0, 0, 1, &vkCopy );
        }

        void CDDI::DestroyDescriptorSetLayout( NativeAPI::DescriptorSetLayout* phLayout, const void* pAllocator )
        {
            DDI_DESTROY_OBJECT( DescriptorSetLayout, phLayout, pAllocator );
        }

        NativeAPI::PipelineLayout CDDI::CreatePipelineLayout( const SPipelineLayoutDesc& Desc, const void* pAllocator )
        {
            VKE_ASSERT( !Desc.IsDebugNameEmpty() );
            NativeAPI::PipelineLayout  hLayout = NativeAPI::Null;
            VkPipelineLayoutCreateInfo ci;
            ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            ci.pNext = nullptr;
            ci.flags = 0;

            // VKE_ASSERT2( !Desc.vDescriptorSetLayouts.IsEmpty(), "There should be at least one DescriptorSetLayout."
            // );
            ci.setLayoutCount           = Desc.vDescriptorSetLayouts.GetCount();
            static const auto MAX_COUNT = Config::RenderSystem::Pipeline::MAX_PIPELINE_LAYOUT_DESCRIPTOR_SET_COUNT;
            Utils::TCDynamicArray< VkDescriptorSetLayout, MAX_COUNT > vVkDescLayouts;
            for( uint32_t i = 0; i < ci.setLayoutCount; ++i )
            {
                // NativeAPI::DescriptorSetLayout hDDIObj = m_pCtx->GetDescriptorSetLayout(
                // Desc.vDescriptorSetLayouts[i] )->GetDDIObject();
                NativeAPI::DescriptorSetLayout hDDIObj =
                    m_pCtx->GetDescriptorSetLayout( Desc.vDescriptorSetLayouts[ i ] );
                vVkDescLayouts.PushBack( hDDIObj );
            }
            ci.pSetLayouts            = vVkDescLayouts.GetData();
            ci.pPushConstantRanges    = nullptr;
            ci.pushConstantRangeCount = 0;

            VK_ERR( DDI_CREATE_OBJECT( PipelineLayout, ci, pAllocator, &hLayout ) );
            SetObjectDebugName( (uint64_t)hLayout, VK_OBJECT_TYPE_PIPELINE_LAYOUT, Desc.GetDebugName() );
            return hLayout;
        }

        void CDDI::DestroyPipelineLayout( NativeAPI::PipelineLayout* phLayout, const void* pAllocator )
        {
            DDI_DESTROY_OBJECT( PipelineLayout, phLayout, pAllocator );
        }

        NativeAPI::Shader CDDI::CreateShader( const SShaderData& Data, const void* pAllocator )
        {
            VKE_ASSERT2( Data.stage == ShaderCompilationStages::COMPILED_IR_BINARY && Data.codeSize > 0 &&
                             Data.codeSize % 4 == 0 && Data.pCode != nullptr,
                         "Invalid shader data." );

            NativeAPI::Shader        hShader = NativeAPI::Null;
            VkShaderModuleCreateInfo ci;
            ci.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            ci.pNext    = nullptr;
            ci.flags    = 0;
            ci.pCode    = reinterpret_cast< const uint32_t* >( Data.pCode );
            ci.codeSize = Data.codeSize;
            VK_ERR( DDI_CREATE_OBJECT( ShaderModule, ci, pAllocator, &hShader ) );
            return hShader;
        }

        void CDDI::DestroyShader( NativeAPI::Shader* phShader, const void* pAllocator )
        {
            DDI_DESTROY_OBJECT( ShaderModule, phShader, pAllocator );
        }

        NativeAPI::Sampler CDDI::CreateSampler( const SSamplerDesc& Desc, const void* pAllocator )
        {
            NativeAPI::Sampler  hSampler = NativeAPI::Null;
            VkSamplerCreateInfo ci;
            ci.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            ci.pNext                   = nullptr;
            ci.flags                   = 0;
            ci.addressModeU            = Map::AddressMode( Desc.AddressMode.U );
            ci.addressModeV            = Map::AddressMode( Desc.AddressMode.V );
            ci.addressModeW            = Map::AddressMode( Desc.AddressMode.W );
            ci.anisotropyEnable        = Desc.enableAnisotropy;
            ci.borderColor             = Convert::BorderColor( Desc.borderColor );
            ci.compareEnable           = Desc.enableCompare;
            ci.compareOp               = Map::CompareOperation( Desc.compareFunc );
            ci.magFilter               = Convert::Filter( Desc.Filter.mag );
            ci.maxAnisotropy           = Desc.maxAnisotropy;
            ci.maxLod                  = Desc.LOD.max;
            ci.minFilter               = Convert::Filter( Desc.Filter.min );
            ci.minLod                  = Desc.LOD.min;
            ci.mipLodBias              = Desc.mipLODBias;
            ci.mipmapMode              = Map::MipmapMode( Desc.mipmapMode );
            ci.unnormalizedCoordinates = Desc.unnormalizedCoordinates;
            VK_ERR( DDI_CREATE_OBJECT( Sampler, ci, pAllocator, &hSampler ) );
            return hSampler;
        }

        void CDDI::DestroySampler( NativeAPI::Sampler* phSampler, const void* pAllocator )
        {
            DDI_DESTROY_OBJECT( Sampler, phSampler, pAllocator );
        }

        NativeAPI::Event CDDI::CreateEvent( const SEventDesc&, const void* pAllocator )
        {
            static const VkEventCreateInfo ci = { VK_STRUCTURE_TYPE_EVENT_CREATE_INFO };
            NativeAPI::Event               hRet;
            VK_ERR( DDI_CREATE_OBJECT( Event, ci, pAllocator, &hRet ) );
            return hRet;
        }

        void CDDI::DestroyEvent( NativeAPI::Event* phEvent, const void* pAllocator )
        {
            DDI_DESTROY_OBJECT( Event, phEvent, pAllocator );
        }

        Result CDDI::CreateDescriptorSets( const AllocateDescs::SDescSet& Info, NativeAPI::DescriptorSet* pSets )
        {
            Result                      ret = VKE_FAIL;
            VkDescriptorSetAllocateInfo ai;
            ai.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            ai.pNext              = nullptr;
            ai.descriptorPool     = Info.hPool;
            ai.descriptorSetCount = Info.count;
            ai.pSetLayouts        = Info.phLayouts;
            VkResult res          = m_Implementation.m_ICD.vkAllocateDescriptorSets( m_hDevice, &ai, pSets );

            switch( res )
            {
                case VK_SUCCESS:
                    ret = VKE_OK;
                    break;
                case VK_ERROR_OUT_OF_POOL_MEMORY:
                    ret = VKE_ENOMEMORY;
                    break;
                default:
                    VK_ERR( res );
            }

            VKE_ASSERT2( strlen( Info.GetDebugName() ) > 0, "Debug name must be set in Debug mode" );
#if VKE_RENDER_SYSTEM_DEBUG
            if( VKE_SUCCEEDED( ret ) )
            {
                for( uint32_t i = 0; i < ai.descriptorSetCount; ++i )
                {
                    SetObjectDebugName( (uint64_t)pSets[ i ], VK_OBJECT_TYPE_DESCRIPTOR_SET, Info.GetDebugName() );
                }
            }
#endif
            return ret;
        }

        void CDDI::FreeObjects( const FreeDescs::SDescSet& Desc )
        {
            m_Implementation.m_ICD.vkFreeDescriptorSets( m_hDevice, Desc.hPool, Desc.count, Desc.phSets );
        }

        Result CDDI::CreateCommandBuffers( const SAllocateCommandBufferInfo& Info, NativeAPI::CommandBuffer* pBuffers )
        {
            Result                      ret = VKE_FAIL;
            VkCommandBufferAllocateInfo ai;
            ai.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            ai.pNext              = nullptr;
            ai.level              = Map::CommandBufferLevel( Info.level );
            ai.commandBufferCount = Info.count;
            ai.commandPool        = Info.hDDIPool;
            VkResult res          = m_Implementation.m_ICD.vkAllocateCommandBuffers( m_hDevice, &ai, pBuffers );
            VK_ERR( res );
            ret = res == VK_SUCCESS ? VKE_OK : VKE_ENOMEMORY;
            return ret;
        }

        void CDDI::FreeObjects( const SFreeCommandBufferInfo& Info )
        {
            m_Implementation.m_ICD.vkFreeCommandBuffers(
                m_hDevice, Info.hDDIPool, Info.count, Info.pDDICommandBuffers );
        }

        size_t CDDI::GetMemoryHeapTotalSize( MEMORY_HEAP_TYPE type ) const
        {
            const auto idx = HeapMap.TypeToIndex[ type ];
            return m_DeviceProperties.Properties.Memory.memoryProperties.memoryHeaps[ idx ].size;
        }

        size_t CDDI::GetMemoryHeapCurrentSize( MEMORY_HEAP_TYPE type ) const
        {
            const auto idx = HeapMap.TypeToIndex[ type ];
            return m_Implementation.m_aHeapSizes[ idx ];
        }

        vke_force_inline int32_t FindMemoryTypeIndex( const VkPhysicalDeviceMemoryProperties* pMemProps,
                                                      uint32_t                                requiredMemBits,
                                                      VkMemoryPropertyFlags                   requiredProperties )
        {
            const uint32_t memCount = pMemProps->memoryTypeCount;
            for( uint32_t memIdx = 0; memIdx < memCount; ++memIdx )
            {
                const uint32_t              memTypeBits       = ( 1 << memIdx );
                const bool                  isRequiredMemType = requiredMemBits & memTypeBits;
                const VkMemoryPropertyFlags props             = pMemProps->memoryTypes[ memIdx ].propertyFlags;
                const bool                  hasRequiredProps  = ( props & requiredProperties ) == requiredProperties;
                if( isRequiredMemType && hasRequiredProps )
                {
                    return static_cast< int32_t >( memIdx );
                }
            }
            return -1;
        }

        MEMORY_HEAP_TYPE CDDI::GetMemoryHeapType( MEMORY_USAGE usage ) const
        {
            MEMORY_HEAP_TYPE      ret             = MemoryHeapTypes::OTHER;
            VkMemoryPropertyFlags vkPropertyFlags = Convert::MemoryUsagesToVkMemoryPropertyFlags( usage );
            const auto&           VkMemProps      = m_DeviceProperties.Properties.Memory.memoryProperties;
            const int32_t         idx             = FindMemoryTypeIndex( &VkMemProps, UINT32_MAX, vkPropertyFlags );
            if( idx >= 0 )
            {
                // const auto heapIdx = VkMemProps.memoryTypes[ idx ].heapIndex;
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
            Result                ret             = VKE_FAIL;
            VkMemoryPropertyFlags vkPropertyFlags = Convert::MemoryUsagesToVkMemoryPropertyFlags( Desc.usage );

            const auto& VkMemProps = m_DeviceProperties.Properties.Memory.memoryProperties;
            int32_t     idx        = FindMemoryTypeIndex( &VkMemProps, UINT32_MAX, vkPropertyFlags );
            // const uint32_t idx = HeapMap.TypeToIndex[  ];
            NativeAPI::Memory hMemory;
            if( idx >= 0 )
            {
                auto heapIdx  = VkMemProps.memoryTypes[ idx ].heapIndex;
                auto memFlags = VkMemProps.memoryTypes[ idx ].propertyFlags;
                // If there is no space left in upload heap try to allocate on CPU
                if( ( Desc.usage & MemoryUsages::UPLOAD ) == MemoryUsages::UPLOAD &&
                    m_Implementation.m_aHeapSizes[ heapIdx ] < Desc.size )
                {
                    VKE_LOG_WARN( "No free space left on UPLOAD heap: "
                                  << VKE_LOG_MEM_SIZE( m_Implementation.m_aHeapSizes[ heapIdx ] )
                                  << ", requested allocation size: " << VKE_LOG_MEM_SIZE( Desc.size )
                                  << ". Trying to allocate on a CPU heap instead." );
                    MEMORY_USAGE newUsages = MemoryUsages::STAGING_BUFFER;
                    vkPropertyFlags        = Convert::MemoryUsagesToVkMemoryPropertyFlags( newUsages );
                    idx                    = FindMemoryTypeIndex( &VkMemProps, UINT32_MAX, vkPropertyFlags );
                    VKE_ASSERT2( idx >= 0, "" );
                    heapIdx  = VkMemProps.memoryTypes[ idx ].heapIndex;
                    memFlags = VkMemProps.memoryTypes[ idx ].propertyFlags;
                }
                VKE_ASSERT2( m_Implementation.m_aHeapSizes[ heapIdx ] >= Desc.size, "" );
                VkMemoryAllocateInfo ai = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
                ai.allocationSize       = Desc.size;
                ai.memoryTypeIndex      = idx;
                VkResult res            = m_Implementation.m_ICD.vkAllocateMemory( m_hDevice, &ai, nullptr, &hMemory );
                VK_ERR( res );
                if( res == VK_SUCCESS )
                {
                    m_Implementation.m_aHeapSizes[ heapIdx ] -= ai.allocationSize;

                    pOut->hDDIMemory = hMemory;
                    pOut->sizeLeft   = static_cast< uint32_t >( m_Implementation.m_aHeapSizes[ heapIdx ] );
                    pOut->heapType   = Map::VkMemPropertyFlagsToHeapType( memFlags );
                }
                ret = res == VK_SUCCESS ? VKE_OK : VKE_ENOMEMORY;
            }
            else
            {
                VKE_LOG_ERR( "Required memory usage: " << Desc.usage << " is not suitable for this GPU." );
            }
            return ret;
        }

        Result CDDI::GetTextureMemoryRequirements( const STextureDesc& Desc,
                                                   SAllocationMemoryRequirementInfo* pOut )
        {
            NativeAPI::Texture hImage = NativeAPI::Null;
            VkImageCreateInfo  ci;
            {
                ci.sType                 = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
                ci.pNext                 = nullptr;
                ci.flags                 = 0;
                ci.format                = Map::Format( Desc.format );
                ci.imageType             = Map::ImageType( Desc.type );
                ci.initialLayout         = VK_IMAGE_LAYOUT_UNDEFINED;
                ci.mipLevels             = Desc.mipmapCount;
                ci.samples               = Map::SampleCount( Desc.multisampling );
                ci.pQueueFamilyIndices   = nullptr;
                ci.queueFamilyIndexCount = 0;
                ci.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
                ci.tiling                = Convert::ImageUsageToTiling( Desc.usage );
                ci.arrayLayers           = Desc.arrayElementCount;
                ci.extent.width          = Desc.Size.width;
                ci.extent.height         = Desc.Size.height;
                ci.extent.depth          = 1;
                ci.usage                 = Map::ImageUsage( Desc.usage );
            }
            
            VkResult vkRes = m_Implementation.m_ICD.vkCreateImage( m_hDevice, &ci, nullptr, &hImage );
            VK_ERR( vkRes );

            VkMemoryRequirements VkReq;
            m_Implementation.m_ICD.vkGetImageMemoryRequirements( m_hDevice, hImage, &VkReq );
            pOut->alignment = static_cast< uint32_t >( VkReq.alignment );
            pOut->size      = static_cast< uint32_t >( VkReq.size );
            pOut->reserved  = reinterpret_cast<handle_t>(hImage); // Return the image handle so we can destroy it later
            return VKE_OK;
        }

        Result CDDI::GetBufferMemoryRequirements( const SBufferDesc& Desc,
                                                  SAllocationMemoryRequirementInfo* pOut )
        {
            Result             ret = VKE_FAIL;
            VkBufferCreateInfo ci;
            NativeAPI::Buffer  hBuffer = VK_NULL_HANDLE;
            Vulkan::InitInfo( &ci, VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO );
            ci.flags                 = 0;
            ci.pQueueFamilyIndices   = nullptr;
            ci.queueFamilyIndexCount = 0;
            ci.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
            ci.size                  = Desc.CalcSize();
            ci.usage                 = Convert::BufferUsage( Desc.usage );
            if( Desc.memoryUsage & MemoryUsages::GPU_ACCESS )
            {
                ci.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            }

            VkResult vkRes = DDI_CREATE_OBJECT( Buffer, ci, nullptr, &hBuffer );
            VK_ERR( vkRes );
            if( vkRes == VK_SUCCESS )
            {
                VkMemoryRequirements VkReq;
                m_Implementation.m_ICD.vkGetBufferMemoryRequirements( m_hDevice, hBuffer, &VkReq );
                {
                    ret             = VKE_OK;
                    pOut->alignment = static_cast< uint32_t >( VkReq.alignment );
                    pOut->size      = static_cast< uint32_t >( VkReq.size );
                    pOut->reserved =
                        reinterpret_cast< handle_t >( hBuffer ); // Return the buffer handle so we can destroy it later
                }
            }
            else
            {
                VKE_LOG_ERR( "Unable to create vkBuffer: " << Desc.GetDebugName() );
            }
            return ret;
        }

        void CDDI::Free( NativeAPI::Memory* phMemory, const void* pAllocator )
        {
            if( *phMemory != NativeAPI::Null )
            {
                m_Implementation.m_ICD.vkFreeMemory(
                    m_hDevice, *phMemory, reinterpret_cast< const VkAllocationCallbacks* >( pAllocator ) );
            }
            *phMemory = NativeAPI::Null;
        }

        bool CDDI::IsSignaled( const NativeAPI::CPUFence& hFence ) const
        {
            // return WaitForFences( hFence, 0 ) == VKE_OK;
            VkResult res = m_Implementation.m_ICD.vkGetFenceStatus( m_hDevice, hFence );
            return res == VK_SUCCESS;
        }

        bool CDDI::IsSignaled( const NativeAPI::Fence& hFence ) const
        {
            const auto& Fences = hFence->vFences;
            return IsSignaled( Fences[ hFence->counter.load() ].hFence );
        }

        NativeAPI::FenceValue CDDI::GetCompletedValue( const NativeAPI::Fence& hFence ) const
        {
            /// TODO: handle TDR
            if( hFence->isNativeMonitored )
            {
                uint64_t v;
                m_Implementation.m_ICD.vkGetSemaphoreCounterValue( m_hDevice, hFence->GetFences( 0 )->hSemaphore, &v );
                return v;
            }
            return hFence->GetLastSignaledValue( this );
        }

        void CDDI::Reset( NativeAPI::CPUFence* phFence )
        {
            VK_ERR( m_Implementation.m_ICD.vkResetFences( m_hDevice, 1, phFence ) );
        }

        void CDDI::Reset( NativeAPI::Fence* phFence, NativeAPI::FenceValue value )
        {
            auto& Fence = *phFence;
            Fence->Reset( this, value );
        }

        Result CDDI::WaitForFences( const NativeAPI::CPUFence& hFence, uint64_t timeout ) const
        {
            VKE_ASSERT( hFence != NativeAPI::Null );
            VkResult res = m_Implementation.m_ICD.vkWaitForFences( m_hDevice, 1, &hFence, VK_TRUE, timeout );

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

        Result CDDI::WaitForFence( NativeAPI::Fence hFence, NativeAPI::FenceValue value ) const
        {
            if( hFence->isNativeMonitored && hFence->isBinary == false )
            {
                VkSemaphoreWaitInfo VkWaitInfo;
                VkWaitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
                VkWaitInfo.pNext = nullptr;
                VkWaitInfo.flags = 0;
                VkWaitInfo.semaphoreCount = 1;
                VkWaitInfo.pSemaphores    = &hFence->GetFences( value )->hSemaphore;
                VkWaitInfo.pValues        = &value;
                VkResult res = m_Implementation.m_ICD.vkWaitSemaphores( m_hDevice, &VkWaitInfo, UINT64_MAX );
                switch( res )
                {
                    case VK_SUCCESS:
                        return VKE_OK;
                    case VK_TIMEOUT:
                        return VKE_TIMEOUT;
                    case VK_ERROR_DEVICE_LOST:
                        return VKE_EDEVICELOST;
                    default:
                        return VKE_FAIL;
                };
            }
            return WaitForFences( hFence->GetFences( value )->hFence, UINT64_MAX );
        }

        Result CDDI::WaitForQueue( const NativeAPI::Queue& hQueue )
        {
            VkResult res = m_Implementation.m_ICD.vkQueueWaitIdle( hQueue );
            VK_ERR( res );
            return res == VK_SUCCESS ? VKE_OK : VKE_FAIL;
        }

        Result CDDI::WaitForDevice()
        {
            VkResult res = m_Implementation.m_ICD.vkDeviceWaitIdle( m_hDevice );
            VK_ERR( res );
            return res == VK_SUCCESS ? VKE_OK : VKE_FAIL;
        }

        void* CDDI::MapMemory( const SMapMemoryInfo& Info )
        {
            void*    pData;
            VkResult res =
                m_Implementation.m_ICD.vkMapMemory( m_hDevice, Info.hMemory, Info.offset, Info.size, 0, &pData );
            if( res != VK_SUCCESS )
            {
                pData = nullptr;
            }
            VK_ERR( res );
            return pData;
        }

        void CDDI::UnmapMemory( const SMapMemoryInfo& Info )
        {
            m_Implementation.m_ICD.vkUnmapMemory( m_hDevice, Info.hMemory );
        }

        void CDDI::Draw( const NativeAPI::CommandBuffer& hCommandBuffer, const uint32_t& vertexCount,
                         const uint32_t& instanceCount, const uint32_t& firstVertex, const uint32_t& firstInstance )
        {
            m_Implementation.m_ICD.vkCmdDraw( hCommandBuffer, vertexCount, instanceCount, firstVertex, firstInstance );
        }

        void CDDI::DrawIndexed( const NativeAPI::CommandBuffer& hCommandBuffer, const SDrawParams& Params )
        {
            m_Implementation.m_ICD.vkCmdDrawIndexed( hCommandBuffer,
                                                     Params.Indexed.indexCount,
                                                     Params.Indexed.instanceCount,
                                                     Params.Indexed.startIndex,
                                                     Params.Indexed.vertexOffset,
                                                     Params.Indexed.startInstance );
        }

        void CDDI::DrawMesh( const NativeAPI::CommandBuffer& hCommandBuffer, uint32_t width, uint32_t height,
                             uint32_t depth )
        {
            m_Implementation.m_ICD.vkCmdDrawMeshTasksEXT( hCommandBuffer, width, height, depth );
        }

        void CDDI::Copy( const NativeAPI::CommandBuffer& hCmdBuffer, const SCopyBufferToTextureInfo& Info )
        {
            Utils::TCDynamicArray< VkBufferImageCopy > vRegions( Info.vRegions.GetCount() );
            for( uint32_t i = 0; i < vRegions.GetCount(); ++i )
            {
                const auto& Region         = Info.vRegions[ i ];
                auto&       VkRegion       = vRegions[ i ];
                VkRegion.bufferImageHeight = Region.bufferTextureHeight;
                VkRegion.bufferRowLength   = Region.bufferRowLength;
                VkRegion.bufferOffset      = Region.bufferOffset;
                VkRegion.imageExtent       = { Region.textureWidth, Region.textureHeight, Region.textureDepth };
                VkRegion.imageOffset       = { (int32_t)Region.textureOffsetX,
                                               (int32_t)Region.textureOffsetY,
                                               (int32_t)Region.textureOffsetZ };
                VkRegion.imageSubresource.aspectMask     = Map::ImageAspect( Region.TextureSubresource.aspect );
                VkRegion.imageSubresource.baseArrayLayer = Region.TextureSubresource.beginArrayLayer;
                VkRegion.imageSubresource.layerCount     = Region.TextureSubresource.layerCount;
                VkRegion.imageSubresource.mipLevel       = Region.TextureSubresource.beginMipmapLevel;
            }
            VkImageLayout vkLayout = Map::ImageLayout( Info.textureState );
            m_Implementation.m_ICD.vkCmdCopyBufferToImage(
                hCmdBuffer, Info.hDDISrcBuffer, Info.hDDIDstTexture, vkLayout, vRegions.GetCount(), &vRegions[ 0 ] );
        }

        void CDDI::Copy( const NativeAPI::CommandBuffer& hDDICmdBuffer, const SCopyBufferInfo& Info )
        {
            VkBufferCopy VkCopy;
            VkCopy.srcOffset = Info.Region.srcBufferOffset;
            VkCopy.dstOffset = Info.Region.dstBufferOffset;
            VkCopy.size      = Info.Region.size;

            m_Implementation.m_ICD.vkCmdCopyBuffer(
                hDDICmdBuffer, Info.hDDISrcBuffer, Info.pDstBuffer->GetDDIObject(), 1, &VkCopy );
        }

        void TextureSubresourceToNativeSubresource( const STextureSubresourceRange& Subres,
                                                    VkImageSubresourceLayers*       pOut )
        {
            pOut->aspectMask     = Map::ImageAspect( Subres.aspect );
            pOut->baseArrayLayer = Subres.beginArrayLayer;
            pOut->layerCount     = Subres.layerCount;
            pOut->mipLevel       = Subres.beginMipmapLevel;
        }

        void CDDI::Copy( const NativeAPI::CommandBuffer& hDDICmdBuffer, const SCopyTextureInfoEx& Info )
        {
            VkImageLayout vkSrcLayout = Map::ImageLayout( Info.srcTextureState );
            VkImageLayout vkDstLayout = Map::ImageLayout( Info.dstTextureState );

            VkImageCopy VkCopy;

            VkCopy.extent    = { Info.pBaseInfo->Size.width,
                                 Info.pBaseInfo->Size.height,
                                 Math::Max( 1u, Info.pBaseInfo->depth ) };
            VkCopy.srcOffset = { Info.pBaseInfo->SrcOffset.x, Info.pBaseInfo->SrcOffset.y };
            VkCopy.dstOffset = { Info.pBaseInfo->DstOffset.x, Info.pBaseInfo->DstOffset.y };

            TextureSubresourceToNativeSubresource( Info.DstSubresource, &VkCopy.dstSubresource );
            TextureSubresourceToNativeSubresource( Info.SrcSubresource, &VkCopy.srcSubresource );

            m_Implementation.m_ICD.vkCmdCopyImage( hDDICmdBuffer,
                                                   Info.pBaseInfo->hDDISrcTexture,
                                                   vkSrcLayout,
                                                   Info.pBaseInfo->hDDIDstTexture,
                                                   vkDstLayout,
                                                   1,
                                                   &VkCopy );
        }

        void CDDI::Blit( const NativeAPI::CommandBuffer& hAPICmdBuffer, const SBlitTextureInfo& Info )
        {
            Utils::TCDynamicArray< VkImageBlit2KHR > vNativeRegions( Info.vRegions.GetCount() );
            for( uint32_t i = 0; i < Info.vRegions.GetCount(); ++i )
            {
                const auto& Region = Info.vRegions[ i ];
                auto&       Native = vNativeRegions[ i ];
                Native.sType       = VK_STRUCTURE_TYPE_IMAGE_BLIT_2_KHR;
                Native.pNext       = nullptr;
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

            VkBlitImageInfo2KHR NativeInfo = { .sType          = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2_KHR,
                                               .pNext          = nullptr,
                                               .srcImage       = Info.hAPISrcTexture,
                                               .srcImageLayout = Map::ImageLayout( Info.srcTextureState ),
                                               .dstImage       = Info.hAPIDstTexture,
                                               .dstImageLayout = Map::ImageLayout( Info.dstTextureState ),
                                               .regionCount    = Info.vRegions.GetCount(),
                                               .pRegions       = vNativeRegions.GetData(),
                                               .filter         = Map::Filter( Info.filter ) };

            m_Implementation.m_ICD.vkCmdBlitImage2KHR( hAPICmdBuffer, &NativeInfo );
        }

        void CDDI::SetEvent( const NativeAPI::Event& hDDIEvent )
        {
            m_Implementation.m_ICD.vkSetEvent( m_hDevice, hDDIEvent );
        }

        void CDDI::SetEvent( const NativeAPI::CommandBuffer& hDDICmdBuffer, const NativeAPI::Event& hDDIEvent,
                             const PIPELINE_STAGES& stages )
        {
            m_Implementation.m_ICD.vkCmdSetEvent( hDDICmdBuffer, hDDIEvent, Convert::PipelineStages( stages ) );
        }

        void CDDI::Reset( const NativeAPI::Event& hDDIInOut )
        {
            m_Implementation.m_ICD.vkResetEvent( m_hDevice, hDDIInOut );
        }

        void CDDI::Reset( const NativeAPI::CommandBuffer& hDDICmdBuffer, const NativeAPI::Event& hDDIEvent,
                          const PIPELINE_STAGES& stages )
        {
            m_Implementation.m_ICD.vkCmdResetEvent( hDDICmdBuffer, hDDIEvent, Convert::PipelineStages( stages ) );
        }

        bool CDDI::IsSet( const NativeAPI::Event& hDDIEvent )
        {
            VkResult res = m_Implementation.m_ICD.vkGetEventStatus( m_hDevice, hDDIEvent );
            return res == VK_EVENT_SET;
        }

        Result CDDI::Submit( const SSubmitInfo& Info )
        {
            Result ret = VKE_FAIL;

            static VkPipelineStageFlags                   vkWaitMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            Utils::TCDynamicArray< VkPipelineStageFlags > vWaitMask( Info.waitSemaphoreCount, vkWaitMask );
            NativeAPI::CPUFence                           hSignalFence = Info.hDDIFence;
            const NativeAPI::GPUFence*                          phWaitForSemaphores = Info.waitSemaphoreCount? Info.pDDIWaitSemaphores : NativeAPI::Null;
            const NativeAPI::GPUFence* phSignalSemaphores =
                Info.signalSemaphoreCount ? Info.pDDISignalSemaphores : NativeAPI::Null;
            uint32_t waitForFenceCount = Info.waitSemaphoreCount;
            uint32_t signalSemaphoreCount = Info.signalSemaphoreCount;
            
            VKE_ASSERT( ( Info.hDDIFence != NativeAPI::Null && Info.hSignalFence == NativeAPI::Null ) ||
                        ( Info.hDDIFence == NativeAPI::Null && Info.hSignalFence != NativeAPI::Null ) );
            VKE_ASSERT( ( Info.waitSemaphoreCount != 0 && Info.hWaitForFence == NativeAPI::Null ) ||
                        ( Info.waitSemaphoreCount == 0 && Info.hWaitForFence != NativeAPI::Null ) ||
                        ( Info.waitSemaphoreCount == 0 && Info.hWaitForFence == NativeAPI::Null ) );
            VKE_ASSERT( ( Info.signalSemaphoreCount != 0 && Info.hSignalFence == NativeAPI::Null ) ||
                        ( Info.signalSemaphoreCount == 0 && Info.hSignalFence != NativeAPI::Null ) || 
                        ( Info.signalSemaphoreCount == 0 && Info.hSignalFence == NativeAPI::Null ) );
            //VKE_ASSERT( ( Info.signalSemaphoreCount <= 1 && Info.waitSemaphoreCount <= 1 ) );

            VkSubmitInfo si;
            si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            si.pNext = nullptr;
            SVulkanNext Next( si );

            if( Info.hWaitForFence != NativeAPI::Null )
            {
                auto pFences        = Info.hWaitForFence->GetFences( Info.waitForFenceValue );
                phWaitForSemaphores = &pFences->hSemaphore;
                waitForFenceCount   = phWaitForSemaphores != NativeAPI::Null ? 1 : 0;
            }
            if(Info.hSignalFence != NativeAPI::Null)
            {
                const auto pFences = Info.hSignalFence->Signal( this, Info.signalFenceValue );
                VKE_ASSERT( pFences != nullptr );
                hSignalFence = pFences->hFence;
                phSignalSemaphores = &pFences->hSemaphore;
                signalSemaphoreCount = phSignalSemaphores != NativeAPI::Null ? 1 : 0;
                if( Info.hSignalFence->isNativeMonitored && Info.hSignalFence->isBinary == false )
                {
                    VkTimelineSemaphoreSubmitInfo TimelineInfo;
                    TimelineInfo.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
                    TimelineInfo.pNext = nullptr;
                    TimelineInfo.waitSemaphoreValueCount   = waitForFenceCount;
                    TimelineInfo.signalSemaphoreValueCount = 1;
                    TimelineInfo.pSignalSemaphoreValues    = &Info.signalFenceValue;
                    TimelineInfo.pWaitSemaphoreValues      = &Info.waitForFenceValue;
                    Next.Add( &TimelineInfo );
                }
            }
            

            
            si.pSignalSemaphores    = phSignalSemaphores;
            si.signalSemaphoreCount = signalSemaphoreCount;
            si.pWaitSemaphores      = phWaitForSemaphores;
            si.waitSemaphoreCount   = waitForFenceCount;
            si.pWaitDstStageMask    = vWaitMask.GetData();
            si.commandBufferCount   = Info.commandBufferCount;
            si.pCommandBuffers      = &Info.pDDICommandBuffers[ 0 ];
            // VK_ERR( m_pQueue->Submit( ICD, si, pSubmit->m_hDDIFence ) );
            VkResult res = m_Implementation.m_ICD.vkQueueSubmit( Info.hDDIQueue, 1, &si, hSignalFence );
            VK_ERR( res );
            ret = res == VK_SUCCESS ? VKE_OK : VKE_FAIL;
            return ret;
        }

        Result CDDI::Present( const SPresentData& Info )
        {
            using SemaphoreArray = Utils::TCDynamicArray< NativeAPI::GPUFence, 8 > ;
            
            SemaphoreArray   vWaitSemaphores, vSignalSemaphores;
            for( uint32_t i = 0; i < Info.vWaitForFenceValues.GetCount(); ++i )
            {
                auto value = Info.vWaitForFenceValues[ i ];
                const auto& pFences = Info.vWaitForFences[ i ]->GetFences( value );
                if( pFences->hSemaphore != NativeAPI::Null )
                {
                    //vWaitSemaphores.PushBack( hSemaphore );
                }
                if( pFences->hFence != NativeAPI::Null )
                {
                    WaitForFence( Info.vWaitForFences[ i ], Info.vWaitForFenceValues[ i ] );
                }
            }
            
            VkPresentInfoKHR pi;
            pi.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
            pi.pNext              = nullptr;
            pi.pImageIndices      = &Info.vImageIndices[ 0 ];
            pi.pSwapchains        = &Info.vSwapchains[ 0 ];
            pi.pWaitSemaphores    = vWaitSemaphores.GetData();
            pi.pResults           = nullptr;
            pi.swapchainCount     = Info.vSwapchains.GetCount();
            pi.waitSemaphoreCount = vWaitSemaphores.GetCount();

            VkResult res = m_Implementation.m_ICD.vkQueuePresentKHR( Info.hQueue, &pi );
            Result   ret = VKE_OK;
            // VK_ERR( res );
            // return res == VK_SUCCESS ? VKE_OK : VKE_FAIL;
            if( res != VK_SUCCESS )
            {
                switch( res )
                {
                    case VK_ERROR_OUT_OF_DATE_KHR:
                    case VK_ERROR_SURFACE_LOST_KHR: {
                        ret = VKE_EOUTOFDATE;
                    }
                    break;
                    default: {
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
                                                             SPresentSurfaceFormat*    pOut )
        {
            Result ret = VKE_OK;
            switch( vkFormat.colorSpace )
            {
                case VK_COLOR_SPACE_SRGB_NONLINEAR_KHR: {
                    pOut->colorSpace = ColorSpaces::SRGB;
                }
                break;
                default: {
                    ret = VKE_FAIL;
                }
                break;
            };
            // pOut->format = Convert::ImageFormat( vkFormat.format );
            pOut->format = Convert::ImageFormat( vkFormat.format );
            return ret;
        }

        Result CDDI::CreateSwapChain( const SSwapChainDesc& Desc, const void*, SDDISwapChain* pOut )
        {
            Result                    ret = VKE_FAIL;
            VkResult                  vkRes;
            NativeAPI::PresentSurface hSurface     = pOut->hSurface;
            uint16_t                  elementCount = Desc.backBufferCount;
            VkSwapchainKHR            hSwapChain   = NativeAPI::Null;

            ExtentU16 Size = Desc.Size;
            if( Desc.pWindow!= nullptr )
            {
                Size = Desc.pWindow->GetDesc().Size;
            }

            Helper::SAllocData    AllocData;
            VkAllocationCallbacks VkDummyCallbacks;
            VkDummyCallbacks.pUserData             = &AllocData;
            VkDummyCallbacks.pfnAllocation         = Helper::DummyAllocCallback;
            VkDummyCallbacks.pfnInternalAllocation = Helper::DummyInternalAllocCallback;
            VkDummyCallbacks.pfnFree               = Helper::DummyFreeCallback;
            VkDummyCallbacks.pfnInternalFree       = Helper::DummyInternalFreeCallback;
            VkDummyCallbacks.pfnReallocation       = Helper::DummyReallocCallback;

            VkAllocationCallbacks*       pVkCallbacks = nullptr;
            Helper::SSwapChainAllocator* pInternalAllocator =
                reinterpret_cast< Helper::SSwapChainAllocator* >( pOut->pInternalAllocator );
            if( pOut->pInternalAllocator == nullptr )
            {
                // pInternalAllocator = VKE_NEW Helper::SSwapChainAllocator;
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

            if( pOut->hSurface == NativeAPI::Null )
            {
#if VKE_USE_VULKAN_WINDOWS
                HINSTANCE                   hInst = reinterpret_cast< HINSTANCE >( Desc.pWindow->GetDesc().hProcess );
                HWND                        hWnd  = reinterpret_cast< HWND >( Desc.pWindow->GetDesc().hWnd );
                VkWin32SurfaceCreateInfoKHR SurfaceCI;
                Vulkan::InitInfo( &SurfaceCI, VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR );
                SurfaceCI.flags     = 0;
                SurfaceCI.hinstance = hInst;
                SurfaceCI.hwnd      = hWnd;
                vkRes               = NativeAPI::SImplementation::sInstanceICD.vkCreateWin32SurfaceKHR(
                    NativeAPI::SImplementation::sVkInstance, &SurfaceCI, pVkCallbacks, &hSurface );
#elif VKE_USE_VULKAN_LINUX
                VkXcbSurfaceCreateInfoKHR SurfaceCI;
                Vulkan::InitInfo( &SurfaceCI, VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR );
                SurfaceCI.flags      = 0;
                SurfaceCI.connection = reinterpret_cast< xcb_connection_t* >( m_Desc.hPlatform );
                SurfaceCI.window     = m_Desc.hWnd;
                EXPECT_SUCCESS( Vk.vkCreateXcbSurfaceKHR( s_instance, &SurfaceCI, NO_ALLOC_CALLBACK, &s_surface ) )
#elif VKE_USE_VULKAN_ANDROID
                VkAndroidSurfaceCreateInfoKHR SurfaceCI;
                Vulkan::InitInfo( &SurfaceCI, VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR );
                SurfaceCI.flags  = 0;
                SurfaceCI.window = m_Desc.hWnd;
                EXPECT_SUCCESS( Vk.vkCreateAndroidSurfaceKHR(
                    s_instance, s_window.window->getNativeHandle(), NO_ALLOC_CALLBACK, &s_surface ) );
#endif
                VK_ERR( vkRes );
                if( vkRes == VK_SUCCESS )
                {
                    VkBool32   isSurfaceSupported = VK_FALSE;
                    const auto queueIndex         = Desc.queueFamilyIndex;
                    VK_ERR( NativeAPI::SImplementation::sInstanceICD.vkGetPhysicalDeviceSurfaceSupportKHR(
                        m_hAdapter, queueIndex, hSurface, &isSurfaceSupported ) );
                    if( !isSurfaceSupported )
                    {
                        VKE_LOG_ERR( "Queue index: " << queueIndex << " does not support the surface." );
                        NativeAPI::SImplementation::sInstanceICD.vkDestroySurfaceKHR(
                            NativeAPI::SImplementation::sVkInstance, hSurface, pVkCallbacks );
                    }
                }
            }
            {
                SPresentSurfaceCaps& Caps = pOut->Caps;
                ret                       = QueryPresentSurfaceCaps( hSurface, &Caps );
                Size                      = Caps.CurrentSize;
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
                            found        = true;
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
                    found      = Caps.vModes.Find( pOut->mode ) != Caps.vModes.Npos();
                }
                else
                {
                    pOut->mode = PresentModes::MAILBOX;
                    found      = Caps.vModes.Find( pOut->mode ) != Caps.vModes.Npos();
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
                pOut->Size     = Caps.CurrentSize;
                pOut->hSurface = hSurface;
                if( Constants::_SOptimal::IsOptimal( elementCount ) )
                {
                    elementCount = std::min< uint16_t >( static_cast< uint16_t >( Caps.minImageCount ), 2u );
                }
                else
                {
                    elementCount = std::min< uint16_t >( elementCount, static_cast< uint16_t >( Caps.maxImageCount ) );
                }
            }
            static const VkColorSpaceKHR aVkColorSpaces[] = { VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };

            static const VkPresentModeKHR aVkModes[] = { VK_PRESENT_MODE_FIFO_KHR,      // as undefined
                                                         VK_PRESENT_MODE_IMMEDIATE_KHR, // immediate
                                                         VK_PRESENT_MODE_MAILBOX_KHR,   // mailbox
                                                         VK_PRESENT_MODE_FIFO_KHR,      // fifo
                                                         VK_PRESENT_MODE_FIFO_KHR };

            static const VkComponentMapping vkDefaultMapping = {
                // VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A
                VK_COMPONENT_SWIZZLE_IDENTITY,
                VK_COMPONENT_SWIZZLE_IDENTITY,
                VK_COMPONENT_SWIZZLE_IDENTITY,
                VK_COMPONENT_SWIZZLE_IDENTITY,
            };

            {
                uint32_t                 familyIndex = Desc.queueFamilyIndex;
                VkResult                 res;
                VkSwapchainCreateInfoKHR SwapChainCI;
                {
                    auto& ci                 = SwapChainCI;
                    ci.sType                 = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
                    ci.pNext                 = nullptr;
                    ci.clipped               = VK_TRUE;
                    ci.compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
                    ci.flags                 = 0;
                    ci.imageArrayLayers      = 1;
                    ci.imageColorSpace       = aVkColorSpaces[ pOut->Format.colorSpace ];
                    ci.imageExtent.width     = Size.width;
                    ci.imageExtent.height    = Size.height;
                    ci.imageFormat           = Map::Format( pOut->Format.format );
                    ci.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
                    ci.imageUsage            = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
                    ci.minImageCount         = elementCount;
                    ci.oldSwapchain          = pOut->hSwapChain;
                    ci.pQueueFamilyIndices   = &familyIndex;
                    ci.queueFamilyIndexCount = 1;
                    ci.presentMode           = aVkModes[ pOut->mode ];
                    ci.preTransform          = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
                    ci.surface               = pOut->hSurface;
                    res = m_Implementation.m_ICD.vkCreateSwapchainKHR( m_hDevice, &ci, pVkCallbacks, &hSwapChain );
                }
                VK_ERR( res );
                if( res == VK_SUCCESS )
                {
                    uint32_t imgCount = 0;
                    res = m_Implementation.m_ICD.vkGetSwapchainImagesKHR( m_hDevice, hSwapChain, &imgCount, nullptr );
                    VK_ERR( res );
                    if( res == VK_SUCCESS )
                    {
                        if( imgCount <= Desc.backBufferCount )
                        {
                            pOut->vImages.Resize( imgCount );
                            pOut->vImageViews.Resize( imgCount );
                            pOut->vFramebuffers.Resize( imgCount );
                            res = m_Implementation.m_ICD.vkGetSwapchainImagesKHR(
                                m_hDevice, hSwapChain, &imgCount, &pOut->vImages[ 0 ] );
                            VK_ERR( res );
                            if( res == VK_SUCCESS )
                            {
                                Utils::TCDynamicArray< VkImageMemoryBarrier > vVkBarriers;

                                for( uint32_t i = 0; i < imgCount; ++i )
                                {
                                    VkImageViewCreateInfo ci;
                                    ci.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                                    ci.pNext                           = nullptr;
                                    ci.flags                           = 0;
                                    ci.format                          = SwapChainCI.imageFormat;
                                    ci.image                           = pOut->vImages[ i ];
                                    ci.components                      = vkDefaultMapping;
                                    ci.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
                                    ci.subresourceRange.baseArrayLayer = 0;
                                    ci.subresourceRange.baseMipLevel   = 0;
                                    ci.subresourceRange.layerCount     = 1;
                                    ci.subresourceRange.levelCount     = 1;
                                    ci.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
                                    NativeAPI::TextureView hView;
                                    res = m_Implementation.m_ICD.vkCreateImageView(
                                        m_hDevice, &ci, pVkCallbacks, &hView );
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
                                        vkBarrier.sType                       = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                                        vkBarrier.pNext                       = nullptr;
                                        vkBarrier.image                       = pOut->vImages[ i ];
                                        vkBarrier.oldLayout                   = VK_IMAGE_LAYOUT_UNDEFINED;
                                        vkBarrier.newLayout                   = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
                                        vkBarrier.dstAccessMask               = VK_ACCESS_MEMORY_READ_BIT;
                                        vkBarrier.dstQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED;
                                        vkBarrier.srcQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED;
                                        vkBarrier.srcAccessMask               = 0;
                                        vkBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                                        vkBarrier.subresourceRange.baseArrayLayer = 0;
                                        vkBarrier.subresourceRange.baseMipLevel   = 0;
                                        vkBarrier.subresourceRange.layerCount     = 1;
                                        vkBarrier.subresourceRange.levelCount     = 1;
                                        vVkBarriers.PushBack( vkBarrier );
                                    }
                                    // Create framebuffers for render pass
                                    {
                                        _CreateDebugInfo< VK_OBJECT_TYPE_IMAGE >(
                                            this, pOut->vImages[ i ], "Swapchain Image" );
                                        _CreateDebugInfo< VK_OBJECT_TYPE_IMAGE_VIEW >(
                                            this, pOut->vImageViews[ i ], "Swapchain ImageView" );
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
                                    // VKE_ASSERT2( Desc.pCtx != nullptr, "GraphicsContext must be set." );
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
                DestroyTextureView( &pOut->vImageViews[ i ], pVkCallbacks );
            }
            if( hSwapChain != NativeAPI::Null )
            {
                m_Implementation.m_ICD.vkDestroySwapchainKHR( m_hDevice, hSwapChain, pVkCallbacks );
            }
            if( hSurface != NativeAPI::Null )
            {
                NativeAPI::SImplementation::sInstanceICD.vkDestroySurfaceKHR(
                    NativeAPI::SImplementation::sVkInstance, hSurface, pVkCallbacks );
            }
            pInternalAllocator->Reset();
            return ret;
        }

        Result CDDI::ReCreateSwapChain( const SSwapChainDesc& Desc, SDDISwapChain* pOut )
        {
            Result ret                = VKE_FAIL;
            auto   pInternalAllocator = reinterpret_cast< Helper::SSwapChainAllocator* >( pOut->pInternalAllocator );
            VkAllocationCallbacks* pVkAllocator = &pInternalAllocator->VkCallbacks;

            // Desc.pCtx->GetCommandBuffer()->ExecuteBarriers();

            for( uint32_t i = 0; i < pOut->vImageViews.GetCount(); ++i )
            {
                DestroyTextureView( &pOut->vImageViews[ i ], pVkAllocator );
                DestroyFramebuffer( &pOut->vFramebuffers[ i ], pVkAllocator );
            }
            if( pOut->hSwapChain != NativeAPI::Null )
            {
                m_Implementation.m_ICD.vkDestroySwapchainKHR( m_hDevice, pOut->hSwapChain, pVkAllocator );
                pOut->hSwapChain = NativeAPI::Null;
            }
            if( pOut->hSurface != NativeAPI::Null )
            {
                NativeAPI::SImplementation::sInstanceICD.vkDestroySurfaceKHR(
                    NativeAPI::SImplementation::sVkInstance, pOut->hSurface, pVkAllocator );
                pOut->hSurface = NativeAPI::Null;
            }
            if( pOut->hDDIRenderPass != NativeAPI::Null )
            {
                m_Implementation.m_ICD.vkDestroyRenderPass( m_hDevice, pOut->hDDIRenderPass->hNativeRenderPass, pVkAllocator );
                pOut->hDDIRenderPass = NativeAPI::Null;
            }
            pOut->vFramebuffers.Clear();
            pOut->vImages.Clear();
            pOut->vImageViews.Clear();
            pOut->hSwapChain = NativeAPI::Null;
            pInternalAllocator->FreeCurrentChunk();
            // DestroySwapChain( pOut, nullptr );
            ret = CreateSwapChain( Desc, nullptr, pOut );
            return ret;
        }

        Result CDDI::QueryPresentSurfaceCaps( const NativeAPI::PresentSurface& hSurface, SPresentSurfaceCaps* pOut )
        {
            Result   ret = VKE_FAIL;
            VkResult res;

            VkSurfaceCapabilitiesKHR vkSurfaceCaps;
            NativeAPI::SImplementation::sInstanceICD.vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
                m_hAdapter, hSurface, &vkSurfaceCaps );
            auto hasColorAttachment = vkSurfaceCaps.supportedUsageFlags | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

            // Select surface format
            Utils::TCDynamicArray< VkSurfaceFormatKHR > vSurfaceFormats;
            uint32_t                                    formatCount = 0;
            res = NativeAPI::SImplementation::sInstanceICD.vkGetPhysicalDeviceSurfaceFormatsKHR(
                m_hAdapter, hSurface, &formatCount, nullptr );
            VK_ERR( res );

            if( res == VK_SUCCESS )
            {
                if( formatCount > 0 )
                {
                    vSurfaceFormats.Resize( formatCount );
                    res = NativeAPI::SImplementation::sInstanceICD.vkGetPhysicalDeviceSurfaceFormatsKHR(
                        m_hAdapter, hSurface, &formatCount, &vSurfaceFormats[ 0 ] );
                    VK_ERR( res );
                    if( res == VK_SUCCESS )
                    {
                        for( VkSurfaceFormatKHR& vkFormat: vSurfaceFormats )
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
                uint32_t                                     presentCount = 0;
                Utils::TCDynamicArray< VkPresentModeKHR, 8 > vPresents;
                res = NativeAPI::SImplementation::sInstanceICD.vkGetPhysicalDeviceSurfacePresentModesKHR(
                    m_hAdapter, hSurface, &presentCount, nullptr );
                VK_ERR( res );
                if( res == VK_SUCCESS )
                {
                    if( presentCount > 0 )
                    {
                        vPresents.Resize( presentCount );
                        res = NativeAPI::SImplementation::sInstanceICD.vkGetPhysicalDeviceSurfacePresentModesKHR(
                            m_hAdapter, hSurface, &presentCount, &vPresents[ 0 ] );
                        VK_ERR( res );
                        if( res == VK_SUCCESS )
                        {
                            const static std::unordered_map< VkPresentModeKHR, PRESENT_MODE > mModes = {
                                { VkPresentModeKHR::VK_PRESENT_MODE_IMMEDIATE_KHR, PresentModes::IMMEDIATE },
                                { VkPresentModeKHR::VK_PRESENT_MODE_MAILBOX_KHR, PresentModes::MAILBOX },
                                { VkPresentModeKHR::VK_PRESENT_MODE_FIFO_KHR, PresentModes::FIFO },
                                { VkPresentModeKHR::VK_PRESENT_MODE_FIFO_RELAXED_KHR, PresentModes::FIFO },
                            };

                            for( VkPresentModeKHR& vkMode: vPresents )
                            {
                                auto presentMode = mModes.find( vkMode );
                                if( presentMode == mModes.end() )
                                {
                                    VKE_LOG_WARN( "Unsupported present mode: " << static_cast< int >( vkMode ) );
                                    pOut->vModes.PushBack( PresentModes::UNDEFINED );
                                }
                                else
                                {
                                    pOut->vModes.PushBack( presentMode->second );
                                }
                            }
                        }
                    }
                }

                if( vkSurfaceCaps.maxImageCount == 0 )
                {
                    vkSurfaceCaps.maxImageCount = Constants::RenderSystem::MAX_SWAP_CHAIN_ELEMENTS;
                }

                pOut->MinSize.width           = static_cast< uint16_t >( vkSurfaceCaps.minImageExtent.width );
                pOut->MinSize.height          = static_cast< uint16_t >( vkSurfaceCaps.minImageExtent.height );
                pOut->MaxSize.width           = static_cast< uint16_t >( vkSurfaceCaps.maxImageExtent.width );
                pOut->MaxSize.height          = static_cast< uint16_t >( vkSurfaceCaps.maxImageExtent.height );
                pOut->CurrentSize.width       = static_cast< uint16_t >( vkSurfaceCaps.currentExtent.width );
                pOut->CurrentSize.height      = static_cast< uint16_t >( vkSurfaceCaps.currentExtent.height );
                pOut->minImageCount           = static_cast< uint16_t >( vkSurfaceCaps.minImageCount );
                pOut->maxImageCount           = static_cast< uint16_t >( vkSurfaceCaps.maxImageCount );
                pOut->canBeUsedAsRenderTarget = hasColorAttachment;
                // pOut->transform = vkSurfaceCaps.currentTransform
            }
            return ret;
        }

        void CDDI::DestroySwapChain( SDDISwapChain* pInOut, const void* )
        {
            Helper::SSwapChainAllocator* pInternalAllocator =
                reinterpret_cast< Helper::SSwapChainAllocator* >( pInOut->pInternalAllocator );
            const VkAllocationCallbacks* pVkAllocator = &pInternalAllocator->VkCallbacks;
            for( uint32_t i = 0; i < pInOut->vImageViews.GetCount(); ++i )
            {
                DestroyTextureView( &pInOut->vImageViews[ i ], pVkAllocator );
            }
            if( pInOut->hSwapChain != NativeAPI::Null )
            {
                m_Implementation.m_ICD.vkDestroySwapchainKHR( m_hDevice, pInOut->hSwapChain, pVkAllocator );
                pInOut->hSwapChain = NativeAPI::Null;
            }
            if( pInOut->hSurface != NativeAPI::Null )
            {
                NativeAPI::SImplementation::sInstanceICD.vkDestroySurfaceKHR(
                    NativeAPI::SImplementation::sVkInstance, pInOut->hSurface, pVkAllocator );
                pInOut->hSurface = NativeAPI::Null;
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
            VkFence  hFence = Info.hSignalCPUFence;
            VkSemaphore hSemaphore = Info.hSignalGPUFence;

            if( Info.hSignalFence )
            {
                const auto pFences = Info.hSignalFence->Signal( this, Info.signalFenceValue );
                //hFence              = pFences->hFence;
                hFence = pFences->hFence;
                hSemaphore = NativeAPI::Null;
                VKE_ASSERT( hFence != NativeAPI::Null || hSemaphore != NativeAPI::Null );
            }
            VkResult res = m_Implementation.m_ICD.vkAcquireNextImageKHR(
                m_hDevice, SwapChain.hSwapChain, Info.waitTimeout, hSemaphore, hFence, pOut );
            
            switch( res )
            {
                case VK_SUCCESS: {
                    ret = VKE_OK;
                }
                break;
                case VK_TIMEOUT: {
                    ret = VKE_TIMEOUT;
                    break;
                }
                case VK_NOT_READY:
                case VK_SUBOPTIMAL_KHR: {
                    ret = VKE_ENOTREADY;
                    break;
                }
                case VK_ERROR_VALIDATION_FAILED_EXT: {

                    VKE_LOG( res );
                }
                break;
                case VK_ERROR_DEVICE_LOST: {
                    ret = VKE_EDEVICELOST;
                }
                break;
                case VK_ERROR_OUT_OF_DATE_KHR:
                case VK_ERROR_SURFACE_LOST_KHR: {
                    ret = VKE_EOUTOFDATE;
                }
                break;
                default: {
                    VK_ERR( res );
                }
                break;
            }
            
            return ret;
        }

        void CDDI::Reset( const NativeAPI::CommandBuffer&     hCommandBuffer,
                          const NativeAPI::CommandBufferPool& hCommandBufferPool )
        {
            const auto flags = VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT;
            VK_ERR( m_Implementation.m_ICD.vkResetCommandBuffer( hCommandBuffer, flags ) );
        }

        void CDDI::BeginCommandBuffer( const NativeAPI::CommandBuffer& hCommandBuffer, const NativeAPI::CommandBufferPool& hCommandBufferPool )
        {
            VkCommandBufferBeginInfo bi;
            bi.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            bi.pNext            = nullptr;
            bi.flags            = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            bi.pInheritanceInfo = nullptr;
            VK_ERR( m_Implementation.m_ICD.vkBeginCommandBuffer( hCommandBuffer, &bi ) );
        }

        void CDDI::EndCommandBuffer( const NativeAPI::CommandBuffer& hCommandBuffer )
        {
            VK_ERR( m_Implementation.m_ICD.vkEndCommandBuffer( hCommandBuffer ) );
        }

        void CDDI::Bind( const SBindPipelineInfo& Info )
        {
            VKE_ASSERT2( Info.pCmdBuffer != nullptr && Info.pCmdBuffer->GetDDIObject() != NativeAPI::Null &&
                             Info.pPipeline != nullptr && Info.pPipeline->GetDDIObject() != NativeAPI::Null,
                         "Invalid parameter" );
            m_Implementation.m_ICD.vkCmdBindPipeline( Info.pCmdBuffer->GetDDIObject(),
                                                      Convert::PipelineTypeToBindPoint( Info.pPipeline->GetType() ),
                                                      Info.pPipeline->GetDDIObject() );
        }

        void CDDI::UnbindPipeline( const NativeAPI::CommandBuffer&, const NativeAPI::Pipeline& )
        {
        }

     

        void CDDI::BeginRenderPass( NativeAPI::CommandBuffer hCommandBuffer, const SBeginRenderPassInfo& Info )
        {
            if( Info.hDDIRenderPass->hNativeRenderPass != NativeAPI::Null )
            {
                
                
                m_Implementation.m_ICD.vkCmdBeginRenderPass( hCommandBuffer, &Info.hDDIRenderPass->NativeBeginInfo, VK_SUBPASS_CONTENTS_INLINE );
            }
            else
            {
                VkRenderingInfo VkInfo = Info.hDDIRenderPass->VkInfo;
                if( Info.RenderArea.Size.width > 0 )
                {
                    Convert::RenderSystemToVkRect2D( Info.RenderArea, &VkInfo.renderArea );
                }
                m_Implementation.m_ICD.vkCmdBeginRenderingKHR( hCommandBuffer, &VkInfo );
            }
        }

        void CDDI::BeginRenderPass( NativeAPI::CommandBuffer hCommandBuffer, const SBeginRenderPassInfo2& Info )
        {
            Utils::TCDynamicArray< VkRenderingAttachmentInfoKHR, 8 > vVkAttachments;

            VkRenderingInfoKHR vkInfo;
            vkInfo.sType                = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
            vkInfo.pNext                = nullptr;
            vkInfo.flags                = 0;
            vkInfo.colorAttachmentCount = Info.vColorRenderTargetInfos.GetCount();
            Convert::RenderSystemToVkRect2D( Info.RenderArea, &vkInfo.renderArea );
            vkInfo.layerCount = Info.renderTargetLayerCount;
            vkInfo.viewMask   = Info.renderTargetLayerIndex;

            VkRenderingAttachmentInfoKHR vkRTInfo;
            vkRTInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
            vkRTInfo.pNext = nullptr;

            for( uint32_t i = 0; i < Info.vColorRenderTargetInfos.GetCount(); ++i )
            {
                const auto& RTInfo = Info.vColorRenderTargetInfos[ i ];
                Convert::ClearValues( &RTInfo.ClearColor, 1, &vkRTInfo.clearValue );
                vkRTInfo.imageLayout        = Map::ImageLayout( RTInfo.state );
                vkRTInfo.imageView          = RTInfo.hDDIView;
                vkRTInfo.loadOp             = Convert::UsageToLoadOp( RTInfo.renderPassOp );
                vkRTInfo.storeOp            = Convert::UsageToStoreOp( RTInfo.renderPassOp );
                vkRTInfo.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                vkRTInfo.resolveImageView   = VK_NULL_HANDLE;
                vkRTInfo.resolveMode        = VK_RESOLVE_MODE_NONE;
                vVkAttachments.PushBack( vkRTInfo );
            }

            VkRenderingAttachmentInfoKHR vkDepthAttachment   = { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR };
            VkRenderingAttachmentInfoKHR vkStencilAttachment = { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR };

            vkInfo.pDepthAttachment   = nullptr;
            vkInfo.pStencilAttachment = nullptr;

            if( Info.DepthRenderTargetInfo.hDDIView != NativeAPI::Null )
            {
                const auto& RT           = Info.DepthRenderTargetInfo;
                auto&       vkAttachment = vkDepthAttachment;
                Convert::ClearValues( &RT.ClearColor, 1, &vkAttachment.clearValue );
                vkAttachment.imageLayout        = Map::ImageLayout( RT.state );
                vkAttachment.imageView          = RT.hDDIView;
                vkAttachment.loadOp             = Convert::UsageToLoadOp( RT.renderPassOp );
                vkAttachment.storeOp            = Convert::UsageToStoreOp( RT.renderPassOp );
                vkAttachment.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                vkAttachment.resolveImageView   = VK_NULL_HANDLE;
                vkAttachment.resolveMode        = VK_RESOLVE_MODE_NONE;

                vkInfo.pDepthAttachment = &vkAttachment;
            }

            if( Info.StencilRenderTargetInfo.hDDIView != NativeAPI::Null )
            {
                const auto& RT           = Info.StencilRenderTargetInfo;
                auto&       vkAttachment = vkStencilAttachment;
                Convert::ClearValues( &RT.ClearColor, 1, &vkAttachment.clearValue );
                vkAttachment.imageLayout        = Map::ImageLayout( RT.state );
                vkAttachment.imageView          = RT.hDDIView;
                vkAttachment.loadOp             = Convert::UsageToLoadOp( RT.renderPassOp );
                vkAttachment.storeOp            = Convert::UsageToStoreOp( RT.renderPassOp );
                vkAttachment.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                vkAttachment.resolveImageView   = VK_NULL_HANDLE;
                vkAttachment.resolveMode        = VK_RESOLVE_MODE_NONE;

                vkInfo.pStencilAttachment = &vkAttachment;
            }

            vkInfo.pColorAttachments = vVkAttachments.GetDataOrNull();

            m_Implementation.m_ICD.vkCmdBeginRenderingKHR( hCommandBuffer, &vkInfo );
        }

        /*void CDDI::EndRenderPass( NativeAPI::CommandBuffer hDDICommandBuffer )
        {
            m_Implementation.m_ICD.vkCmdEndRenderingKHR( hDDICommandBuffer );
        }*/

        void CDDI::EndRenderPass( NativeAPI::CommandBuffer hDDICommandBuffer, NativeAPI::RenderPass hPass )
        {
            if( hPass->hNativeRenderPass != NativeAPI::Null )
            {
                m_Implementation.m_ICD.vkCmdEndRenderPass( hDDICommandBuffer );
            }
            else
            {
                m_Implementation.m_ICD.vkCmdEndRenderingKHR( hDDICommandBuffer );
            }
        }

        void CDDI::Bind( const SBindDDIDescriptorSetsInfo& Info )
        {
            m_Implementation.m_ICD.vkCmdBindDescriptorSets( Info.hDDICommandBuffer,
                                                            Convert::PipelineTypeToBindPoint( Info.pipelineType ),
                                                            Info.hDDIPipelineLayout,
                                                            Info.firstSet,
                                                            Info.setCount,
                                                            Info.aDDISetHandles,
                                                            Info.dynamicOffsetCount,
                                                            Info.aDynamicOffsets );
        }

        void CDDI::Bind( const NativeAPI::CommandBuffer& hDDICmdBuffer, const NativeAPI::Buffer& hDDIBuffer,
                         const uint32_t offset )
        {
            VkDeviceSize ddiOffset = offset;
            m_Implementation.m_ICD.vkCmdBindVertexBuffers( hDDICmdBuffer, 0, 1, &hDDIBuffer, &ddiOffset );
        }

        void CDDI::Bind( const NativeAPI::CommandBuffer& hDDICmdBuffer, const NativeAPI::Buffer& hDDIBuffer,
                         const uint32_t offset, const INDEX_TYPE& type )
        {
            m_Implementation.m_ICD.vkCmdBindIndexBuffer( hDDICmdBuffer, hDDIBuffer, offset, Map::IndexType( type ) );
        }

        void CDDI::SetState( const NativeAPI::CommandBuffer& hCommandBuffer, const SViewportDesc& Desc )
        {
            VkViewport Viewport;
            Viewport.width = Desc.Size.width;
            Viewport.x     = Desc.Position.x;
#if VKE_VULKAN_NEGATIVE_VIEWPORT_HEIGT
            Viewport.y      = Desc.Size.height + Desc.Position.y;
            Viewport.height = -Viewport.y;
#else
            Viewport.height = Desc.Size.height;
            Viewport.y      = Desc.Position.y;
#endif
            Viewport.minDepth = Desc.MinMaxDepth.min;
            Viewport.maxDepth = Desc.MinMaxDepth.max;
            m_Implementation.m_ICD.vkCmdSetViewport( hCommandBuffer, 0, 1, &Viewport );
        }

        void CDDI::SetState( const NativeAPI::CommandBuffer& hCommandBuffer, const SScissorDesc& Desc )
        {
            VkRect2D Scissor;
            Scissor.extent.width  = Desc.Size.width;
            Scissor.extent.height = Desc.Size.height;
            Scissor.offset.x      = Desc.Position.x;
            Scissor.offset.y      = Desc.Position.y;
            m_Implementation.m_ICD.vkCmdSetScissor( hCommandBuffer, 0, 1, &Scissor );
        }

        void CDDI::Barrier( const NativeAPI::CommandBuffer& hCommandBuffer, const SBarrierInfo& Info )
        {
            VkMemoryBarrier*       pVkMemBarriers = nullptr;
            VkImageMemoryBarrier*  pVkImgBarriers = nullptr;
            VkBufferMemoryBarrier* pVkBuffBarrier = nullptr;
            VkPipelineStageFlags   srcStage       = 0;
            VkPipelineStageFlags   dstStage       = 0;

            Utils::TCDynamicArray< VkMemoryBarrier, SBarrierInfo::MAX_BARRIER_COUNT > vVkMemBarriers(
                Info.vMemoryBarriers.GetCount() );
            Utils::TCDynamicArray< VkImageMemoryBarrier, SBarrierInfo::MAX_BARRIER_COUNT >
                vVkImgBarriers /*( Info.vTextureBarriers.GetCount() )*/;
            Utils::TCDynamicArray< VkBufferMemoryBarrier, SBarrierInfo::MAX_BARRIER_COUNT > vVkBufferBarriers(
                Info.vBufferBarriers.GetCount() );

            {
                const auto& Barriers = Info.vMemoryBarriers;
                if( !Barriers.IsEmpty() )
                {
                    for( uint32_t i = 0; i < Barriers.GetCount(); ++i )
                    {
                        vVkMemBarriers[ i ] = { VK_STRUCTURE_TYPE_MEMORY_BARRIER };
                        Convert::Barrier( &vVkMemBarriers[ i ], Barriers[ i ] );
                        dstStage |= Convert::AccessMaskToPipelineStage( Barriers[ i ].dstMemoryAccess );
                        srcStage |= Convert::AccessMaskToPipelineStage( Barriers[ i ].srcMemoryAccess );
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
                        // vVkImgBarriers[i] = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
                        vVkImgBarriers.PushBack( { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER } );
                        auto& VkBarrier = vVkImgBarriers.Back();
                        Convert::Barrier( &VkBarrier, Barriers[ i ] );
                        VkBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                        VkBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                        dstStage |= Convert::AccessMaskToPipelineStage( Barriers[ i ].dstMemoryAccess );
                        srcStage |= Convert::AccessMaskToPipelineStage( Barriers[ i ].srcMemoryAccess );
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
                        vVkBufferBarriers[ i ] = { VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
                        Convert::Barrier( &vVkBufferBarriers[ i ], Barriers[ i ] );
                        vVkBufferBarriers[ i ].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                        vVkBufferBarriers[ i ].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                        // const VkAccessFlags flags = vVkBufferBarriers[i].dstAccessMask;
                        dstStage |= Convert::AccessMaskToPipelineStage( Barriers[ i ].dstMemoryAccess );
                        srcStage |= Convert::AccessMaskToPipelineStage( Barriers[ i ].srcMemoryAccess );
                    }
                    pVkBuffBarrier = vVkBufferBarriers.GetData();
                }
            }

            m_Implementation.m_ICD.vkCmdPipelineBarrier( hCommandBuffer,
                                                         srcStage,
                                                         dstStage,
                                                         0,
                                                         Info.vMemoryBarriers.GetCount(),
                                                         pVkMemBarriers,
                                                         Info.vBufferBarriers.GetCount(),
                                                         pVkBuffBarrier,
                                                         Info.vTextureBarriers.GetCount(),
                                                         pVkImgBarriers );
        }

        void CDDI::Convert( const SClearValue& In, NativeAPI::ClearValue* pOut )
        {
            Memory::Copy( pOut, sizeof( NativeAPI::ClearValue ), &In, sizeof( SClearValue ) );
        }

        void CDDI::BeginDebugInfo( const NativeAPI::CommandBuffer& hDDICmdBuff, const SDebugInfo* pInfo )
        {
            if( NativeAPI::SImplementation::sInstanceICD.vkCmdBeginDebugUtilsLabelEXT && pInfo )
            {
                VkDebugUtilsLabelEXT li = {};
                li.color[ 0 ]           = pInfo->Color.r;
                li.color[ 1 ]           = pInfo->Color.g;
                li.color[ 2 ]           = pInfo->Color.b;
                li.color[ 3 ]           = pInfo->Color.a;
                li.pLabelName           = pInfo->pText;
                li.pNext                = nullptr;
                li.sType                = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
                NativeAPI::SImplementation::sInstanceICD.vkCmdBeginDebugUtilsLabelEXT( hDDICmdBuff, &li );
            }
        }

        void CDDI::EndDebugInfo( const NativeAPI::CommandBuffer& hDDICmdBuff )
        {
            if( NativeAPI::SImplementation::sInstanceICD.vkCmdEndDebugUtilsLabelEXT )
            {
                NativeAPI::SImplementation::sInstanceICD.vkCmdEndDebugUtilsLabelEXT( hDDICmdBuff );
            }
        }

        void CDDI::SetObjectDebugName( const uint64_t& handle, const uint32_t& objType, cstr_t pName ) const
        {
#if VKE_RENDER_SYSTEM_DEBUG
            if( NativeAPI::SImplementation::sInstanceICD.vkSetDebugUtilsObjectNameEXT && pName )
            {
                VKE_ASSERT2( strlen( pName ) > 0, "VKE_RENDER_SYSTEM_DEBUG requires debug names for all objects." );
                VKE_ASSERT2( m_hDevice != NativeAPI::Null, "Device must be created first!" );
                VkDebugUtilsObjectNameInfoEXT ni;
                ni.sType        = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
                ni.pNext        = nullptr;
                ni.objectHandle = handle;
                ni.objectType   = (VkObjectType)objType;
                ni.pObjectName  = pName;
                NativeAPI::SImplementation::sInstanceICD.vkSetDebugUtilsObjectNameEXT( m_hDevice, &ni );
            }
#endif
        }

        void CDDI::SetQueueDebugName( uint64_t handle, cstr_t pName ) const
        {
            SetObjectDebugName( handle, VK_OBJECT_TYPE_QUEUE, pName );
        }

        VKAPI_ATTR VkBool32 VKAPI_CALL VkDebugCallback( VkDebugReportFlagsEXT      msgFlags,
                                                        VkDebugReportObjectTypeEXT objType, uint64_t srcObject,
                                                        size_t location, int32_t msgCode, const char* pLayerPrefix,
                                                        const char* pMsg, void* )
        {
            std::ostringstream message;
            (void)location;
            (void)srcObject;
            (void)objType;
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
            str      = std::regex_replace( str, std::regex( ";" ), "\n" );
            VKE_LOG( str );
            VKE_ASSERT2( ( msgFlags & VK_DEBUG_REPORT_ERROR_BIT_EXT ) == 0, message.str().c_str() );
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
            VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes,
            const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* /*pUserData*/ )
        {
#if VKE_LOG_RENDER_API_ERRORS
            (void)messageTypes;
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

    } // namespace RenderSystem
} // namespace VKE
#endif // VKE_RENDER_SYSTEM_VULKAN