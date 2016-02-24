#include "Vulkan.h"
#include "Platform/CPlatform.h"
#include "Utils/CLogger.h"

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

#define VKE_EXPORT_FUNC(_name, _handle, _getProcAddr) \
    pOut->_name = (PFN_##_name)(_getProcAddr((_handle), #_name)); \
    if(!pOut->_name) \
            { /*VKE_LOG_ERR("Unable to load function: " << #_name);*/ err = VKE_ENOTFOUND; }

        Result LoadGlobalFunctions(handle_t hLib, ICD::Global* pOut)
        {
            Result err = VKE_OK;

#define VK_EXPORTED_FUNCTION(_name) VKE_EXPORT_FUNC(_name, hLib, Platform::GetProcAddress)
#include "ThirdParty/vulkan/VKEICD.h"
#undef VK_EXPORTED_FUNCTION
#define VKE_ICD_GLOBAL(_name) VKE_EXPORT_FUNC(_name, VK_NULL_HANDLE, pOut->vkGetInstanceProcAddr)
#include "ThirdParty/vulkan/VKEICD.h"
#undef VKE_ICD_GLOBAL
            return err;
        }

        Result LoadInstanceFunctions(VkInstance vkInstance, const ICD::Global& Global,
                                     ICD::Instance* pOut)
        {
            Result err = VKE_OK;
#define VKE_INSTANCE_ICD(_name) VKE_EXPORT_FUNC(_name, vkInstance, Global.vkGetInstanceProcAddr)
#include "ThirdParty/vulkan/VKEICD.h"
#undef VKE_INSTANCE_ICD
            return err;
        }

        Result LoadDeviceFunctions(VkDevice vkDevice, const ICD::Instance& Instance, ICD::Device* pOut)
        {
            Result err = VKE_OK;
#define VKE_DEVICE_ICD(_name) VKE_EXPORT_FUNC(_name, vkDevice, Instance.vkGetDeviceProcAddr)
#include "ThirdParty/vulkan/VKEICD.h"
#undef VKE_DEVICE_ICD
            return err;
        }
    }
}