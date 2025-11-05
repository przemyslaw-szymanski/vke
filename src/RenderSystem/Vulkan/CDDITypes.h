#pragma once

#if VKE_VULKAN_RENDER_SYSTEM

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
#include "RenderSystem/Vulkan/Vulkan.h"
#include "ThirdParty/vulkan/vulkan.h"

namespace VKE::RenderSystem
{
    static const uint32_t DEFAULT_QUEUE_FAMILY_PROPERTY_COUNT = 16;

    namespace NativeAPI
    {
        static const decltype( VK_NULL_HANDLE ) Null;

        using Buffer                = VkBuffer;
        using Pipeline              = VkPipeline;
        using Texture               = VkImage;
        using Sampler               = VkSampler;
        using RenderPass            = VkRenderPass;
        using CommandBuffer         = VkCommandBuffer;
        using TextureView           = VkImageView;
        using BufferView            = VkBufferView;
        using CPUFence              = VkFence;
        using GPUFence              = VkSemaphore;
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
        using Memory                = VkDeviceMemory;
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

        struct VKE_API SDDIExtension
        {
            vke_string name;

            bool required  = false;
            bool supported = false;
            bool enabled   = false;
        };

        using DDIExtArray = Utils::TCDynamicArray< SDDIExtension, 1 >;
        using DDIExtMap   = vke_hash_map< vke_string, SDDIExtension >;

        struct VKE_API SDDIExtensionLayer
        {
            vke_string name;

            bool required  = false;
            bool supported = false;
            bool enabled   = false;
        };

        using DDIExtLayerArray = Utils::TCDynamicArray< SDDIExtensionLayer, 1 >;

        struct SImplementation
        {
            static const uint32_t MAX_MEMORY_HEAPS = VK_MAX_MEMORY_HEAPS;

            using GlobalICD   = VkICD::Global;
            using InstanceICD = VkICD::Instance;
            using DeviceICD   = VkICD::Device;

            static DDIExtArray      svExtensions;
            static DDIExtLayerArray svLayers;

            static GlobalICD   sGlobalICD;
            static InstanceICD sInstanceICD;
            static handle_t    shICD;
            static VkInstance  sVkInstance;

            static VkDebugReportCallbackEXT sVkDebugReportCallback;
            static VkDebugUtilsMessengerEXT sVkDebugMessengerCallback;

            DeviceICD    m_ICD;
            VkDeviceSize m_aHeapSizes[ MAX_MEMORY_HEAPS ];
            uint32_t     m_instanceVersion = 0;
            DDIExtMap    m_mExtensions;

            struct SDeviceFeatures
            {
                VkPhysicalDeviceFeatures2                      Device;
                VkPhysicalDeviceVulkan11Features               Device11;
                VkPhysicalDeviceVulkan12Features               Device12;
                VkPhysicalDeviceMeshShaderFeaturesEXT          MeshShaderEXT;
                VkPhysicalDeviceMeshShaderFeaturesNV           MeshShaderNV;
                VkPhysicalDeviceRayTracingPipelineFeaturesKHR  Raytracing10;
                VkPhysicalDeviceRayQueryFeaturesKHR            Raytracing11;
                VkPhysicalDeviceRayTracingMotionBlurFeaturesNV Raytracing12;
                VkPhysicalDeviceDynamicRenderingFeaturesKHR    DynamicRendering;
            } Features; // struct SDeviceFeatures

            struct SDeviceProperties
            {
                VkPhysicalDeviceProperties2                     Device;
                VkPhysicalDeviceVulkan11Properties              Device11;
                VkPhysicalDeviceVulkan12Properties              Device12;
                VkPhysicalDeviceMemoryProperties2               Memory;
                VkPhysicalDeviceMeshShaderPropertiesEXT         MeshShaderEXT;
                VkPhysicalDeviceMeshShaderPropertiesNV          MeshShaderNV;
                VkPhysicalDeviceRayTracingPipelinePropertiesKHR Raytracing10;
                VkPhysicalDeviceDescriptorIndexingProperties    DescriptorIndexing;
                VkFormatProperties                              aFormatProperties[ Formats::_MAX_COUNT ];
            } Properties; // struct SDeviceProperties

            const SDDIExtension& GetExtensionInfo( cstr_t pName ) const;

        }; // struct SImplementation

    } // namespace NativeAPI

} // namespace VKE::RenderSystem

#endif // VKE_VULKAN_RENDER_SYSTEM