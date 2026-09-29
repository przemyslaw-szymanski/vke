#pragma once
#if VKE_COMPILE_VULKAN_RHI
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
#include <vulkan/vulkan.h>

namespace VKE
{
    namespace RenderSystem
    {
        struct VulkanTraits
        {
            static constexpr auto RHI::Null = VK_NULL_HANDLE;
            using RHI::Buffer               = VkBuffer;
            using RHI::Pipeline             = VkPipeline;
            using RHI::Texture              = VkImage;
            using RHI::Sampler              = VkSampler;
            using RHI::RenderPass           = VkRenderPass;
            using RHI::CommandBuffer        = VkCommandBuffer;
            using RHI::TextureView          = VkImageView;
            using RHI::BufferView           = VkBufferView;
            using RHI::CPUFence             = VkFence;
            using RHI::GPUFence             = VkSemaphore;
            using RHI::Device               = VkDevice;
            using RHI::DescriptorPool       = VkDescriptorPool;
            using RHI::DescriptorSet        = VkDescriptorSet;
            using RHI::DescriptorSetLayout  = VkDescriptorSetLayout;
            using RHI::CommandBufferPool    = VkCommandPool;
            using RHI::Framebuffer          = VkFramebuffer;
            using RHI::ClearValue           = VkClearValue;
            using RHI::Queue                = VkQueue;
            using RHI::Format               = VkFormat;
            using RHI::ImageType            = VkImageType;
            using RHI::ImageViewType        = VkImageViewType;
            using RHI::ImageLayout          = VkImageLayout;
            using RHI::ImageUsageFlags      = VkImageUsageFlags;
            using RHI::MemoryProperties     = VkDeviceMemory;
            using RHI::PresentSurface       = VkSurfaceKHR;
            using RHI::SwapChain            = VkSwapchainKHR;
            using RHI::Adapter              = VkPhysicalDevice;
            using RHI::Shader               = VkShaderModule;
            using RHI::PipelineLayout       = VkPipelineLayout;
            using RHI::DeviceSize           = VkDeviceSize;
            using RHI::Event                = VkEvent;

            struct
            {
                static const decltype( VK_NULL_HANDLE ) Null;
                using Buffer              = VkBuffer;
                using Pipeline            = VkPipeline;
                using Texture             = VkImage;
                using Sampler             = VkSampler;
                using RenderPass          = VkRenderPass;
                using CommandBuffer       = VkCommandBuffer;
                using TextureView         = VkImageView;
                using BufferView          = VkBufferView;
                using CPUFence            = VkFence;
                using GPUFence            = VkSemaphore;
                using Device              = VkDevice;
                using DescriptorPool      = VkDescriptorPool;
                using DescriptorSet       = VkDescriptorSet;
                using DescriptorSetLayout = VkDescriptorSetLayout;
                using CommandBufferPool   = VkCommandPool;
                using Framebuffer         = VkFramebuffer;
                using ClearValue          = VkClearValue;
                using Queue               = VkQueue;
                using Format              = VkFormat;
                using ImageType           = VkImageType;
                using ImageViewType       = VkImageViewType;
                using ImageLayout         = VkImageLayout;
                using ImageUsageFlags     = VkImageUsageFlags;
                using MemoryProperties    = VkDeviceMemory;
                using PresentSurface      = VkSurfaceKHR;
                using SwapChain           = VkSwapchainKHR;
                using Adapter             = VkPhysicalDevice;
                using Shader              = VkShaderModule;
                using PipelineLayout      = VkPipelineLayout;
                using DeviceSize          = VkDeviceSize;
                using Event               = VkEvent;
            } RHI;
        };
    } // namespace RenderSystem
} // namespace VKE

#endif // VKE_COMPILE_VULKAN_RHI