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
                const ICD::Instance* pInstance; // retreived from CRenderSystem
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
            m_pInternal->ICD.pInstance = reinterpret_cast<const ICD::Instance*>(m_pRenderSystem->_GetInstanceFunctions());
            const ICD::Global& Global = *m_pInternal->ICD.pGlobal;

            auto* pEngine = m_pRenderSystem->GetEngine();
            const auto& EngineInfo = pEngine->GetInfo();

            SDeviceInfo DevInfo;
            DevInfo.pAdapterInfo = Info.pAdapterInfo;
            VKE_RETURN_IF_FAILED(_CreateDevice(DevInfo));

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