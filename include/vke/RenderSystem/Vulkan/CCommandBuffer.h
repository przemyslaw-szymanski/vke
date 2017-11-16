#pragma once

#include "RenderSystem/Common.h"
#if VKE_VULKAN_RENDERER
#include "RenderSystem/Vulkan/Vulkan.h"
#include "RenderSystem/Vulkan/CResourceBarrierManager.h"
namespace VKE
{
    namespace RenderSystem
    {
        class CDevice;

        class VKE_API CCommandBuffer
        {
            friend class CDeviceContext;
            friend class CGraphicsContext;
            friend class CResourceBarrierManager;

            public:

                struct States
                {
                    enum STATE : uint8_t
                    {
                        UNKNOWN,
                        BEGIN,
                        END,
                        FLUSH,
                        _MAX_COUNT
                    };
                };
                using STATE = States::STATE;

            public:

                        CCommandBuffer();
                        ~CCommandBuffer();

                void    Init(const VkICD::Device* pICD, const VkCommandBuffer& vkCb);
                void    Begin();
                void    End();
                void    Barrier(const CResourceBarrierManager::SImageBarrierInfo& Barrier);
                void    ExecuteBarriers();
                void    Flush();

                VkCommandBuffer     GetNative() const { return m_vkCommandBuffer; }

            protected:

                const VkICD::Device*        m_pICD = nullptr;
                CResourceBarrierManager     m_BarrierMgr;
                VkCommandBuffer             m_vkCommandBuffer = VK_NULL_HANDLE;
                STATE                       m_state = States::UNKNOWN;
        };
    } // RendeSystem
} // VKE
#endif // VKE_VULKAN_RENDERER
