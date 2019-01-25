
#if VKE_VULKAN_RENDERER
#include "RenderSystem/Vulkan/CRenderSystem.h"

#include "CVkEngine.h"

#include "RenderSystem/CGraphicsContext.h"
#include "RenderSystem/CDeviceContext.h"
#include "RenderSystem/Vulkan/CSampler.h"
#include "RenderSystem/Vulkan/CSwapChain.h"
#include "RenderSystem/Resources/CFramebuffer.h"

#include "Core/Memory/TCFreeListManager.h"
#include "Core/Memory/CFreeListPool.h"
#include "Core/Memory/TCFreeList.h"

#include "Core/Platform/CPlatform.h"
#include "Core/Platform/CWindow.h"

#include "Core/Utils/CLogger.h"
#include "Core/Memory/Memory.h"

#include "RenderSystem/Vulkan/Vulkan.h"
#include "RenderSystem/CDDI.h"

namespace VKE
{
    namespace RenderSystem
    {
        struct SCurrContext
        {
            CGraphicsContext*   pCtx = nullptr;
            std::atomic_int     locked;

            SCurrContext()
            {
                locked.store(false);
            }
        };

        struct SPrivateToDeviceCtx
        {
            Vulkan::ICD::Instance&   ICD;
            SPrivateToDeviceCtx(Vulkan::ICD::Instance& I) : ICD(I) {}
            void operator=(const SPrivateToDeviceCtx&) = delete;
        };

        struct SRSInternal
        {
            using PhysicalDeviceVec = vke_vector< VkPhysicalDevice >;
            handle_t hAPILibrary = 0;
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

            Vulkan::ICD::Instance   ICD;

            struct
            {
                std::atomic_int locked;
                CGraphicsContext*       pCtx = nullptr;
            } aCurrCtxs[ContextScopes::_MAX_COUNT];
        };

        static uint16_t g_aRSResourceTypeSizes[RenderSystem::ResourceTypes::_MAX_COUNT];
        static uint16_t g_aRSResourceTypeDefaultSizes[RenderSystem::ResourceTypes::_MAX_COUNT];
        void SetResourceTypes();

        Result GetPhysicalDevices(VkInstance vkInstance, const VkICD::Instance& Instance,
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
            Threads::ScopedLock l(m_SyncObj);
            for (auto& pDevice : m_vpDevices)
            {
                Memory::DestroyObject(&HeapAllocator, &pDevice);
            }


            for (auto& pList : m_vpFreeLists)
            {
                pList->Destroy();
            }
            m_vpFreeLists.clear();

            //SInternal* pInternal = reinterpret_cast<SInternal*>(m_pPrivate);
            if (m_pPrivate)
            {
                //Platform::DynamicLibrary::Close(m_pPrivate->hAPILibrary);
                VKE_DELETE(m_pPrivate);
                m_pPrivate = nullptr;
            }
        }

        Result CRenderSystem::Create(const SRenderSystemDesc& Info)
        {
            //m_Desc = Info;
            Memory::Copy(&m_Desc, sizeof(m_Desc), &Info, sizeof(Info));
            m_pPrivate = VKE_NEW SRSInternal;

            VKE_RETURN_IF_FAILED(_AllocMemory(&m_Desc));
            VKE_RETURN_IF_FAILED(_InitAPI());
            //VKE_RETURN_IF_FAILED(_CreateDevices());

            return VKE_OK;
        }

        /*VkInstance CRenderSystem::_GetInstance() const
        {
            return m_pPrivate->Vulkan.vkInstance;
        }*/

        Result CRenderSystem::_CreateFreeListMemory(uint32_t id, uint16_t* pElemCountOut, uint16_t defaultElemCount,
            size_t memSize)
        {
            auto* pFreeList = m_vpFreeLists[id];
            if (*pElemCountOut == UNDEFINED)
                *pElemCountOut = defaultElemCount;
            VKE_RETURN_IF_FAILED(pFreeList->Create(*pElemCountOut, memSize, 1));
            return VKE_OK;
        }

        Result CRenderSystem::_AllocMemory(SRenderSystemDesc* /*pInfoOut*/)
        {
            VKE_STL_TRY(m_vpFreeLists.reserve(RenderSystem::ResourceTypes::_MAX_COUNT), VKE_ENOMEMORY);
            auto& FreeListMgr = Memory::CFreeListManager::GetSingleton();
            for (size_t i = 0; i < RenderSystem::ResourceTypes::_MAX_COUNT; ++i)
            {
                auto* pPtr = FreeListMgr.CreatePool(RANDOM_HANDLE, false);
                m_vpFreeLists.push_back(pPtr);
            }

            //auto& Mem = pInfoOut->Memory;

            /// @todo use freelists
            /*for (uint32_t i = 0; i < RenderSystem::ResourceTypes::_MAX_COUNT; ++i)
            {
                VKE_RETURN_IF_FAILED(_CreateFreeListMemory(i, &Mem.aResourceTypes[i], g_aRSResourceTypeDefaultSizes[i],
                    g_aRSResourceTypeSizes[i]));
            }*/

            return VKE_OK;
        }

//        Result GetInstanceValidationLayers(bool bEnable, const VkICD::Global& Global,
//            vke_vector<VkLayerProperties>* pvProps, CStrVec* pvNames)
//        {
//            if (!bEnable)
//                return VKE_OK;
//
//            static const char* apNames[] =
//            {
//                "VK_LAYER_LUNARG_parameter_validation",
//                "VK_LAYER_LUNARG_device_limits",
//                "VK_LAYER_LUNARG_object_tracker",
//                "VK_LAYER_LUNARG_core_validation",
//                "VK_LAYER_LUNARG_swapchain"
//            };
//            /*vNames.push_back("VK_LAYER_GOOGLE_threading");
//            vNames.push_back("VK_LAYER_LUNARG_parameter_validation");
//            vNames.push_back("VK_LAYER_LUNARG_device_limits");
//            vNames.push_back("VK_LAYER_LUNARG_object_tracker");
//            vNames.push_back("VK_LAYER_LUNARG_image");
//            vNames.push_back("VK_LAYER_LUNARG_core_validation");
//            vNames.push_back("VK_LAYER_LUNARG_swapchain");
//            vNames.push_back("VK_LAYER_GOOGLE_unique_objects");*/
//
//            uint32_t count = 0;
//            auto& vProps = *pvProps;
//            VK_ERR(Global.vkEnumerateInstanceLayerProperties(&count, nullptr));
//            vProps.resize(count);
//            VK_ERR(Global.vkEnumerateInstanceLayerProperties(&count, &vProps[0]));
//
//            for (uint32_t i = 0; i < ARRAYSIZE(apNames); ++i)
//            {
//                auto pName = apNames[i];
//                for (auto& Prop : vProps)
//                {
//                    if (strcmp(Prop.layerName, pName) == 0)
//                    {
//                        pvNames->push_back(pName);
//                    }
//                }
//            }
//            return VKE_OK;
//        }
//
//        CStrVec GetInstanceExtensionNames(const VkICD::Global& Global)
//        {
//            CStrVec vNames;
//            vke_vector< VkExtensionProperties > vProps;
//            uint32_t count = 0;
//            VK_ERR(Global.vkEnumerateInstanceExtensionProperties("", &count, nullptr));
//            vProps.resize(count);
//            VK_ERR(Global.vkEnumerateInstanceExtensionProperties("", &count, &vProps[0]));
//
//            // Required extensions
//            vNames.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
//            vNames.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
//#if VKE_WINDOWS
//            vNames.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
//#elif VKE_LINUX
//            vNames.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
//#elif VKE_ANDROID
//            vNames.push_back(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME);
//#endif
//
//            // Check extension availability
//            for (auto& pExt : vNames)
//            {
//                bool bAvailable = false;
//                for (auto& Prop : vProps)
//                {
//                    if (strcmp(Prop.extensionName, pExt) == 0)
//                    {
//                        bAvailable = true;
//                        break;
//                    }
//                }
//                if (!bAvailable)
//                {
//                    VKE_LOG_ERR("There is no required extension: " << pExt << " supported by this GPU");
//                    vNames.clear();
//                    return vNames;
//                }
//            }
//            return vNames;
//        }
//
//        Result EnableInstanceExtensions(bool bEnable)
//        {
//            (void)bEnable;
//            return VKE_OK;
//        }
//
//        Result EnableInstanceLayers(bool bEnable)
//        {
//            (void)bEnable;
//            return VKE_OK;
//        }

        Result CRenderSystem::_InitAPI()
        {
            assert(m_pPrivate);
            const auto& EngineInfo = m_pEngine->GetInfo();
            SDDILoadInfo LoadInfo;
            LoadInfo.AppInfo.engineVersion = EngineInfo.version;
            LoadInfo.AppInfo.pEngineName = EngineInfo.pName;
            LoadInfo.AppInfo.applicationVersion = EngineInfo.applicationVersion;
            LoadInfo.AppInfo.pApplicationName = EngineInfo.pApplicationName;
            Result ret = CDDI::LoadICD( LoadInfo );
            if( VKE_SUCCEEDED( ret ) )
            {
                ret = CDDI::QueryAdapters( &m_vAdapterInfos );
                if( VKE_SUCCEEDED( ret ) )
                {

                }
                else
                {

                }
            }
            else
            {

            }
            /*auto& Vk = m_pPrivate->Vulkan;

            m_pPrivate->hAPILibrary = Platform::DynamicLibrary::Load(Vulkan::g_pVulkanLibName);
            if (!m_pPrivate->hAPILibrary)
            {
                VKE_LOG_ERR("Unable to load library: " << Vulkan::g_pVulkanLibName);
                return VKE_FAIL;
            }

            VKE_RETURN_IF_FAILED(Vulkan::LoadGlobalFunctions(m_pPrivate->hAPILibrary, &m_pPrivate->ICD.Global));
            const auto& Global = m_pPrivate->ICD.Global;

            const auto& EngineInfo = m_pEngine->GetInfo();

            bool bEnabled = true;
            auto vExtNames = GetInstanceExtensionNames(Global);
            if (vExtNames.empty())
            {
                return VKE_FAIL;
            }

            CStrVec vLayerNames;
            vke_vector< VkLayerProperties > vLayerProps;
            VKE_RETURN_IF_FAILED(GetInstanceValidationLayers(bEnabled, Global,
                &vLayerProps, &vLayerNames));

            Vk.AppInfo.apiVersion = VK_API_VERSION_1_0;
            Vk.AppInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
            Vk.AppInfo.pNext = nullptr;
            Vk.AppInfo.applicationVersion = EngineInfo.applicationVersion;
            Vk.AppInfo.engineVersion = EngineInfo.version;
            Vk.AppInfo.pApplicationName = EngineInfo.pApplicationName;
            Vk.AppInfo.pEngineName = EngineInfo.pName;

            VkInstanceCreateInfo InstInfo;
            Vulkan::InitInfo(&InstInfo, VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO);
            InstInfo.enabledExtensionCount = static_cast<uint32_t>(vExtNames.size());
            InstInfo.enabledLayerCount = static_cast<uint32_t>(vLayerNames.size());
            InstInfo.flags = 0;
            InstInfo.pApplicationInfo = &Vk.AppInfo;
            InstInfo.ppEnabledExtensionNames = vExtNames.data();
            InstInfo.ppEnabledLayerNames = vLayerNames.data();

            VK_ERR(Global.vkCreateInstance(&InstInfo, nullptr, &m_pPrivate->Vulkan.vkInstance));
            auto vkInstance = m_pPrivate->Vulkan.vkInstance;

            VKE_RETURN_IF_FAILED(Vulkan::LoadInstanceFunctions(vkInstance, Global, &m_pPrivate->ICD.Instance));

            VKE_RETURN_IF_FAILED(GetPhysicalDevices(vkInstance, m_pPrivate->ICD.Instance, &Vk.vPhysicalDevices,
                &m_pPrivate->vAdapters));*/


            return VKE_OK;
        }

        const CRenderSystem::AdapterVec& CRenderSystem::GetAdapters() const
        {
            return m_vAdapterInfos;
        }

        //Result GetPhysicalDevices(VkInstance vkInstance, const VkICD::Instance& Instance,
        //    SRSInternal::PhysicalDeviceVec* pVecOut, CRenderSystem::AdapterVec* pAdaptersOut)
        //{
        //    uint32_t deviceCount = 0;
        //    // Get device count
        //    VK_ERR(Instance.vkEnumeratePhysicalDevices(vkInstance, &deviceCount, nullptr));
        //    if (deviceCount == 0)
        //    {
        //        VKE_LOG_ERR("No physical device available for this machine");
        //        VKE_LOG_ERR("Vulkan is not supported for this GPU");
        //        return VKE_FAIL;
        //    }
        //    // For now engine supports up to 10 devices
        //    //deviceCount = Min(deviceCount, Constants::RenderSystem::MAX_PHYSICAL_DEVICES);

        //    auto& vPhysicalDevices = *pVecOut;
        //    auto& vAdapters = *pAdaptersOut;

        //    vPhysicalDevices.resize(deviceCount);

        //    // Enum devices
        //    VK_ERR(Instance.vkEnumeratePhysicalDevices(vkInstance, &deviceCount, &vPhysicalDevices[0]));

        //    const uint32_t nameLen = Min(VK_MAX_PHYSICAL_DEVICE_NAME_SIZE, Constants::MAX_NAME_LENGTH);

        //    for (size_t i = 0; i < vPhysicalDevices.size(); ++i)
        //    {
        //        const auto& vkPhysicalDevice = vPhysicalDevices[i];

        //        VkPhysicalDeviceProperties Props;
        //        Instance.vkGetPhysicalDeviceProperties(vkPhysicalDevice, &Props);
        //        RenderSystem::SAdapterInfo Info = {};
        //        Info.apiVersion = Props.apiVersion;
        //        Info.deviceID = Props.deviceID;
        //        Info.driverVersion = Props.driverVersion;
        //        Info.type = static_cast<RenderSystem::ADAPTER_TYPE>(Props.deviceType);
        //        Info.vendorID = Props.vendorID;
        //        Info.handle = reinterpret_cast<handle_t>(vkPhysicalDevice);
        //        Memory::Copy(Info.name, sizeof(Info.name), Props.deviceName, nameLen);

        //        vAdapters.PushBack(Info);
        //    }

        //    return VKE_OK;
        //}

        CDeviceContext* CRenderSystem::CreateDeviceContext(const SDeviceContextDesc& Desc)
        {
            RenderSystem::CDeviceContext* pCtx;
            if( VKE_FAILED( Memory::CreateObject( &HeapAllocator, &pCtx, this ) ) )
            {
                VKE_LOG_ERR( "Unable to create CDeviceContext object. No memory." );
                return nullptr;
            }

            SDeviceContextDesc CtxDesc = Desc;

            if( VKE_FAILED( pCtx->Create( CtxDesc ) ) )
            {
                Memory::DestroyObject( &HeapAllocator, &pCtx );
                return nullptr;
            }
            m_vpDevices.PushBack( pCtx );
            return pCtx;
        }

        void CRenderSystem::DestroyDeviceContext(CDeviceContext** ppOut)
        {
            assert(ppOut);
            auto idx = m_vpDevices.Find(*ppOut);
            CDeviceContext* pCtx = m_vpDevices[ idx ];
            assert(pCtx);
            pCtx->_Destroy();
            Memory::DestroyObject(&HeapAllocator, &pCtx);
            m_vpDevices.Remove(idx);
            *ppOut = nullptr;

            if( m_vpDevices.IsEmpty() )
            {
                m_pEngine->StopRendering();
            }
        }

        Result CRenderSystem::MakeCurrent(RenderSystem::CGraphicsContext* pCtx, CONTEXT_SCOPE scope)
        {
            auto& Ctx = m_pPrivate->aCurrCtxs[scope];
            if (pCtx)
            {
                if (Ctx.locked.load() != VKE_TRUE)
                {
                    Ctx.locked.store(VKE_FALSE);
                    Ctx.pCtx = pCtx;
                    return VKE_OK;
                }
                return VKE_FAIL;
            }
            else
            {
                Ctx.locked.store(false);
                if (scope != ContextScopes::ALL)
                {
                    Ctx.pCtx = nullptr;
                }
            }
            return VKE_FAIL;
        }

        CGraphicsContext* CRenderSystem::GetCurrentContext(CONTEXT_SCOPE scope)
        {
            return m_pPrivate->aCurrCtxs[scope].pCtx;
        }

        void CRenderSystem::RenderFrame(const WindowPtr pWnd)
        {
            Threads::ScopedLock l( m_SyncObj );
            for( uint32_t i = 0; i < m_vpDevices.GetCount(); ++i )
            {
                m_vpDevices[ i ]->RenderFrame( pWnd );
            }
        }

        handle_t CRenderSystem::CreateFramebuffer(const RenderSystem::SFramebufferDesc& /*Info*/)
        {
            assert(m_pPrivate->aCurrCtxs[ContextScopes::FRAMEBUFFER].pCtx);
            //return m_pPrivate->aCurrCtxs[ContextScopes::FRAMEBUFFER].pCtx->CreateFramebuffer(Info);
            return 0;
        }

        VkInstance CRenderSystem::_GetVkInstance() const
        {
            return m_pPrivate->Vulkan.vkInstance;
        }

        // GLOBALS
        void SetResourceTypes()
        {
            using RenderSystem::ResourceTypes;
            g_aRSResourceTypeSizes[ResourceTypes::CONSTANT_BUFFER] = 1; // sizeof(CConstantBuffer);
            g_aRSResourceTypeSizes[ResourceTypes::TEXTURE] = sizeof(uint32_t);
            g_aRSResourceTypeSizes[ResourceTypes::INDEX_BUFFER] = sizeof(uint32_t);
            g_aRSResourceTypeSizes[ResourceTypes::PIPELINE] = sizeof(uint32_t);
            g_aRSResourceTypeSizes[ResourceTypes::SAMPLER] = sizeof(CSampler);
            g_aRSResourceTypeSizes[ResourceTypes::VERTEX_BUFFER] = sizeof(uint32_t);
            g_aRSResourceTypeSizes[ResourceTypes::VERTEX_SHADER] = 1; // sizeof(CConstantBuffer);
            g_aRSResourceTypeSizes[ResourceTypes::HULL_SHADER] = 1; // sizeof(CConstantBuffer);
            g_aRSResourceTypeSizes[ResourceTypes::DOMAIN_SHADER] = 1; // sizeof(CConstantBuffer);
            g_aRSResourceTypeSizes[ResourceTypes::GEOMETRY_SHADER] = 1; // sizeof(CConstantBuffer);
            g_aRSResourceTypeSizes[ResourceTypes::PIXEL_SHADER] = 1; // sizeof(CConstantBuffer);
            g_aRSResourceTypeSizes[ResourceTypes::COMPUTE_SHADER] = 1; // sizeof(CConstantBuffer);
            g_aRSResourceTypeSizes[ResourceTypes::FRAMEBUFFER] = sizeof(CFramebuffer);

            g_aRSResourceTypeDefaultSizes[ResourceTypes::CONSTANT_BUFFER] = 64;
            g_aRSResourceTypeDefaultSizes[ResourceTypes::TEXTURE] = 64;
            g_aRSResourceTypeDefaultSizes[ResourceTypes::INDEX_BUFFER] = 64;
            g_aRSResourceTypeDefaultSizes[ResourceTypes::PIPELINE] = 64;
            g_aRSResourceTypeDefaultSizes[ResourceTypes::SAMPLER] = 64;
            g_aRSResourceTypeDefaultSizes[ResourceTypes::VERTEX_BUFFER] = 64;
            g_aRSResourceTypeDefaultSizes[ResourceTypes::VERTEX_SHADER] = 64;
            g_aRSResourceTypeDefaultSizes[ResourceTypes::HULL_SHADER] = 64;
            g_aRSResourceTypeDefaultSizes[ResourceTypes::DOMAIN_SHADER] = 64;
            g_aRSResourceTypeDefaultSizes[ResourceTypes::GEOMETRY_SHADER] = 64;
            g_aRSResourceTypeDefaultSizes[ResourceTypes::PIXEL_SHADER] = 64;
            g_aRSResourceTypeDefaultSizes[ResourceTypes::COMPUTE_SHADER] = 64;
            g_aRSResourceTypeDefaultSizes[ResourceTypes::FRAMEBUFFER] = 64;
        }
    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER