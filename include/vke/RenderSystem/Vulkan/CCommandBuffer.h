#pragma once

#include "RenderSystem/Common.h"
#if VKE_VULKAN_RENDERER
#include "RenderSystem/CDDI.h"
#include "RenderSystem/Vulkan/CResourceBarrierManager.h"
#include "RenderSystem/CPipeline.h"
#include "RenderSystem/CRenderPass.h"

namespace VKE
{
#define VKE_ENABLE_SIMPLE_COMMAND_BUFFER 1

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
                using DDISemaphoreArray = Utils::TCDynamicArray< DDISemaphore, 8 >;
                using UintArray = Utils::TCDynamicArray< uint32_t >;
                using HandleArray = Utils::TCDynamicArray< handle_t >;

            public:

                CCommandBuffer();
                ~CCommandBuffer();

                void    Init( const SCommandBufferInitInfo& Info );

                CContextBase* GetContext() const { return m_pBaseCtx; }

                bool    IsExecuted();
                void    AddWaitOnSemaphore( const DDISemaphore& hDDISemaphore );

                RenderPassRefPtr        GetCurrentRenderPass() const { return m_pCurrentRenderPass; }
                const DDIRenderPass&    GetCurrentDDIRenderPass() const { return m_hDDICurrentRenderPass; }

                void    Begin();
                Result  End( EXECUTE_COMMAND_BUFFER_FLAGS flag, DDISemaphore* phDDIOut );
                STATE   GetState() const { return m_state; }
                bool    IsDirty() const { return m_isDirty; }

                void    Barrier( const STextureBarrierInfo& Info );
                void    Barrier( const SBufferBarrierInfo& Info );
                void    Barrier( const SMemoryBarrierInfo& Info );
                void    ExecuteBarriers();
                //Result  Flush(const uint64_t& timeout);

                uint8_t GetBackBufferIndex() const { return static_cast<uint8_t>( m_currBackBufferIdx ); }

                // Create resources
                void    Bind( const RenderTargetHandle& hRT );

                // Commands
                void    DrawFast( const uint32_t& vertexCount, const uint32_t& instanceCount, const uint32_t& firstVertex, const uint32_t& firstInstance );
                void    DrawIndexedFast( const SDrawParams& Params );
                void    DrawFast( const uint32_t& vertexCount ) { DrawFast( vertexCount, 1, 0, 0 ); }
                void    DrawWithCheck( const uint32_t& vertexCount, const uint32_t& instanceCount, const uint32_t& firstVertex, const uint32_t& firstInstance );
                void    DrawIndexedWithCheck( const SDrawParams& Params );
                void    DrawWithCheck( const uint32_t& vertexCount ) { DrawWithCheck( vertexCount, 1, 0, 0 ); }

                template<bool CheckState = true>
                void    Draw( const uint32_t& vertexCount, const uint32_t& instanceCount, const uint32_t& firstVertex, const uint32_t& firstInstance );
                template<bool CheckState = true>
                void    DrawIndexed( const uint32_t& indexCount, const uint32_t& instanceCount, const uint32_t& firstIndex, const uint32_t& vertexOffset, const uint32_t& firstInstance );
                template<bool CheckState = true>
                void    DrawIndexed( const SDrawParams& Params );
                template<bool CheckState = true>
                void    Draw( const uint32_t& vertexCount ) { Draw<CheckState>( vertexCount, 1, 0, 0 ); }
                template<bool CheckState = true>
                void    DrawIndexed( const uint32_t& indexCount ) { DrawIndexed<CheckState>( indexCount, 1, 0, 0, 0 ); }

                void BeginRenderPass(const SBeginRenderPassInfo2&);
                void BeginRenderPass( RenderPassPtr pPass ) { BeginRenderPass( pPass->m_BeginInfo2 ); }
                void EndRenderPass();

                void    Dispatch( uint32_t x, uint32_t y, uint32_t z );
                // Bindings
                void    Bind( RenderPassPtr pRenderPass );
                void    Bind( VertexBufferPtr pBuffer, const uint32_t offset = 0 );
                void    Bind( const VertexBufferHandle& hBuffer, const uint32_t offset = 0 );
                void    Bind( const IndexBufferHandle& hBuffer, const uint32_t offset = 0 );
                void    Bind( const SDDISwapChain& SwapChain );
                void    Bind( CSwapChain* );
                void    Bind( PipelinePtr pPipeline );
                void    Bind( const DescriptorSetHandle& hSet, const uint32_t offset );
                void    Bind( const uint32_t& index, const DescriptorSetHandle& hDescSet, const uint32_t& offset ) { Bind( index, hDescSet, &offset, 1 ); }
                void    Bind( const uint32_t& index, const DescriptorSetHandle& hDescSet, const uint32_t* pOffsets, const uint16_t& offsetCount );
                void    Bind( const SBindDDIDescriptorSetsInfo& Info );
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
                void    SetState( const ShaderHandle& hShader );
                void    SetState( TEXTURE_STATE state, TextureHandle* phTexInOut );
                //void    SetState( const TEXTURE_STATE& newState, TexturePtr* ppInOut );
                // Resource state
                //void    SetVertexBuffer(BufferPtr pBuffer, uint32_t firstBinding, uint32_t bindingCount);
                //void    SetIndexBuffer(BufferPtr pBuffer, size_t offset, INDEX_TYPE type);

                // Copy
                void    Copy( const SCopyBufferInfo& Info );
                void    Copy( const SCopyTextureInfoEx& Info );
                void    Copy( const SCopyBufferToTextureInfo& Info );

                void    SetEvent( const DDIEvent& hDDIEvent, const PIPELINE_STAGES& stages );
                void    SetEvent( const EventHandle& hEvent, const PIPELINE_STAGES& stages );
                void    ResetEvent( const DDIEvent& hDDIEvent, const PIPELINE_STAGES& stages );
                void    ResetEvent( const EventHandle& hEvent, const PIPELINE_STAGES& stages );

                // Debug
                void    BeginDebugInfo( const SDebugInfo* pInfo );
                void    EndDebugInfo();

                const HandleArray&  GetStagingBufferAllocations() const { return m_vStagingBufferAllocations; }
                void                ClearStagingBufferAllocations() { m_vStagingBufferAllocations.ClearFull(); }
                handle_t            GetLastUsedStagingBufferAllocation() const;
                void                UpdateStagingBufferAllocation(const handle_t& hStagingBuffer);
                void                AddStagingBufferAllocation(const handle_t& hStagingBuffer) { m_vStagingBufferAllocations.PushBack( hStagingBuffer ); }

            protected:

                void    _BeginProlog();
                Result  _DrawProlog();
                void    _Reset();
                void    _BindDescriptorSets();

                void    _FreeDescriptorSet( const DescriptorSetHandle& hSet );

                Result  _UpdateCurrentPipeline();
                Result  _UpdateCurrentRenderPass();

                void    _SetFence(const DDIFence& hDDIFence) { m_hDDIFence = hDDIFence; }

                void    _FreeResources();

                handle_t _GetHandlePool() const { return m_hPool; }

            protected:

                CContextBase*               m_pBaseCtx = nullptr;
                CResourceBarrierManager     m_BarrierMgr;
                SBarrierInfo                m_BarrierInfo;
                DescSetArray                m_vBindings;
                DDIDescSetArray             m_vDDIBindings;
                UintArray                   m_vBindingOffsets;
                DescSetArray                m_vUsedSets;
                DDISemaphoreArray           m_vDDIWaitOnSemaphores;
                HandleArray                 m_vStagingBufferAllocations;
                handle_t                    m_hPool = INVALID_HANDLE;
                STATE                       m_state = States::UNKNOWN;
#if !VKE_ENABLE_SIMPLE_COMMAND_BUFFER
                SPipelineCreateDesc         m_CurrentPipelineDesc;
                SPipelineLayoutDesc         m_CurrentPipelineLayoutDesc;
                PipelineLayoutRefPtr        m_pCurrentPipelineLayout;
                DDIPipelineLayout           m_hDDILastUsedLayout = DDI_NULL_HANDLE;
                SRenderPassDesc             m_CurrentRenderPassDesc;
#endif
                PipelineRefPtr              m_pCurrentPipeline;
                RenderPassHandle            m_hCurrentdRenderPass = INVALID_HANDLE;
                RenderPassRefPtr            m_pCurrentRenderPass;
                DDIRenderPass               m_hDDICurrentRenderPass = DDI_NULL_HANDLE;
                DDIFence                    m_hDDIFence = DDI_NULL_HANDLE;
                uint32_t                    m_currViewportHash = 0;
                uint32_t                    m_currScissorHash = 0;
                handle_t                    m_hStagingBuffer = UNDEFINED_U64;
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

        template<bool CheckState>
        void CCommandBuffer::DrawIndexed( const SDrawParams& Params )
        {
            if( CheckState )
            {
                DrawIndexedWithCheck( Params );
            }
            else
            {
                DrawIndexedFast( Params );
            }
        }

    } // RendeSystem
} // VKE
#endif // VKE_VULKAN_RENDERER
