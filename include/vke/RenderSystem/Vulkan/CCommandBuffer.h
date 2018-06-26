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
            friend class CCommandBufferManager;

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

                        CCommandBuffer(CGraphicsContext* pCtx);
                        ~CCommandBuffer();

                void    Init(const VkCommandBuffer& vkCb);
                void    Begin();
                void    End();
                void    Barrier(const CResourceBarrierManager::SImageBarrierInfo& Barrier);
                void    ExecuteBarriers();
                void    Flush();

                void    Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance);
                void    Draw(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, uint32_t vertexOffset, uint32_t firstInstance);
                // Statedd
                void    SetShader(ShaderPtr pShader);
                void    SetDepthStencil(const SPipelineDesc::SDepthStencil& DepthStencil);
                void    SetRasterization(const SPipelineDesc::SRasterization& Rasterization);

                VkCommandBuffer     GetNative() const { return m_vkCommandBuffer; }

        protected:

                Result  _DrawProlog();

            protected:

                CGraphicsContext*           m_pCtx;
                const Vulkan::ICD::Device&  m_ICD;
                CResourceBarrierManager     m_BarrierMgr;
                SPipelineCreateDesc         m_PipelineDesc;
                VkCommandBuffer             m_vkCommandBuffer = VK_NULL_HANDLE;
                STATE                       m_state = States::UNKNOWN;
                bool                        m_pipelineDescDirty = false;
        };
    } // RendeSystem
} // VKE
#endif // VKE_VULKAN_RENDERER
