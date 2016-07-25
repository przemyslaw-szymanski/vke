#include "RenderSystem/Vulkan/CContext.h"
#include "RenderSystem/Vulkan/CRenderSystem.h"
#include "CDevice.h"
#include "CVkEngine.h"
#include "CSwapChain.h"

#include "Vulkan.h"
#include "Core/Utils/CLogger.h"

#include "Core/Memory/Memory.h"

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

        CContext::CContext()
   
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