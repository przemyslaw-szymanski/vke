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

        namespace Map
        {

            VkSampleCountFlagBits SampleCount(RenderSystem::MULTISAMPLING_TYPE type)
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

            VkImageType ImageType(RenderSystem::TEXTURE_TYPE type)
            {
                static const VkImageType aVkImageTypes[] =
                {
                    VK_IMAGE_TYPE_1D,
                    VK_IMAGE_TYPE_2D,
                    VK_IMAGE_TYPE_3D,
                };
                return aVkImageTypes[ type ];
            }

            VkImageViewType ImageViewType(RenderSystem::TEXTURE_VIEW_TYPE type)
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

            VkImageUsageFlags ImageUsage(RenderSystem::TEXTURE_USAGE usage)
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

            VkImageAspectFlags ImageAspect(RenderSystem::TEXTURE_ASPECT aspect)
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
        } // Map

        namespace Convert
        {
            VkImageAspectFlags UsageToAspectMask(VkImageUsageFlags usage)
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
                assert(0 && "Invalid image usage");
                return VK_IMAGE_ASPECT_COLOR_BIT;
            }

            VkImageViewType ImageTypeToViewType(VkImageType type)
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

            VkAttachmentLoadOp UsageToLoadOp(RenderSystem::RENDER_TARGET_WRITE_ATTACHMENT_USAGE usage)
            {
                static const VkAttachmentLoadOp aLoads[] =
                {
                    VK_ATTACHMENT_LOAD_OP_DONT_CARE, // undefined
                    VK_ATTACHMENT_LOAD_OP_DONT_CARE, // color
                    VK_ATTACHMENT_LOAD_OP_CLEAR, // color clear
                    VK_ATTACHMENT_LOAD_OP_DONT_CARE, // color store
                    VK_ATTACHMENT_LOAD_OP_CLEAR, // color clear store
                    VK_ATTACHMENT_LOAD_OP_DONT_CARE, // depth
                    VK_ATTACHMENT_LOAD_OP_CLEAR, // depth clear
                    VK_ATTACHMENT_LOAD_OP_DONT_CARE, // depth store
                    VK_ATTACHMENT_LOAD_OP_CLEAR, // depth clear store
                };
                return aLoads[ usage ];
            }

            VkAttachmentLoadOp UsageToLoadOp(RenderSystem::RENDER_TARGET_READ_ATTACHMENT_USAGE usage)
            {
                static const VkAttachmentLoadOp aLoads[] =
                {
                    VK_ATTACHMENT_LOAD_OP_DONT_CARE, // undefined
                    VK_ATTACHMENT_LOAD_OP_DONT_CARE, // color
                    VK_ATTACHMENT_LOAD_OP_CLEAR, // color clear
                    VK_ATTACHMENT_LOAD_OP_DONT_CARE, // color store
                    VK_ATTACHMENT_LOAD_OP_CLEAR, // color clear store
                    VK_ATTACHMENT_LOAD_OP_DONT_CARE, // depth
                    VK_ATTACHMENT_LOAD_OP_CLEAR, // depth clear
                    VK_ATTACHMENT_LOAD_OP_DONT_CARE, // depth store
                    VK_ATTACHMENT_LOAD_OP_CLEAR, // depth clear store
                };
                return aLoads[ usage ];
            }

            VkAttachmentStoreOp UsageToStoreOp(RenderSystem::RENDER_TARGET_WRITE_ATTACHMENT_USAGE usage)
            {
                static const VkAttachmentStoreOp aStores[] =
                {
                    VK_ATTACHMENT_STORE_OP_DONT_CARE, // undefined
                    VK_ATTACHMENT_STORE_OP_DONT_CARE, // color
                    VK_ATTACHMENT_STORE_OP_DONT_CARE, // color clear
                    VK_ATTACHMENT_STORE_OP_STORE, // color store
                    VK_ATTACHMENT_STORE_OP_STORE, // color clear store
                    VK_ATTACHMENT_STORE_OP_DONT_CARE, // depth
                    VK_ATTACHMENT_STORE_OP_DONT_CARE, // depth clear
                    VK_ATTACHMENT_STORE_OP_STORE, // depth store
                    VK_ATTACHMENT_STORE_OP_STORE, // depth clear store
                };
                return aStores[ usage ];
            }

            VkAttachmentStoreOp UsageToStoreOp(RenderSystem::RENDER_TARGET_READ_ATTACHMENT_USAGE usage)
            {
                static const VkAttachmentStoreOp aStores[] =
                {
                    VK_ATTACHMENT_STORE_OP_DONT_CARE, // undefined
                    VK_ATTACHMENT_STORE_OP_DONT_CARE, // color
                    VK_ATTACHMENT_STORE_OP_DONT_CARE, // color clear
                    VK_ATTACHMENT_STORE_OP_STORE, // color store
                    VK_ATTACHMENT_STORE_OP_STORE, // color clear store
                    VK_ATTACHMENT_STORE_OP_DONT_CARE, // depth
                    VK_ATTACHMENT_STORE_OP_DONT_CARE, // depth clear
                    VK_ATTACHMENT_STORE_OP_STORE, // depth store
                    VK_ATTACHMENT_STORE_OP_STORE, // depth clear store
                };
                return aStores[ usage ];
            }

            VkImageLayout ImageUsageToLayout(VkImageUsageFlags vkFlags)
            {
                const auto imgSampled = vkFlags & VK_IMAGE_USAGE_SAMPLED_BIT;
                const auto inputAttachment = vkFlags & VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
                const auto isReadOnly = imgSampled || inputAttachment;

                if( vkFlags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT )
                {
                    return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                }
                else if( vkFlags & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT )
                {
                    if( isReadOnly )
                        return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
                    return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                }
                else if( vkFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT )
                {
                    return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                }
                else if( vkFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT )
                {
                    return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                }
                else if( isReadOnly )
                {
                    return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                }
                assert(0 && "Invalid image usage flags");
                VKE_LOG_ERR("Usage flags: " << vkFlags << " are invalid.");
                return VK_IMAGE_LAYOUT_UNDEFINED;
            }

            VkImageLayout ImageUsageToInitialLayout(VkImageUsageFlags vkFlags)
            {
                if( vkFlags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT )
                {
                    return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                }
                else if( vkFlags & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT )
                {
                    return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                }
                assert(0 && "Invalid image usage flags");
                VKE_LOG_ERR("Usage flags: " << vkFlags << " are invalid.");
                return VK_IMAGE_LAYOUT_UNDEFINED;
            }

            VkImageLayout ImageUsageToFinalLayout(VkImageUsageFlags vkFlags)
            {
                const auto imgSampled = vkFlags & VK_IMAGE_USAGE_SAMPLED_BIT;
                const auto inputAttachment = vkFlags & VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
                bool isReadOnly = imgSampled || inputAttachment;

                if( vkFlags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT )
                {
                    if( isReadOnly )
                        return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                }
                else if( vkFlags & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT )
                {
                    if( isReadOnly )
                        return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
                    return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                }
                
                assert(0 && "Invalid image usage flags");
                VKE_LOG_ERR("Usage flags: " << vkFlags << " are invalid.");
                return VK_IMAGE_LAYOUT_UNDEFINED;
            }

            VkImageLayout NextAttachmentLayoutRread(VkImageLayout currLayout)
            {
                if( currLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL )
                {
                    return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                }
                if( currLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL )
                {
                    return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
                }
                assert(0 && "Incorrect initial layout for attachment.");
                return VK_IMAGE_LAYOUT_UNDEFINED;
            }

            VkImageLayout NextAttachmentLayoutOptimal(VkImageLayout currLayout)
            {
                if( currLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL )
                {
                    return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                }
                if( currLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL )
                {
                    return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                }
                assert(0 && "Incorrect initial layout for attachment.");
                return VK_IMAGE_LAYOUT_UNDEFINED;
            }

            RenderSystem::TEXTURE_FORMAT ImageFormat(VkFormat vkFormat)
            {
                switch( vkFormat )
                {
                    case VK_FORMAT_B8G8R8A8_UNORM: return RenderSystem::TextureFormats::B8G8R8A8_UNORM;
                    case VK_FORMAT_R8G8B8A8_UNORM: return RenderSystem::TextureFormats::R8G8B8A8_UNORM;
                }
                assert(0 && "Cannot convert VkFormat to RenderSystem format");
                return RenderSystem::TextureFormats::UNDEFINED;
            }

        } // Convert

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