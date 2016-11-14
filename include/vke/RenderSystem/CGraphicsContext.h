#pragma once

#include "RenderSystem/Common.h"
#include "RenderSystem/Vulkan/Vulkan.h"
#include "Core/Utils/TCDynamicRingArray.h"

namespace VKE
{
    namespace RenderSystem
    {
        class CSwapChain;
        class CGraphicsQueue;
        class CDeviceContext;
        struct SInternal;

        class VKE_API CGraphicsContext
        {
            friend class CRenderSystem;
            friend class CDeviceContext;
            friend class CSwapChain;
            struct SPrivate;

            static const uint32_t DEFAULT_CMD_BUFFER_COUNT = 32;
            using CommandBufferArray = Utils::TCDynamicRingArray< VkCommandBuffer, DEFAULT_CMD_BUFFER_COUNT >;
            using UintArray = Utils::TCDynamicArray< uint32_t, DEFAULT_CMD_BUFFER_COUNT >;

            struct SCommnadBuffers
            {
                CommandBufferArray  vCmdBuffers;
                CommandBufferArray  vFreeCmdBuffers;
            };

            using CommandBufferArrays = SCommnadBuffers[ RenderQueueUsages::_MAX_COUNT ];


            public:

                CGraphicsContext(CDeviceContext* pCtx);
                ~CGraphicsContext();

                Result Create(const SGraphicsContextDesc& Info);
                void Destroy();

                void Resize(uint32_t width, uint32_t height);

                void RenderFrame();

                Result  CreateSwapChain(const SSwapChainDesc& Info);

                Vulkan::ICD::Device&    _GetICD() const;

                vke_force_inline
                CDeviceContext*        GetDeviceContext() const { return m_pDeviceCtx; }

                handle_t CreateGraphicsQueue(const SGraphicsQueueInfo&);

            protected:         

                Vulkan::CDeviceWrapper& _GetDevice() const { return m_VkDevice; }
                Result          _CreateSwapChain(const SSwapChainDesc&);
                VkCommandBuffer _CreateCommandBuffer(RENDER_QUEUE_USAGE usage);
                void            _FreeCommandBuffer(RENDER_QUEUE_USAGE usage, VkCommandBuffer vkCb);

                bool            _BeginFrame();
                void            _EndFrame();

            protected:

                SGraphicsContextDesc    m_Desc;
                CDeviceContext*         m_pDeviceCtx = nullptr;
                Vulkan::CDeviceWrapper& m_VkDevice;
                CommandBufferArrays     m_avCmdBuffers;
                VkCommandPool           m_vkCommandPool = VK_NULL_HANDLE;
                SPrivate*               m_pPrivate = nullptr;
        };
    } // RenderSystem
} // vke