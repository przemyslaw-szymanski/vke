#pragma once

//#include "Core/VKECommon.h"
//#include "Core/CObject.h"
//#include "Core/Threads/Common.h"
//#include "Core/Utils/CLogger.h"
//#include "RenderSystem/Vulkan/VKEImageFormats.h"
//#include "Core/Utils/TCDynamicArray.h"
#include "RenderSystem/Common.h"

#define VKE_USE_VULKAN_KHR 1
#if VKE_WINDOWS
#   define VKE_USE_VULKAN_WINDOWS 1
#   define VK_USE_PLATFORM_WIN32_KHR 1
#elif VKE_LINUX
#   define VKE_USE_VULKAN_LINUX 1
#   define VKE_USE_VULKAN_LINUX 1
#   define VK_USE_PLATFORM_XCB_KHR 1
#elif VKE_ANDROID
#   define VKE_USE_VULKAN_ANDROID 1
#error implement here
#endif // VKE_WINDOWS
//#include "CDDITypes.h"
#ifdef __cplusplus
extern "C" {
#endif
#define VKE_AUTO_ICD 1
#define VK_EXPORTED_FUNCTION(name) PFN_##name name
#define VKE_ICD_GLOBAL(name) VK_EXPORTED_FUNCTION(name)
#define VKE_INSTANCE_ICD(name) VK_EXPORTED_FUNCTION(name)
#define VKE_INSTANCE_EXT_ICD(name) VK_EXPORTED_FUNCTION(name)
#define VKE_DEVICE_ICD(name) VK_EXPORTED_FUNCTION(name)
#define VKE_DEVICE_EXT_ICD(name) VK_EXPORTED_FUNCTION(name)
#define VKE_DECLARE_GLOBAL_ICD 1
#define VKE_DECLARE_INSTANCE_ICD 1
#define VKE_DECLARE_DEVICE_ICD 1
//#include "RenderSystem/Vulkan/VKEICD.h"
#undef VKE_DEVICE_ICD
#undef VKE_DEVICE_EXT_ICD
#undef VKE_INSTANCE_ICD
#undef VKE_INSTANCE_EXT_ICD
#undef VKE_ICD_GLOBAL
#undef VK_EXPORTED_FUNCTION
#undef VKE_DECLARE_GLOBAL_ICD
#undef VKE_DECLARE_INSTANCE_ICD
#undef VKE_DECLARE_DEVICE_ICD
#ifdef __cplusplus
} // extern "C"
#endif


#if VKE_USE_VULKAN_WINDOWS
#   define VK_KHR_PLATFORM_SURFACE_EXTENSION_NAME VK_KHR_WIN32_SURFACE_EXTENSION_NAME
#elif VKE_USE_VULKAN_LINUX
#   define VK_KHR_PLATFORM_SURFACE_EXTENSION_NAME VK_KHR_XCB_SURFACE_EXTENSION_NAME
#elif VKE_USE_VULKAN_ANDROID
#error implement here
#endif

#define VKE_VULKAN_NEGATIVE_VIEWPORT_HEIGT 1

namespace VKE::RenderSystem::Vulkan
{


        /*struct ICD
        {
            typedef struct
            {
                VkICD::Global   Global;
            } Global;

            typedef struct
            {
                VkICD::Global   Global;
                VkICD::Instance Instance;
            } Instance;

            typedef struct _Device
            {
                VkICD::Global&      Global;
                VkICD::Instance&    Instance;
                VkICD::Device       Device;

                _Device(VkICD::Global& G, VkICD::Instance& I) :
                    Global(G), Instance(I) {}
                void operator=(const _Device&) = delete;
            } Device;
        };*/

        
        //namespace Map
        //{
        //    VkSampleCountFlagBits SampleCount(const RenderSystem::SAMPLE_COUNT& count);
        //    VkImageType ImageType(RenderSystem::TEXTURE_TYPE type);
        //    VkImageViewType ImageViewType(RenderSystem::TEXTURE_VIEW_TYPE type);
        //    VkImageUsageFlags ImageUsage(RenderSystem::TEXTURE_USAGE usage);
        //    VkImageAspectFlags ImageAspect(RenderSystem::TEXTURE_ASPECT aspect);
        //    VkImageLayout ImageLayout(RenderSystem::TEXTURE_STATE layout);
        //    VkMemoryPropertyFlags MemoryProperyFlags(RenderSystem::MEMORY_USAGE usages);
        //    VkBlendOp BlendOp(const RenderSystem::BLEND_OPERATION& op);
        //    VkColorComponentFlags ColorComponent(const RenderSystem::ColorComponent& component);
        //    VkBlendFactor BlendFactor(const RenderSystem::BLEND_FACTOR& factor);
        //    VkLogicOp LogicOperation(const RenderSystem::LOGIC_OPERATION& op);
        //    VkStencilOp StencilOperation(const RenderSystem::STENCIL_FUNCTION& op);
        //    VkCompareOp CompareOperation(const RenderSystem::COMPARE_FUNCTION& op);
        //    VkPrimitiveTopology PrimitiveTopology(const RenderSystem::PRIMITIVE_TOPOLOGY& topology);
        //    VkCullModeFlags CullMode(const RenderSystem::CULL_MODE& mode);
        //    VkFrontFace FrontFace(const RenderSystem::FRONT_FACE& face);
        //    VkPolygonMode PolygonMode(const RenderSystem::POLYGON_MODE& mode);
        //    VkShaderStageFlagBits ShaderStage(const RenderSystem::SHADER_TYPE& type);
        //    VkVertexInputRate InputRate(const RenderSystem::VERTEX_INPUT_RATE& rate);
        //    VkDescriptorType DescriptorType(const RenderSystem::DESCRIPTOR_SET_TYPE& type);
        //} // Mapping

        //namespace Convert
        //{
        //    VkImageViewType ImageTypeToViewType(VkImageType type);
        //    VkImageAspectFlags UsageToAspectMask(VkImageUsageFlags usage);
        //    VkAttachmentStoreOp UsageToStoreOp(RenderSystem::RENDER_TARGET_RENDER_PASS_OP usage);
        //    VkAttachmentLoadOp UsageToLoadOp(RenderSystem::RENDER_TARGET_RENDER_PASS_OP usage);
        //    VkImageLayout ImageUsageToLayout(VkImageUsageFlags vkFlags);
        //    VkImageLayout ImageUsageToInitialLayout(VkImageUsageFlags vkFlags);
        //    VkImageLayout ImageUsageToFinalLayout(VkImageUsageFlags vkFlags);
        //    VkImageLayout NextAttachmentLayoutRread(VkImageLayout currLayout);
        //    VkImageLayout NextAttachmentLayoutOptimal(VkImageLayout currLayout);
        //    RenderSystem::TEXTURE_FORMAT ImageFormat(VkFormat vkFormat);
        //    VkPipelineStageFlags PipelineStages(const RenderSystem::PIPELINE_STAGES& stages);
        //    VkBufferUsageFlags BufferUsage( const RenderSystem::BUFFER_USAGE& usage );
        //    VkImageTiling ImageUsageToTiling( const RenderSystem::TEXTURE_USAGE& usage );
        //    VkMemoryPropertyFlags MemoryUsagesToVkMemoryPropertyFlags( const RenderSystem::MEMORY_USAGE& usages );
        //} // Convert

} // vke