#include "CDevice.h"
#include "Vulkan.h"
#include "Core/Utils/CLogger.h"

#include "RenderSystem/Vulkan/CContext.h"
#include "CVKEngine.h"
#include "CSwapChain.h"
#include "RenderSystem/Vulkan/CRenderSystem.h"

#include "Core/Memory/CDefaultAllocator.h"

#include "Core/Platform/CWindow.h"

#include "RenderSystem/Vulkan/CCommandBufferManager.h"
#include "RenderSystem/Vulkan/CCommandBuffer.h"

#include "Internals.h"

namespace VKE
{
    namespace RenderSystem
    {
        struct SDeviceInternal
        {
            CDeviceContext*         pDeviceCtx = nullptr;
            struct
            {
                const ICD::Global* pGlobal; // retreived from context
                const ICD::Instance* pInstnace; // retreived from context
                ICD::Device Device;
            } ICD;

            struct
            {
                VkInstance          vkInstance;
                VkDevice            vkDevice;
                VkPhysicalDevice    vkPhysicalDevice;
            } Vulkan;
        };

        CDevice::CDevice(CRenderSystem* pRS) :
            m_pRenderSystem(pRS)
        {

        }

        CDevice::~CDevice()
        {
            Destroy();
        }

        const ICD::Device& CDevice::GetDeviceFunctions() const
        {
            return m_pInternal->ICD.Device;
        }

        const ICD::Instance& CDevice::GetInstanceFunctions() const
        {
            return *m_pInternal->ICD.pInstnace;
        }

        const ICD::Global& CDevice::GetGlobalFunctions() const
        {
            return *m_pInternal->ICD.pGlobal;
        }

        VkDevice CDevice::GetDevice() const
        {
            return m_pInternal->Vulkan.vkDevice;
        }

        VkInstance CDevice::GetInstance() const
        {
            return m_pInternal->Vulkan.vkInstance;
        }

        uint32_t CDevice::GetQueueIndex(QUEUE_TYPE eType) const
        {
            /// @todo Implement use of more than one queue
            return m_aQueueTypes[eType][0];
        }

        VkQueue CDevice::GetQueue(uint32_t familyIndex) const
        {
            /// @todo Implement use of more than one queue
            return GetQueue(familyIndex, 0);
        }

        VkQueue CDevice::GetQueue(QUEUE_TYPE eType) const
        {
            const auto& index = GetQueueIndex(eType);
            return GetQueue(index);
        }

        VkPhysicalDevice CDevice::GetPhysicalDevice() const
        {
            return m_pInternal->Vulkan.vkPhysicalDevice;
        }

        void CDevice::Destroy()
        {
            Memory::DestroyObject(&HeapAllocator, &m_pCmdBuffMgr);
            auto& Instance = GetInstanceFunctions();
            for (auto& pSwapChain : m_vSwapChains)
            {
                Memory::DestroyObject(&Memory::CHeapAllocator::GetInstance(), &pSwapChain);
            }
            m_vSwapChains.clear();
            auto& VkData = m_pInternal->Vulkan;
            Instance.vkDestroyDevice(VkData.vkDevice, nullptr);
            Memory::DestroyObject(&HeapAllocator, &m_pInternal->pDeviceCtx);
            Memory::DestroyObject(&HeapAllocator, &m_pInternal);
        }

        Result CDevice::Create(const SDeviceInfo& Info)
        {
            m_Info = Info;
            assert(m_pInternal == nullptr);
            VKE_RETURN_IF_FAILED(Memory::CreateObject(&HeapAllocator, &m_pInternal));

            m_pInternal->ICD.pInstnace = reinterpret_cast< const ICD::Instance* >(m_pRenderSystem->_GetInstanceFunctions());
            m_pInternal->ICD.pGlobal = reinterpret_cast< const ICD::Global* >(m_pRenderSystem->_GetGlobalFunctions());

            VKE_RETURN_IF_FAILED(Memory::CreateObject(&HeapAllocator, &m_pCmdBuffMgr, this));
            // By default set the max 32 command buffers
            VKE_RETURN_IF_FAILED(m_pCmdBuffMgr->Create(32));

            auto& Instance = GetInstanceFunctions();

            static cstr_t aExtensions[] =
            {
                VK_KHR_SWAPCHAIN_EXTENSION_NAME
            };

            static const uint32_t extCount = sizeof(aExtensions) / sizeof(aExtensions[0]);

            auto& VkData = m_pInternal->Vulkan;
            VkData.vkPhysicalDevice = reinterpret_cast<VkPhysicalDevice>(Info.pAdapterInfo->handle);

            VKE_RETURN_IF_FAILED(_GetProperties(VkData.vkPhysicalDevice));
            VKE_RETURN_IF_FAILED(_CheckExtensions(VkData.vkPhysicalDevice, aExtensions, extCount));

            std::vector<VkDeviceQueueCreateInfo> vQis;
            for (auto& Family : m_vQueueFamilies)
            {
                if(!Family.vQueues.empty())
                {
                    VkDeviceQueueCreateInfo qi;
                    Vulkan::InitInfo(&qi, VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO);
                    qi.flags = 0;
                    qi.pQueuePriorities = &Family.vPriorities[0];
                    qi.queueFamilyIndex = Family.index;
                    qi.queueCount = static_cast<uint32_t>(Family.vQueues.size());
                    vQis.push_back(qi);
                }
            }

            VkPhysicalDeviceFeatures df = {};

            VkDeviceCreateInfo di;
            Vulkan::InitInfo(&di, VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO);
            di.enabledExtensionCount = extCount;
            di.enabledLayerCount = 0;
            di.pEnabledFeatures = nullptr;
            di.ppEnabledExtensionNames = aExtensions;
            di.ppEnabledLayerNames = nullptr;
            di.pQueueCreateInfos = &vQis[0];
            di.queueCreateInfoCount = static_cast<uint32_t>(vQis.size());
            di.flags = 0;
            
            VK_ERR(Instance.vkCreateDevice(VkData.vkPhysicalDevice, &di, nullptr, &VkData.vkDevice));
            VKE_DEBUG_CODE(m_DeviceInfo = di);
            VKE_RETURN_IF_FAILED(Vulkan::LoadDeviceFunctions(VkData.vkDevice, Instance, &m_pInternal->ICD.Device));
            auto& Device = GetDeviceFunctions();
            //m_pInternal->pDeviceCtx = VKE_NEW CDeviceContext(m_vkDevice, Device);
            VKE_RETURN_IF_FAILED(Memory::CreateObject(&HeapAllocator, &m_pInternal->pDeviceCtx, VkData.vkDevice, Device));

            for (auto& Family : m_vQueueFamilies)
            {
                for (auto& vkQueue : Family.vQueues)
                {
                    Device.vkGetDeviceQueue(VkData.vkDevice, Family.index, 0, &vkQueue);
                }
            }

            VKE_RETURN_IF_FAILED(CreateSwapChain(Info.SwapChain));

            return VKE_OK;
        }

        Result CDevice::CreateSwapChain(const SSwapChainInfo& Info)
        {
            CSwapChain* pSwapChain;
            if(VKE_FAILED(Memory::CreateObject(&HeapAllocator, &pSwapChain, this, m_pInternal->pDeviceCtx)))
            {
                VKE_LOG_ERR("No memory to create swap chain object");
                return VKE_ENOMEMORY;
            }
            Result err;
            if(VKE_FAILED((err = pSwapChain->Create(Info))))
            {
                VKE_DELETE(pSwapChain);
                return err;
            }
            m_vSwapChains.push_back(pSwapChain);
            auto pWnd = GetRenderSystem()->GetEngine()->GetWindow(Info.hWnd);
            pWnd->SetSwapChainHandle(reinterpret_cast<handle_t>(pSwapChain));
            return VKE_OK;
        }

        Result CDevice::_GetProperties(VkPhysicalDevice vkPhysicalDevice)
        {
            auto& Instance = GetInstanceFunctions();
            uint32_t propCount = 0;
            Instance.vkGetPhysicalDeviceQueueFamilyProperties(vkPhysicalDevice, &propCount, nullptr);
            propCount = Min(propCount, Constants::RenderSystem::MAX_PHYSICAL_DEVICES_QUEUE_FAMILY_PROPERTIES);
            if(propCount == 0)
            {
                VKE_LOG_ERR("No device queue family properties");
                return VKE_FAIL;
            }

            VkQueueFamilyProperties aProperties[Constants::RenderSystem::MAX_PHYSICAL_DEVICES_QUEUE_FAMILY_PROPERTIES];
            Instance.vkGetPhysicalDeviceQueueFamilyProperties(vkPhysicalDevice, &propCount, aProperties);
            // Choose a family index
            for(uint32_t i = 0; i < propCount; ++i)
            {
                uint32_t isGraphics = aProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT;
                uint32_t isCompute = aProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT;
                uint32_t isTransfer = aProperties[i].queueFlags & VK_QUEUE_TRANSFER_BIT;
                uint32_t isSparse = aProperties[i].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT;

                SQueueFamily Family;
                Family.vQueues.resize(aProperties[i].queueCount);
                Family.vPriorities.resize(aProperties[i].queueCount, 1.0f);
                Family.index = i;
                Family.isGraphics = true;
                Family.isCompute = isCompute != 0;
                Family.isTransfer = isTransfer != 0;
                Family.isSparse = isSparse != 0;

                if(isGraphics)
                {                                     
                    m_aQueueTypes[QueueTypes::GRAPHICS].push_back(i);
                }
                if(isCompute)
                {
                    m_aQueueTypes[QueueTypes::COMPUTE].push_back(i);
                }

                m_vQueueFamilies.push_back(Family);
            }
            if(m_aQueueTypes[QueueTypes::GRAPHICS].empty())
            {
                VKE_LOG_ERR("Unable to find a graphics queue");
                return VKE_ENOTFOUND;
            }

            Instance.vkGetPhysicalDeviceMemoryProperties(vkPhysicalDevice, &m_vkMemProperty);
            Instance.vkGetPhysicalDeviceFeatures(vkPhysicalDevice, &m_vkFeatures);

            for(uint32_t i = 0; i < RenderSystem::ImageFormats::_MAX_COUNT; ++i)
            {
                const auto& fmt = RenderSystem::g_aFormats[i];
                Instance.vkGetPhysicalDeviceFormatProperties(vkPhysicalDevice, fmt, &m_aFormatsProperties[i]);
            }

            return VKE_OK;
        }

        Result CDevice::_CheckExtensions(VkPhysicalDevice vkPhysicalDevice, cstr_t* ppExtensions,
                                        uint32_t extCount)
        {
            auto& Instance = GetInstanceFunctions();
            uint32_t count = 0;
            VK_ERR(Instance.vkEnumerateDeviceExtensionProperties(vkPhysicalDevice, nullptr, &count, nullptr));
            count = Min(count, Constants::RenderSystem::MAX_EXTENSION_COUNT);
            VkExtensionProperties aProperties[Constants::RenderSystem::MAX_EXTENSION_COUNT];
            VK_ERR(Instance.vkEnumerateDeviceExtensionProperties(vkPhysicalDevice, nullptr, &count, aProperties));
            
            std::string ext;
            Result err = VKE_OK;

            for(uint32_t e = 0; e < extCount; ++e)
            {
                ext = ppExtensions[e];
                bool found = false;
                for(uint32_t p = 0; p < count; ++p)
                {
                    if(ext == aProperties[p].extensionName)
                    {
                        found = true;
                        break;
                    }
                }
                if(!found)
                {
                    VKE_LOG_ERR("Extension: " << ext << " is not supported by the device.");
                    err = VKE_ENOTFOUND;
                }
            }

            return err;
        }

        Result CDevice::BeginFrame(const CSwapChain*)
        {
            return VKE_OK;
        }

        Result CDevice::EndFrame(const CSwapChain*)
        {
            return VKE_OK;
        }

        Result CDevice::SubmitFrame(const CSwapChain* pSC)
        {
            assert(m_pInternal);
            assert(pSC);
            const auto& El = pSC->GetCurrentElement();
            VkSemaphore vkSemaphore = El.vkSemaphore;
            VkQueue vkQueue = GetQueue(QueueTypes::GRAPHICS);
            VkPipelineStageFlags flags = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
            VkSubmitInfo si;
            Vulkan::InitInfo(&si, VK_STRUCTURE_TYPE_SUBMIT_INFO);
            si.commandBufferCount = 1;
            si.pCommandBuffers = nullptr; // todo must be at least one command buffer
            si.pSignalSemaphores = nullptr;
            si.pWaitDstStageMask = &flags;
            si.pWaitSemaphores = &vkSemaphore;
            si.signalSemaphoreCount = 0;
            si.waitSemaphoreCount = 1;
            m_pInternal->ICD.Device.vkQueueSubmit(vkQueue, 1, &si, VK_NULL_HANDLE);
            return VKE_OK;
        }

        CommandBufferPtr CDevice::CreateCommandBuffer()
        {
            assert(m_pCmdBuffMgr);
            CCommandBuffer::SCreateInfo Info;
            //CommandBufferPtr pPtr = m_pCmdBuffMgr->CreateResource(&Info);
            //return pPtr;
            return CommandBufferPtr();
        }

        Result CDevice::DestroyCommandBuffer(CommandBufferPtr* ppOut)
        {
            assert(m_pCmdBuffMgr);
            //return m_pCmdBuffMgr->DestroyResource(ppOut);
            return VKE_FAIL;
        }

    } // RenderSystem
} // VKE