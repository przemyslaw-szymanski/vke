#pragma once
#if VKE_VULKAN_RENDERER

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
        static const auto DDI_NULL_HANDLE = VK_NULL_HANDLE;
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
} // VKE

#endif // VKE_VULKAN_RENDERER