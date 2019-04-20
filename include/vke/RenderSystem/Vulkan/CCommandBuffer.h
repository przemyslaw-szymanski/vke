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
        class CContextBase;

        struct SCommandBufferInitInfo
        {
            CContextBase*   pBaseCtx = nullptr;
            uint8_t         backBufferIdx = UNDEFINED_U8;
        };

        class VKE_API CCommandBuffer
        {
            friend class CDeviceContext;
            friend class CGraphicsContext;
            friend class CResourceBarrierManager;
            friend class CCommandBufferManager;
            friend class CCommandBufferBatch;
            friend class CContextBase;

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
                Result  End(COMMAND_BUFFER_END_FLAG flag = CommandBufferEndFlags::END);
                void    Barrier( const STextureBarrierInfo& Info );
                void    Barrier( const SBufferBarrierInfo& Info );
                void    Barrier( const SMemoryBarrierInfo& Info );
                void    ExecuteBarriers();
                //Result  Flush(const uint64_t& timeout);

                uint8_t GetBackBufferIndex() const { return m_currBackBufferIdx; }

                // Commands
                void    Draw( const uint32_t& vertexCount, const uint32_t& instanceCount, const uint32_t& firstVertex, const uint32_t& firstInstance );
                void    DrawIndexed( const uint32_t& indexCount, const uint32_t& instanceCount, const uint32_t& firstIndex, const uint32_t& vertexOffset, const uint32_t& firstInstance );
                void    Draw( const uint32_t& vertexCount ) { Draw( vertexCount, 1, 0, 0 ); }
                void    DrawIndexed( const uint32_t& indexCount ) { DrawIndexed( indexCount, 1, 0, 0, 0 ); }

                void    Dispatch( uint32_t x, uint32_t y, uint32_t z );
                // Bindings
                void    Bind( RenderPassPtr pRenderPass );
                void    Bind( PipelineLayoutPtr pLayout );
                void    Bind( PipelinePtr pPipeline );
                void    Bind( ShaderPtr pShader );
                void    Bind( VertexBufferPtr pBuffer );
                void    Bind( const SDDISwapChain& SwapChain );
                void    Bind( CSwapChain* );
                // State
                void    SetState( const SPipelineDesc::SDepthStencil& DepthStencil );
                void    SetState( const SPipelineDesc::SRasterization& Rasterization );
                void    SetState( const SVertexInputLayoutDesc& InputLayout );
                void    SetState( const SPipelineDesc::SInputLayout& InputLayout );
                // Resource state
                //void    SetVertexBuffer(BufferPtr pBuffer, uint32_t firstBinding, uint32_t bindingCount);
                //void    SetIndexBuffer(BufferPtr pBuffer, size_t offset, INDEX_TYPE type);

            protected:

                Result  _DrawProlog();
                void    _Reset();

            protected:

                CContextBase*               m_pBaseCtx = nullptr;
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

        class CCommandBufferContext
        {
            friend class CDeviceContext;
            friend class CGraphicsContext;
            friend class CComputeContext;
            friend class CTransferContext;

            public:

                void vke_force_inline
                Barrier( const STextureBarrierInfo& Info ) { m_pCurrentCommandBuffer->Barrier( Info ); }
                
                void vke_force_inline
                Barrier( const SBufferBarrierInfo& Info ) { m_pCurrentCommandBuffer->Barrier( Info ); }
                
                void vke_force_inline
                Barrier( const SMemoryBarrierInfo& Info ) { m_pCurrentCommandBuffer->Barrier( Info ); }
                
                void vke_force_inline
                ExecuteBarriers() { m_pCurrentCommandBuffer->ExecuteBarriers(); }

            protected:

                void    Begin() { m_pCurrentCommandBuffer->Begin(); }
                void    End( COMMAND_BUFFER_END_FLAG flag = CommandBufferEndFlags::END ) { m_pCurrentCommandBuffer->End( flag ); }

            protected:

                CommandBufferPtr    m_pCurrentCommandBuffer;
        };

        class CCommandBufferTransferContext : public CCommandBufferContext
        {
            public:

            protected:
        };

        class CCommandBufferComputeContext : public CCommandBufferContext
        {
            public:

                // Bindings
                void    Bind( PipelineLayoutPtr pLayout ) { this->m_pCurrentCommandBuffer->Bind( pLayout ); }
                void    Bind( PipelinePtr pPipeline ) { this->m_pCurrentCommandBuffer->Bind( pPipeline ); }
                void    Bind( ShaderPtr pShader ) { this->m_pCurrentCommandBuffer->Bind( pShader ); }

            protected:
        };

        class CCommandBufferGraphicsContext : public CCommandBufferContext
        {
                // Commands
            public:

                void vke_force_inline
                Draw( const uint32_t& vertexCount, const uint32_t& instanceCount,
                    const uint32_t& firstVertex, const uint32_t& firstInstance )
                {
                    this->m_pCurrentCommandBuffer->Draw( vertexCount, instanceCount, firstVertex, firstInstance );
                }

                void vke_force_inline
                DrawIndexed( const uint32_t& indexCount, const uint32_t& instanceCount,
                    const uint32_t& firstIndex, const uint32_t& vertexOffset, const uint32_t& firstInstance )
                {
                    this->m_pCurrentCommandBuffer->DrawIndexed( indexCount, instanceCount, firstIndex, vertexOffset, firstInstance );
                }
                
                void vke_force_inline
                Draw( const uint32_t& vertexCount ) { this->m_pCurrentCommandBuffer->Draw( vertexCount ); }
                
                void vke_force_inline
                DrawIndexed( const uint32_t& indexCount ) { this->m_pCurrentCommandBuffer->DrawIndexed( indexCount ); }

                // Bindings
            public:

                void vke_force_inline
                Bind( RenderPassPtr pRenderPass ) { this->m_pCurrentCommandBuffer->Bind( pRenderPass ); }
                
                void vke_force_inline
                Bind( PipelineLayoutPtr pLayout ) { this->m_pCurrentCommandBuffer->Bind( pLayout ); }
                
                void vke_force_inline
                Bind( PipelinePtr pPipeline ) { this->m_pCurrentCommandBuffer->Bind( pPipeline ); }
                
                void vke_force_inline
                Bind( ShaderPtr pShader ) { this->m_pCurrentCommandBuffer->Bind( pShader ); }
                
                void vke_force_inline
                Bind( VertexBufferPtr pBuffer ) { this->m_pCurrentCommandBuffer->Bind(pBuffer); }

                // State
            public:

                void vke_force_inline
                SetState( const SPipelineDesc::SDepthStencil& DepthStencil ) { this->m_pCurrentCommandBuffer->SetState( DepthStencil ); }
                
                void vke_force_inline
                SetState( const SPipelineDesc::SRasterization& Rasterization ) { this->m_pCurrentCommandBuffer->SetState( Rasterization ); }
                void vke_force_inline
                SetState( const SVertexInputLayoutDesc& InputLayout ) { this->m_pCurrentCommandBuffer->SetState( InputLayout ); }
                
                void vke_force_inline
                SetState( const SPipelineDesc::SInputLayout& InputLayout ) { this->m_pCurrentCommandBuffer->SetState( InputLayout ); }
                
            protected:

                void vke_force_inline
                Bind( const SDDISwapChain& SwapChain ) { this->m_pCurrentCommandBuffer->Bind(SwapChain); }
                
                void vke_force_inline
                Bind( CSwapChain* pSwapChain ) { this->m_pCurrentCommandBuffer->Bind( pSwapChain ); }

            protected:
        };

    } // RendeSystem
} // VKE
#endif // VKE_VULKAN_RENDERER
