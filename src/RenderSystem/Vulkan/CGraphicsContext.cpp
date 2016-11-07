#include "RenderSystem/CGraphicsContext.h"
#include "RenderSystem/CRenderSystem.h"
#include "CDevice.h"
#include "CVkEngine.h"
#include "CSwapChain.h"
#include "RenderSystem/CDeviceContext.h"

#include "Vulkan.h"
#include "Core/Utils/CLogger.h"

#include "Core/Memory/Memory.h"

#include "Core/Platform/CWindow.h"

#include "RenderSystem/Vulkan/CGraphicsQueue.h"

#include "Core/Utils/TCDynamicArray.h"

namespace VKE
{
    namespace RenderSystem
    {
        using DeviceVec = vke_vector< CDevice* >;
        using WndSwapChainMap = vke_vector< CSwapChain* >;
        using SwapChainVec = vke_vector< CSwapChain* >;
        using GraphicsQueueVec = vke_vector< CGraphicsQueue* >;
        using HandleBuffer = Utils::TCDynamicArray< handle_t, 256 >;
        

        struct SInternal
        {
            DeviceVec               vDevices;
            WndSwapChainMap         vWndSwapChainMap;
            CSwapChain*				pSwapChain = nullptr;
            GraphicsQueueVec        vpGraphicsQueues;

            VkInstance              vkInstance;

            struct  
            {
                const VkICD::Global* pGlobal; // retreived from CRenderSystem
                const VkICD::Instance* pInstance; // retreived from CRenderSystem
            } VkICD;

            

            bool					needRenderFrame = false;
        };

        CGraphicsContext::CGraphicsContext(CDevice* pDevice) :
            m_pDevice(pDevice)
            //, m_pDeviceCtx(pDevice->_GetDeviceContext())
        {

        }

        CGraphicsContext::~CGraphicsContext()
        {
            Destroy();
        }

        void CGraphicsContext::Destroy()
        {
            for (auto& pQueue : m_pInternal->vpGraphicsQueues)
            {
                Memory::DestroyObject(&HeapAllocator, &pQueue);
            }

            Memory::DestroyObject(&HeapAllocator, &m_pInternal->pSwapChain);
            Memory::DestroyObject(&HeapAllocator, &m_pInternal);
        }

        Result CGraphicsContext::Create(const SGraphicsContextDesc& Info)
        {
            VKE_RETURN_IF_FAILED(Memory::CreateObject(&HeapAllocator, &m_pInternal));
            const auto& SwapChains = Info.SwapChains;
            
            if (SwapChains.count)
            {
                for (uint32_t i = 0; i < SwapChains.count; ++i)
                {
                    VKE_RETURN_IF_FAILED(CreateSwapChain(SwapChains.pData[i]));
                }
            }
            // Create dummy queue
            CreateGraphicsQueue({});
            return VKE_OK;
        }

        Result CGraphicsContext::CreateSwapChain(const SSwapChainInfo& Info)
        {
            CSwapChain* pSwapChain;
            if (VKE_FAILED(Memory::CreateObject(&HeapAllocator, &pSwapChain, this)))
            {
                VKE_LOG_ERR("No memory to create swap chain object");
                return VKE_ENOMEMORY;
            }
            Result err;
            if (VKE_FAILED((err = pSwapChain->Create(Info))))
            {
                Memory::DestroyObject(&HeapAllocator, &pSwapChain);
                return err;
            }

            m_pInternal->pSwapChain = pSwapChain;
            auto pWnd = m_pDevice->GetRenderSystem()->GetEngine()->FindWindow(Info.hWnd);
            //pWnd->SetRenderingContext(this);
            pWnd->AddUpdateCallback([&](CWindow* pWnd)
            {
                (void)pWnd;
                if( this->m_pInternal->needRenderFrame )
                {
                    if( this->_BeginFrame() )
                    {
                        this->_EndFrame();
                    }
                    this->m_pInternal->needRenderFrame = false;
                }
            });
            return VKE_OK;
        }

        handle_t CGraphicsContext::CreateGraphicsQueue(const SGraphicsQueueInfo& Info)
        {
            CGraphicsQueue* pQueue;
            if (VKE_SUCCEEDED(Memory::CreateObject(&HeapAllocator, &pQueue, this)))
            {
                if (VKE_SUCCEEDED(pQueue->Create(Info)))
                {
                    m_pInternal->vpGraphicsQueues.push_back(pQueue);
                }
                return NULL_HANDLE;
            }
            VKE_LOG_ERR("No memory to create GraphicsQueue object.");
            return NULL_HANDLE;
        }

        const void* CGraphicsContext::_GetGlobalFunctions() const
        {
            return m_pInternal->VkICD.pGlobal;
        }

        const void* CGraphicsContext::_GetInstanceFunctions() const
        {
            return m_pInternal->VkICD.pInstance;
        }

        Result CGraphicsContext::_CreateDevices()
        {
            
            return VKE_OK;
        }

        void* CGraphicsContext::_GetInstance() const
        {
            return m_pInternal->vkInstance;
        }

        void CGraphicsContext::RenderFrame()
        {
            m_pInternal->needRenderFrame = true;
        }

        bool CGraphicsContext::_BeginFrame()
        {
            m_pInternal->pSwapChain->BeginPresent();
            return true;
        }

        void CGraphicsContext::_EndFrame()
        {
            
            m_pInternal->pSwapChain->EndPresent();
        }

        void CGraphicsContext::Resize(uint32_t width, uint32_t height)
        {
            (void)width;
            (void)height;
        }

    } // RenderSystem
} // VKE