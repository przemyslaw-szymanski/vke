#include "RenderSystem/CRenderSystem.h"

#include "CVkEngine.h"

#include "RenderSystem/CContext.h"
#include "RenderSystem/CDevice.h"
#include "RenderSystem/CPipeline.h"
#include "RenderSystem/CImage.h"
#include "RenderSystem/CVertexBuffer.h"
#include "RenderSystem/CIndexBuffer.h"
#include "RenderSystem/CSampler.h"
#include "RenderSystem/CSwapChain.h"

#include "Memory/TCFreeListManager.h"
#include "Memory/CFreeListPool.h"
#include "Memory/TCFreeList.h"

#include "Platform/CPlatform.h"
#include "Platform/CWindow.h"

#include "Utils/CLogger.h"
#include "Memory/Memory.h"

#include "Vulkan.h"

namespace VKE
{
    struct SRSInternal
    {
        using PhysicalDeviceVec = vke_vector< VkPhysicalDevice >;
        handle_t hAPILibrary = NULL_HANDLE;
        CRenderSystem::AdapterVec vAdapters;

        struct  
        {
        } Objects;

        struct
        {
            VkApplicationInfo AppInfo;
            PhysicalDeviceVec vPhysicalDevices;
        } Vulkan;

        struct
        {
            ICD::Global Global;
        } ICD;
    };

    static uint16_t g_aRSResourceTypeSizes[ RenderSystem::ResourceTypes::_MAX_COUNT ];
    static uint16_t g_aRSResourceTypeDefaultSies[ RenderSystem::ResourceTypes::_MAX_COUNT];
    void SetResourceTypes();

    Result GetPhysicalDevices(VkInstance vkInstance, const ICD::Instance& Instance,
                              SRSInternal::PhysicalDeviceVec* pVecOut, CRenderSystem::AdapterVec* pAdaptersOut);

    CRenderSystem::CRenderSystem(CVkEngine* pEngine) :
        m_pEngine(pEngine)
    {
        SetResourceTypes();
    }

    CRenderSystem::~CRenderSystem()
    {
        Destroy();
    }

    void CRenderSystem::Destroy()
    {

        for(auto& pCtx : m_vpContexts)
        {
            Memory::DestroyObject(&Memory::CHeapAllocator::GetInstance(), &pCtx);
        }


        for(auto& pList : m_vpFreeLists)
        {
            pList->Destroy();
        }
        m_vpFreeLists.clear();

        //SInternal* pInternal = reinterpret_cast<SInternal*>(m_pInternal);
        Platform::CloseDynamicLibrary(m_pInternal->hAPILibrary);
        VKE_DELETE(m_pInternal);
        m_pInternal = nullptr;
    }

    Result CRenderSystem::Create(const SRenderSystemInfo& Info)
    {
        m_Info = Info;
        m_pInternal = VKE_NEW SRSInternal;

        VKE_RETURN_IF_FAILED(_AllocMemory(&m_Info));
        VKE_RETURN_IF_FAILED(_InitAPI());

        return VKE_OK;
    }

    const void* CRenderSystem::_GetGlobalFunctions() const
    {
        return &m_pInternal->ICD.Global;
    }

    Result CRenderSystem::_CreateFreeListMemory(uint32_t id, uint16_t* pElemCountOut, uint16_t defaultElemCount,
                                               size_t memSize)
    {
        auto* pFreeList = m_vpFreeLists[id];
        if(*pElemCountOut == UNDEFINED)
            *pElemCountOut = defaultElemCount;
        VKE_RETURN_IF_FAILED(pFreeList->Create(*pElemCountOut, memSize, 1));
        return VKE_OK;
    }

    Result CRenderSystem::_AllocMemory(SRenderSystemInfo* pInfoOut)
    {
        VKE_STL_TRY(m_vpFreeLists.reserve(RenderSystem::ResourceTypes::_MAX_COUNT), VKE_ENOMEMORY);
        auto& FreeListMgr = Memory::CFreeListManager::GetSingleton();
        for(size_t i = 0; i < RenderSystem::ResourceTypes::_MAX_COUNT; ++i)
        {
            auto* pPtr = FreeListMgr.CreatePool(RANDOM_HANDLE, false);
            m_vpFreeLists.push_back(pPtr);
        }

        auto& Mem = pInfoOut->Memory;

        for(uint32_t i = 0; i < RenderSystem::ResourceTypes::_MAX_COUNT; ++i)
        {
            VKE_RETURN_IF_FAILED(_CreateFreeListMemory(i, &Mem.aResourceTypes[i], g_aRSResourceTypeDefaultSies[i],
                g_aRSResourceTypeSizes[i]));
        }

        return VKE_OK;
    }

    Result CRenderSystem::_InitAPI()
    {
        assert(m_pInternal);

        auto& Vk = m_pInternal->Vulkan;
       
        m_pInternal->hAPILibrary = Platform::LoadDynamicLibrary(Vulkan::g_pVulkanLibName);
        if(!m_pInternal->hAPILibrary)
        {
            VKE_LOG_ERR("Unable to load library: " << Vulkan::g_pVulkanLibName);
        }

        VKE_RETURN_IF_FAILED(Vulkan::LoadGlobalFunctions(m_pInternal->hAPILibrary, &m_pInternal->ICD.Global));
        const auto& Global = *reinterpret_cast< const ICD::Global* >(_GetGlobalFunctions());

        Vk.AppInfo.apiVersion = VK_API_VERSION;
        Vk.AppInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        Vk.AppInfo.pNext = nullptr;

        VkInstanceCreateInfo InstInfo;
        Vulkan::InitInfo(&InstInfo, VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO);
        InstInfo.enabledExtensionCount = 0;
        InstInfo.enabledLayerCount = 0;
        InstInfo.flags = 0;
        InstInfo.pApplicationInfo = &Vk.AppInfo;
        InstInfo.ppEnabledExtensionNames = nullptr;
        InstInfo.ppEnabledLayerNames = nullptr;

        VkInstance vkInstance;
        VK_ERR(Global.vkCreateInstance(&InstInfo, nullptr, &vkInstance));

        ICD::Instance InstanceFunctions;
        VKE_RETURN_IF_FAILED(Vulkan::LoadInstanceFunctions(vkInstance, Global, &InstanceFunctions));

        VKE_RETURN_IF_FAILED(GetPhysicalDevices(vkInstance, InstanceFunctions, &Vk.vPhysicalDevices,
            &m_pInternal->vAdapters));
    
        //VK_DESTROY(InstanceFunctions.vkDestroyInstance, vkInstance, nullptr);
        InstanceFunctions.vkDestroyInstance(vkInstance, nullptr);

        // For each physical device create context
        if(m_Info.Contexts.count > 0 && m_Info.Contexts.pData)
        {
            for(uint32_t i = 0; i < m_Info.Contexts.count; ++i)
            {
                auto* pCtx = CreateContext(m_Info.Contexts.pData[i]);
                if(!pCtx)
                {
                    return VKE_FAIL;
                }
            }
        }
        else
        {
            // Create one context per one window

            for(size_t i = 0; i < Vk.vPhysicalDevices.size(); ++i)
            {
                RenderSystem::SContextInfo Info;
                auto* pCtx = CreateContext(Info);
                if(!pCtx)
                {
                    return VKE_FAIL;
                }
            }
        }

        return VKE_OK;
    }

    RenderSystem::CContext* CRenderSystem::CreateContext(const RenderSystem::SContextInfo& Info)
    {
        RenderSystem::CContext* pCtx;
        if(VKE_FAILED(Memory::CreateObject(&HeapAllocator, &pCtx, this)))
        {
            return nullptr;
        }
        if(VKE_FAILED(pCtx->Create(Info)))
        {
            Memory::DestroyObject(&Memory::CHeapAllocator::GetInstance(), &pCtx);
            return nullptr;
        }
        m_vpContexts.push_back(pCtx);
        return pCtx;
    }

    const CRenderSystem::AdapterVec& CRenderSystem::GetAdapters() const
    {
        return m_pInternal->vAdapters;
    }

    Result GetPhysicalDevices(VkInstance vkInstance, const ICD::Instance& Instance,
                              SRSInternal::PhysicalDeviceVec* pVecOut, CRenderSystem::AdapterVec* pAdaptersOut)
    {
        uint32_t deviceCount = 0;
        // Get device count
        VK_ERR(Instance.vkEnumeratePhysicalDevices(vkInstance, &deviceCount, nullptr));
        // For now engine supports up to 10 devices
        deviceCount = Min(deviceCount, Constants::RenderSystem::MAX_PHYSICAL_DEVICES);
        
        auto& vPhysicalDevices = *pVecOut;
        auto& vAdapters = *pAdaptersOut;

        vPhysicalDevices.resize(deviceCount);

        // Enum devices
        VK_ERR(Instance.vkEnumeratePhysicalDevices(vkInstance, &deviceCount, &vPhysicalDevices[0]));

        const uint32_t nameLen = Min(VK_MAX_PHYSICAL_DEVICE_NAME_SIZE, Constants::MAX_NAME_LENGTH);

        for(size_t i = 0; i < vPhysicalDevices.size(); ++i)
        {
            VkPhysicalDeviceProperties Props;
            Instance.vkGetPhysicalDeviceProperties(vPhysicalDevices[i], &Props);
            RenderSystem::SAdapterInfo Info = {};
            Info.apiVersion = Props.apiVersion;
            Info.deviceID = Props.deviceID;
            Info.driverVersion = Props.driverVersion;
            Info.type = static_cast<RenderSystem::ADAPTER_TYPE>(Props.deviceType);
            Info.vendorID = Props.vendorID;
            Info.handle = reinterpret_cast<handle_t>(vPhysicalDevices[i]);
            memcpy(Info.name, Props.deviceName, nameLen);
            vAdapters.push_back(Info);
        }

        return VKE_OK;
    }

    void CRenderSystem::RenderFrame(const WindowPtr pWnd)
    {
        for(auto& pCtx : m_vpContexts)
        {
            pCtx->RenderFrame(pWnd->GetSwapChainHandle());
        }
    }

    // GLOBALS
    void SetResourceTypes()
    {
        using RenderSystem::ResourceTypes;
        g_aRSResourceTypeSizes[ResourceTypes::CONSTANT_BUFFER] = 1; // sizeof(CConstantBuffer);
        g_aRSResourceTypeSizes[ResourceTypes::IMAGE] = sizeof(RenderSystem::CImage);
        g_aRSResourceTypeSizes[ResourceTypes::INDEX_BUFFER] = sizeof(RenderSystem::CIndexBuffer);
        g_aRSResourceTypeSizes[ResourceTypes::PIPELINE] = sizeof(RenderSystem::CPipeline);
        g_aRSResourceTypeSizes[ResourceTypes::SAMPLER] = sizeof(RenderSystem::CSampler);
        g_aRSResourceTypeSizes[ResourceTypes::VERTEX_BUFFER] = sizeof(RenderSystem::CVertexBuffer);
        g_aRSResourceTypeSizes[ResourceTypes::VERTEX_SHADER] = 1; // sizeof(CConstantBuffer);
        g_aRSResourceTypeSizes[ResourceTypes::HULL_SHADER] = 1; // sizeof(CConstantBuffer);
        g_aRSResourceTypeSizes[ResourceTypes::DOMAIN_SHADER] = 1; // sizeof(CConstantBuffer);
        g_aRSResourceTypeSizes[ResourceTypes::GEOMETRY_SHADER] = 1; // sizeof(CConstantBuffer);
        g_aRSResourceTypeSizes[ResourceTypes::PIXEL_SHADER] = 1; // sizeof(CConstantBuffer);
        g_aRSResourceTypeSizes[ResourceTypes::COMPUTE_SHADER] = 1; // sizeof(CConstantBuffer);

        g_aRSResourceTypeDefaultSies[ResourceTypes::CONSTANT_BUFFER] = 64;
        g_aRSResourceTypeDefaultSies[ResourceTypes::IMAGE] = 64;
        g_aRSResourceTypeDefaultSies[ResourceTypes::INDEX_BUFFER] = 64;
        g_aRSResourceTypeDefaultSies[ResourceTypes::PIPELINE] = 64;
        g_aRSResourceTypeDefaultSies[ResourceTypes::SAMPLER] = 64;
        g_aRSResourceTypeDefaultSies[ResourceTypes::VERTEX_BUFFER] = 64;
        g_aRSResourceTypeDefaultSies[ResourceTypes::VERTEX_SHADER] = 64;
        g_aRSResourceTypeDefaultSies[ResourceTypes::HULL_SHADER] = 64;
        g_aRSResourceTypeDefaultSies[ResourceTypes::DOMAIN_SHADER] = 64;
        g_aRSResourceTypeDefaultSies[ResourceTypes::GEOMETRY_SHADER] = 64;
        g_aRSResourceTypeDefaultSies[ResourceTypes::PIXEL_SHADER] = 64;
        g_aRSResourceTypeDefaultSies[ResourceTypes::COMPUTE_SHADER] = 64;
    }

} // VKE
