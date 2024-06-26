#pragma once
#if VKE_VULKAN_RENDER_SYSTEM

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

namespace VKE
{
    namespace RenderSystem
    {
#define DDI_NULL_HANDLE VK_NULL_HANDLE
        //using DDI_NULL_HANDLE = nullptr_t;
        using DDIBuffer = VkBuffer;
        using DDIPipeline = VkPipeline;
        using DDITexture = VkImage;
        using DDISampler = VkSampler;
        using DDIRenderPass = VkRenderPass;
        using DDICommandBuffer = VkCommandBuffer;
        using DDITextureView = VkImageView;
        using DDIBufferView = VkBufferView;
        using DDIFence = VkFence;
        using DDISemaphore = VkSemaphore;
        using DDIDevice = VkDevice;
        using DDIDescriptorPool = VkDescriptorPool;
        using DDIDescriptorSet = VkDescriptorSet;
        using DDIDescriptorSetLayout = VkDescriptorSetLayout;
        using DDICommandBufferPool = VkCommandPool;
        using DDIFramebuffer = VkFramebuffer;
        using DDIClearValue = VkClearValue;
        using DDIQueue = VkQueue;
        using DDIFormat = VkFormat;
        using DDIImageType = VkImageType;
        using DDIImageViewType = VkImageViewType;
        using DDIImageLayout = VkImageLayout;
        using DDIImageUsageFlags = VkImageUsageFlags;
        using DDIMemory = VkDeviceMemory;
        using DDIPresentSurface = VkSurfaceKHR;
        using DDISwapChain = VkSwapchainKHR;
        using DDIAdapter = VkPhysicalDevice;
        using DDIShader = VkShaderModule;
        using DDIPipelineLayout = VkPipelineLayout;
        using DDIDeviceSize = VkDeviceSize;
        using DDIEvent = VkEvent;
    } // RenderSystem
    namespace RenderSystem::NativeAPI
    {
        static const decltype( VK_NULL_HANDLE ) Null;
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
       
    }
} // VKE

#endif // VKE_VULKAN_RENDER_SYSTEM