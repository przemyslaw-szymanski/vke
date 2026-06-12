#pragma once

#include "RenderSystem/Common.h"
#include "Core/Memory/CFreeListPool.h"
#include "Core/Memory/CMemoryPoolManager.h"

namespace VKE::RenderSystem
{
    // Forward declarations
    class CDeviceContext;

    template<class RenderApiT>
    class VKE_API TCAPI
    {
        friend RenderApiT;
    public:

        RenderApiT* Reinterpret()
        {
            RenderApiT* pRet = static_cast< RenderApiT* >( this );
            return pRet;
        }

        const RenderApiT* Reinterpret() const
        {
            return static_cast< const RenderApiT* >( this );
        }

        Result CreateDevice(const SCreateDeviceDesc& Info, CDeviceContext* pCtx)
        {
            return Reinterpret()->CreateDeviceImpl( Info, pCtx );
        }
        void   DestroyDevice()
        {
            Reinterpret()->DestroyDeviceImpl();
        }

        static Result Load(const SDDILoadInfo& Info, SDriverInfo* pOut)
        {
            return RenderApiT::Load( Info, pOut );
        }

        const NativeAPI::Device& GetDevice() const
        {
            return Reinterpret()->GetDeviceImpl();
        }

        const NativeAPI::Adapter& GetAdapter() const
        {
            return Reinterpret()->GetAdapterImpl();
        }

        const QueueFamilyInfoArray& GetDeviceQueueInfos() const
        {
            return Reinterpret()->GetDeviceQueueInfosImpl();
        }

        static Result QueryAdapters(AdapterInfoArray* pOut)
        {
            return RenderApiT::QueryAdaptersImpl( pOut );
        }

        void QueryDeviceInfo( SDeviceInfo* pOut )
        {
            Reinterpret()->QueryDeviceInfoImpl( pOut );
        }

        NativeAPI::Buffer CreateBuffer( const SBufferDesc& Desc, const SBindMemoryInfo& MemInfo )
        {
            return Reinterpret()->CreateBufferImpl( Desc, MemInfo );
        }
        void                           DestroyBuffer(NativeAPI::Buffer* phBuffer)
        {
            return Reinterpret()->DestroyBufferImpl( phBuffer, nullptr );
        }
        NativeAPI::BufferView          CreateBufferView(const SBufferViewDesc& Desc)
        {
            return Reinterpret()->CreateBufferView( Desc, nullptr );
        }
        void                           DestroyBufferView(NativeAPI::BufferView* phBufferView)
        {
            return Reinterpret()->DestroyBufferViewImpl( phBufferView, nullptr );
        }
        Result                         GetTextureFormatProperties(const STextureDesc& Desc, STextureFormatProperties* pOut)
        {
            return Reinterpret()->GetTextureFormatPropertiesImpl( Desc, pOut );
        }

        NativeAPI::Texture CreateTexture( const STextureDesc& Desc, const SBindMemoryInfo& MemInfo )
        {
            return Reinterpret()->CreateTextureImpl( Desc, MemInfo );
        }

        void DestroyTexture( NativeAPI::Texture* phImage )
        {
            return Reinterpret()->DestroyTextureImpl( phImage, nullptr );
        }
        NativeAPI::TextureView         CreateTextureView(const STextureViewDesc& Desc)
        {
            return Reinterpret()->CreateTextureViewImpl( Desc, nullptr );
        }

        void DestroyTextureView( NativeAPI::TextureView* phImageView )
        {
            return Reinterpret()->DestroyTextureViewImpl( phImageView, nullptr );
        }
        NativeAPI::Framebuffer         CreateFramebuffer(const SFramebufferDesc& Desc)
        {
            return Reinterpret()->CreateFramebufferImpl( Desc, nullptr );
        }

        void DestroyFramebuffer( NativeAPI::Framebuffer* phFramebuffer )
        {
            return Reinterpret()->DestroyFramebufferImpl( phFramebuffer, nullptr );
        }

        NativeAPI::CPUFence CreateFence( const SFenceDesc& Desc ) const
        {
            return Reinterpret()->CreateFenceImpl( Desc, nullptr );
        }
        NativeAPI::Fence               CreateFence2(const SFenceDesc& Desc) const
        {
            return Reinterpret()->CreateFence2Impl( Desc );
        }
        void                           DestroyFence(NativeAPI::CPUFence* phFence)
        {
            return Reinterpret()->DestroyFenceImpl( phFence, nullptr );
        }

        void DestroyFence( NativeAPI::Fence* phFence )
        {
            return Reinterpret()->DestroyFenceImpl( phFence );
        }
        NativeAPI::GPUFence            CreateSemaphore(const SSemaphoreDesc& Desc) const
        {
            return Reinterpret()->CreateSemaphoreImpl( Desc, nullptr );
        }

        void DestroySemaphore( NativeAPI::GPUFence* phSemaphore )
        {
            return Reinterpret()->DestroySemaphoreImpl( phSemaphore, nullptr );
        }
        NativeAPI::RenderPass          CreateRenderPass(const SRenderPassDesc& Desc)
        {
            return Reinterpret()->CreateRenderPassImpl( Desc, nullptr );
        }

        void DestroyRenderPass( NativeAPI::RenderPass* phPass )
        {
            return Reinterpret()->DestroyRenderPassImpl( phPass, nullptr );
        }
        NativeAPI::CommandBufferPool   CreateCommandBufferPool(const SCommandBufferPoolDesc& Desc)
        {
            return Reinterpret()->CreateCommandBufferPoolImpl( Desc, nullptr );
        }

        void DestroyCommandBufferPool( NativeAPI::CommandBufferPool* phPool )
        {
            return Reinterpret()->DestroyCommandBufferPoolImpl( phPool, nullptr );
        }
        NativeAPI::DescriptorPool      CreateDescriptorPool(const SDescriptorPoolDesc& Desc)
        {
            return Reinterpret()->CreateDescriptorPoolImpl( Desc, nullptr );
        }

        void DestroyDescriptorPool( NativeAPI::DescriptorPool* phPool )
        {
            return Reinterpret()->DestroyDescriptorPoolImpl( phPool, nullptr );
        }
        NativeAPI::DescriptorSetLayout CreateDescriptorSetLayout(const SDescriptorSetLayoutDesc& Desc)
        {
            return Reinterpret()->CreateDescriptorSetLayoutImpl( Desc, nullptr );
        }

        void DestroyDescriptorSetLayout( NativeAPI::DescriptorSetLayout* phLayout )
        {
            return Reinterpret()->DestroyDescriptorSetLayoutImpl( phLayout, nullptr );
        }
        NativeAPI::Pipeline       CreatePipeline(const SPipelineDesc& Desc)
        {
            return Reinterpret()->CreatePipelineImpl( Desc, nullptr );
        }

        void DestroyPipeline( NativeAPI::Pipeline* phPipeline )
        {
            return Reinterpret()->DestroyPipelineImpl( phPipeline, nullptr );
        }
        NativeAPI::PipelineLayout CreatePipelineLayout(const SPipelineLayoutDesc& Desc)
        {
            return Reinterpret()->CreatePipelineLayoutImpl( Desc, nullptr );
        }

        void DestroyPipelineLayout( NativeAPI::PipelineLayout* phLayout )
        {
            return Reinterpret()->DestroyPipelineLayoutImpl( phLayout, nullptr );
        }
        NativeAPI::Shader         CreateShader(const SShaderData& Desc)
        {
            return Reinterpret()->CreateShaderImpl( Desc, nullptr );
        }

        void DestroyShader( NativeAPI::Shader* phShader )
        {
            return Reinterpret()->DestroyShaderImpl( phShader, nullptr );
        }
        NativeAPI::Sampler        CreateSampler(const SSamplerDesc& Desc)
        {
            return Reinterpret()->CreateSamplerImpl( Desc, nullptr );
        }

        void DestroySampler( NativeAPI::Sampler* phSampler )
        {
            return Reinterpret()->DestroySamplerImpl( phSampler, nullptr );
        }
        NativeAPI::Event          CreateEvent(const SEventDesc& Desc)
        {
            return Reinterpret()->CreateEventImpl( Desc, nullptr );
        }

        void DestroyEvent( NativeAPI::Event* phEvent )
        {
            return Reinterpret()->DestroyEventImpl( phEvent, nullptr );
        }

        Result CreateDescriptorSets( const AllocateDescs::SDescSet& Info, NativeAPI::DescriptorSet* pSets )
        {
            return Reinterpret()->CreateDescriptorSetsImpl( Info, pSets );
        }

        void FreeObjects( const FreeDescs::SDescSet& Sets )
        {
            return Reinterpret()->FreeObjectsImpl( Sets );
        }

        Result CreateCommandBuffers( const SAllocateCommandBufferInfo& Info, NativeAPI::CommandBuffer* pBuffers )
        {
            return Reinterpret()->CreateCommandBuffersImpl( Info, pBuffers );
        }

        void FreeObjects( const SFreeCommandBufferInfo& Info )
        {
            return Reinterpret()->FreeObjectsImpl( Info );
        }

        Result GetBufferMemoryRequirements(const SBufferDesc& Desc, SAllocationMemoryRequirementInfo* pOut)
        {
            return Reinterpret()->GetBufferMemoryRequirementsImpl( Desc, pOut );
       }
        Result GetTextureMemoryRequirements(const STextureDesc& Desc, SAllocationMemoryRequirementInfo* pOut)
        {
            return Reinterpret()->GetTextureMemoryRequirementsImpl( Desc, pOut );
        }
        void   UpdateDesc(SBufferDesc* pInOut)
        {
            return Reinterpret()->UpdateDescImpl( pInOut );
        }

        void GetFormatFeatures( FORMAT fmt, STextureFormatFeatures* pOut ) const
        {
            return Reinterpret()->GetFormatFeaturesImpl( fmt, pOut );
        }

        Result Bind(RESOURCE_TYPE type, const SBindMemoryInfo& Info)
        {
            return Reinterpret()->BindImpl( type, Info );
        }
        void   Bind(const SBindPipelineInfo& Info)
        {
            return Reinterpret()->BindImpl( Info );
        }
        void   Bind(const SBindDDIDescriptorSetsInfo& Info)
        {
            return Reinterpret()->BindImpl( Info );
        }
        void   Bind(const SBindRenderPassInfo& Info)
        {
            return Reinterpret()->BindImpl( Info );
        }
        void   Bind(const NativeAPI::CommandBuffer& hDDICmdBuffer, const NativeAPI::Buffer& hDDIBuffer,
            const uint32_t offset)
        {
            return Reinterpret()->BindImpl( hDDICmdBuffer, hDDIBuffer, offset );
        }
        void   Bind(const NativeAPI::CommandBuffer& hDDICmdBuffer, const NativeAPI::Buffer& hDDIBuffer,
            const uint32_t offset, const INDEX_TYPE& type)
        {
            return Reinterpret()->BindImpl( hDDICmdBuffer, hDDIBuffer, offset, type );
        }
        void   UnbindPipeline(const NativeAPI::CommandBuffer& hCmdBuffer, const NativeAPI::Pipeline& hPipeline)
        {
            return Reinterpret()->UnbindPipelineImpl( hCmdBuffer, hPipeline );
        }
        void   UnbindRenderPass(const NativeAPI::CommandBuffer& hCmdBuffer, const NativeAPI::RenderPass& hRenderPass)
        {
            return Reinterpret()->UnbindRenderPassImpl( hCmdBuffer, hRenderPass );
        }

        void Free(NativeAPI::Memory* phMemory = nullptr)
        {
            return Reinterpret()->FreeImpl( phMemory );
        }

        void Update(const SUpdateBufferDescriptorSetInfo& Info)
        {
            return Reinterpret()->UpdateImpl( Info );
        }
        void Update(const SUpdateTextureDescriptorSetInfo& Info)
        {
            return Reinterpret()->UpdateImpl( Info );
        }
        void Update(const NativeAPI::DescriptorSet& hDDISet, const SUpdateBindingsHelper& Info)
        {
            return Reinterpret()->UpdateImpl( hDDISet, Info );
        }

        void Update( const NativeAPI::DescriptorSet& hDDISrcSet, NativeAPI::DescriptorSet* phDDIDstOut )
        {
            return Reinterpret()->UpdateImpl( hDDISrcSet, phDDIDstOut );
        }

        Result           Allocate(const SAllocateMemoryDesc& Desc, SAllocateMemoryData* pOut)
        {
            return Reinterpret()->AllocateImpl( Desc, pOut );
        }

        MEMORY_HEAP_TYPE GetMemoryHeapType( MEMORY_USAGE usage ) const
        {
            return Reinterpret()->GetMemoryHeapTypeImpl( usage );
        }

        size_t GetMemoryHeapTotalSize( MEMORY_HEAP_TYPE type ) const
        {
            return Reinterpret()->GetMemoryHeapTotalSizeImpl( type );
        }
        size_t           GetMemoryHeapCurrentSize(MEMORY_HEAP_TYPE type) const
        {
            return Reinterpret()->GetMemoryHeapCurrentSizeImpl( type );
        }
        void* MapMemory(const SMapMemoryInfo& Info)
        {
            return Reinterpret()->MapMemoryImpl( Info );
        }

        void UnmapMemory( const SMapMemoryInfo& Info )
        {
            return Reinterpret()->UnmapMemoryImpl( Info );
        }

        void Reset(const NativeAPI::CommandBuffer& hCommandBuffer)
        {
            return Reinterpret()->ResetImpl( hCommandBuffer );
        }

        void BeginCommandBuffer(const NativeAPI::CommandBuffer& hCommandBuffer,
            const NativeAPI::CommandBufferPool& hCommandBufferPool)
        {
            return Reinterpret()->BeginCommandBufferImpl( hCommandBuffer, hCommandBufferPool );
        }
        void EndCommandBuffer(const NativeAPI::CommandBuffer& hCommandBuffer)
        {
            return Reinterpret()->EndCommandBufferImpl( hCommandBuffer );
        }

        void Reset(const NativeAPI::CommandBuffer& hCommandBuffer,
            const NativeAPI::CommandBufferPool& hCommandBufferPool)
        {
            return Reinterpret()->ResetImpl( hCommandBuffer, hCommandBufferPool );
        }

        void Barrier(const NativeAPI::CommandBuffer& hCommandBuffer, const SBarrierInfo& Info)
        {
            return Reinterpret()->BarrierImpl( hCommandBuffer, Info );
        }

        // Command Buffer
        void SetState(const NativeAPI::CommandBuffer& hCommandBuffer, const SViewportDesc& Desc)
        {
            return Reinterpret()->SetStateImpl( hCommandBuffer, Desc );
        }
        void SetState( const NativeAPI::CommandBuffer& hCommandBuffer, const SScissorDesc& Desc )
        {
            return Reinterpret()->SetStateImpl( hCommandBuffer, Desc );
        }

        void Draw(const NativeAPI::CommandBuffer& hCommandBuffer, const uint32_t& vertexCount,
            const uint32_t& instanceCount, const uint32_t& firstVertex, const uint32_t& firstInstance)
        {
            return Reinterpret()->DrawImpl( hCommandBuffer, vertexCount, instanceCount, firstVertex, firstInstance );
        }
        void DrawIndexed(const NativeAPI::CommandBuffer& hCommandBuffer, const SDrawParams& Params)
        {
            return Reinterpret()->DrawIndexedImpl( hCommandBuffer, Params );
        }
        void DrawMesh(const NativeAPI::CommandBuffer& hCommandBuffer, uint32_t width, uint32_t height,
            uint32_t depth)
        {
            return Reinterpret()->DrawMeshImpl( hCommandBuffer, width, height, depth );
        }

        // Dynamic rendering
        void BeginRenderPass(NativeAPI::CommandBuffer hCmdBuffer, const SBeginRenderPassInfo2& Info)
        {
            return Reinterpret()->BeginRenderPassImpl( hCmdBuffer, Info );
        }

        void BeginRenderPass( NativeAPI::CommandBuffer hCmdBuffer, const SBeginRenderPassInfo& Info )
        {
            return Reinterpret()->BeginRenderPassImpl( hCmdBuffer, Info );
        }

        void EndRenderPass( NativeAPI::CommandBuffer hCmdBuffer, NativeAPI::RenderPass hPass)
        {
            return Reinterpret()->EndRenderPassImpl( hCmdBuffer, hPass );
        }

        // Copy
        void Copy(const NativeAPI::CommandBuffer& hDDICmdBuffer, const SCopyTextureInfoEx& Info)
        {
            return Reinterpret()->CopyImpl( hDDICmdBuffer, Info );
        }
        void Copy(const NativeAPI::CommandBuffer& hCmdBuffer, const SCopyBufferInfo& Info)
        {
            return Reinterpret()->CopyImpl( hCmdBuffer, Info );
        }
        void Copy(const NativeAPI::CommandBuffer& hDDICmdBuffer, const SCopyBufferToTextureInfo& Info)
        {
            return Reinterpret()->CopyImpl( hDDICmdBuffer, Info );
        }
        void Blit(const NativeAPI::CommandBuffer& hAPICmdBuffer, const SBlitTextureInfo& Info)
        {
            return Reinterpret()->BlitImpl( hAPICmdBuffer, Info );
        }

        Result Submit( const SSubmitInfo& Info )
        {
            return Reinterpret()->SubmitImpl( Info );
        }

        Result Present( const SPresentData& Info )
        {
            return Reinterpret()->PresentImpl( Info );
        }

        Result CreateSwapChain(const SSwapChainDesc& Desc, SDDISwapChain* pInOut)
        {
            return Reinterpret()->CreateSwapChainImpl( Desc, nullptr, pInOut );
        }

        void DestroySwapChain( SDDISwapChain* pInOut = nullptr )
        {
            return Reinterpret()->DestroySwapChainImpl( pInOut );
        }

        Result ReCreateSwapChain( const SSwapChainDesc& Desc, SDDISwapChain* pOut )
        {
            return Reinterpret()->ReCreateSwapChainImpl( Desc, pOut );
        }

        Result QueryPresentSurfaceCaps( const NativeAPI::PresentSurface& hSurface, SPresentSurfaceCaps* pOut )
        {
            return Reinterpret()->QueryPresentSurfaceCapsImpl( hSurface, pOut );
        }
        Result GetCurrentBackBufferIndex(const SDDISwapChain& SwapChain, const SDDIGetBackBufferInfo& Info,
            uint32_t* pOut)
        {
            return Reinterpret()->GetCurrentBackBufferIndexImpl( SwapChain, Info, pOut );
        }

        static void Convert(const SClearValue& In, NativeAPI::ClearValue* pOut)
        {
            typename RenderApiT::Convert( In, pOut );
        }

        // Debug
        void BeginDebugInfo(const NativeAPI::CommandBuffer& hDDICmdBuff, const SDebugInfo* pInfo)
        {
            return Reinterpret()->BeginDebugInfoImpl( hDDICmdBuff, pInfo );
        }
        void EndDebugInfo(const NativeAPI::CommandBuffer& hDDICmdBuff)
        {
            return Reinterpret()->EndDebugInfoImpl( hDDICmdBuff );
        }
        void SetObjectDebugName(const uint64_t& handle, const uint32_t& objType, cstr_t pName) const
        {
            return Reinterpret()->SetObjectDebugNameImpl( handle, objType, pName );
        }
        void SetQueueDebugName(uint64_t handle, cstr_t pName) const
        {
            return Reinterpret()->SetQueueDebugNameImpl( handle, pName );
        }

        bool   IsSignaled(const NativeAPI::CPUFence& hFence) const
        {
            return Reinterpret()->IsSignaledImpl( hFence );
        }

        bool IsSignaled( const NativeAPI::Fence& hFence ) const
        {
            return Reinterpret()->IsSignaledImpl( hFence );
        }
        NativeAPI::FenceValue GetCompletedValue(const NativeAPI::Fence& hFence) const
        {
            return Reinterpret()->GetCompletedValueImpl( hFence );
        }
        void   Reset(NativeAPI::CPUFence* phFence)
        {
            return Reinterpret()->ResetImpl( phFence );
        }

        void Reset( NativeAPI::Fence* phFence, NativeAPI::FenceValue value )
        {
            return Reinterpret()->ResetImpl( phFence, value );
        }

        Result WaitForFences( const NativeAPI::CPUFence& hFence, uint64_t timeout ) const
        {
            return Reinterpret()->WaitForFencesImpl( hFence, timeout );
        }

        Result WaitForFence( NativeAPI::Fence      Fence, NativeAPI::FenceValue value ) const
        {
            return Reinterpret()->WaitForFenceImpl( Fence, value );
        }

        Result WaitForQueue( const NativeAPI::Queue& hQueue )
        {
            return Reinterpret()->WaitForQueueImpl( hQueue );
        }

        Result WaitForDevice()
        {
            return Reinterpret()->WaitForDeviceImpl();
        }
    };

} // namespace VKE::RenderSystem
