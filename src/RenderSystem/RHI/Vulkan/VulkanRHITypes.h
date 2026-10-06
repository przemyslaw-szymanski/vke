#pragma once

#define VKE_USE_VULKAN_KHR 1

#if VKE_WINDOWS
#define VKE_USE_VULKAN_WINDOWS 1
#define VK_USE_PLATFORM_WIN32_KHR 1

#elif VKE_LINUX
#define VKE_USE_VULKAN_LINUX 1
#define VKE_USE_VULKAN_LINUX 1
#define VK_USE_PLATFORM_XCB_KHR 1

#elif VKE_ANDROID
#define VKE_USE_VULKAN_ANDROID 1

#error implement here
#endif // VKE_WINDOWS

#include "RenderSystem/Common.h"
#include "RenderSystem/RHI/Vulkan/Vulkan.h"
#include <vulkan/vulkan.h>

namespace VKE::RenderSystem::RHI
{
    namespace VulkanAPI
    {
    struct NativeAPI
    {
        static const uint32_t             DEFAULT_QUEUE_FAMILY_PROPERTY_COUNT = 16;
        inline static const decltype( VK_NULL_HANDLE ) Null = nullptr;

        struct SRenderPass;

        struct SFence;

        using Buffer                = VkBuffer;
        using Pipeline              = VkPipeline;
        using Texture               = VkImage;
        using Sampler               = VkSampler;
        using RenderPass            = SRenderPass*;
        using CommandBuffer         = VkCommandBuffer;
        using TextureView           = VkImageView;
        using BufferView            = VkBufferView;
        using CPUFence              = VkFence;
        using GPUFence              = VkSemaphore;
        using Fence                 = SFence*;
        using Device                = VkDevice;
        using DescriptorPool        = VkDescriptorPool;
        using DescriptorSet         = VkDescriptorSet;
        using DescriptorSetLayout   = VkDescriptorSetLayout;
        using CommandBufferPool     = VkCommandPool;
        using Framebuffer           = VkFramebuffer;
        using ClearValue            = VkClearValue;
        using Queue                 = VkQueue;
        using Format                = VkFormat;
        using ImageType             = VkImageType;
        using ImageViewType         = VkImageViewType;
        using ImageLayout           = VkImageLayout;
        using ImageUsageFlags       = VkImageUsageFlags;
        using MemoryHeap            = VkDeviceMemory;
        using PresentSurface        = VkSurfaceKHR;
        using SwapChain             = VkSwapchainKHR;
        using Adapter               = VkPhysicalDevice;
        using Shader                = VkShaderModule;
        using PipelineLayout        = VkPipelineLayout;
        using DeviceSize            = VkDeviceSize;
        using Event                 = VkEvent;
        using QueueFamilyProperties = VkQueueFamilyProperties;
        using DeviceLimits          = VkPhysicalDeviceLimits;
        using Result                = VkResult;
        using FenceValue = uint64_t;

        struct VKE_API SNativeExtension
        {
            vke_string name;

            bool required  = false;
            bool supported = false;
            bool enabled   = false;
        };

        using NativeExtArray = Utils::TCDynamicArray< SNativeExtension, 1 >;
        using NativeExtMap   = vke_hash_map< vke_string, SNativeExtension >;

        struct VKE_API SNativeExtensionLayer
        {
            vke_string name;

            bool required  = false;
            bool supported = false;
            bool enabled   = false;
        };

        using NativeExtLayerArray = Utils::TCDynamicArray< SNativeExtensionLayer, 1 >;

       

    }; // namespace RHI

    struct SImplementation
    {
        static const uint32_t MAX_MEMORY_HEAPS = VK_MAX_MEMORY_HEAPS;

        using GlobalICD   = VkICD::Global;
        using InstanceICD = VkICD::Instance;
        using DeviceICD   = VkICD::Device;

        static NativeAPI::NativeExtArray      svExtensions;
        static NativeAPI::NativeExtLayerArray svLayers;

        static GlobalICD   sGlobalICD;
        static InstanceICD sInstanceICD;
        static handle_t    shICD;
        static VkInstance  sVkInstance;

        static VkDebugReportCallbackEXT sVkDebugReportCallback;
        static VkDebugUtilsMessengerEXT sVkDebugMessengerCallback;

        DeviceICD        m_ICD;
        VkDevice         m_hDevice;
        VkPhysicalDevice m_hAdapter;
        VkDeviceSize     m_aHeapSizes[ MAX_MEMORY_HEAPS ];
        uint32_t         m_instanceVersion = 0;
        NativeAPI::NativeExtMap        m_mExtensions;

        VKE::RenderSystem::SDeviceProperties EngineDeviceProperties;

        struct SDeviceFeatures
        {
            VkPhysicalDeviceFeatures2                             Device;
            VkPhysicalDeviceMeshShaderFeaturesEXT                 MeshShaderEXT;
            VkPhysicalDeviceRayTracingPipelineFeaturesKHR         Raytracing10;
            VkPhysicalDeviceRayQueryFeaturesKHR                   Raytracing11;
            VkPhysicalDeviceRayTracingInvocationReorderFeaturesNV Raytracing12NV;
            VkPhysicalDeviceDynamicRenderingFeaturesKHR           DynamicRendering;
            VkPhysicalDeviceDescriptorIndexingFeatures            DescriptorIndexing;
            VkPhysicalDevice16BitStorageFeatures                  Buffer16Bit;
            VkPhysicalDeviceShaderDrawParametersFeatures          ShaderDrawParameters;
            VkPhysicalDeviceSubgroupSizeControlFeatures           SubgroupSizeControl;
            VkPhysicalDeviceTimelineSemaphoreFeatures             TimelineSemaphore;
        } Features; // struct SDeviceFeatures

        struct SDeviceProperties
        {
            VkPhysicalDeviceProperties2                     Device;
            VkPhysicalDeviceMeshShaderPropertiesEXT         MeshShaderEXT;
            VkPhysicalDeviceRayTracingPipelinePropertiesKHR Raytracing10;
            VkPhysicalDeviceRayTracingInvocationReorderPropertiesNV Raytracing12NV;
            VkPhysicalDeviceDescriptorIndexingProperties    DescriptorIndexing;
            VkPhysicalDeviceSubgroupProperties              Subgroup;
            VkPhysicalDeviceSubgroupSizeControlProperties   SubgroupSizeControl;
            VkPhysicalDeviceMaintenance3Properties          Maintenance3;
            VkPhysicalDeviceIDProperties                    DeviceID;
            VkPhysicalDeviceDriverProperties                Driver;
            //VkPhysicalDeviceFloatControlsProperties         FloatControl;
            VkPhysicalDeviceTimelineSemaphoreProperties     TimelineSemaphore;
            //VkPhysicalDeviceDepthStencilResolveProperties   DepthStencilResolve;
            //VkPhysicalDeviceSamplerFilterMinmaxProperties   SamplerMinMax;
            VkFormatProperties                              aFormatProperties[ Formats::_MAX_COUNT ];
        } DeviceProperties; // struct SDeviceProperties

        struct SMemoryProperties
        {
            VkPhysicalDeviceMemoryProperties2         Memory;
            VkPhysicalDeviceMemoryBudgetPropertiesEXT MemoryBudget;
        } MemoryProperties;

        VkPhysicalDeviceLimits Limits;

        const NativeAPI::SNativeExtension& GetExtensionInfo( cstr_t pName ) const;

    }; // struct SImplementation

    } // namespace VulkanAPI
    using namespace VulkanAPI;
} // namespace VKE::RenderSystem::RHI
