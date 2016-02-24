#ifndef VK_EXPORTED_FUNCTION
#define VK_EXPORTED_FUNCTION(name)
#endif

#if VKE_DECLARE_GLOBAL_ICD && VKE_DECLARE_DEVICE_ICD && VKE_DECLARE_INSTANCE_ICD
struct ICD
{
#endif

#if VKE_DECLARE_GLOBAL_ICD
    struct Global
    {
#endif
        VK_EXPORTED_FUNCTION(vkGetInstanceProcAddr);


#ifndef VKE_ICD_GLOBAL
#define VKE_ICD_GLOBAL(name)
#endif

        VKE_ICD_GLOBAL(vkEnumerateInstanceExtensionProperties);
        VKE_ICD_GLOBAL(vkEnumerateInstanceLayerProperties);
        VKE_ICD_GLOBAL(vkCreateInstance);

#if VKE_DECLARE_GLOBAL_ICD
    };
#endif

#if VKE_DECLARE_INSTANCE_ICD
    struct Instance
    {
#endif

#ifndef VKE_INSTANCE_ICD
#define VKE_INSTANCE_ICD(name)
#endif

        VKE_INSTANCE_ICD(vkDestroyInstance);
        VKE_INSTANCE_ICD(vkEnumeratePhysicalDevices);
        VKE_INSTANCE_ICD(vkGetPhysicalDeviceFeatures);
        VKE_INSTANCE_ICD(vkGetPhysicalDeviceFormatProperties);
        VKE_INSTANCE_ICD(vkGetPhysicalDeviceProperties);
        VKE_INSTANCE_ICD(vkGetPhysicalDeviceQueueFamilyProperties);
        VKE_INSTANCE_ICD(vkGetPhysicalDeviceMemoryProperties);
        VKE_INSTANCE_ICD(vkGetDeviceProcAddr);
        VKE_INSTANCE_ICD(vkCreateDevice);
        VKE_INSTANCE_ICD(vkDestroyDevice);
        VKE_INSTANCE_ICD(vkEnumerateDeviceExtensionProperties);
        VKE_INSTANCE_ICD(vkEnumerateDeviceLayerProperties);

        VKE_INSTANCE_ICD(vkDestroySurfaceKHR);
        VKE_INSTANCE_ICD(vkGetPhysicalDeviceSurfaceSupportKHR);
        VKE_INSTANCE_ICD(vkGetPhysicalDeviceSurfaceCapabilitiesKHR);
        VKE_INSTANCE_ICD(vkGetPhysicalDeviceSurfaceFormatsKHR);
        VKE_INSTANCE_ICD(vkGetPhysicalDeviceSurfacePresentModesKHR);

#ifdef VK_USE_PLATFORM_XCB_KHR
        //VK_KHR_xcb_surface
        VKE_INSTANCE_ICD(vkCreateXcbSurfaceKHR)
            VKE_INSTANCE_ICD(vkGetPhysicalDeviceXcbPresentationSupportKHR);
#endif

#ifdef VK_USE_PLATFORM_ANDROID_KHR
        //VK_KHR_android_surface
        VKE_INSTANCE_ICD(vkCreateAndroidSurfaceKHR)
#endif

#ifdef VK_USE_PLATFORM_WIN32_KHR
            //VK_KHR_win32_surface
            VKE_INSTANCE_ICD(vkCreateWin32SurfaceKHR);
        VKE_INSTANCE_ICD(vkGetPhysicalDeviceWin32PresentationSupportKHR);
#endif


#if VKE_DECLARE_INSTANCE_ICD
    };
#endif

#ifndef VKE_DEVICE_ICD
#define VKE_DEVICE_ICD(name)
#endif

#if VKE_DECLARE_DEVICE_ICD
    struct Device
    {
#endif

        VKE_DEVICE_ICD(vkGetDeviceQueue);
        VKE_DEVICE_ICD(vkQueueSubmit);
        VKE_DEVICE_ICD(vkQueueWaitIdle);
        VKE_DEVICE_ICD(vkDeviceWaitIdle);
        VKE_DEVICE_ICD(vkAllocateMemory);
        VKE_DEVICE_ICD(vkFreeMemory);
        VKE_DEVICE_ICD(vkMapMemory);
        VKE_DEVICE_ICD(vkUnmapMemory);
        VKE_DEVICE_ICD(vkFlushMappedMemoryRanges);
        VKE_DEVICE_ICD(vkInvalidateMappedMemoryRanges);
        VKE_DEVICE_ICD(vkBindBufferMemory);
        VKE_DEVICE_ICD(vkBindImageMemory);
        VKE_DEVICE_ICD(vkGetImageMemoryRequirements);
        VKE_DEVICE_ICD(vkGetBufferMemoryRequirements);
        VKE_DEVICE_ICD(vkQueueBindSparse);
        VKE_DEVICE_ICD(vkCreateFence);
        VKE_DEVICE_ICD(vkResetFences);
        VKE_DEVICE_ICD(vkGetFenceStatus);
        VKE_DEVICE_ICD(vkWaitForFences);
        VKE_DEVICE_ICD(vkCreateSemaphore);
        VKE_DEVICE_ICD(vkCreateEvent);
        VKE_DEVICE_ICD(vkGetEventStatus);
        VKE_DEVICE_ICD(vkSetEvent);
        VKE_DEVICE_ICD(vkResetEvent);
        VKE_DEVICE_ICD(vkCreateQueryPool);
        VKE_DEVICE_ICD(vkGetQueryPoolResults);
        VKE_DEVICE_ICD(vkCreateBuffer);
        VKE_DEVICE_ICD(vkCreateBufferView);
        VKE_DEVICE_ICD(vkCreateImage);
        VKE_DEVICE_ICD(vkGetImageSubresourceLayout);
        VKE_DEVICE_ICD(vkCreateImageView);
        VKE_DEVICE_ICD(vkCreateShaderModule);
        VKE_DEVICE_ICD(vkCreatePipelineCache);
        VKE_DEVICE_ICD(vkGetPipelineCacheData);
        VKE_DEVICE_ICD(vkMergePipelineCaches);
        VKE_DEVICE_ICD(vkCreateGraphicsPipelines);
        VKE_DEVICE_ICD(vkCreateComputePipelines);
        VKE_DEVICE_ICD(vkCreatePipelineLayout);
        VKE_DEVICE_ICD(vkCreateSampler);
        VKE_DEVICE_ICD(vkCreateDescriptorSetLayout);
        VKE_DEVICE_ICD(vkCreateDescriptorPool);
        VKE_DEVICE_ICD(vkResetDescriptorPool);
        VKE_DEVICE_ICD(vkAllocateDescriptorSets);
        VKE_DEVICE_ICD(vkFreeDescriptorSets);
        VKE_DEVICE_ICD(vkUpdateDescriptorSets);
        VKE_DEVICE_ICD(vkAllocateCommandBuffers);
        VKE_DEVICE_ICD(vkBeginCommandBuffer);
        VKE_DEVICE_ICD(vkEndCommandBuffer);
        VKE_DEVICE_ICD(vkResetCommandBuffer);
        VKE_DEVICE_ICD(vkCreateCommandPool);
        VKE_DEVICE_ICD(vkDestroyFence);
        VKE_DEVICE_ICD(vkDestroySemaphore);
        VKE_DEVICE_ICD(vkDestroyEvent);
        VKE_DEVICE_ICD(vkDestroyQueryPool);
        VKE_DEVICE_ICD(vkDestroyBuffer);
        VKE_DEVICE_ICD(vkDestroyBufferView);
        VKE_DEVICE_ICD(vkDestroyImage);
        VKE_DEVICE_ICD(vkDestroyImageView);
        VKE_DEVICE_ICD(vkDestroyShaderModule);
        VKE_DEVICE_ICD(vkDestroyPipelineCache);
        VKE_DEVICE_ICD(vkDestroyPipeline);
        VKE_DEVICE_ICD(vkDestroyPipelineLayout);
        VKE_DEVICE_ICD(vkDestroySampler);
        VKE_DEVICE_ICD(vkDestroyDescriptorSetLayout);
        VKE_DEVICE_ICD(vkDestroyDescriptorPool);
        VKE_DEVICE_ICD(vkDestroyFramebuffer);
        VKE_DEVICE_ICD(vkDestroyRenderPass);
        VKE_DEVICE_ICD(vkFreeCommandBuffers);
        VKE_DEVICE_ICD(vkDestroyCommandPool);
        VKE_DEVICE_ICD(vkCmdBindPipeline);
        VKE_DEVICE_ICD(vkCmdSetViewport);
        VKE_DEVICE_ICD(vkCmdSetScissor);
        VKE_DEVICE_ICD(vkCmdSetLineWidth);
        VKE_DEVICE_ICD(vkCmdSetDepthBias);
        VKE_DEVICE_ICD(vkCmdSetBlendConstants);
        VKE_DEVICE_ICD(vkCmdSetDepthBounds);
        VKE_DEVICE_ICD(vkCmdSetStencilCompareMask);
        VKE_DEVICE_ICD(vkCmdSetStencilWriteMask);
        VKE_DEVICE_ICD(vkCmdSetStencilReference);
        VKE_DEVICE_ICD(vkCmdBindDescriptorSets);
        VKE_DEVICE_ICD(vkCmdBindIndexBuffer);
        VKE_DEVICE_ICD(vkCmdBindVertexBuffers);
        VKE_DEVICE_ICD(vkCmdDraw);
        VKE_DEVICE_ICD(vkCmdDrawIndexed);
        VKE_DEVICE_ICD(vkCmdDrawIndirect);
        VKE_DEVICE_ICD(vkCmdDrawIndexedIndirect);
        VKE_DEVICE_ICD(vkCmdDispatch);
        VKE_DEVICE_ICD(vkCmdDispatchIndirect);
        VKE_DEVICE_ICD(vkCmdCopyBuffer);
        VKE_DEVICE_ICD(vkCmdCopyImage);
        VKE_DEVICE_ICD(vkCmdBlitImage);
        VKE_DEVICE_ICD(vkCmdCopyBufferToImage);
        VKE_DEVICE_ICD(vkCmdCopyImageToBuffer);
        VKE_DEVICE_ICD(vkCmdUpdateBuffer);
        VKE_DEVICE_ICD(vkCmdFillBuffer);
        VKE_DEVICE_ICD(vkCmdClearColorImage);
        VKE_DEVICE_ICD(vkCmdClearAttachments);
        VKE_DEVICE_ICD(vkCmdClearDepthStencilImage);
        VKE_DEVICE_ICD(vkCmdResolveImage);
        VKE_DEVICE_ICD(vkCmdSetEvent);
        VKE_DEVICE_ICD(vkCmdResetEvent);
        VKE_DEVICE_ICD(vkCmdWaitEvents);
        VKE_DEVICE_ICD(vkCmdPipelineBarrier);
        VKE_DEVICE_ICD(vkCmdBeginQuery);
        VKE_DEVICE_ICD(vkCmdEndQuery);
        VKE_DEVICE_ICD(vkCmdResetQueryPool);
        VKE_DEVICE_ICD(vkCmdWriteTimestamp);
        VKE_DEVICE_ICD(vkCmdCopyQueryPoolResults);
        VKE_DEVICE_ICD(vkCreateFramebuffer);
        VKE_DEVICE_ICD(vkCreateRenderPass);
        VKE_DEVICE_ICD(vkCmdBeginRenderPass);
        VKE_DEVICE_ICD(vkCmdEndRenderPass);
        VKE_DEVICE_ICD(vkCmdExecuteCommands);
        VKE_DEVICE_ICD(vkCmdPushConstants);

        VKE_DEVICE_ICD(vkCreateSwapchainKHR);
        VKE_DEVICE_ICD(vkDestroySwapchainKHR);
        VKE_DEVICE_ICD(vkGetSwapchainImagesKHR);
        VKE_DEVICE_ICD(vkAcquireNextImageKHR);
        VKE_DEVICE_ICD(vkQueuePresentKHR);

#if VKE_DECLARE_DEVICE_ICD
    };
#endif

#if VKE_DECLARE_GLOBAL_ICD && VKE_DECLARE_DEVICE_ICD && VKE_DECLARE_INSTANCE_ICD
}; // struct ICD
#endif

#undef VK_EXPORTED_FUNCTION
#undef VKE_ICD_GLOBAL
#undef VKE_INSTANCE_ICD
#undef VKE_DEVICE_ICD  