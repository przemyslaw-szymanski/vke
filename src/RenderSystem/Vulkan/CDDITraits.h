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
            static constexpr auto NativeTypes::Null = VK_NULL_HANDLE;
            using NativeTypes::Buffer               = VkBuffer;
            using NativeTypes::Pipeline             = VkPipeline;
            using NativeTypes::Texture              = VkImage;
            using NativeTypes::Sampler              = VkSampler;
            using NativeTypes::RenderPass           = VkRenderPass;
            using NativeTypes::CommandBuffer        = VkCommandBuffer;
            using NativeTypes::TextureView          = VkImageView;
            using NativeTypes::BufferView           = VkBufferView;
            using NativeTypes::CPUFence             = VkFence;
            using NativeTypes::GPUFence             = VkSemaphore;
            using DDIDevice                       = VkDevice;
            using NativeTypes::DescriptorPool       = VkDescriptorPool;
            using NativeTypes::DescriptorSet        = VkDescriptorSet;
            using NativeTypes::DescriptorSetLayout  = VkDescriptorSetLayout;
            using NativeTypes::CommandBufferPool    = VkCommandPool;
            using NativeTypes::Framebuffer          = VkFramebuffer;
            using NativeTypes::ClearValue           = VkClearValue;
            using NativeTypes::Queue                = VkQueue;
            using DDIFormat                       = VkFormat;
            using DDIImageType                    = VkImageType;
            using DDIImageViewType                = VkImageViewType;
            using DDIImageLayout                  = VkImageLayout;
            using DDIImageUsageFlags              = VkImageUsageFlags;
            using NativeTypes::Memory               = VkDeviceMemory;
            using NativeTypes::PresentSurface       = VkSurfaceKHR;
            using DDISwapChain                    = VkSwapchainKHR;
            using NativeTypes::Adapter              = VkPhysicalDevice;
            using NativeTypes::Shader               = VkShaderModule;
            using NativeTypes::PipelineLayout       = VkPipelineLayout;
            using DDIDeviceSize                   = VkDeviceSize;
            using NativeTypes::Event                = VkEvent;

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
                using Memory              = VkDeviceMemory;
                using PresentSurface      = VkSurfaceKHR;
                using SwapChain           = VkSwapchainKHR;
                using Adapter             = VkPhysicalDevice;
                using Shader              = VkShaderModule;
                using PipelineLayout      = VkPipelineLayout;
                using DeviceSize          = VkDeviceSize;
                using Event               = VkEvent;
            } NativeTypes;
        };
    } // namespace RenderSystem
} // namespace VKE

#endif // VKE_COMPILE_VULKAN_RHI