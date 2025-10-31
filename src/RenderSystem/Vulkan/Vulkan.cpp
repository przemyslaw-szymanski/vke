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

        void SetLastError( VkResult err )
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
            Vulkan::InitInfo( &m_PresentInfo, VK_STRUCTURE_TYPE_PRESENT_INFO_KHR );
            m_PresentInfo.pResults = nullptr;
        }

        VkResult SQueue::Submit( const VkICD::Device& ICD, const VkSubmitInfo& Info, const VkFence& vkFence )
        {
            Lock();
            auto res = ICD.vkQueueSubmit( vkQueue, 1, &Info, vkFence );
            Unlock();
            return res;
        }

        bool SQueue::IsPresentDone()
        {
            return m_isPresentDone;
        }

        void SQueue::ReleasePresentNotify()
        {
            Lock();
            if( m_presentCount-- < 0 )
                m_presentCount = 0;
            m_isPresentDone = m_presentCount == 0;
            Unlock();
        }

        void SQueue::Wait( const VkICD::Device& ICD )
        {
            ICD.vkQueueWaitIdle( vkQueue );
        }

        Result SQueue::Present( const VkICD::Device& ICD, uint32_t imgIdx, VkSwapchainKHR vkSwpChain,
                                VkSemaphore vkWaitSemaphore )
        {
            Result res = VKE_ENOTREADY;
            Lock();
            m_PresentData.vImageIndices.PushBack( imgIdx );
            m_PresentData.vSwapChains.PushBack( vkSwpChain );
            m_PresentData.vWaitSemaphores.PushBack( vkWaitSemaphore );
            m_presentCount++;
            m_isPresentDone = false;
            if( this->GetRefCount() == m_PresentData.vSwapChains.GetCount() )
            {
                m_PresentInfo.pImageIndices = &m_PresentData.vImageIndices[ 0 ];
                m_PresentInfo.pSwapchains = &m_PresentData.vSwapChains[ 0 ];
                m_PresentInfo.pWaitSemaphores = &m_PresentData.vWaitSemaphores[ 0 ];
                m_PresentInfo.swapchainCount = m_PresentData.vSwapChains.GetCount();
                m_PresentInfo.waitSemaphoreCount = m_PresentData.vWaitSemaphores.GetCount();
                VK_ERR( ICD.vkQueuePresentKHR( vkQueue, &m_PresentInfo ) );
                // $TID Present: q={vkQueue}, sc={m_PresentInfo.pSwapchains[0]}, imgIdx={m_PresentInfo.pImageIndices[0]}, ws={m_PresentInfo.pWaitSemaphores[0]}
                m_isPresentDone = true;
                m_PresentData.vImageIndices.Clear();
                m_PresentData.vSwapChains.Clear();
                m_PresentData.vWaitSemaphores.Clear();
                res = VKE_OK;
            }
            Unlock();
            return res;
        }

        bool IsColorImage( VkFormat format )
        {
            switch( format )
            {
                case VK_FORMAT_UNDEFINED:
                case VK_FORMAT_D16_UNORM:
                case VK_FORMAT_D16_UNORM_S8_UINT:
                case VK_FORMAT_D24_UNORM_S8_UINT:
                case VK_FORMAT_D32_SFLOAT:
                case VK_FORMAT_D32_SFLOAT_S8_UINT:
                case VK_FORMAT_X8_D24_UNORM_PACK32:
                case VK_FORMAT_S8_UINT:
                return false;
            }
            return true;
        }

        bool IsDepthImage( VkFormat format )
        {
            switch( format )
            {
                case VK_FORMAT_D16_UNORM:
                case VK_FORMAT_D16_UNORM_S8_UINT:
                case VK_FORMAT_D24_UNORM_S8_UINT:
                case VK_FORMAT_D32_SFLOAT:
                case VK_FORMAT_D32_SFLOAT_S8_UINT:
                case VK_FORMAT_X8_D24_UNORM_PACK32:
                case VK_FORMAT_S8_UINT:
                return true;
            }
            return false;
        }

        bool IsStencilImage( VkFormat format )
        {
            switch( format )
            {
                case VK_FORMAT_D16_UNORM_S8_UINT:
                case VK_FORMAT_D24_UNORM_S8_UINT:
                case VK_FORMAT_D32_SFLOAT_S8_UINT:
                case VK_FORMAT_S8_UINT:
                return true;
            }
            return false;
        }

#define VKE_EXPORT_FUNC(_name, _handle, _getProcAddr) \
    pOut->_name = (PFN_##_name)(_getProcAddr((_handle), #_name)); \
    if(!pOut->_name) \
            { VKE_LOG_ERR("Unable to load function: " << #_name); err = VKE_ENOTFOUND; }

#define VKE_EXPORT_EXT_FUNC(_name, _handle, _getProcAddr) \
    pOut->_name = (PFN_##_name)(_getProcAddr((_handle), #_name)); \
    if(!pOut->_name) \
            { VKE_LOG_WARN("Unable to load EXT function: " << #_name); }

        Result LoadGlobalFunctions( handle_t hLib, VkICD::Global* pOut )
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
            pOut->vkGetInstanceProcAddr = reinterpret_cast< PFN_vkGetInstanceProcAddr >( Platform::GetProcAddress( hLib, "vkGetInstanceProcAddr" ) );
            pOut->vkCreateInstance = reinterpret_cast< PFN_vkCreateInstance >( pOut->vkGetInstanceProcAddr( VK_NULL_HANDLE, "vkCreateInstance" ) );
            //pOut->vkDestroyInstance = reinterpret_cast< PFN_vkDestroyInstance >( pOut->vkGetInstanceProcAddr( VK_NULL_HANDLE, "vkDestroyInstance" ) );
            pOut->vkEnumerateInstanceExtensionProperties = reinterpret_cast< PFN_vkEnumerateInstanceExtensionProperties >( pOut->vkGetInstanceProcAddr( VK_NULL_HANDLE, "vkEnumerateInstanceExtensionProperties" ) );
            pOut->vkEnumerateInstanceLayerProperties = reinterpret_cast< PFN_vkEnumerateInstanceLayerProperties >( pOut->vkGetInstanceProcAddr( VK_NULL_HANDLE, "vkEnumerateInstanceLayerProperties" ) );
#endif // VKE_AUTO_ICD
            return err;
        }

        Result LoadInstanceFunctions( VkInstance vkInstance, const VkICD::Global& Global,
                                      VkICD::Instance* pOut )
        {
            Result err = VKE_OK;
#if VKE_AUTO_ICD
#   undef VKE_INSTANCE_ICD
#   undef VKE_INSTANCE_EXT_ICD
#   define VKE_INSTANCE_ICD(_name) VKE_EXPORT_FUNC(_name, vkInstance, Global.vkGetInstanceProcAddr)
#   define VKE_INSTANCE_EXT_ICD(_name) VKE_EXPORT_EXT_FUNC(_name, vkInstance, Global.vkGetInstanceProcAddr)
#       include "ThirdParty/vulkan/VKEICD.h"
#   undef VKE_INSTANCE_ICD
#   undef VKE_INSTANCE_EXT_ICD
#else // VKE_AUTO_ICD
            pOut->vkDestroySurfaceKHR = reinterpret_cast< PFN_vkDestroySurfaceKHR >( Global.vkGetInstanceProcAddr( vkInstance, "vkDestroySurfaceKHR" ) );
#endif // VKE_AUTO_ICD
            return err;
        }

        Result LoadDeviceFunctions( VkDevice vkDevice, const VkICD::Instance& Instance, VkICD::Device* pOut )
        {
            Result err = VKE_OK;
#if VKE_AUTO_ICD
#   undef VKE_DEVICE_ICD
#   undef VKE_DEVICE_EXT_ICD
#   define VKE_DEVICE_ICD(_name) VKE_EXPORT_FUNC(_name, vkDevice, Instance.vkGetDeviceProcAddr)
#   define VKE_DEVICE_EXT_ICD(_name) VKE_EXPORT_EXT_FUNC(_name, vkDevice, Instance.vkGetDeviceProcAddr);
#       include "ThirdParty/vulkan/VKEICD.h"
#   undef VKE_DEVICE_ICD
#   undef VKE_DEVICE_EXT_ICD
#else // VKE_AUTO_ICD

#endif // VKE_AUTO_ICD
            return err;
        }
    }
}