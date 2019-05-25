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
            bool            initGraphicsShaders = false;
            bool            initComputeShader = false;
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

                using DescSetArray = Utils::TCDynamicArray< DescriptorSetHandle >;
                using DDIDescSetArray = Utils::TCDynamicArray< DDIDescriptorSet >;

            public:

                CCommandBuffer();
                ~CCommandBuffer();

                void    Init( const SCommandBufferInitInfo& Info );

                void    Begin();
                Result  End( COMMAND_BUFFER_END_FLAGS flag = CommandBufferEndFlags::END );
                STATE   GetState() const { return m_state; }
                bool    IsDirty() const { return m_isDirty; }

                void    Barrier( const STextureBarrierInfo& Info );
                void    Barrier( const SBufferBarrierInfo& Info );
                void    Barrier( const SMemoryBarrierInfo& Info );
                void    ExecuteBarriers();
                //Result  Flush(const uint64_t& timeout);

                uint8_t GetBackBufferIndex() const { return m_currBackBufferIdx; }

                // Create resources
                void    Bind( const RenderTargetHandle& hRT );

                // Commands
                void    DrawFast( const uint32_t& vertexCount, const uint32_t& instanceCount, const uint32_t& firstVertex, const uint32_t& firstInstance );
                void    DrawIndexedFast( const uint32_t& indexCount, const uint32_t& instanceCount, const uint32_t& firstIndex, const uint32_t& vertexOffset, const uint32_t& firstInstance );
                void    DrawFast( const uint32_t& vertexCount ) { DrawFast( vertexCount, 1, 0, 0 ); }
                void    DrawIndexedFast( const uint32_t& indexCount ) { DrawIndexedFast( indexCount, 1, 0, 0, 0 ); }
                void    DrawWithCheck( const uint32_t& vertexCount, const uint32_t& instanceCount, const uint32_t& firstVertex, const uint32_t& firstInstance );
                void    DrawIndexedWithCheck( const uint32_t& indexCount, const uint32_t& instanceCount, const uint32_t& firstIndex, const uint32_t& vertexOffset, const uint32_t& firstInstance );
                void    DrawWithCheck( const uint32_t& vertexCount ) { DrawWithCheck( vertexCount, 1, 0, 0 ); }
                void    DrawIndexedWithCheck( const uint32_t& indexCount ) { DrawIndexedWithCheck( indexCount, 1, 0, 0, 0 ); }

                template<bool CheckState = true>
                void    Draw( const uint32_t& vertexCount, const uint32_t& instanceCount, const uint32_t& firstVertex, const uint32_t& firstInstance );
                template<bool CheckState = true>
                void    DrawIndexed( const uint32_t& indexCount, const uint32_t& instanceCount, const uint32_t& firstIndex, const uint32_t& vertexOffset, const uint32_t& firstInstance );
                template<bool CheckState = true>
                void    Draw( const uint32_t& vertexCount ) { Draw<CheckState>( vertexCount, 1, 0, 0 ); }
                template<bool CheckState = true>
                void    DrawIndexed( const uint32_t& indexCount ) { DrawIndexed<CheckState>( indexCount, 1, 0, 0, 0 ); }

                void    Dispatch( uint32_t x, uint32_t y, uint32_t z );
                // Bindings
                void    Bind( RenderPassPtr pRenderPass );
                void    Bind( VertexBufferPtr pBuffer );
                void    Bind( const SDDISwapChain& SwapChain );
                void    Bind( CSwapChain* );
                void    Bind( PipelinePtr pPipeline );
                void    Bind( const DescriptorSetHandle& hSet );
                // State
                void    SetState( const SPipelineDesc::SDepthStencil& DepthStencil );
                void    SetState( const SPipelineDesc::SRasterization& Rasterization );
                void    SetState( const SVertexInputLayoutDesc& InputLayout );
                void    SetState( const SPipelineDesc::SInputLayout& InputLayout );
                void    SetState( PipelineLayoutPtr pLayout );
                void    SetState( ShaderPtr pShader );
                void    SetState( const SViewportDesc& Viewport );
                void    SetState( const SScissorDesc& Scissor );
                void    SetState( const SViewportDesc& Viewport, bool immediate );
                void    SetState( const SScissorDesc& Scissor, bool immediate );
                void    SetState( const PRIMITIVE_TOPOLOGY& topology );
                // Resource state
                //void    SetVertexBuffer(BufferPtr pBuffer, uint32_t firstBinding, uint32_t bindingCount);
                //void    SetIndexBuffer(BufferPtr pBuffer, size_t offset, INDEX_TYPE type);

                // Copy
                void    Copy( const SCopyBufferInfo& Info );
                void    Copy( const SCopyTextureInfoEx& Info );

            protected:

                void    _BeginProlog();
                Result  _DrawProlog();
                void    _Reset();
                void    _BindDescriptorSets();

                void    _FreeDescriptorSet( const DescriptorSetHandle& hSet );

                Result  _UpdateCurrentPipeline();
                Result  _UpdateCurrentRenderPass();

            protected:

                CContextBase*               m_pBaseCtx = nullptr;
                CResourceBarrierManager     m_BarrierMgr;
                SBarrierInfo                m_BarrierInfo;
                DescSetArray                m_vBindings;
                DDIDescSetArray             m_vDDIBindings;
                DescSetArray                m_vUsedSets;

                STATE                       m_state = States::UNKNOWN;
                SPipelineCreateDesc         m_CurrentPipelineDesc;
                SPipelineLayoutDesc         m_CurrentPipelineLayoutDesc;
                PipelineRefPtr              m_pCurrentPipeline;
                PipelineLayoutRefPtr        m_pCurrentPipelineLayout;
                DDIPipelineLayout           m_hDDILastUsedLayout = DDI_NULL_HANDLE;
                SRenderPassDesc             m_CurrentRenderPassDesc;
                RenderPassHandle            m_hCurrentdRenderPass = NULL_HANDLE;
                RenderPassPtr               m_pCurrentRenderPass;
                uint32_t                    m_currViewportHash = 0;
                uint32_t                    m_currScissorHash = 0;
                SViewportDesc               m_CurrViewport;
                SScissorDesc                m_CurrScissor;
                uint32_t                    m_currBackBufferIdx = 0;
                uint32_t                    m_needNewPipeline : 1;
                uint32_t                    m_needNewPipelineLayout : 1;
                uint32_t                    m_needExecuteBarriers : 1;
                uint32_t                    m_needNewRenderPass : 1;
                uint32_t                    m_isRenderPassBound : 1;
                uint32_t                    m_isPipelineBound : 1;
                uint32_t                    m_isDirty : 1;
        };

        template<bool CheckState>
        void CCommandBuffer::Draw( const uint32_t& vertexCount, const uint32_t& instanceCount,
            const uint32_t& firstVertex, const uint32_t& firstInstance )
        {
            if( CheckState )
            {
                DrawWithCheck( vertexCount, instanceCount, firstVertex, firstInstance );
            }
            else
            {
                DrawFast( vertexCount, instanceCount, firstVertex, firstInstance );
            }
        }

        template<bool CheckState>
        void CCommandBuffer::DrawIndexed( const uint32_t& indexCount, const uint32_t& instanceCount,
            const uint32_t& firstIndex, const uint32_t& vertexOffset, const uint32_t& firstInstance )
        {
            if( CheckState )
            {
                DrawIndexedWithCheck( indexCount, instanceCount, firstIndex, vertexOffset, firstInstance );
            }
            else
            {
                DrawIndexedFast( indexCount, instanceCount, firstIndex, vertexOffset, firstInstance );
            }
        }


        class VKE_API CCommandBufferContext
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

                void vke_force_inline
                Copy( const SCopyBufferInfo& Info ) { m_pCurrentCommandBuffer->Copy( Info ); }

                void vke_force_inline
                Copy(const SCopyTextureInfoEx& Info) { m_pCurrentCommandBuffer->Copy( Info ); }

            protected:

                void vke_force_inline
                _Begin() { m_pCurrentCommandBuffer->Begin(); }

                void vke_force_inline
                _End( COMMAND_BUFFER_END_FLAGS flag = CommandBufferEndFlags::END )
                {
                    if( m_pCurrentCommandBuffer != nullptr )
                    {
                        m_pCurrentCommandBuffer->End( flag );
                        m_pCurrentCommandBuffer = nullptr;
                    }
                }

            protected:

                CCommandBuffer*    m_pCurrentCommandBuffer = nullptr;
        };

        class VKE_API CCommandBufferTransferContext : public virtual CCommandBufferContext
        {
            public:

                void vke_force_inline Copy( const SCopyBufferInfo& Info ) { m_pCurrentCommandBuffer->Copy( Info ); }

            protected:
        };

        class VKE_API CCommandBufferComputeContext : public virtual CCommandBufferContext
        {
            public:

                // Bindings
                void    SetState( PipelineLayoutPtr pLayout ) { this->m_pCurrentCommandBuffer->SetState( pLayout ); }
                void    Bind( PipelinePtr pPipeline ) { this->m_pCurrentCommandBuffer->Bind( pPipeline ); }
                void    SetState( ShaderPtr pShader ) { this->m_pCurrentCommandBuffer->SetState( pShader ); }
                void    Bind( const DescriptorSetHandle& hSet ) { this->m_pCurrentCommandBuffer->Bind( hSet ); }

            protected:
        };

        class VKE_API CCommandBufferGraphicsContext : public virtual CCommandBufferComputeContext
        {
                // Commands
            public:

                void vke_force_inline
                Draw( const uint32_t& vertexCount, const uint32_t& instanceCount,
                    const uint32_t& firstVertex, const uint32_t& firstInstance )
                {
                    this->m_pCurrentCommandBuffer->Draw( vertexCount, instanceCount, firstVertex, firstInstance );
                }

                template<bool CheckState = true>
                void vke_force_inline
                DrawIndexed( const uint32_t& indexCount, const uint32_t& instanceCount,
                    const uint32_t& firstIndex, const uint32_t& vertexOffset, const uint32_t& firstInstance )
                {
                    this->m_pCurrentCommandBuffer->DrawIndexed<CheckState>( indexCount, instanceCount, firstIndex, vertexOffset, firstInstance );
                }
                
                template<bool CheckState = true>
                void vke_force_inline
                Draw( const uint32_t& vertexCount ) { this->m_pCurrentCommandBuffer->Draw<CheckState>( vertexCount ); }
                
                template<bool CheckState = true>
                void vke_force_inline
                DrawIndexed( const uint32_t& indexCount ) { this->m_pCurrentCommandBuffer->DrawIndexed<CheckState>( indexCount ); }

                // Bindings
            public:

                void vke_force_inline
                Bind(const RenderTargetHandle& hRT) { this->m_pCurrentCommandBuffer->Bind( hRT ); }

                void vke_force_inline
                Bind( RenderPassPtr pRenderPass ) { this->m_pCurrentCommandBuffer->Bind( pRenderPass ); }
                
                void vke_force_inline
                SetState( PipelineLayoutPtr pLayout ) { this->m_pCurrentCommandBuffer->SetState( pLayout ); }
                
                void vke_force_inline
                Bind( PipelinePtr pPipeline ) { this->m_pCurrentCommandBuffer->Bind( pPipeline ); }
                
                void vke_force_inline
                SetState( ShaderPtr pShader ) { this->m_pCurrentCommandBuffer->SetState( pShader ); }
                
                void vke_force_inline
                SetState(const PRIMITIVE_TOPOLOGY& topology) {this->m_pCurrentCommandBuffer->SetState( topology ); }

                void vke_force_inline
                Bind( VertexBufferPtr pBuffer ) { this->m_pCurrentCommandBuffer->Bind(pBuffer); }

                void vke_force_inline
                Bind( const DescriptorSetHandle& hSet ) { CCommandBufferComputeContext::Bind( hSet ); }

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
                
                void vke_force_inline
                SetState( const SViewportDesc& Viewport ) { this->m_pCurrentCommandBuffer->SetState( Viewport ); }

                void vke_force_inline
                SetState( const SScissorDesc& Scissor ) { this->m_pCurrentCommandBuffer->SetState( Scissor ); }

                void vke_force_inline
                Bind( CSwapChain* pSwapChain ) { this->m_pCurrentCommandBuffer->Bind( pSwapChain ); }

            protected:

                void vke_force_inline
                Bind( const SDDISwapChain& SwapChain ) { this->m_pCurrentCommandBuffer->Bind(SwapChain); }
                
                

            protected:
        };

    } // RendeSystem
} // VKE
#endif // VKE_VULKAN_RENDERER
