#include "RenderSystem/Vulkan/CRenderSystem.h"

#include "CVkEngine.h"

#include "RenderSystem/Vulkan/CContext.h"
#include "RenderSystem/Vulkan/CDevice.h"
#include "RenderSystem/Vulkan/CPipeline.h"
#include "RenderSystem/Vulkan/CImage.h"
#include "RenderSystem/Vulkan/CVertexBuffer.h"
#include "RenderSystem/Vulkan/CIndexBuffer.h"
#include "RenderSystem/Vulkan/CSampler.h"
#include "RenderSystem/Vulkan/CSwapChain.h"

#include "Core/Memory/TCFreeListManager.h"
#include "Core/Memory/CFreeListPool.h"
#include "Core/Memory/TCFreeList.h"

#include "Core/Platform/CPlatform.h"
#include "Core/Platform/CWindow.h"

#include "Core/Utils/CLogger.h"
#include "Core/Memory/Memory.h"

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
            VkApplicationInfo   AppInfo;
            PhysicalDeviceVec   vPhysicalDevices;
            VkInstance          vkInstance = VK_NULL_HANDLE;
        } Vulkan;

        struct
        {
            ICD::Global Global;
            ICD::Instance Instance;
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

        for(auto& pDevice : m_vpDevices)
        {
            Memory::DestroyObject(&HeapAllocator, &pDevice);
        }


        for(auto& pList : m_vpFreeLists)
        {
            pList->Destroy();
        }
        m_vpFreeLists.clear();

        //SInternal* pInternal = reinterpret_cast<SInternal*>(m_pInternal);
        if (m_pInternal)
        {
            Platform::CloseDynamicLibrary(m_pInternal->hAPILibrary);
            VKE_DELETE(m_pInternal);
            m_pInternal = nullptr;
        }
    }

    Result CRenderSystem::Create(const SRenderSystemInfo& Info)
    {
        m_Info = Info;
        m_pInternal = VKE_NEW SRSInternal;

        VKE_RETURN_IF_FAILED(_AllocMemory(&m_Info));
        VKE_RETURN_IF_FAILED(_InitAPI());
        VKE_RETURN_IF_FAILED(_CreateDevices());

        return VKE_OK;
    }

    handle_t CRenderSystem::_GetInstance() const
    {
        return reinterpret_cast<handle_t>( m_pInternal->Vulkan.vkInstance );
    }

    const void* CRenderSystem::_GetGlobalFunctions() const
    {
        return &m_pInternal->ICD.Global;
    }

    const void* CRenderSystem::_GetInstanceFunctions() const
    {
        return &m_pInternal->ICD.Instance;
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

    Result GetInstanceValidationLayers(bool bEnable, const ICD::Global& Global,
        vke_vector<VkLayerProperties>* pvProps, CStrVec* pvNames)
    {
        if (!bEnable)
            return VKE_OK;

        static const char* apNames[] =
        {
            "VK_LAYER_LUNARG_parameter_validation",
            "VK_LAYER_LUNARG_device_limits",
            "VK_LAYER_LUNARG_object_tracker",
            "VK_LAYER_LUNARG_core_validation",
            "VK_LAYER_LUNARG_swapchain"
        };
        /*vNames.push_back("VK_LAYER_GOOGLE_threading");
        vNames.push_back("VK_LAYER_LUNARG_parameter_validation");
        vNames.push_back("VK_LAYER_LUNARG_device_limits");
        vNames.push_back("VK_LAYER_LUNARG_object_tracker");
        vNames.push_back("VK_LAYER_LUNARG_image");
        vNames.push_back("VK_LAYER_LUNARG_core_validation");
        vNames.push_back("VK_LAYER_LUNARG_swapchain");
        vNames.push_back("VK_LAYER_GOOGLE_unique_objects");*/

        uint32_t count = 0;
        auto& vProps = *pvProps;
        VK_ERR( Global.vkEnumerateInstanceLayerProperties( &count, nullptr ) );
        vProps.resize( count );
        VK_ERR( Global.vkEnumerateInstanceLayerProperties( &count, &vProps[ 0 ] ) );

        for( uint32_t i = 0; i < ARRAYSIZE( apNames ); ++i )
        {
            auto pName = apNames[ i ];
            for( auto& Prop : vProps )
            {
                if( strcmp( Prop.layerName, pName ) == 0 )
                {
                    pvNames->push_back( pName );
                }
            }
        }
        return VKE_OK;
    }

    CStrVec GetInstanceExtensionNames(const ICD::Global& Global)
    {
        CStrVec vNames;
        vke_vector< VkExtensionProperties > vProps;
        uint32_t count = 0;
        VK_ERR( Global.vkEnumerateInstanceExtensionProperties( "", &count, nullptr ) );
        vProps.resize( count );
        VK_ERR( Global.vkEnumerateInstanceExtensionProperties( "", &count, &vProps[ 0 ] ) );

        // Required extensions
        vNames.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
        vNames.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
#if VKE_WINDOWS
        vNames.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#elif VKE_LINUX
        vNames.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
#elif VKE_ANDROID
        vNames.push_back(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME);
#endif

        // Check extension availability
        for( auto& pExt : vNames )
        {
            bool bAvailable = false;
            for( auto& Prop : vProps )
            {
                if( strcmp( Prop.extensionName, pExt ) == 0 )
                {
                    bAvailable = true;
                    break;
                }
            }
            if( !bAvailable )
            {
                VKE_LOG_ERR( "There is no required extension: " << pExt << " supported by this GPU" );
                vNames.clear();
                return vNames;
            }
        }
        return vNames;
    }

    Result EnableInstanceExtensions(bool bEnable)
    {
        return VKE_OK;
    }

    Result EnableInstanceLayers(bool bEnable)
    {
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

        const auto& EngineInfo = m_pEngine->GetInfo();

        bool bEnabled = true;
        auto vExtNames = GetInstanceExtensionNames(Global);
        if( vExtNames.empty() )
        {
            return VKE_FAIL;
        }

        CStrVec vLayerNames;
        vke_vector< VkLayerProperties > vLayerProps;
        VKE_RETURN_IF_FAILED( GetInstanceValidationLayers( bEnabled, Global,
            &vLayerProps, &vLayerNames ) );

        Vk.AppInfo.apiVersion = VK_API_VERSION_1_0;
        Vk.AppInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        Vk.AppInfo.pNext = nullptr;
        Vk.AppInfo.applicationVersion = EngineInfo.applicationVersion;
        Vk.AppInfo.engineVersion = EngineInfo.version;
        Vk.AppInfo.pApplicationName = EngineInfo.pApplicationName;
        Vk.AppInfo.pEngineName = EngineInfo.pName;

        VkInstanceCreateInfo InstInfo;
        Vulkan::InitInfo(&InstInfo, VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO);
        InstInfo.enabledExtensionCount = vExtNames.size();
        InstInfo.enabledLayerCount = vLayerNames.size();
        InstInfo.flags = 0;
        InstInfo.pApplicationInfo = &Vk.AppInfo;
        InstInfo.ppEnabledExtensionNames = vExtNames.data();
        InstInfo.ppEnabledLayerNames = vLayerNames.data();

        VK_ERR(Global.vkCreateInstance(&InstInfo, nullptr, &m_pInternal->Vulkan.vkInstance));
        auto vkInstance = m_pInternal->Vulkan.vkInstance;

        VKE_RETURN_IF_FAILED(Vulkan::LoadInstanceFunctions(vkInstance, Global, &m_pInternal->ICD.Instance));

        VKE_RETURN_IF_FAILED(GetPhysicalDevices(vkInstance, m_pInternal->ICD.Instance, &Vk.vPhysicalDevices,
            &m_pInternal->vAdapters));

        
        return VKE_OK;
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
        if( deviceCount == 0 )
        {
            VKE_LOG_ERR( "No physical device available for this machine" );
            VKE_LOG_ERR( "Vulkan is not supported for this GPU" );
            return VKE_FAIL;
        }
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
            const auto& vkPhysicalDevice = vPhysicalDevices[i];
            
            VkPhysicalDeviceProperties Props;
            Instance.vkGetPhysicalDeviceProperties(vkPhysicalDevice, &Props);
            RenderSystem::SAdapterInfo Info = {};
            Info.apiVersion = Props.apiVersion;
            Info.deviceID = Props.deviceID;
            Info.driverVersion = Props.driverVersion;
            Info.type = static_cast<RenderSystem::ADAPTER_TYPE>(Props.deviceType);
            Info.vendorID = Props.vendorID;
            Info.handle = reinterpret_cast<handle_t>(vkPhysicalDevice);
            Memory::Copy(Info.name, sizeof(Info.name), Props.deviceName, nameLen);
            
            vAdapters.push_back(Info);
        }

        return VKE_OK;
    }

    Result CRenderSystem::_CreateDevices()
    {
        for( auto& AdapterInfo : m_pInternal->vAdapters )
        {
            VKE_RETURN_IF_FAILED( _CreateDevice( AdapterInfo ) );
        }
        return VKE_OK;
    }

    Result CRenderSystem::_CreateDevice(const RenderSystem::SAdapterInfo& Info)
    {
        RenderSystem::CDevice* pDevice;
        VKE_RETURN_IF_FAILED( Memory::CreateObject( &HeapAllocator, &pDevice, this ) );
        RenderSystem::SDeviceInfo DevInfo;
        DevInfo.pAdapterInfo = &Info;

        // For each window create separate context
        // Each context contains zero or more swap chains
        // Each context contains at least one queue
        vke_vector< RenderSystem::SContextInfo > vContextInfos;
        RenderSystem::SSwapChainInfo SwapInfos[128];
        if( m_Info.Windows.count >= 128 )
        {
            VKE_LOG_ERR("Too many windows created.");
            return VKE_FAIL;
        }

        for (uint32_t i = 0; i < m_Info.Windows.count; ++i)
        {
            RenderSystem::SContextInfo CtxInfo;
            CtxInfo.pAdapterInfo = DevInfo.pAdapterInfo;
            CtxInfo.SwapChains.count = 1;
            
            SwapInfos[i].hWnd = m_Info.Windows.pData[i].wndHandle;
            SwapInfos[i].hPlatform = m_Info.Windows.pData[i].platformHandle;
            CtxInfo.SwapChains.pData = &SwapInfos[i];
            vContextInfos.push_back(CtxInfo);
        }

        DevInfo.Contexts.count = vContextInfos.size();
        DevInfo.Contexts.pData = vContextInfos.data();
        DevInfo.hAPIInstance = reinterpret_cast<handle_t>(m_pInternal->Vulkan.vkInstance);
          
        if( VKE_FAILED( pDevice->Create( DevInfo ) ) )
        {
            Memory::DestroyObject( &HeapAllocator, &pDevice );
            return VKE_ENOMEMORY;
        }
        m_vpDevices.push_back( pDevice );
        return VKE_OK;
    }

    void CRenderSystem::RenderFrame(const WindowPtr pWnd)
    {

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
