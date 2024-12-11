#ifndef VK_EXPORTED_FUNCTION
#define VK_EXPORTED_FUNCTION(name)
#endif

#if VKE_AUTO_ICD

#if VKE_VULKAN_1_1
#define VKE_VULKAN_1_1_NAME(_name) _name
#else
#define VKE_VULKAN_1_1_NAME(_name) _name
#endif // VKE_VULKAN_1_1

#if VKE_DECLARE_GLOBAL_ICD && VKE_DECLARE_DEVICE_ICD && VKE_DECLARE_INSTANCE_ICD
struct VkICD
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
        VKE_ICD_GLOBAL( vkEnumerateInstanceVersion );
        //VKE_ICD_GLOBAL(vkDestroyInstance);

#if VKE_DECLARE_GLOBAL_ICD
    };
#endif

#if VKE_DECLARE_INSTANCE_ICD
    struct Instance
    {
#endif

#ifndef VKE_INSTANCE_ICD
#   define VKE_INSTANCE_ICD(name)
#   define VKE_INSTANCE_EXT_ICD(name)
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

        VKE_INSTANCE_EXT_ICD( VKE_VULKAN_1_1_NAME( vkGetPhysicalDeviceFeatures2 ) );
        VKE_INSTANCE_EXT_ICD( VKE_VULKAN_1_1_NAME( vkGetPhysicalDeviceProperties2 ) );
        VKE_INSTANCE_EXT_ICD( VKE_VULKAN_1_1_NAME( vkGetPhysicalDeviceFormatProperties2 ) );
        VKE_INSTANCE_EXT_ICD( VKE_VULKAN_1_1_NAME( vkGetPhysicalDeviceImageFormatProperties2 ) );
        VKE_INSTANCE_EXT_ICD( VKE_VULKAN_1_1_NAME( vkGetPhysicalDeviceQueueFamilyProperties2 ) );
        VKE_INSTANCE_EXT_ICD( VKE_VULKAN_1_1_NAME( vkGetPhysicalDeviceMemoryProperties2 ) );
        VKE_INSTANCE_EXT_ICD( VKE_VULKAN_1_1_NAME( vkGetPhysicalDeviceSparseImageFormatProperties2 ) );

        // VK_EXT_debug_utils
        VKE_INSTANCE_EXT_ICD( vkSetDebugUtilsObjectNameEXT );
        VKE_INSTANCE_EXT_ICD( vkSetDebugUtilsObjectTagEXT );
        VKE_INSTANCE_EXT_ICD( vkQueueBeginDebugUtilsLabelEXT );
        VKE_INSTANCE_EXT_ICD( vkQueueEndDebugUtilsLabelEXT );
        VKE_INSTANCE_EXT_ICD( vkQueueInsertDebugUtilsLabelEXT );
        VKE_INSTANCE_EXT_ICD( vkCmdBeginDebugUtilsLabelEXT );
        VKE_INSTANCE_EXT_ICD( vkCmdEndDebugUtilsLabelEXT );
        VKE_INSTANCE_EXT_ICD( vkCmdInsertDebugUtilsLabelEXT );
        VKE_INSTANCE_EXT_ICD( vkCreateDebugUtilsMessengerEXT );
        VKE_INSTANCE_EXT_ICD( vkSubmitDebugUtilsMessageEXT );
        VKE_INSTANCE_EXT_ICD( vkDestroyDebugUtilsMessengerEXT );
        VKE_INSTANCE_EXT_ICD( vkCmdDebugMarkerBeginEXT );
        VKE_INSTANCE_EXT_ICD( vkCmdDebugMarkerEndEXT );
        VKE_INSTANCE_EXT_ICD( vkCmdDebugMarkerInsertEXT );

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

        //VK_EXT_debug_report
        VKE_INSTANCE_EXT_ICD(vkCreateDebugReportCallbackEXT);
        VKE_INSTANCE_EXT_ICD(vkDestroyDebugReportCallbackEXT);
        VKE_INSTANCE_EXT_ICD(vkDebugReportMessageEXT);


#if VKE_DECLARE_INSTANCE_ICD
    };
#endif

#ifndef VKE_DEVICE_ICD
#   define VKE_DEVICE_ICD(name)
#   define VKE_DEVICE_EXT_ICD(name)
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
        VKE_DEVICE_ICD( vkCmdBlitImage2KHR );
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
        VKE_DEVICE_ICD(vkCmdNextSubpass);
        VKE_DEVICE_ICD(vkCmdEndRenderPass);
        VKE_DEVICE_ICD(vkCmdExecuteCommands);
        VKE_DEVICE_ICD(vkCmdPushConstants);

        VKE_DEVICE_ICD(vkCreateSwapchainKHR);
        VKE_DEVICE_ICD(vkDestroySwapchainKHR);
        VKE_DEVICE_ICD(vkGetSwapchainImagesKHR);
        VKE_DEVICE_ICD(vkAcquireNextImageKHR);
        VKE_DEVICE_ICD(vkQueuePresentKHR);

        VKE_DEVICE_EXT_ICD( VKE_VULKAN_1_1_NAME( vkGetDescriptorSetLayoutSupport ) );

        VKE_DEVICE_EXT_ICD( vkDebugMarkerSetObjectNameEXT );
        VKE_DEVICE_EXT_ICD( vkDebugMarkerSetObjectTagEXT );
        VKE_DEVICE_EXT_ICD( vkCmdDebugMarkerBeginEXT );
        VKE_DEVICE_EXT_ICD( vkCmdDebugMarkerEndEXT );
        VKE_DEVICE_EXT_ICD( vkCmdDebugMarkerInsertEXT );
        

        VKE_DEVICE_EXT_ICD( vkCreateDebugReportCallbackEXT );
        VKE_DEVICE_EXT_ICD( vkDebugReportMessageEXT );
        VKE_DEVICE_EXT_ICD( vkDestroyDebugReportCallbackEXT );

        VKE_DEVICE_EXT_ICD( vkCmdSetCheckpointNV );
        VKE_DEVICE_EXT_ICD( vkGetQueueCheckpointDataNV );

        // VULKAN 1.3
        VKE_DEVICE_ICD( vkCmdBeginRenderingKHR );
        VKE_DEVICE_ICD( vkCmdEndRenderingKHR );

        // EXT
        VKE_DEVICE_EXT_ICD( vkCmdDrawMeshTasksEXT );
        //VKE_DEVICE_EXT_ICD( vkCmdDrawMeshTasksIndirectEXT );
        //VKE_DEVICE_EXT_ICD( vkCmdDrawMeshTasksIndirectCountEXT );

#if VKE_DECLARE_DEVICE_ICD
    };
#endif

#if VKE_DECLARE_GLOBAL_ICD && VKE_DECLARE_DEVICE_ICD && VKE_DECLARE_INSTANCE_ICD
}; // struct ICD
#endif

#else
#define VKE_FUNC(_name) PFN_##_name _name
#define VKE_EXT_FUNC(_name) PFN_##_name _name
struct ICD
{
    struct Global
    {
        VKE_FUNC(vkGetInstanceProcAddr);
        VKE_FUNC(vkEnumerateInstanceExtensionProperties);
        VKE_FUNC(vkEnumerateInstanceLayerProperties);
        VKE_FUNC(vkCreateInstance);
        VKE_FUNC( vkDestroyInstance );
    };

    struct Instance
    {
        VKE_FUNC(vkDestroyInstance);
        VKE_FUNC(vkEnumeratePhysicalDevices);
        VKE_FUNC(vkGetPhysicalDeviceFeatures);
        VKE_FUNC(vkGetPhysicalDeviceFormatProperties);
        VKE_FUNC(vkGetPhysicalDeviceProperties);
        VKE_FUNC(vkGetPhysicalDeviceQueueFamilyProperties);
        VKE_FUNC(vkGetPhysicalDeviceMemoryProperties);
        VKE_FUNC(vkGetDeviceProcAddr);
        VKE_FUNC(vkCreateDevice);
        VKE_FUNC(vkDestroyDevice);
        VKE_FUNC(vkEnumerateDeviceExtensionProperties);
        VKE_FUNC(vkEnumerateDeviceLayerProperties);

        VKE_FUNC(vkDestroySurfaceKHR);
        VKE_FUNC(vkGetPhysicalDeviceSurfaceSupportKHR);
        VKE_FUNC(vkGetPhysicalDeviceSurfaceCapabilitiesKHR);
        VKE_FUNC(vkGetPhysicalDeviceSurfaceFormatsKHR);
        VKE_FUNC(vkGetPhysicalDeviceSurfacePresentModesKHR);

        VKE_FUNC( vkGetPhysicalDeviceFeatures2 );
        VKE_FUNC( vkGetPhysicalDeviceProperties2 );
        VKE_FUNC( vkGetPhysicalDeviceFormatProperties2 );
        VKE_FUNC( vkGetPhysicalDeviceImageFormatProperties2 );
        VKE_FUNC( vkGetPhysicalDeviceQueueFamilyProperties2 );
        VKE_FUNC( vkGetPhysicalDeviceMemoryProperties2 );
        VKE_FUNC( vkGetPhysicalDeviceSparseImageFormatProperties2 );

        // VK_EXT_debug_utils
        VKE_FUNC_EXT( vkSetDebugUtilsObjectNameEXT );
        VKE_FUNC_EXT( vkSetDebugUtilsObjectTagEXT );
        VKE_FUNC_EXT( vkQueueBeginDebugUtilsLabelEXT );
        VKE_FUNC_EXT( vkQueueEndDebugUtilsLabelEXT );
        VKE_FUNC_EXT( vkQueueInsertDebugUtilsLabelEXT );
        VKE_FUNC_EXT( vkCmdBeginDebugUtilsLabelEXT );
        VKE_FUNC_EXT( vkCmdEndDebugUtilsLabelEXT );
        VKE_FUNC_EXT( vkCmdInsertDebugUtilsLabelEXT );
        VKE_FUNC_EXT( vkCreateDebugUtilsMessengerEXT );
        VKE_FUNC_EXT( vkSubmitDebugUtilsMessageEXT );
        VKE_FUNC_EXT( vkDestroyDebugUtilsMessengerEXT );


#ifdef VK_USE_PLATFORM_XCB_KHR
        //VK_KHR_xcb_surface
        VKE_FUNC(vkCreateXcbSurfaceKHR)
        VKE_FUNC(vkGetPhysicalDeviceXcbPresentationSupportKHR);
#endif

#ifdef VK_USE_PLATFORM_ANDROID_KHR
        //VK_KHR_android_surface
        VKE_FUNC(vkCreateAndroidSurfaceKHR)
#endif

#ifdef VK_USE_PLATFORM_WIN32_KHR
            //VK_KHR_win32_surface
        VKE_FUNC(vkCreateWin32SurfaceKHR);
        VKE_FUNC(vkGetPhysicalDeviceWin32PresentationSupportKHR);
#endif

        //VK_EXT_debug_report
        VKE_FUNC(vkCreateDebugReportCallbackEXT);
        VKE_FUNC(vkDestroyDebugReportCallbackEXT);
        VKE_FUNC(vkDebugReportMessageEXT);
    };

    struct Device
    {
        VKE_FUNC(vkGetDeviceQueue);
        VKE_FUNC(vkQueueSubmit);
        VKE_FUNC(vkQueueWaitIdle);
        VKE_FUNC(vkDeviceWaitIdle);
        VKE_FUNC(vkAllocateMemory);
        VKE_FUNC(vkFreeMemory);
        VKE_FUNC(vkMapMemory);
        VKE_FUNC(vkUnmapMemory);
        VKE_FUNC(vkFlushMappedMemoryRanges);
        VKE_FUNC(vkInvalidateMappedMemoryRanges);
        VKE_FUNC(vkBindBufferMemory);
        VKE_FUNC(vkBindImageMemory);
        VKE_FUNC(vkGetImageMemoryRequirements);
        VKE_FUNC(vkGetBufferMemoryRequirements);
        VKE_FUNC(vkQueueBindSparse);
        VKE_FUNC(vkCreateFence);
        VKE_FUNC(vkResetFences);
        VKE_FUNC(vkGetFenceStatus);
        VKE_FUNC(vkWaitForFences);
        VKE_FUNC(vkCreateSemaphore);
        VKE_FUNC(vkCreateEvent);
        VKE_FUNC(vkGetEventStatus);
        VKE_FUNC(vkSetEvent);
        VKE_FUNC(vkResetEvent);
        VKE_FUNC(vkCreateQueryPool);
        VKE_FUNC(vkGetQueryPoolResults);
        VKE_FUNC(vkCreateBuffer);
        VKE_FUNC(vkCreateBufferView);
        VKE_FUNC(vkCreateImage);
        VKE_FUNC(vkGetImageSubresourceLayout);
        VKE_FUNC(vkCreateImageView);
        VKE_FUNC(vkCreateShaderModule);
        VKE_FUNC(vkCreatePipelineCache);
        VKE_FUNC(vkGetPipelineCacheData);
        VKE_FUNC(vkMergePipelineCaches);
        VKE_FUNC(vkCreateGraphicsPipelines);
        VKE_FUNC(vkCreateComputePipelines);
        VKE_FUNC(vkCreatePipelineLayout);
        VKE_FUNC(vkCreateSampler);
        VKE_FUNC(vkCreateDescriptorSetLayout);
        VKE_FUNC(vkCreateDescriptorPool);
        VKE_FUNC(vkResetDescriptorPool);
        VKE_FUNC(vkAllocateDescriptorSets);
        VKE_FUNC(vkFreeDescriptorSets);
        VKE_FUNC(vkUpdateDescriptorSets);
        VKE_FUNC(vkAllocateCommandBuffers);
        VKE_FUNC(vkBeginCommandBuffer);
        VKE_FUNC(vkEndCommandBuffer);
        VKE_FUNC(vkResetCommandBuffer);
        VKE_FUNC(vkCreateCommandPool);
        VKE_FUNC(vkDestroyFence);
        VKE_FUNC(vkDestroySemaphore);
        VKE_FUNC(vkDestroyEvent);
        VKE_FUNC(vkDestroyQueryPool);
        VKE_FUNC(vkDestroyBuffer);
        VKE_FUNC(vkDestroyBufferView);
        VKE_FUNC(vkDestroyImage);
        VKE_FUNC(vkDestroyImageView);
        VKE_FUNC(vkDestroyShaderModule);
        VKE_FUNC(vkDestroyPipelineCache);
        VKE_FUNC(vkDestroyPipeline);
        VKE_FUNC(vkDestroyPipelineLayout);
        VKE_FUNC(vkDestroySampler);
        VKE_FUNC(vkDestroyDescriptorSetLayout);
        VKE_FUNC(vkDestroyDescriptorPool);
        VKE_FUNC(vkDestroyFramebuffer);
        VKE_FUNC(vkDestroyRenderPass);
        VKE_FUNC(vkFreeCommandBuffers);
        VKE_FUNC(vkDestroyCommandPool);
        VKE_FUNC(vkCmdBindPipeline);
        VKE_FUNC(vkCmdSetViewport);
        VKE_FUNC(vkCmdSetScissor);
        VKE_FUNC(vkCmdSetLineWidth);
        VKE_FUNC(vkCmdSetDepthBias);
        VKE_FUNC(vkCmdSetBlendConstants);
        VKE_FUNC(vkCmdSetDepthBounds);
        VKE_FUNC(vkCmdSetStencilCompareMask);
        VKE_FUNC(vkCmdSetStencilWriteMask);
        VKE_FUNC(vkCmdSetStencilReference);
        VKE_FUNC(vkCmdBindDescriptorSets);
        VKE_FUNC(vkCmdBindIndexBuffer);
        VKE_FUNC(vkCmdBindVertexBuffers);
        VKE_FUNC(vkCmdDraw);
        VKE_FUNC(vkCmdDrawIndexed);
        VKE_FUNC(vkCmdDrawIndirect);
        VKE_FUNC(vkCmdDrawIndexedIndirect);
        VKE_FUNC(vkCmdDispatch);
        VKE_FUNC(vkCmdDispatchIndirect);
        VKE_FUNC(vkCmdCopyBuffer);
        VKE_FUNC(vkCmdCopyImage);
        VKE_FUNC(vkCmdBlitImage);
        VKE_FUNC( vkCmdBlitImage2KHR );
        VKE_FUNC(vkCmdCopyBufferToImage);
        VKE_FUNC(vkCmdCopyImageToBuffer);
        VKE_FUNC(vkCmdUpdateBuffer);
        VKE_FUNC(vkCmdFillBuffer);
        VKE_FUNC(vkCmdClearColorImage);
        VKE_FUNC(vkCmdClearAttachments);
        VKE_FUNC(vkCmdClearDepthStencilImage);
        VKE_FUNC(vkCmdResolveImage);
        VKE_FUNC(vkCmdSetEvent);
        VKE_FUNC(vkCmdResetEvent);
        VKE_FUNC(vkCmdWaitEvents);
        VKE_FUNC(vkCmdPipelineBarrier);
        VKE_FUNC(vkCmdBeginQuery);
        VKE_FUNC(vkCmdEndQuery);
        VKE_FUNC(vkCmdResetQueryPool);
        VKE_FUNC(vkCmdWriteTimestamp);
        VKE_FUNC(vkCmdCopyQueryPoolResults);
        VKE_FUNC(vkCreateFramebuffer);
        VKE_FUNC(vkCreateRenderPass);
        VKE_FUNC(vkCmdBeginRenderPass);
        VKE_FUNC(vkCmdNextSubpass);
        VKE_FUNC(vkCmdEndRenderPass);
        VKE_FUNC(vkCmdExecuteCommands);
        VKE_FUNC(vkCmdPushConstants);

        VKE_FUNC(vkCreateSwapchainKHR);
        VKE_FUNC(vkDestroySwapchainKHR);
        VKE_FUNC(vkGetSwapchainImagesKHR);
        VKE_FUNC(vkAcquireNextImageKHR);
        VKE_FUNC(vkQueuePresentKHR);

        VKE_FUNC( vkGetDescriptorSetLayoutSupport );

        VKE_EXT_FUNC( vkCmdDebugMarkerBeginEXT );
        VKE_EXT_FUNC( vkCmdDebugMarkerEndEXT );
        VKE_EXT_FUNC( vkCmdDebugMarkerInsertEXT );

        VKE_EXT_FUNC( vkCreateDebugReportCallbackEXT );
        VKE_EXT_FUNC( vkDebugReportMessageEXT );
        VKE_EXT_FUNC( vkDestroyDebugReportCallbackEXT );

        VKE_EXT_FUNC( vkCmdSetCheckpointNV );
        VKE_EXT_FUNC( vkGetQueueCheckpointDataNV );

        // EXT
        VKE_EXT_FUNC( vkCmdDrawMeshTasksEXT );
        //VKE_EXT_FUNC( vkCmdDrawMeshTasksIndirectEXT );
        //VKE_EXT_FUNC( vkCmdDrawMeshTasksIndirectCountEXT );

    };
};
#endif

#undef VK_EXPORTED_FUNCTION
#undef VKE_ICD_GLOBAL
#undef VKE_INSTANCE_ICD
#undef VKE_DEVICE_ICD  