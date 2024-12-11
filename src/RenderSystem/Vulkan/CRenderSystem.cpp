
#if VKE_VULKAN_RENDER_SYSTEM
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

#include "RenderSystem/Managers/CFrameGraphManager.h"
#include "RenderSystem/CFrameGraph.h"

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
            m_pFrameGraphMgr->_Destroy();
            Memory::DestroyObject( &HeapAllocator, &m_pFrameGraphMgr );

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
            VKE_LOG_PROG( "VKEngine render system creating" );
            VKE_RETURN_IF_FAILED(_AllocMemory(&m_Desc));
            VKE_RETURN_IF_FAILED(_InitAPI());
            //VKE_RETURN_IF_FAILED(_CreateDevices());
            VKE_RETURN_IF_FAILED( Memory::CreateObject( &HeapAllocator, &m_pFrameGraphMgr ) );
            SFrameGraphManagerDesc FrameGraphMgrDesc;
            VKE_RETURN_IF_FAILED( m_pFrameGraphMgr->_Create( FrameGraphMgrDesc ) );
            
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
            VKE_LOG_PROG( "VKEngine free list pool manager created" );
            //auto& Mem = pInfoOut->Memory;

            /// @todo use freelists
            /*for (uint32_t i = 0; i < RenderSystem::ResourceTypes::_MAX_COUNT; ++i)
            {
                VKE_RETURN_IF_FAILED(_CreateFreeListMemory(i, &Mem.aResourceTypes[i], g_aRSResourceTypeDefaultSizes[i],
                    g_aRSResourceTypeSizes[i]));
            }*/

            return VKE_OK;
        }



        Result CRenderSystem::_InitAPI()
        {
            VKE_LOG_PROG( "VKEngine API initialization" );
            assert(m_pPrivate);
            SDDILoadInfo LoadInfo;
            const auto& EngineInfo = m_pEngine->GetInfo();
            
            LoadInfo.AppInfo.engineVersion = EngineInfo.version;
            LoadInfo.AppInfo.pEngineName = EngineInfo.pName;
            LoadInfo.AppInfo.applicationVersion = EngineInfo.applicationVersion;
            LoadInfo.AppInfo.pApplicationName = EngineInfo.pApplicationName;
            LoadInfo.enableDebugMode = m_Desc.debugMode;
            Result ret = CDDI::LoadICD( LoadInfo, &m_DriverData );
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
      

            return ret;
        }

        const CRenderSystem::AdapterVec& CRenderSystem::GetAdapters() const
        {
            return m_vAdapterInfos;
        }

 
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
                //m_vpDevices[ i ]->RenderFrame( pWnd );
            }
            if( GetFrameGraph() )
            {
                GetFrameGraph()->Run();
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

        CFrameGraph* CRenderSystem::CreateFrameGraph( const SFrameGraphDesc& Desc)
        {
            return m_pFrameGraphMgr->CreateFrameGraph( Desc );
        }

        CFrameGraph* CRenderSystem::GetFrameGraph()
        {
            return m_pFrameGraphMgr->GetFrameGraph();
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
#endif // VKE_VULKAN_RENDER_SYSTEM