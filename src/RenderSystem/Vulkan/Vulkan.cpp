#include "RenderSystem/Vulkan/Vulkan.h"
#include "Core/Platform/CPlatform.h"
#include "Core/Utils/CLogger.h"

//#undef VKE_VK_FUNCTION
//#define VKE_VK_FUNCTION(_name) PFN_##_name _name
//#undef VK_EXPORTED_FUNCTION
//#undef VKE_ICD_GLOBAL
//#undef VKE_INSTANCE_ICD
//#undef VKE_DEVICE_ICD
//#define VK_EXPORTED_FUNCTION(name) PFN_##name name = 0
//#define VKE_ICD_GLOBAL(name) PFN_##name name = 0
//#define VKE_INSTANCE_ICD(name) PFN_##name name = 0
//#define VKE_DEVICE_ICD(name) PFN_##name name = 0
//#include "ThirdParty/vulkan/funclist.h"
//#undef VKE_DEVICE_ICD
//#undef VKE_INSTANCE_ICD
//#undef VKE_ICD_GLOBAL
//#undef VK_EXPORTED_FUNCTION
//#undef VKE_VK_FUNCTION

namespace VKE
{
    namespace Vulkan
    {

        using ErrorMap = std::map< std::thread::id, VkResult >;
        ErrorMap g_mErrors;
        std::mutex g_ErrorMutex;

        void SetLastError( VkResult err )
        {
            g_ErrorMutex.lock();
            g_mErrors[ std::this_thread::get_id() ] = err;
            g_ErrorMutex.unlock();
        }

        VkResult GetLastError()
        {
            g_ErrorMutex.lock();
            auto ret = g_mErrors[ std::this_thread::get_id() ];
            g_ErrorMutex.unlock();
            return ret;
        }

        SQueue::SQueue()
        {
            this->m_objRefCount = 0;
            Vulkan::InitInfo( &m_PresentInfo, VK_STRUCTURE_TYPE_PRESENT_INFO_KHR );
            m_PresentInfo.pResults = nullptr;
        }

        VkResult SQueue::Submit( const VkICD::Device& ICD, const VkSubmitInfo& Info, const VkFence& vkFence )
        {
            Lock();
            auto res = ICD.vkQueueSubmit( vkQueue, 1, &Info, vkFence );
            Unlock();
            return res;
        }

        bool SQueue::IsPresentDone()
        {
            return m_isPresentDone;
        }

        void SQueue::ReleasePresentNotify()
        {
            Lock();
            if( m_presentCount-- < 0 )
                m_presentCount = 0;
            m_isPresentDone = m_presentCount == 0;
            Unlock();
        }

        void SQueue::Wait( const VkICD::Device& ICD )
        {
            ICD.vkQueueWaitIdle( vkQueue );
        }

        Result SQueue::Present( const VkICD::Device& ICD, uint32_t imgIdx, VkSwapchainKHR vkSwpChain,
                                VkSemaphore vkWaitSemaphore )
        {
            Result res = VKE_ENOTREADY;
            Lock();
            m_PresentData.vImageIndices.PushBack( imgIdx );
            m_PresentData.vSwapChains.PushBack( vkSwpChain );
            m_PresentData.vWaitSemaphores.PushBack( vkWaitSemaphore );
            m_presentCount++;
            m_isPresentDone = false;
            if( this->GetRefCount() == m_PresentData.vSwapChains.GetCount() )
            {
                m_PresentInfo.pImageIndices = &m_PresentData.vImageIndices[ 0 ];
                m_PresentInfo.pSwapchains = &m_PresentData.vSwapChains[ 0 ];
                m_PresentInfo.pWaitSemaphores = &m_PresentData.vWaitSemaphores[ 0 ];
                m_PresentInfo.swapchainCount = m_PresentData.vSwapChains.GetCount();
                m_PresentInfo.waitSemaphoreCount = m_PresentData.vWaitSemaphores.GetCount();
                VK_ERR( ICD.vkQueuePresentKHR( vkQueue, &m_PresentInfo ) );
                // $TID Present: q={vkQueue}, sc={m_PresentInfo.pSwapchains[0]}, imgIdx={m_PresentInfo.pImageIndices[0]}, ws={m_PresentInfo.pWaitSemaphores[0]}
                m_isPresentDone = true;
                m_PresentData.vImageIndices.Clear();
                m_PresentData.vSwapChains.Clear();
                m_PresentData.vWaitSemaphores.Clear();
                res = VKE_OK;
            }
            Unlock();
            return res;
        }

        bool IsColorImage( VkFormat format )
        {
            switch( format )
            {
                case VK_FORMAT_UNDEFINED:
                case VK_FORMAT_D16_UNORM:
                case VK_FORMAT_D16_UNORM_S8_UINT:
                case VK_FORMAT_D24_UNORM_S8_UINT:
                case VK_FORMAT_D32_SFLOAT:
                case VK_FORMAT_D32_SFLOAT_S8_UINT:
                case VK_FORMAT_X8_D24_UNORM_PACK32:
                case VK_FORMAT_S8_UINT:
                return false;
            }
            return true;
        }

        bool IsDepthImage( VkFormat format )
        {
            switch( format )
            {
                case VK_FORMAT_D16_UNORM:
                case VK_FORMAT_D16_UNORM_S8_UINT:
                case VK_FORMAT_D24_UNORM_S8_UINT:
                case VK_FORMAT_D32_SFLOAT:
                case VK_FORMAT_D32_SFLOAT_S8_UINT:
                case VK_FORMAT_X8_D24_UNORM_PACK32:
                case VK_FORMAT_S8_UINT:
                return true;
            }
            return false;
        }

        bool IsStencilImage( VkFormat format )
        {
            switch( format )
            {
                case VK_FORMAT_D16_UNORM_S8_UINT:
                case VK_FORMAT_D24_UNORM_S8_UINT:
                case VK_FORMAT_D32_SFLOAT_S8_UINT:
                case VK_FORMAT_S8_UINT:
                return true;
            }
            return false;
        }

        namespace Map
        {

            VkImageType ImageType( RenderSystem::TEXTURE_TYPE type )
            {
                static const VkImageType aVkImageTypes[] =
                {
                    VK_IMAGE_TYPE_1D,
                    VK_IMAGE_TYPE_2D,
                    VK_IMAGE_TYPE_3D,
                    VK_IMAGE_TYPE_2D, // cube
                };
                return aVkImageTypes[ type ];
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
                return aVkTypes[ type ];
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
                return aVkLayouts[ layout ];
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
                return aVkAspects[ aspect ];
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
                return aVkOps[ op ];
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
                return aVkFactors[ factor ];
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
                return aVkOps[ op ];
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
                return aVkOps[ op ];
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
                return aVkOps[ op ];
            }

            VkPrimitiveTopology PrimitiveTopology(const RenderSystem::PRIMITIVE_TOPOLOGY& topology)
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
                return aVkTopologies[ topology ];
            }

            VkSampleCountFlagBits SampleCount(const RenderSystem::SAMPLE_COUNT& count)
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
                return aVkSamples[ count ];
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
                return aVkModes[ mode ];
            }

            VkFrontFace FrontFace( const RenderSystem::FRONT_FACE& face )
            {
                static const VkFrontFace aVkFaces[] =
                {
                    VK_FRONT_FACE_COUNTER_CLOCKWISE,
                    VK_FRONT_FACE_CLOCKWISE
                };
                return aVkFaces[ face ];
            }

            VkPolygonMode PolygonMode( const RenderSystem::POLYGON_MODE& mode )
            {
                static const VkPolygonMode aVkModes[] =
                {
                    VK_POLYGON_MODE_FILL,
                    VK_POLYGON_MODE_LINE,
                    VK_POLYGON_MODE_POINT
                };
                return aVkModes[ mode ];
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
                return aVkBits[ type ];
            }

            VkVertexInputRate InputRate( const RenderSystem::VERTEX_INPUT_RATE& rate )
            {
                static const VkVertexInputRate aVkRates[] =
                {
                    VK_VERTEX_INPUT_RATE_VERTEX,
                    VK_VERTEX_INPUT_RATE_INSTANCE
                };
                return aVkRates[ rate ];
            }

            VkDescriptorType DescriptorType(const RenderSystem::DESCRIPTOR_SET_TYPE& type)
            {
                static const VkDescriptorType aVkDescriptorType[] =
                {
                    VK_DESCRIPTOR_TYPE_SAMPLER,
                    VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
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
                return aTypes[ type ];
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
                return aLoads[ usage ];
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
                return aStores[ usage ];
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
                };
                assert( 0 && "Cannot convert VkFormat to RenderSystem format" );
                return RenderSystem::Formats::UNDEFINED;
            }

            VkPipelineStageFlags PipelineStages(const RenderSystem::PIPELINE_STAGES& stages)
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
                    //vkTiling = VK_IMAGE_TILING_LINEAR;
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

        } // Convert

#define VKE_EXPORT_FUNC(_name, _handle, _getProcAddr) \
    pOut->_name = (PFN_##_name)(_getProcAddr((_handle), #_name)); \
    if(!pOut->_name) \
            { VKE_LOG_ERR("Unable to load function: " << #_name); err = VKE_ENOTFOUND; }

#define VKE_EXPORT_EXT_FUNC(_name, _handle, _getProcAddr) \
    pOut->_name = (PFN_##_name)(_getProcAddr((_handle), #_name)); \
    if(!pOut->_name) \
            { VKE_LOG_WARN("Unable to load EXT function: " << #_name); }

        Result LoadGlobalFunctions( handle_t hLib, VkICD::Global* pOut )
        {
            Result err = VKE_OK;
#if VKE_AUTO_ICD
#define VK_EXPORTED_FUNCTION(_name) VKE_EXPORT_FUNC(_name, hLib, Platform::DynamicLibrary::GetProcAddress)
#include "ThirdParty/vulkan/VKEICD.h"
#undef VK_EXPORTED_FUNCTION
#define VKE_ICD_GLOBAL(_name) VKE_EXPORT_FUNC(_name, VK_NULL_HANDLE, pOut->vkGetInstanceProcAddr)
#include "ThirdParty/vulkan/VKEICD.h"
#undef VKE_ICD_GLOBAL
#else // VKE_AUTO_ICD
            pOut->vkGetInstanceProcAddr = reinterpret_cast< PFN_vkGetInstanceProcAddr >( Platform::GetProcAddress( hLib, "vkGetInstanceProcAddr" ) );
            pOut->vkCreateInstance = reinterpret_cast< PFN_vkCreateInstance >( pOut->vkGetInstanceProcAddr( VK_NULL_HANDLE, "vkCreateInstance" ) );
            //pOut->vkDestroyInstance = reinterpret_cast< PFN_vkDestroyInstance >( pOut->vkGetInstanceProcAddr( VK_NULL_HANDLE, "vkDestroyInstance" ) );
            pOut->vkEnumerateInstanceExtensionProperties = reinterpret_cast< PFN_vkEnumerateInstanceExtensionProperties >( pOut->vkGetInstanceProcAddr( VK_NULL_HANDLE, "vkEnumerateInstanceExtensionProperties" ) );
            pOut->vkEnumerateInstanceLayerProperties = reinterpret_cast< PFN_vkEnumerateInstanceLayerProperties >( pOut->vkGetInstanceProcAddr( VK_NULL_HANDLE, "vkEnumerateInstanceLayerProperties" ) );
#endif // VKE_AUTO_ICD
            return err;
        }

        Result LoadInstanceFunctions( VkInstance vkInstance, const VkICD::Global& Global,
                                      VkICD::Instance* pOut )
        {
            Result err = VKE_OK;
#if VKE_AUTO_ICD
#   undef VKE_INSTANCE_ICD
#   undef VKE_INSTANCE_EXT_ICD
#   define VKE_INSTANCE_ICD(_name) VKE_EXPORT_FUNC(_name, vkInstance, Global.vkGetInstanceProcAddr)
#   define VKE_INSTANCE_EXT_ICD(_name) VKE_EXPORT_EXT_FUNC(_name, vkInstance, Global.vkGetInstanceProcAddr)
#       include "ThirdParty/vulkan/VKEICD.h"
#   undef VKE_INSTANCE_ICD
#   undef VKE_INSTANCE_EXT_ICD
#else // VKE_AUTO_ICD
            pOut->vkDestroySurfaceKHR = reinterpret_cast< PFN_vkDestroySurfaceKHR >( Global.vkGetInstanceProcAddr( vkInstance, "vkDestroySurfaceKHR" ) );
#endif // VKE_AUTO_ICD
            return err;
        }

        Result LoadDeviceFunctions( VkDevice vkDevice, const VkICD::Instance& Instance, VkICD::Device* pOut )
        {
            Result err = VKE_OK;
#if VKE_AUTO_ICD
#   undef VKE_DEVICE_ICD
#   undef VKE_DEVICE_EXT_ICD
#   define VKE_DEVICE_ICD(_name) VKE_EXPORT_FUNC(_name, vkDevice, Instance.vkGetDeviceProcAddr)
#   define VKE_DEVICE_EXT_ICD(_name) VKE_EXPORT_EXT_FUNC(_name, vkDevice, Instance.vkGetDeviceProcAddr);
#       include "ThirdParty/vulkan/VKEICD.h"
#   undef VKE_DEVICE_ICD
#   undef VKE_DEVICE_EXT_ICD
#else // VKE_AUTO_ICD

#endif // VKE_AUTO_ICD
            return err;
        }
    }
}