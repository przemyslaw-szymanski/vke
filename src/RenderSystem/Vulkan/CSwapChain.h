#ifndef __VKE_RENDER_SYSTEM_CSWAP_CHAIN_H__
#define __VKE_RENDER_SYSTEM_CSWAP_CHAIN_H__

#include "RenderSystem/Vulkan/Common.h"
#include "Vulkan.h"

namespace VKE
{
    namespace Vulkan
    {
        class CDeviceWrapper;
    }

    namespace RenderSystem
    {
        class CDevice;
        class CGraphicsContext;
        struct SFrameData;

        struct SSwapChainElement
        {
            VkImage         vkImage = VK_NULL_HANDLE;
            VkImageView     vkImageView = VK_NULL_HANDLE;
            VkSemaphore     vkSemaphore = VK_NULL_HANDLE;
            SFrameData*     pFrameData = nullptr;
        };

        class CSwapChain
        {
            friend class CDevice;
            struct SInternal;
           
            public:

                CSwapChain(CGraphicsContext* pCtx);
                ~CSwapChain();

                Result Create(const SSwapChainInfo& Info);
                void Destroy();

                Result Resize(uint32_t width, uint32_t height);

                Result    GetNextElement();

                const SSwapChainElement* GetCurrentElement() const;

                void    BeginPresent();
                void    EndPresent();
                void    SetQueue(VkQueue vkQueue);

                vke_force_inline
                CDevice*      GetDevice() const { return m_pDevice; }

                vke_force_inline
                const SSwapChainInfo&   GetInfo() const { return m_Info; }

            protected:


            protected:
              
                SSwapChainInfo              m_Info;
                
                CGraphicsContext*           m_pCtx = nullptr;
                CDevice*                    m_pDevice = nullptr;
                Vulkan::CDeviceWrapper*     m_pDeviceCtx = nullptr;
                SInternal*                  m_pInternal = nullptr;
                
                VKE_DEBUG_CODE(VkSwapchainCreateInfoKHR m_vkCreateInfo);
        };
    } // RenderSystem
} // VKE

#endif // __VKE_CDEVICE_H__