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
        using Fence = VkSemaphore;
        using FenceValue = uint64_t;
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
        using Size = VkDeviceSize;
        using Event = VkEvent;
       
    }
} // VKE

#endif // VKE_VULKAN_RENDER_SYSTEM