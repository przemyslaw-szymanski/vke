#pragma once

#include "RenderSystem/Common.h"
#if VKE_VULKAN_RENDERER
#include "RenderSystem/CDDI.h"
#include "RenderSystem/Vulkan/CResourceBarrierManager.h"
namespace VKE
{
    namespace RenderSystem
    {
        class CDevice;
        class CCommandBufferBatch;

        struct SCommandBufferInitInfo
        {
            CDeviceContext*         pCtx = nullptr;
            CCommandBufferBatch*    pBatch = nullptr;
            DDICommandBuffer        hDDIObject = DDI_NULL_HANDLE;
            DDISemaphore            hDDISignalSemaphore = DDI_NULL_HANDLE;
        };

        class VKE_API CCommandBuffer
        {
            friend class CDeviceContext;
            friend class CGraphicsContext;
            friend class CResourceBarrierManager;
            friend class CCommandBufferManager;
            friend class CCommandBufferBatch;

            VKE_ADD_DDI_OBJECT( DDICommandBuffer );

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

                void    Init(const SCommandBufferInitInfo& Info);

                const DDISemaphore& GetDDIWaitSemaphore() const { return m_hDDIWaitSemaphore; }
                const DDISemaphore& GetDDISignalSemaphore() const { return m_hDDISignalSemaphore; }

                void    Begin();
                void    End();
                void    Barrier(const CResourceBarrierManager::SImageBarrierInfo& Barrier);
                void    Barrier( const STextureBarrierInfo& Info );
                void    Barrier( const SBufferBarrierInfo& Info );
                void    Barrier( const SMemoryBarrierInfo& Info );
                void    ExecuteBarriers();
                void    Flush();

                void    Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance);
                void    Draw(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, uint32_t vertexOffset, uint32_t firstInstance);
                // State
                void    SetShader(ShaderPtr pShader);
                void    SetDepthStencil(const SPipelineDesc::SDepthStencil& DepthStencil);
                void    SetRasterization(const SPipelineDesc::SRasterization& Rasterization);
                // Resource state
                //void    SetVertexBuffer(BufferPtr pBuffer, uint32_t firstBinding, uint32_t bindingCount);
                //void    SetIndexBuffer(BufferPtr pBuffer, size_t offset, INDEX_TYPE type);

        protected:

                Result  _DrawProlog();

            protected:

                CDeviceContext*             m_pCtx = nullptr;
                CCommandBufferBatch*        m_pBatch = nullptr;
                CResourceBarrierManager     m_BarrierMgr;
                SBarrierInfo                m_BarrierInfo;
                
                DDISemaphore                m_hDDIWaitSemaphore = DDI_NULL_HANDLE;
                DDISemaphore                m_hDDISignalSemaphore = DDI_NULL_HANDLE;
                STATE                       m_state = States::UNKNOWN;
                SPipelineCreateDesc         m_CurrentPipelineDesc;
                PipelineRefPtr              m_pCurrentPipeline;
                bool                        m_needNewPipeline = true;
        };
    } // RendeSystem
} // VKE
#endif // VKE_VULKAN_RENDERER
