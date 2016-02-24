#include "RenderSystem/CContext.h"
#include "RenderSystem/CRenderSystem.h"
#include "CDevice.h"
#include "CVkEngine.h"
#include "CSwapChain.h"

#include "Vulkan.h"
#include "Utils/CLogger.h"

#include "Memory/Memory.h"

namespace VKE
{
    namespace RenderSystem
    {
        using DeviceVec = vke_vector< CDevice* >;
        using WndSwapChainMap = vke_vector< CSwapChain* >;

        struct SInternal
        {
            DeviceVec               vDevices;
            WndSwapChainMap         vWndSwapChainMap;
            VkInstance              vkInstance;

            struct  
            {
                const ICD::Global* pGlobal; // retreived from CRenderSystem
                ICD::Instance Instance;
            } ICD;
        };

        CContext::CContext(CRenderSystem* pRS) :
            m_pRenderSystem(pRS)
        {

        }

        CContext::~CContext()
        {
            Destroy();
        }

        void CContext::Destroy()
        {
            for (auto& pDevice : m_pInternal->vDevices)
            {
                Memory::DestroyObject(&Memory::CHeapAllocator::GetInstance(), &pDevice);
            }

            VKE_DELETE(m_pInternal);
        }

        Result CContext::Create(const SContextInfo& Info)
        {
            m_Info = Info;
            m_pInternal = VKE_NEW SInternal;

            m_pInternal->ICD.pGlobal = reinterpret_cast< const ICD::Global* >(m_pRenderSystem->_GetGlobalFunctions());
            const ICD::Global& Global = *m_pInternal->ICD.pGlobal;

            auto* pEngine = m_pRenderSystem->GetEngine();
            const auto& EngineInfo = pEngine->GetInfo();

            static const char* aExts[] = 
            {
                VK_KHR_SURFACE_EXTENSION_NAME,
                VK_KHR_PLATFORM_SURFACE_EXTENSION_NAME
            };

            static const uint32_t extCount = sizeof(aExts) / sizeof(aExts[0]);

            VkApplicationInfo AppInfo;
            Vulkan::InitInfo(&AppInfo, VK_STRUCTURE_TYPE_APPLICATION_INFO);
            AppInfo.apiVersion = VK_API_VERSION;
            AppInfo.applicationVersion = EngineInfo.version;
            AppInfo.engineVersion = Constants::ENGINE_VERSION;
            AppInfo.pApplicationName = EngineInfo.pApplicationName;
            AppInfo.pEngineName = Constants::ENGINE_NAME;

            VkInstanceCreateInfo InstInfo;
            Vulkan::InitInfo(&InstInfo, VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO);
            InstInfo.enabledExtensionCount = extCount;
            InstInfo.enabledLayerCount = 0;
            InstInfo.flags = 0;
            InstInfo.pApplicationInfo = &AppInfo;
            InstInfo.ppEnabledExtensionNames = aExts;
            InstInfo.ppEnabledLayerNames = nullptr;

            VK_ERR(Global.vkCreateInstance(&InstInfo, nullptr, &m_pInternal->vkInstance));
            VKE_RETURN_IF_FAILED(Vulkan::LoadInstanceFunctions(m_pInternal->vkInstance, Global,
                &m_pInternal->ICD.Instance));

            VKE_RETURN_IF_FAILED(_CreateDevices());

            return VKE_OK;
        }

        const void* CContext::_GetGlobalFunctions() const
        {
            return m_pInternal->ICD.pGlobal;
        }

        const void* CContext::_GetInstanceFunctions() const
        {
            return &m_pInternal->ICD.Instance;
        }

        Result CContext::_CreateDevices()
        {
            if(!m_Info.pAdapterInfo)
            {
                m_Info.pAdapterInfo = &m_pRenderSystem->GetAdapters()[0];
            }
            // Create devices
            if(m_Info.Devices.count)
            {
                if(m_Info.Devices.pData)
                {
                    for(uint32_t i = 0; i < m_Info.Devices.count; ++i)
                    {
                        const auto& DeviceInfo = m_Info.Devices.pData[i];
                        VKE_RETURN_IF_FAILED(_CreateDevice(DeviceInfo));
                    }
                }
                else
                {
                    SDeviceInfo DeviceInfo;
                    DeviceInfo.pAdapterInfo = m_Info.pAdapterInfo;
                    for (uint32_t i = 0; i < m_Info.Devices.count; ++i)
                    {
                        VKE_RETURN_IF_FAILED(_CreateDevice(DeviceInfo));
                    }
                }
            }
            return VKE_OK;
        }

        Result CContext::_CreateDevice(const SDeviceInfo& Info)
        {
            CDevice* pDevice;
            VKE_RETURN_IF_FAILED(Memory::CreateObject(&HeapAllocator, &pDevice, this));
            if(VKE_FAILED(pDevice->Create(Info)))
            {
                Memory::DestroyObject(&Memory::CHeapAllocator::GetInstance(), &pDevice);
            }
            m_pInternal->vDevices.push_back(pDevice);
            return VKE_OK;
        }

        void* CContext::_GetInstance() const
        {
            return m_pInternal->vkInstance;
        }

        void CContext::RenderFrame(const handle_t& hSwapChain)
        {
            assert(m_pInternal);
            CSwapChain* pSC = reinterpret_cast<CSwapChain*>(hSwapChain);
            auto pDevice = pSC->GetDevice();
            pDevice->BeginFrame(pSC);
            pDevice->EndFrame(pSC);
            pSC->BeginPresent();
            pDevice->SubmitFrame(pSC);
            pSC->EndPresent();
        }

    } // RenderSystem
} // VKE