#include "RenderSystem/Vulkan/CContext.h"
#include "RenderSystem/Vulkan/CRenderSystem.h"
#include "CDevice.h"
#include "CVkEngine.h"
#include "CSwapChain.h"
#include "CDeviceContext.h"

#include "Vulkan.h"
#include "Core/Utils/CLogger.h"

#include "Core/Memory/Memory.h"

#include "Core/Platform/CWindow.h"

#include "RenderSystem/Vulkan/CGraphicsQueue.h"

namespace VKE
{
    namespace RenderSystem
    {
        using DeviceVec = vke_vector< CDevice* >;
        using WndSwapChainMap = vke_vector< CSwapChain* >;
        using SwapChainVec = vke_vector< CSwapChain* >;
        using GraphicsQueueVec = vke_vector< CGraphicsQueue* >;

        struct SInternal
        {
            DeviceVec               vDevices;
            WndSwapChainMap         vWndSwapChainMap;
            CSwapChain*				pSwapChain = nullptr;
            GraphicsQueueVec        vpGraphicsQueues;

            VkInstance              vkInstance;

            struct  
            {
                const ICD::Global* pGlobal; // retreived from CRenderSystem
                const ICD::Instance* pInstance; // retreived from CRenderSystem
            } ICD;

			bool					needRenderFrame = false;
        };

        CContext::CContext(CDevice* pDevice) :
            m_pDevice(pDevice)
            , m_pDeviceCtx(pDevice->_GetDeviceContext())
        {

        }

        CContext::~CContext()
        {
            Destroy();
        }

        void CContext::Destroy()
        {
            for (auto& pQueue : m_pInternal->vpGraphicsQueues)
            {
                Memory::DestroyObject(&HeapAllocator, &pQueue);
            }

			Memory::DestroyObject(&HeapAllocator, &m_pInternal->pSwapChain);
            Memory::DestroyObject(&HeapAllocator, &m_pInternal);
        }

        Result CContext::Create(const SContextInfo& Info)
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

        Result CContext::CreateSwapChain(const SSwapChainInfo& Info)
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
            pWnd->SetRenderingContext(this);
			pWnd->AddUpdateCallback([&](CWindow* pWnd)
			{
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

        handle_t CContext::CreateGraphicsQueue(const SGraphicsQueueInfo& Info)
        {
            CGraphicsQueue* pQueue;
            if (VKE_SUCCEEDED(Memory::CreateObject(&HeapAllocator, &pQueue, this)))
            {
                if (VKE_SUCCEEDED(pQueue->Create(Info)))
                {
                    handle_t hQueue = static_cast<handle_t>(m_pInternal->vpGraphicsQueues.size());
                    m_pInternal->vpGraphicsQueues.push_back(pQueue);
                }
                return NULL_HANDLE;
            }
            VKE_LOG_ERR("No memory to create GraphicsQueue object.");
            return NULL_HANDLE;
        }

        const void* CContext::_GetGlobalFunctions() const
        {
            return m_pInternal->ICD.pGlobal;
        }

        const void* CContext::_GetInstanceFunctions() const
        {
            return m_pInternal->ICD.pInstance;
        }

        Result CContext::_CreateDevices()
        {
            
            return VKE_OK;
        }

        void* CContext::_GetInstance() const
        {
            return m_pInternal->vkInstance;
        }

        void CContext::RenderFrame()
        {
			m_pInternal->needRenderFrame = true;
        }

        bool CContext::_BeginFrame()
        {
            m_pInternal->pSwapChain->BeginPresent();
            return true;
        }

        void CContext::_EndFrame()
        {
            
            m_pInternal->pSwapChain->EndPresent();
        }

        void CContext::Resize(uint32_t width, uint32_t height)
        {

        }

    } // RenderSystem
} // VKE