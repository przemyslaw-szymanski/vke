#pragma once

#include "Core/VKECommon.h"
#include "Core/CObject.h"
#include "Core/Threads/Common.h"
#include "Core/Utils/CLogger.h"


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
#define VKE_INSTANCE_EXT_ICD(name) VK_EXPORTED_FUNCTION(name)
#define VKE_DEVICE_ICD(name) VK_EXPORTED_FUNCTION(name)
#define VKE_DEVICE_EXT_ICD(name) VK_EXPORTED_FUNCTION(name)
#define VKE_DECLARE_GLOBAL_ICD 1
#define VKE_DECLARE_INSTANCE_ICD 1
#define VKE_DECLARE_DEVICE_ICD 1
#include "ThirdParty/vulkan/VKEICD.h"
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
#include "RenderSystem/Vulkan/VKEImageFormats.h"
#include "ThirdParty/vulkan/vkFormatList.h"
#include "RenderSystem/Vulkan/CVkDeviceWrapper.h"
#include "Core/Utils/TCDynamicArray.h"
#include "RenderSystem/Common.h"

#if VKE_USE_VULKAN_WINDOWS
#   define VK_KHR_PLATFORM_SURFACE_EXTENSION_NAME VK_KHR_WIN32_SURFACE_EXTENSION_NAME
#elif VKE_USE_VULKAN_LINUX
#   define VK_KHR_PLATFORM_SURFACE_EXTENSION_NAME VK_KHR_XCB_SURFACE_EXTENSION_NAME
#elif VKE_USE_VULKAN_ANDROID
#error implement here
#endif

#define VKE_VULKAN_NEGATIVE_VIEWPORT_HEIGT 1

namespace VKE
{
    namespace RenderSystem
    {
        class CDeviceContext;
    }

    namespace RenderSystem
    {

    }

    namespace Vulkan
    {
#if VKE_WINDOWS

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
            Memory::Zero( pInfo, 1 );
            pInfo->sType = type;
        }

        using CommandBufferArray = Utils::TCDynamicArray< VkCommandBuffer >;
        using ImageArray = Utils::TCDynamicArray< VkImage >;
        using ImageViewArray = Utils::TCDynamicArray< VkImageView >;
        using SwapChainArray = Utils::TCDynamicArray< VkSwapchainKHR >;
        using SemaphoreArray = Utils::TCDynamicArray< VkSemaphore >;
        using FenceArray = Utils::TCDynamicArray< VkFence >;
        using UintArray = Utils::TCDynamicArray< uint32_t >;
        using HandleArray = Utils::TCDynamicArray< handle_t >;

        template<typename ICD_Global, typename ICD_Instance, typename ICD_Device>
        struct TICD
        {
            ICD_Global      Global;
            ICD_Instance    Instance;
            ICD_Device      Device;

            TICD(const ICD_Global G, const ICD_Instance I, const ICD_Device D) :
                Global(G), Instance(I), Device(D) {}
        };

        struct ICD
        {
            typedef struct
            {
                VkICD::Global   Global;
            } Global;

            typedef struct
            {
                VkICD::Global   Global;
                VkICD::Instance Instance;
            } Instance;

            typedef struct _Device
            {
                VkICD::Global&      Global;
                VkICD::Instance&    Instance;
                VkICD::Device       Device;

                _Device(VkICD::Global& G, VkICD::Instance& I) :
                    Global(G), Instance(I) {}
                void operator=(const _Device&) = delete;
            } Device;
        };

        struct SCommandBuffer
        {
            VkCommandBuffer vkCommandBuffer;
            VkFence         vkFence;
        };

        struct SPresentData
        {
            SwapChainArray      vSwapChains;
            SemaphoreArray      vWaitSemaphores;
            UintArray           vImageIndices;
        };

        struct SQueue final : protected Core::CObject
        {
            friend class RenderSystem::CDeviceContext;

            SQueue();

            VkQueue             vkQueue = VK_NULL_HANDLE;
            uint32_t            familyIndex = 0;
            Threads::SyncObject SyncObj; // for synchronization if refCount > 1

            void Lock()
            {
                if( this->GetRefCount() > 1 )
                {
                    SyncObj.Lock();
                }
            }

            void Unlock()
            {
                if( SyncObj.IsLocked() )
                {
                    SyncObj.Unlock();
                }
            }

            bool WillNextSwapchainDoPresent() const
            {
                return m_swapChainCount == m_PresentData.vSwapChains.GetCount() + 1;
            }

            bool IsPresentDone();
            void NeedPresent();
            void ReleasePresentNotify();
            void Wait(const VkICD::Device& ICD);

            VkResult Submit(const VkICD::Device& ICD, const VkSubmitInfo&, const VkFence&);
            Result Present(const VkICD::Device& ICD, uint32_t, VkSwapchainKHR, VkSemaphore);

            private:
                SPresentData        m_PresentData;
                VkPresentInfoKHR    m_PresentInfo;
                uint32_t            m_swapChainCount = 0;
                int32_t             m_presentCount = 0;
                bool                m_isPresentDone = false;
        };
        using Queue = SQueue*;

        void SetLastError(VkResult err);
        VkResult GetLastError();

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
                case VK_ERROR_SURFACE_LOST_KHR: return VKE_TO_STRING( VK_ERROR_SURFACE_LOST_KHR ); break;
                case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR: return VKE_TO_STRING( VK_ERROR_NATIVE_WINDOW_IN_USE_KHR ); break;
                case VK_SUBOPTIMAL_KHR: return VKE_TO_STRING( VK_SUBOPTIMAL_KHR ); break;
                case VK_ERROR_OUT_OF_DATE_KHR: return VKE_TO_STRING( VK_ERROR_OUT_OF_DATE_KHR ); break;
                case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR: return VKE_TO_STRING( VK_ERROR_INCOMPATIBLE_DISPLAY_KHR ); break;
                case VK_ERROR_VALIDATION_FAILED_EXT: return VKE_TO_STRING( VK_ERROR_VALIDATION_FAILED_EXT ); break;
            }
            return "Unknown Error";
        }

        Result LoadGlobalFunctions(handle_t hDll, VkICD::Global*);
        Result LoadInstanceFunctions(VkInstance vkInstance, const VkICD::Global&, VkICD::Instance*);
        Result LoadDeviceFunctions(VkDevice vkDevice, const VkICD::Instance&, VkICD::Device*);

        namespace Map
        {
            VkSampleCountFlagBits SampleCount(const RenderSystem::SAMPLE_COUNT& count);
            VkImageType ImageType(RenderSystem::TEXTURE_TYPE type);
            VkImageViewType ImageViewType(RenderSystem::TEXTURE_VIEW_TYPE type);
            VkImageUsageFlags ImageUsage(RenderSystem::TEXTURE_USAGE usage);
            VkImageAspectFlags ImageAspect(RenderSystem::TEXTURE_ASPECT aspect);
            VkImageLayout ImageLayout(RenderSystem::TEXTURE_STATE layout);
            VkMemoryPropertyFlags MemoryProperyFlags(RenderSystem::MEMORY_USAGE usages);
            VkBlendOp BlendOp(const RenderSystem::BLEND_OPERATION& op);
            VkColorComponentFlags ColorComponent(const RenderSystem::ColorComponent& component);
            VkBlendFactor BlendFactor(const RenderSystem::BLEND_FACTOR& factor);
            VkLogicOp LogicOperation(const RenderSystem::LOGIC_OPERATION& op);
            VkStencilOp StencilOperation(const RenderSystem::STENCIL_FUNCTION& op);
            VkCompareOp CompareOperation(const RenderSystem::COMPARE_FUNCTION& op);
            VkPrimitiveTopology PrimitiveTopology(const RenderSystem::PRIMITIVE_TOPOLOGY& topology);
            VkCullModeFlags CullMode(const RenderSystem::CULL_MODE& mode);
            VkFrontFace FrontFace(const RenderSystem::FRONT_FACE& face);
            VkPolygonMode PolygonMode(const RenderSystem::POLYGON_MODE& mode);
            VkShaderStageFlagBits ShaderStage(const RenderSystem::SHADER_TYPE& type);
            VkVertexInputRate InputRate(const RenderSystem::VERTEX_INPUT_RATE& rate);
            VkDescriptorType DescriptorType(const RenderSystem::DESCRIPTOR_SET_TYPE& type);
        } // Mapping

        namespace Convert
        {
            VkImageViewType ImageTypeToViewType(VkImageType type);
            VkImageAspectFlags UsageToAspectMask(VkImageUsageFlags usage);
            VkAttachmentStoreOp UsageToStoreOp(RenderSystem::RENDER_TARGET_RENDER_PASS_OP usage);
            VkAttachmentLoadOp UsageToLoadOp(RenderSystem::RENDER_TARGET_RENDER_PASS_OP usage);
            VkImageLayout ImageUsageToLayout(VkImageUsageFlags vkFlags);
            VkImageLayout ImageUsageToInitialLayout(VkImageUsageFlags vkFlags);
            VkImageLayout ImageUsageToFinalLayout(VkImageUsageFlags vkFlags);
            VkImageLayout NextAttachmentLayoutRread(VkImageLayout currLayout);
            VkImageLayout NextAttachmentLayoutOptimal(VkImageLayout currLayout);
            RenderSystem::TEXTURE_FORMAT ImageFormat(VkFormat vkFormat);
            VkPipelineStageFlags PipelineStages(const RenderSystem::PIPELINE_STAGES& stages);
            VkBufferUsageFlags BufferUsage( const RenderSystem::BUFFER_USAGE& usage );
            VkImageTiling ImageUsageToTiling( const RenderSystem::TEXTURE_USAGE& usage );
            VkMemoryPropertyFlags MemoryUsagesToVkMemoryPropertyFlags( const RenderSystem::MEMORY_USAGE& usages );
        } // Convert
    } // Vulkan
#if VKE_DEBUG
#   define VKE_LOG_VULKAN_ERROR(_err, _exp) \
        VKE_LOG_ERR("Vulkan function: " << VKE_TO_STRING(_exp) << \
        " error (" << (_err) << "): " << Vulkan::ErrorToString((_err)))

#   define VK_ERR(_exp) \
    VKE_CODE(auto err = _exp; if(err != VK_SUCCESS) { \
        VKE_LOG_VULKAN_ERROR(err, _exp); VKE_ASSERT2(err == VK_SUCCESS, ""); SetLastError(err); })

#   define VK_DESTROY(_func, _deviceHandle, _resHandle, _allocator) \
    VKE_CODE( if((_resHandle) != VK_NULL_HANDLE) { \
        _func((_deviceHandle), (_resHandle), (_allocator)); (_resHandle) = VK_NULL_HANDLE; })
#else
#   define VK_ERR(_exp) _exp
#   define VK_DESTROY(_func, _deviceHandle, _resHandle, _allocator) VKE_CODE( _func((_deviceHandle), (_resHandle), (_allocator)); (_resHandle) = VK_NULL_HANDLE; )
#endif // VKE_DEBUG
} // vke