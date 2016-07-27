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
                const ICD::Instance* pInstance; // retreived from CRenderSystem
            } ICD;
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

        }

        Result CContext::Create(const SContextInfo& Info)
        {
           
            const auto& SwapChains = Info.SwapChains;
            
            if (SwapChains.count)
            {
                for (uint32_t i = 0; i < SwapChains.count; ++i)
                {
                    VKE_RETURN_IF_FAILED(CreateSwapChain(SwapChains.pData[i]));
                }
            }
            else
            {
                SSwapChainInfo SwapChainInfo;
                VKE_RETURN_IF_FAILED(CreateSwapChain(SwapChainInfo));
            }
            return VKE_OK;
        }

        Result CContext::CreateSwapChain(const SSwapChainInfo& Info)
        {
            CSwapChain* pSwapChain;
            if (VKE_FAILED(Memory::CreateObject(&HeapAllocator, &pSwapChain, this, m_pDeviceCtx)))
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
            m_vpSwapChains.push_back(pSwapChain);
            auto pWnd = m_pDevice->GetRenderSystem()->GetEngine()->GetWindow(Info.hWnd);
            pWnd->SetSwapChainHandle(reinterpret_cast<handle_t>(pSwapChain));
            return VKE_OK;
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

        void CContext::RenderFrame(const handle_t& hSwapChain)
        {
        }

    } // RenderSystem
} // VKE