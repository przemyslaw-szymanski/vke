#pragma once

#include "Core/VKECommon.h"

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
#ifdef __cplusplus
extern "C" {
#endif
#define VKE_AUTO_ICD 1
#define VK_EXPORTED_FUNCTION(name) PFN_##name name
#define VKE_ICD_GLOBAL(name) VK_EXPORTED_FUNCTION(name)
#define VKE_INSTANCE_ICD(name) VK_EXPORTED_FUNCTION(name)
#define VKE_DEVICE_ICD(name) VK_EXPORTED_FUNCTION(name)
#define VKE_DECLARE_GLOBAL_ICD 1
#define VKE_DECLARE_INSTANCE_ICD 1
#define VKE_DECLARE_DEVICE_ICD 1
#include "ThirdParty/vulkan/VKEICD.h"
#undef VKE_DEVICE_ICD
#undef VKE_INSTANCE_ICD
#undef VKE_ICD_GLOBAL
#undef VK_EXPORTED_FUNCTION
#undef VKE_DECLARE_GLOBAL_ICD
#undef VKE_DECLARE_INSTANCE_ICD
#undef VKE_DECLARE_DEVICE_ICD
#ifdef __cplusplus
} // extern "C"
#endif
#include "RenderSystem/Vulkan/VKEImageFormats.h"
#include "ThirdParty/vulkan/vkFormatList.h"

#if VKE_USE_VULKAN_WINDOWS
#   define VK_KHR_PLATFORM_SURFACE_EXTENSION_NAME VK_KHR_WIN32_SURFACE_EXTENSION_NAME
#elif VKE_USE_VULKAN_LINUX
#   define VK_KHR_PLATFORM_SURFACE_EXTENSION_NAME VK_KHR_XCB_SURFACE_EXTENSION_NAME
#elif VKE_USE_VULKAN_ANDROID
#error implement here
#endif

namespace VKE
{
    namespace Vulkan
    {
#if VKE_WINDOWS
        static cstr_t g_pVkValidationLibName = "igvk64.dll";
#if VKE_ARCHITECTURE_64
        static cstr_t g_pVkLibName = "vulkan-1.dll";
#else
        static cstr_t g_pVkLibName = "vulkan-1.dll";
#endif // ARCHITECTURE
#elif VKE_LINUX || VKE_ANDROID
        static cstr_t g_pVkLibName = "libvulkan-1.so";
#endif

        static cstr_t g_pVulkanLibName = g_pVkLibName;

        template<typename _INFO_, typename _TYPE_> vke_force_inline
        void InitInfo(_INFO_* pInfo, _TYPE_ type)
        {
            pInfo->sType = type;
            pInfo->pNext = nullptr;
        }

        vke_inline
        cstr_t ErrorToString(VkResult err)
        {
            switch(err)
            {
                case VK_ERROR_DEVICE_LOST: return VKE_TO_STRING(VK_ERROR_DEVICE_LOST); break;
                case VK_ERROR_EXTENSION_NOT_PRESENT: return VKE_TO_STRING(VK_ERROR_EXTENSION_NOT_PRESENT); break;
                case VK_ERROR_INCOMPATIBLE_DRIVER: return VKE_TO_STRING(VK_ERROR_INCOMPATIBLE_DRIVER); break;
                case VK_ERROR_INITIALIZATION_FAILED: return VKE_TO_STRING(VK_ERROR_INITIALIZATION_FAILED); break;
                case VK_ERROR_LAYER_NOT_PRESENT: return VKE_TO_STRING(VK_ERROR_LAYER_NOT_PRESENT); break;
                case VK_ERROR_MEMORY_MAP_FAILED: return VKE_TO_STRING(VK_ERROR_MEMORY_MAP_FAILED); break;
                //case VK_ERROR_OUT_OF_DATE_WSI: return VKE_TO_STRING(VK_ERROR_OUT_OF_DATE_WSI); break;
                case VK_ERROR_OUT_OF_DEVICE_MEMORY: return VKE_TO_STRING(VK_ERROR_OUT_OF_DEVICE_MEMORY); break;
                case VK_ERROR_OUT_OF_HOST_MEMORY: return VKE_TO_STRING(VK_ERROR_OUT_OF_HOST_MEMORY); break;
                case VK_SUCCESS: return VKE_TO_STRING(VK_SUCCESS); break;
                case VK_NOT_READY: return VKE_TO_STRING(VK_NOT_READY); break;
                case VK_EVENT_RESET: return VKE_TO_STRING(VK_EVENT_RESET); break;
                case VK_TIMEOUT: return VKE_TO_STRING(VK_TIMEOUT); break;
                case VK_EVENT_SET: return VKE_TO_STRING(VK_EVENT_SET); break;
                case VK_INCOMPLETE: return VKE_TO_STRING(VK_INCOMPLETE); break;
            }
            return "Unknown Error";
        }

        Result LoadGlobalFunctions(handle_t hDll, ICD::Global*);
        Result LoadInstanceFunctions(VkInstance vkInstance, const ICD::Global&, ICD::Instance*);
        Result LoadDeviceFunctions(VkDevice vkDevice, const ICD::Instance&, ICD::Device*);

    } // Vulkan
#if VKE_DEBUG
#   define VKE_LOG_VULKAN_ERROR(_err, _exp) VKE_LOG_ERR("Vulkan function: " << VKE_TO_STRING(_exp) << " error: " << Vulkan::ErrorToString((_err)))
#   define VK_ERR(_exp) VKE_CODE(auto err = _exp; if(err != VK_SUCCESS) { VKE_LOG_VULKAN_ERROR(err, _exp); assert(false); })
#   define VK_DESTROY(_func, _deviceHandle, _resHandle, _allocator) VKE_CODE( if((_resHandle) != VK_NULL_HANDLE) { _func((_deviceHandle), (_resHandle), (_allocator)); (_resHandle) = VK_NULL_HANDLE; })
#else
#   define VK_ERR(_exp) _exp
#   define VK_DESTROY(_func, _deviceHandle, _resHandle, _allocator) VKE_CODE( _func((_deviceHandle), (_resHandle), (_allocator)); (_resHandle) = VK_NULL_HANDLE; )
#endif // VKE_DEBUG
} // vke