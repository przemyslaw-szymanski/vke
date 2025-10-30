#pragma once
#if VKE_VULKAN_RENDER_SYSTEM
#include "RenderSystem/VKEImageFormats.h"
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
#include "ThirdParty/vulkan/vulkan.h"
//#include "RenderSystem/Vulkan/Vulkan.h"
#ifdef __cplusplus
extern "C"
{
#endif
#define VKE_AUTO_ICD 1
#define VK_EXPORTED_FUNCTION( name ) PFN_##name name
#define VKE_ICD_GLOBAL( name ) VK_EXPORTED_FUNCTION( name )
#define VKE_INSTANCE_ICD( name ) VK_EXPORTED_FUNCTION( name )
#define VKE_INSTANCE_EXT_ICD( name ) VK_EXPORTED_FUNCTION( name )
#define VKE_DEVICE_ICD( name ) VK_EXPORTED_FUNCTION( name )
#define VKE_DEVICE_EXT_ICD( name ) VK_EXPORTED_FUNCTION( name )
#define VKE_DECLARE_GLOBAL_ICD 1
#define VKE_DECLARE_INSTANCE_ICD 1
#define VKE_DECLARE_DEVICE_ICD 1
#include "RenderSystem/Vulkan/VKEICD.h"
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

namespace VKE
{
    namespace RenderSystem::NativeAPI
    {
        static const decltype( VK_NULL_HANDLE ) Null;
        using Instance = VkInstance;
        using Buffer = VkBuffer;
        using Pipeline = VkPipeline;
        using Texture = VkImage;
        using Sampler = VkSampler;
        using RenderPass = VkRenderPass;
        using CommandBuffer = VkCommandBuffer;
        using TextureView = VkImageView;
        using BufferView = VkBufferView;
        using CPUFence = VkFence;
        using GPUFence = VkSemaphore;
        using Device = VkDevice;
        using DescriptorPool = VkDescriptorPool;
        using DescriptorSet = VkDescriptorSet;
        using DescriptorSetLayout = VkDescriptorSetLayout;
        using CommandBufferPool = VkCommandPool;
        using Framebuffer = VkFramebuffer;
        using ClearValue = VkClearValue;
        using Queue = VkQueue;
        using Format = VkFormat;
        using ImageType = VkImageType;
        using ImageViewType = VkImageViewType;
        using ImageLayout = VkImageLayout;
        using ImageUsageFlags = VkImageUsageFlags;
        using Memory = VkDeviceMemory;
        using PresentSurface = VkSurfaceKHR;
        using SwapChain = VkSwapchainKHR;
        using Adapter = VkPhysicalDevice;
        using Shader = VkShaderModule;
        using PipelineLayout = VkPipelineLayout;
        using DeviceSize = VkDeviceSize;
        using Event = VkEvent;
        using GlobalAPI = VkICD::Global;
        using InstanceAPI = VkICD::Instance;
        using DeviceAPI = VkICD::Device;
    }
} // VKE

#undef VKE_AUTO_ICD
#undef VKE_DECLARE_GLOBAL_ICD
#undef VKE_DECLARE_DEVICE_ICD
#undef VKE_DECLARE_INSTANCE_ICD

#endif // VKE_VULKAN_RENDER_SYSTEM