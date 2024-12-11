#pragma once

#include "RenderSystem/Common.h"
#if VKE_VULKAN_RENDER_SYSTEM
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

        struct SCommandBufferPoolHandleDecoder
        {
            union
            {
                struct
                {
                    uint32_t threadId : 8;
                    uint32_t index : 24;
                } Decode;
                uint32_t value;
            };
        };

        enum CHECK_STATUS
        {
            DO_NOT_CHECK = 0,
            CHECK = 1
        };

        class VKE_API CCommandBuffer
        {
            friend class CDeviceContext;
            friend class CGraphicsContext;
            friend class CResourceBarrierManager;
            friend class CCommandBufferManager;
            friend class CCommandBufferBatch;
            friend class CContextBase;

            VKE_DECL_OBJECT_TS_REF_COUNT( 1 );
            VKE_ADD_DDI_OBJECT( DDICommandBuffer );

            using States = CommandBufferStates;

            public:

                using DescSetArray = Utils::TCDynamicArray< DescriptorSetHandle >;
                using DDIDescSetArray = Utils::TCDynamicArray< DDIDescriptorSet >;
                using DDISemaphoreArray = Utils::TCDynamicArray< DDISemaphore, 8 >;
                using UintArray = Utils::TCDynamicArray< uint32_t >;
                using HandleArray = Utils::TCDynamicArray< handle_t >;
                using TriboolArray = Utils::TCDynamicArray< tribool_t* >;
                using BoolPtrVec = Utils::TCDynamicArray<bool*>;
                using StringArray = Utils::TCDynamicArray< vke_string, 1024 >;

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
                Result  End();
                void    Reset();
                Result  Flush();
                COMMAND_BUFFER_STATE   GetState() const { return m_state; }
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

                template<CHECK_STATUS CheckState = DO_NOT_CHECK>
                void    Draw( const uint32_t& vertexCount, const uint32_t& instanceCount, const uint32_t& firstVertex, const uint32_t& firstInstance );
                template<CHECK_STATUS CheckState = DO_NOT_CHECK>
                void    DrawIndexed( const uint32_t& indexCount, const uint32_t& instanceCount, const uint32_t& firstIndex, const uint32_t& vertexOffset, const uint32_t& firstInstance );
                template<CHECK_STATUS CheckState = DO_NOT_CHECK>
                void    DrawIndexed( const SDrawParams& Params );
                template<CHECK_STATUS CheckState = DO_NOT_CHECK>
                void    Draw( const uint32_t& vertexCount ) { Draw<CheckState>( vertexCount, 1, 0, 0 ); }
                template<CHECK_STATUS CheckState = DO_NOT_CHECK>
                void    DrawIndexed( const uint32_t& indexCount ) { DrawIndexed<CheckState>( indexCount, 1, 0, 0, 0 ); }
                void    DrawMesh(uint32_t width, uint32_t height, uint32_t depth);

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

                // Copy & Blit
                void    Copy( const SCopyBufferInfo& Info );
                void    Copy( const SCopyTextureInfoEx& Info );
                void    Copy( const SCopyBufferToTextureInfo& Info );
                void    Blit( const SBlitTextureInfo& Info );
                void    GenerateMipmaps( TexturePtr );

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

                DDIFence GetCPUFence() const { return m_hApiCpuFence; }
                DDISemaphore GetGPUFence() const { return m_hApiGpuFence; }

                void AddResourceToNotify(bool* pNotify)
                {
                    m_vpNotifyResources.PushBack( pNotify );
                }

                void Sync( CommandBufferPtr );
                EXECUTE_COMMAND_BUFFER_FLAGS GetExecuteFlags() const { return m_executeFlags;}
                void SignalGPUFence() { m_executeFlags |= ExecuteCommandBufferFlags::SIGNAL_GPU_FENCE; }

                void AddDebugMarkerText(std::string_view&& s)
                {
#if VKE_RENDER_SYSTEM_DEBUG
                    m_vDebugMarkerTexts.PushBack( s.data() );
#endif
                }

                void DumpDebugMarkerTexts()
                {
#if VKE_RENDER_SYSTEM_DEBUG
                    for( uint32_t i = 0; i < m_vDebugMarkerTexts.GetCount(); ++i )
                    {
                        VKE_LOG( this << "(" << this->GetDDIObject() << "): " <<  m_vDebugMarkerTexts[ i ] );
                    }
                    m_vDebugMarkerTexts.Clear();
#endif
                }

                void SetDebugName( cstr_t pDbgName );
                cstr_t GetDebugName() const
                {
#if VKE_RENDER_SYSTEM_DEBUG
                    return m_DbgName.GetData();
#else
                    return "";
#endif
                }

            protected:

                void _ExecutePendingOperations();
                void    _BeginProlog();
                Result  _DrawProlog();
                void    _Reset();
                void    _BindDescriptorSets();

                void    _FreeDescriptorSet( const DescriptorSetHandle& hSet );

                Result  _UpdateCurrentPipeline();
                Result  _UpdateCurrentRenderPass();

                //void    _SetCPUSyncObject(const DDIFence& hDDIFence) { m_hDDIFence = hDDIFence; }
                //void    _SetGPUSyncObject(DDISemaphore hApi) { m_hApiGPUSyncObject = hApi; }

                /// <summary>
                /// Command buffer manager notifies CommandBuffer that is was executed.
                /// </summary>
                void _NotifyExecuted();
                void    _FreeResources();

                handle_t _GetHandlePool() const { return m_hPool.value; }

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
                //handle_t                    m_hPool = INVALID_HANDLE;
                SCommandBufferPoolHandleDecoder m_hPool;
                COMMAND_BUFFER_STATE        m_state = States::UNKNOWN;
                /// <summary>
                /// Resources to notify when command buffer is executed
                /// </summary>
                BoolPtrVec                  m_vpNotifyResources;
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
                DDIFence                    m_hApiCpuFence = DDI_NULL_HANDLE;
                DDISemaphore                m_hApiGpuFence = DDI_NULL_HANDLE;
                DDICommandBufferPool        m_hDDICmdBufferPool = DDI_NULL_HANDLE;
                void*                       m_pExecuteBatch = nullptr;
                uint32_t                    m_currViewportHash = 0;
                uint32_t                    m_currScissorHash = 0;
                handle_t                    m_hStagingBuffer = UNDEFINED_U64;
                SViewportDesc               m_CurrViewport;
                SScissorDesc                m_CurrScissor;
                EXECUTE_COMMAND_BUFFER_FLAGS m_executeFlags = 0;
                uint32_t                    m_currBackBufferIdx = 0;
                uint32_t                    m_needNewPipeline : 1;
                uint32_t                    m_needNewPipelineLayout : 1;
                uint32_t                    m_needExecuteBarriers : 1;
                uint32_t                    m_needNewRenderPass : 1;
                uint32_t                    m_isRenderPassBound : 1;
                uint32_t                    m_isPipelineBound : 1;
                uint32_t                    m_isDirty : 1;
#if VKE_RENDER_SYSTEM_DEBUG
                StringArray                 m_vDebugMarkerTexts;
                ResourceName                m_DbgName;
#endif // VKE_RENDER_SYSTEM_DEBUG
        };

        template<CHECK_STATUS CheckState>
        void CCommandBuffer::Draw( const uint32_t& vertexCount, const uint32_t& instanceCount,
            const uint32_t& firstVertex, const uint32_t& firstInstance )
        {
            if constexpr( CheckState )
            {
                DrawWithCheck( vertexCount, instanceCount, firstVertex, firstInstance );
            }
            else
            {
                DrawFast( vertexCount, instanceCount, firstVertex, firstInstance );
            }
        }

        template<CHECK_STATUS CheckState>
        void CCommandBuffer::DrawIndexed( const uint32_t& indexCount, const uint32_t& instanceCount,
            const uint32_t& firstIndex, const uint32_t& vertexOffset, const uint32_t& firstInstance )
        {
            if constexpr( CheckState )
            {
                DrawIndexedWithCheck( indexCount, instanceCount, firstIndex, vertexOffset, firstInstance );
            }
            else
            {
                DrawIndexedFast( indexCount, instanceCount, firstIndex, vertexOffset, firstInstance );
            }
        }

        template<CHECK_STATUS CheckState>
        void CCommandBuffer::DrawIndexed( const SDrawParams& Params )
        {
            if constexpr( CheckState )
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
#endif // VKE_VULKAN_RENDER_SYSTEM
