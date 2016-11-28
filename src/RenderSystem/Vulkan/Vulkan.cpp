#include "RenderSystem/Vulkan/Vulkan.h"
#include "Core/Platform/CPlatform.h"
#include "Core/Utils/CLogger.h"

//#undef VKE_VK_FUNCTION
//#define VKE_VK_FUNCTION(_name) PFN_##_name _name
//#undef VK_EXPORTED_FUNCTION
//#undef VKE_ICD_GLOBAL
//#undef VKE_INSTANCE_ICD
//#undef VKE_DEVICE_ICD
//#define VK_EXPORTED_FUNCTION(name) PFN_##name name = 0
//#define VKE_ICD_GLOBAL(name) PFN_##name name = 0
//#define VKE_INSTANCE_ICD(name) PFN_##name name = 0
//#define VKE_DEVICE_ICD(name) PFN_##name name = 0
//#include "ThirdParty/vulkan/funclist.h"
//#undef VKE_DEVICE_ICD
//#undef VKE_INSTANCE_ICD
//#undef VKE_ICD_GLOBAL
//#undef VK_EXPORTED_FUNCTION
//#undef VKE_VK_FUNCTION

namespace VKE
{
    namespace Vulkan
    {

        using ErrorMap = std::map< std::thread::id, VkResult >;
        ErrorMap g_mErrors;
        std::mutex g_ErrorMutex;

        void SetLastError(VkResult err)
        {
            g_ErrorMutex.lock();
            g_mErrors[ std::this_thread::get_id() ] = err;
            g_ErrorMutex.unlock();
        }

        VkResult GetLastError()
        {
            g_ErrorMutex.lock();
            auto ret = g_mErrors[ std::this_thread::get_id() ];
            g_ErrorMutex.unlock();
            return ret;
        }

        SQueue::SQueue()
        {
            this->m_objRefCount = 0;
            Vulkan::InitInfo(&m_PresentInfo, VK_STRUCTURE_TYPE_PRESENT_INFO_KHR);
            m_PresentInfo.pResults = nullptr;
        }

        VkResult SQueue::Submit(const VkICD::Device& ICD, const VkSubmitInfo& Info, const VkFence& vkFence)
        {
            Lock();
            auto res = ICD.vkQueueSubmit(vkQueue, 1, &Info, vkFence);
            Unlock();
            return res;
        }

        Result SQueue::Present(const VkICD::Device& ICD, uint32_t imgIdx, VkSwapchainKHR vkSwpChain,
                               VkSemaphore vkWaitSemaphore)
        {
            Lock();
            m_PresentData.vImageIndices.PushBack(imgIdx);
            m_PresentData.vSwapChains.PushBack(vkSwpChain);
            m_PresentData.vWaitSemaphores.PushBack(vkWaitSemaphore);
            if( this->GetRefCount() == m_PresentData.vSwapChains.GetCount() )
            {
                m_PresentInfo.pImageIndices = &m_PresentData.vImageIndices[ 0 ];
                m_PresentInfo.pSwapchains = &m_PresentData.vSwapChains[ 0 ];
                m_PresentInfo.pWaitSemaphores = &m_PresentData.vWaitSemaphores[ 0 ];
                m_PresentInfo.swapchainCount = m_PresentData.vSwapChains.GetCount();
                m_PresentInfo.waitSemaphoreCount = m_PresentData.vWaitSemaphores.GetCount();
                VK_ERR( ICD.vkQueuePresentKHR(vkQueue, &m_PresentInfo) );
                m_PresentData.vImageIndices.Clear<false>();
                m_PresentData.vSwapChains.Clear<false>();
                m_PresentData.vWaitSemaphores.Clear<false>();
                return VKE_OK;
            }
            Unlock();
            return VKE_FAIL;
        }

        VkSampleCountFlagBits GetSampleCount(RenderSystem::MULTISAMPLING_TYPE type)
        {
            static const VkSampleCountFlagBits aSampleBits[] =
            {
                VK_SAMPLE_COUNT_1_BIT,
                VK_SAMPLE_COUNT_2_BIT,
                VK_SAMPLE_COUNT_4_BIT,
                VK_SAMPLE_COUNT_8_BIT,
                VK_SAMPLE_COUNT_16_BIT,
                VK_SAMPLE_COUNT_32_BIT,
                VK_SAMPLE_COUNT_64_BIT
            };
            return aSampleBits[ type ];
        }

        VkImageType GetImageType(RenderSystem::TEXTURE_TYPE type)
        {
            static const VkImageType aVkImageTypes[] =
            {
                VK_IMAGE_TYPE_1D,
                VK_IMAGE_TYPE_2D,
                VK_IMAGE_TYPE_3D,
            };
            return aVkImageTypes[ type ];
        }

        VkImageViewType GetImageViewType(RenderSystem::TEXTURE_VIEW_TYPE type)
        {
            static const VkImageViewType aVkTypes[] =
            {
                VK_IMAGE_VIEW_TYPE_1D,
                VK_IMAGE_VIEW_TYPE_2D,
                VK_IMAGE_VIEW_TYPE_3D,
                VK_IMAGE_VIEW_TYPE_1D_ARRAY,
                VK_IMAGE_VIEW_TYPE_2D_ARRAY,
                VK_IMAGE_VIEW_TYPE_CUBE,
                VK_IMAGE_VIEW_TYPE_CUBE_ARRAY
            };
            return aVkTypes[ type ];
        }

        VkImageUsageFlags GetImageUsage(RenderSystem::TEXTURE_USAGE usage)
        {
            static const VkImageUsageFlags aVkUsages[] =
            {
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
                VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
                VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_IMAGE_USAGE_SAMPLED_BIT
            };
            return aVkUsages[ usage ];
        }

        VkImageAspectFlags GetImageAspect(RenderSystem::TEXTURE_ASPECT aspect)
        {
            static const VkImageAspectFlags aVkAspects[] =
            {
                // UNKNOWN
                0,
                // COLOR
                VK_IMAGE_ASPECT_COLOR_BIT,
                // DEPTH
                VK_IMAGE_ASPECT_DEPTH_BIT,
                // STENCIL
                VK_IMAGE_ASPECT_STENCIL_BIT,
                // DEPTH_STENCIL
                VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT
            };
            return aVkAspects[ aspect ];
        }

        VkImageAspectFlags ConvertUsageToAspectMask(VkImageUsageFlags usage)
        {
            if( usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT )
            {
                return VK_IMAGE_ASPECT_COLOR_BIT;
            }
            if( usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT )
            {
                return VK_IMAGE_ASPECT_DEPTH_BIT;
            }
            VKE_LOG_ERR("Invalid image usage: " << usage << " to use for aspectMask");
            assert(0, "Invalid image usage");
            return VK_IMAGE_ASPECT_COLOR_BIT;
        }

        VkImageViewType ConvertImageTypeToViewType(VkImageType type)
        {
            static const VkImageViewType aTypes[] =
            {
                VK_IMAGE_VIEW_TYPE_1D,
                VK_IMAGE_VIEW_TYPE_2D,
                VK_IMAGE_VIEW_TYPE_3D
            };
            assert(type <= VK_IMAGE_TYPE_3D && "Invalid image type for image view type");
            return aTypes[ type ];
        }

        void ConverRenderTargetAttachmentToVkAttachment(RenderSystem::RENDER_TARGET_ATTACHMENT_USAGE usage,
                                                        VkImageLayout* pInitialOut,
                                                        VkImageLayout* pFinalOut,
                                                        VkAttachmentLoadOp* pLoadOpOut,
                                                        VkAttachmentStoreOp* pStoreOpOut)
        {
            static const VkImageLayout aInitials[] =
            {
                VK_IMAGE_LAYOUT_UNDEFINED, // undefined
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, // color_Write
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, // color_write_clear
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, // color_write_preserved
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, // depth_stencil_write
                VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, // color_read
                VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, // depth_stencil_read
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, // color_write_read
                VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, // depth_stencil_write_read
            };

            static const VkImageLayout aFinals[] =
            {
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, // color_write
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, // color_write_clear
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, // color_write_preserve
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, // color_write_clear_preserve
                VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, // depth_stencil_write
                VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, // depth_stencil_write_clear,
                VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, // dpeth_stencil_write_preserve
                VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, // color_read
                VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, // depth_Stencil_read
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, // color_write_read
                VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, // depth_Stencil_write_read
            };
        }

        VkAttachmentStoreOp ConvertUsageToStoreOp(RenderSystem::RENDER_TARGET_ATTACHMENT_USAGE usage)
        {
            static const VkAttachmentStoreOp aStores[] =
            {
                VK_ATTACHMENT_STORE_OP_DONT_CARE,
                VK_ATTACHMENT_STORE_OP_STORE
            };
            return aStores[ usage ];
        }

#define VKE_EXPORT_FUNC(_name, _handle, _getProcAddr) \
    pOut->_name = (PFN_##_name)(_getProcAddr((_handle), #_name)); \
    if(!pOut->_name) \
            { VKE_LOG_ERR("Unable to load function: " << #_name); err = VKE_ENOTFOUND; }

        Result LoadGlobalFunctions(handle_t hLib, VkICD::Global* pOut)
        {
            Result err = VKE_OK;
#if VKE_AUTO_ICD
#define VK_EXPORTED_FUNCTION(_name) VKE_EXPORT_FUNC(_name, hLib, Platform::DynamicLibrary::GetProcAddress)
#include "ThirdParty/vulkan/VKEICD.h"
#undef VK_EXPORTED_FUNCTION
#define VKE_ICD_GLOBAL(_name) VKE_EXPORT_FUNC(_name, VK_NULL_HANDLE, pOut->vkGetInstanceProcAddr)
#include "ThirdParty/vulkan/VKEICD.h"
#undef VKE_ICD_GLOBAL
#else // VKE_AUTO_ICD
            pOut->vkGetInstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(Platform::GetProcAddress(hLib, "vkGetInstanceProcAddr"));
            pOut->vkCreateInstance = reinterpret_cast<PFN_vkCreateInstance>(pOut->vkGetInstanceProcAddr(VK_NULL_HANDLE, "vkCreateInstance"));
            pOut->vkEnumerateInstanceExtensionProperties = reinterpret_cast<PFN_vkEnumerateInstanceExtensionProperties>(pOut->vkGetInstanceProcAddr(VK_NULL_HANDLE, "vkEnumerateInstanceExtensionProperties"));
            pOut->vkEnumerateInstanceLayerProperties = reinterpret_cast<PFN_vkEnumerateInstanceLayerProperties>(pOut->vkGetInstanceProcAddr(VK_NULL_HANDLE, "vkEnumerateInstanceLayerProperties"));
#endif // VKE_AUTO_ICD
            return err;
        }

        Result LoadInstanceFunctions(VkInstance vkInstance, const VkICD::Global& Global,
                                     VkICD::Instance* pOut)
        {
            Result err = VKE_OK;
#if VKE_AUTO_ICD
#define VKE_INSTANCE_ICD(_name) VKE_EXPORT_FUNC(_name, vkInstance, Global.vkGetInstanceProcAddr)
#include "ThirdParty/vulkan/VKEICD.h"
#undef VKE_INSTANCE_ICD
#else // VKE_AUTO_ICD
            pOut->vkDestroySurfaceKHR = reinterpret_cast<PFN_vkDestroySurfaceKHR>(Global.vkGetInstanceProcAddr(vkInstance, "vkDestroySurfaceKHR"));
#endif // VKE_AUTO_ICD
            return err;
        }

        Result LoadDeviceFunctions(VkDevice vkDevice, const VkICD::Instance& Instance, VkICD::Device* pOut)
        {
            Result err = VKE_OK;
#if VKE_AUTO_ICD
#define VKE_DEVICE_ICD(_name) VKE_EXPORT_FUNC(_name, vkDevice, Instance.vkGetDeviceProcAddr)
#include "ThirdParty/vulkan/VKEICD.h"
#undef VKE_DEVICE_ICD
#else // VKE_AUTO_ICD

#endif // VKE_AUTO_ICD
            return err;
        }
    }
}