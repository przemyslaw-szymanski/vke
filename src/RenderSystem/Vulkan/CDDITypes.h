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
#define NativeAPI::Null VK_NULL_HANDLE
        //using NativeAPI::Null = nullptr_t;
        using NativeAPI::Buffer = VkBuffer;
        using NativeAPI::Pipeline = VkPipeline;
        using NativeAPI::Texture = VkImage;
        using NativeAPI::Sampler = VkSampler;
        using NativeAPI::RenderPass = VkRenderPass;
        using NativeAPI::CommandBuffer = VkCommandBuffer;
        using NativeAPI::TextureView = VkImageView;
        using NativeAPI::BufferView = VkBufferView;
        using NativeAPI::CPUFence = VkFence;
        using NativeAPI::GPUFence = VkSemaphore;
        using NativeAPI::Device = VkDevice;
        using NativeAPI::DescriptorPool = VkDescriptorPool;
        using NativeAPI::DescriptorSet = VkDescriptorSet;
        using NativeAPI::DescriptorSetLayout = VkDescriptorSetLayout;
        using NativeAPI::CommandBufferPool = VkCommandPool;
        using NativeAPI::Framebuffer = VkFramebuffer;
        using NativeAPI::ClearValue = VkClearValue;
        using NativeAPI::Queue = VkQueue;
        using DDIFormat = VkFormat;
        using DDIImageType = VkImageType;
        using DDIImageViewType = VkImageViewType;
        using DDIImageLayout = VkImageLayout;
        using DDIImageUsageFlags = VkImageUsageFlags;
        using NativeAPI::Memory = VkDeviceMemory;
        using NativeAPI::PresentSurface = VkSurfaceKHR;
        using NativeAPI::SwapChain = VkSwapchainKHR;
        using NativeAPI::Adapter = VkPhysicalDevice;
        using NativeAPI::Shader = VkShaderModule;
        using NativeAPI::PipelineLayout = VkPipelineLayout;
        using NativeAPI::DeviceSize = VkDeviceSize;
        using NativeAPI::Event = VkEvent;
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