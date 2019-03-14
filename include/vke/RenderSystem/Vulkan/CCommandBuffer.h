#pragma once

#include "RenderSystem/Common.h"
#if VKE_VULKAN_RENDERER
#include "RenderSystem/CDDI.h"
#include "RenderSystem/Vulkan/CResourceBarrierManager.h"
#include "RenderSystem/CPipeline.h"
#include "RenderSystem/CRenderPass.h"

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
            uint8_t                 backBufferIdx = 0;
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

                void    Init( const SCommandBufferInitInfo& Info );

                void    Begin();
                void    End();
                void    Barrier( const STextureBarrierInfo& Info );
                void    Barrier( const SBufferBarrierInfo& Info );
                void    Barrier( const SMemoryBarrierInfo& Info );
                void    ExecuteBarriers();
                void    Flush();

                uint8_t GetBackBufferIndex() const { return m_currBackBufferIdx; }

                // Commands
                void    Draw( const uint32_t& vertexCount, const uint32_t& instanceCount, const uint32_t& firstVertex, const uint32_t& firstInstance );
                void    DrawIndexed( const uint32_t& indexCount, const uint32_t& instanceCount, const uint32_t& firstIndex, const uint32_t& vertexOffset, const uint32_t& firstInstance );
                void    Draw( const uint32_t& vertexCount ) { Draw( vertexCount, 1, 0, 0 ); }
                void    DrawIndexed( const uint32_t& indexCount ) { DrawIndexed( indexCount, 1, 0, 0, 0 ); }
                // Bindings
                void    Bind( RenderPassPtr pRenderPass );
                void    Bind( PipelineLayoutPtr pLayout );
                void    Bind( PipelinePtr pPipeline );
                void    Bind( ShaderPtr pShader );
                void    Bind( VertexBufferPtr pBuffer );
                void    Bind( const SDDISwapChain& SwapChain );
                void    Bind( CSwapChain* );
                // State
                void    Set( const SPipelineDesc::SDepthStencil& DepthStencil );
                void    Set( const SPipelineDesc::SRasterization& Rasterization );
                // Resource state
                //void    SetVertexBuffer(BufferPtr pBuffer, uint32_t firstBinding, uint32_t bindingCount);
                //void    SetIndexBuffer(BufferPtr pBuffer, size_t offset, INDEX_TYPE type);

            protected:

                Result  _DrawProlog();
                void    _Reset();

            protected:

                CDeviceContext*             m_pCtx = nullptr;
                CDDI*                       m_pDDI = nullptr;
                CCommandBufferBatch*        m_pBatch = nullptr;
                CResourceBarrierManager     m_BarrierMgr;
                SBarrierInfo                m_BarrierInfo;

                STATE                       m_state = States::UNKNOWN;
                SPipelineCreateDesc         m_CurrentPipelineDesc;
                SPipelineLayoutDesc         m_CurrentPipelineLayoutDesc;
                PipelineRefPtr              m_pCurrentPipeline;
                PipelineLayoutRefPtr        m_pCurrentPipelineLayout;
                RenderPassRefPtr            m_pCurrentRenderPass;
                uint8_t                     m_currBackBufferIdx = 0;
                bool                        m_needNewPipeline = true;
                bool                        m_needNewPipelineLayout = true;
                bool                        m_needUnbindRenderPass = false;
                bool                        m_needExecuteBarriers = false;
                bool                        m_isPipelineBound = false;
        };
    } // RendeSystem
} // VKE
#endif // VKE_VULKAN_RENDERER
