#pragma once
#include "Core/VKEPreprocessor.h"
#if VKE_VULKAN_RENDER_SYSTEM
#include "Vulkan.h"
#include "Common.h"
#include "Core/VKEForwardDeclarations.h"
#include "Core/Utils/TCDynamicArray.h"

namespace VKE
{
    namespace RenderSystem
    {
        class CCommandBuffer;
        class CGraphicsContext;
        class CDevice;
        class CRenderTarget;
        class CCommandBufferBatch;

        class CRenderQueue
        {
            friend class CGraphicsContext;

                using CommandBufferArray = Utils::TCDynamicArray< Vulkan::SCommandBuffer >;

            public:

                CRenderQueue(CGraphicsContext* pCtx);
                ~CRenderQueue();

                Result Create(const SRenderQueueDesc& Desc);
                void Destroy();
                Result Begin();
                Result End();
                Result Execute();

                void SetPriority(uint16_t priority);
                uint16_t GetPriority() const { return m_priority; }

                void SetRenderTarget(const RenderTargetHandle& hRenderTarget);

                void IsEnabled(bool enabled);
                bool IsEnabled() const { return m_enabled; }

            protected:

                //const Vulkan::SCommandBuffer&  _GetCommandBuffer() { return *m_pCurrCmdBuffer; }
                VkCommandBuffer _GetCommandBuffer() const { return m_vkCmdBuffer; }

            protected:

                SRenderQueueDesc            m_Desc;
                Vulkan::SCommandBuffer*     m_pCurrCmdBuffer = nullptr;
                VkCommandBuffer             m_vkCmdBuffer = VK_NULL_HANDLE;
                CGraphicsContext*           m_pCtx;
                CCommandBufferBatch*                    m_pSubmit = nullptr;
                CRenderTarget*              m_pRenderTarget = nullptr;
                RENDER_QUEUE_USAGE          m_usage;
                uint16_t                    m_type = 0;
                uint16_t                    m_id = 0;
                uint16_t                    m_priority = 0;
                bool                        m_enabled = false;
        };
    }
} // VKE
#endif // VKE_VULKAN_RENDER_SYSTEM