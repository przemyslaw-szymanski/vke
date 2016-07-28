#ifndef __VKE_RENDER_SYSTEM_CSWAP_CHAIN_H__
#define __VKE_RENDER_SYSTEM_CSWAP_CHAIN_H__

#include "RenderSystem/Vulkan/Common.h"
#include "Vulkan.h"

namespace VKE
{
    namespace RenderSystem
    {
        class CDevice;
        class CContext;
        class CDeviceContext;

        struct SSwapChainElement
        {
            VkImage         vkImage;
            VkImageView     vkImageView;
            VkSemaphore     vkSemaphore;
        };

        class CSwapChain
        {
            friend class CDevice;
           
            public:

                CSwapChain(CContext* pCtx);
                ~CSwapChain();

                Result Create(const SSwapChainInfo& Info);
                void Destroy();

                Result    GetNextElement();
                vke_force_inline
                const SSwapChainElement& GetCurrentElement() const { return m_aElements[m_currElementId]; }

                void    BeginPresent();
                void    EndPresent();
                void    SetQueue(VkQueue vkQueue) { m_vkCurrQueue = vkQueue; }

                vke_force_inline
                CDevice*      GetDevice() const { return m_pDevice; }

                vke_force_inline
                const SSwapChainInfo&   GetInfo() const { return m_Info; }

            protected:


            protected:
              
                SSwapChainInfo              m_Info;
                SSwapChainElement           m_aElements[Constants::RenderSystem::MAX_SWAP_CHAIN_ELEMENTS];
                VkSemaphore                 m_aSemaphores[Constants::RenderSystem::MAX_SWAP_CHAIN_ELEMENTS];
                uint32_t                    m_aElementQueue[Constants::RenderSystem::MAX_SWAP_CHAIN_ELEMENTS];
                CContext*                   m_pCtx = nullptr;
                CDevice*                    m_pDevice = nullptr;
                CDeviceContext*             m_pDeviceCtx = nullptr;
                VkDevice                    m_vkDevice = VK_NULL_HANDLE;
                VkSwapchainKHR              m_vkSwapChain = VK_NULL_HANDLE;
                VkSurfaceKHR                m_vkSurface = VK_NULL_HANDLE;
                VkQueue                     m_vkCurrQueue = VK_NULL_HANDLE;
                VkSurfaceFormatKHR          m_vkSurfaceFormat;
                VkPresentModeKHR            m_vkPresentMode;
                VkSurfaceCapabilitiesKHR    m_vkSurfaceCaps;
                uint32_t                    m_currElementId = 0;
                VKE_DEBUG_CODE(VkSwapchainCreateInfoKHR m_vkCreateInfo);
        };
    } // RenderSystem
} // VKE

#endif // __VKE_CDEVICE_H__