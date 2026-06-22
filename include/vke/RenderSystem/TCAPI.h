#pragma once

#include "RenderSystem/Common.h"
#include "Core/Memory/CFreeListPool.h"
#include "Core/Memory/CMemoryPoolManager.h"

namespace VKE::RenderSystem
{
    // Forward declarations
    class CDeviceContext;

    template<class RenderApiT>
    class VKE_API TCRHI
    {
    protected:

        RenderApiT* Reinterpret()
        {
            RenderApiT* pRet = static_cast< RenderApiT* >( this );
            return pRet;
        }

        const RenderApiT* Reinterpret() const
        {
            return static_cast< const RenderApiT* >( this );
        }

        public:

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
            return RenderApiT::LoadImpl( Info, pOut );
        }

        const NativeTypes::Device GetDevice() const
        {
            return Reinterpret()->GetDeviceImpl();
        }

        const NativeTypes::Adapter& GetAdapter() const
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

        NativeTypes::Buffer CreateBuffer( const SBufferDesc& Desc, const SBindMemoryInfo& MemInfo )
        {
            return Reinterpret()->CreateBufferImpl( Desc, MemInfo );
        }
        void                           DestroyBuffer(NativeTypes::Buffer* phBuffer)
        {
            return Reinterpret()->DestroyBufferImpl( phBuffer, nullptr );
        }
        NativeTypes::BufferView          CreateBufferView(const SBufferViewDesc& Desc)
        {
            return Reinterpret()->CreateBufferView( Desc, nullptr );
        }
        void                           DestroyBufferView(NativeTypes::BufferView* phBufferView)
        {
            return Reinterpret()->DestroyBufferViewImpl( phBufferView, nullptr );
        }
        Result                         GetTextureFormatProperties(const STextureDesc& Desc, STextureFormatProperties* pOut)
        {
            return Reinterpret()->GetTextureFormatPropertiesImpl( Desc, pOut );
        }

        NativeTypes::Texture CreateTexture( const STextureDesc& Desc, const SBindMemoryInfo& MemInfo )
        {
            return Reinterpret()->CreateTextureImpl( Desc, MemInfo );
        }

        void DestroyTexture( NativeTypes::Texture* phImage )
        {
            return Reinterpret()->DestroyTextureImpl( phImage, nullptr );
        }
        NativeTypes::TextureView         CreateTextureView(const STextureViewDesc& Desc)
        {
            return Reinterpret()->CreateTextureViewImpl( Desc, nullptr );
        }

        void DestroyTextureView( NativeTypes::TextureView* phImageView )
        {
            return Reinterpret()->DestroyTextureViewImpl( phImageView, nullptr );
        }
        NativeTypes::Framebuffer         CreateFramebuffer(const SFramebufferDesc& Desc)
        {
            return Reinterpret()->CreateFramebufferImpl( Desc, nullptr );
        }

        void DestroyFramebuffer( NativeTypes::Framebuffer* phFramebuffer )
        {
            return Reinterpret()->DestroyFramebufferImpl( phFramebuffer, nullptr );
        }

        NativeTypes::CPUFence CreateFence( const SFenceDesc& Desc ) const
        {
            return Reinterpret()->CreateFenceImpl( Desc, nullptr );
        }
        NativeTypes::Fence               CreateFence2(const SFenceDesc& Desc) const
        {
            return Reinterpret()->CreateFence2Impl( Desc );
        }
        void                           DestroyFence(NativeTypes::CPUFence* phFence)
        {
            return Reinterpret()->DestroyFenceImpl( phFence, nullptr );
        }

        void DestroyFence( NativeTypes::Fence* phFence )
        {
            return Reinterpret()->DestroyFenceImpl( phFence );
        }
        NativeTypes::GPUFence            CreateGPUFence(const SSemaphoreDesc& Desc) const
        {
            return Reinterpret()->CreateSemaphoreImpl( Desc, nullptr );
        }

        void DestroyGPUFence( NativeTypes::GPUFence* phSemaphore )
        {
            return Reinterpret()->DestroySemaphoreImpl( phSemaphore, nullptr );
        }
        NativeTypes::RenderPass          CreateRenderPass(const SRenderPassDesc& Desc)
        {
            return Reinterpret()->CreateRenderPassImpl( Desc, nullptr );
        }

        void DestroyRenderPass( NativeTypes::RenderPass* phPass )
        {
            return Reinterpret()->DestroyRenderPassImpl( phPass, nullptr );
        }
        NativeTypes::CommandBufferPool   CreateCommandBufferPool(const SCommandBufferPoolDesc& Desc)
        {
            return Reinterpret()->CreateCommandBufferPoolImpl( Desc, nullptr );
        }

        void DestroyCommandBufferPool( NativeTypes::CommandBufferPool* phPool )
        {
            return Reinterpret()->DestroyCommandBufferPoolImpl( phPool, nullptr );
        }
        NativeTypes::DescriptorPool      CreateDescriptorPool(const SDescriptorPoolDesc& Desc)
        {
            return Reinterpret()->CreateDescriptorPoolImpl( Desc, nullptr );
        }

        void DestroyDescriptorPool( NativeTypes::DescriptorPool* phPool )
        {
            return Reinterpret()->DestroyDescriptorPoolImpl( phPool, nullptr );
        }
        NativeTypes::DescriptorSetLayout CreateDescriptorSetLayout(const SDescriptorSetLayoutDesc& Desc)
        {
            return Reinterpret()->CreateDescriptorSetLayoutImpl( Desc, nullptr );
        }

        void DestroyDescriptorSetLayout( NativeTypes::DescriptorSetLayout* phLayout )
        {
            return Reinterpret()->DestroyDescriptorSetLayoutImpl( phLayout, nullptr );
        }
        NativeTypes::Pipeline       CreatePipeline(const SPipelineDesc& Desc)
        {
            return Reinterpret()->CreatePipelineImpl( Desc, nullptr );
        }

        void DestroyPipeline( NativeTypes::Pipeline* phPipeline )
        {
            return Reinterpret()->DestroyPipelineImpl( phPipeline, nullptr );
        }
        NativeTypes::PipelineLayout CreatePipelineLayout(const SPipelineLayoutDesc& Desc)
        {
            return Reinterpret()->CreatePipelineLayoutImpl( Desc, nullptr );
        }

        void DestroyPipelineLayout( NativeTypes::PipelineLayout* phLayout )
        {
            return Reinterpret()->DestroyPipelineLayoutImpl( phLayout, nullptr );
        }
        NativeTypes::Shader         CreateShader(const SShaderData& Desc)
        {
            return Reinterpret()->CreateShaderImpl( Desc, nullptr );
        }

        void DestroyShader( NativeTypes::Shader* phShader )
        {
            return Reinterpret()->DestroyShaderImpl( phShader, nullptr );
        }
        NativeTypes::Sampler        CreateSampler(const SSamplerDesc& Desc)
        {
            return Reinterpret()->CreateSamplerImpl( Desc, nullptr );
        }

        void DestroySampler( NativeTypes::Sampler* phSampler )
        {
            return Reinterpret()->DestroySamplerImpl( phSampler, nullptr );
        }
        NativeTypes::Event          CreateEvent(const SEventDesc& Desc)
        {
            return Reinterpret()->CreateEventImpl( Desc, nullptr );
        }

        void DestroyEvent( NativeTypes::Event* phEvent )
        {
            return Reinterpret()->DestroyEventImpl( phEvent, nullptr );
        }

        Result CreateDescriptorSets( const AllocateDescs::SDescSet& Info, NativeTypes::DescriptorSet* pSets )
        {
            return Reinterpret()->CreateDescriptorSetsImpl( Info, pSets );
        }

        void FreeObjects( const FreeDescs::SDescSet& Sets )
        {
            return Reinterpret()->FreeObjectsImpl( Sets );
        }

        Result CreateCommandBuffers( const SAllocateCommandBufferInfo& Info, NativeTypes::CommandBuffer* pBuffers )
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
        void   Bind(const NativeTypes::CommandBuffer& hDDICmdBuffer, const NativeTypes::Buffer& hDDIBuffer,
            const uint32_t offset)
        {
            return Reinterpret()->BindImpl( hDDICmdBuffer, hDDIBuffer, offset );
        }
        void   Bind(const NativeTypes::CommandBuffer& hDDICmdBuffer, const NativeTypes::Buffer& hDDIBuffer,
            const uint32_t offset, const INDEX_TYPE& type)
        {
            return Reinterpret()->BindImpl( hDDICmdBuffer, hDDIBuffer, offset, type );
        }
        void   UnbindPipeline(const NativeTypes::CommandBuffer& hCmdBuffer, const NativeTypes::Pipeline& hPipeline)
        {
            return Reinterpret()->UnbindPipelineImpl( hCmdBuffer, hPipeline );
        }
        void   UnbindRenderPass(const NativeTypes::CommandBuffer& hCmdBuffer, const NativeTypes::RenderPass& hRenderPass)
        {
            return Reinterpret()->UnbindRenderPassImpl( hCmdBuffer, hRenderPass );
        }

        void Free(NativeTypes::MemoryHeap* phMemory = nullptr)
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
        void Update(const NativeTypes::DescriptorSet& hDDISet, const SUpdateBindingsHelper& Info)
        {
            return Reinterpret()->UpdateImpl( hDDISet, Info );
        }

        void Update( const NativeTypes::DescriptorSet& hDDISrcSet, NativeTypes::DescriptorSet* phDDIDstOut )
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

        void Reset(const NativeTypes::CommandBuffer& hCommandBuffer)
        {
            return Reinterpret()->ResetImpl( hCommandBuffer );
        }

        void BeginCommandBuffer(const NativeTypes::CommandBuffer& hCommandBuffer,
            const NativeTypes::CommandBufferPool& hCommandBufferPool)
        {
            return Reinterpret()->BeginCommandBufferImpl( hCommandBuffer, hCommandBufferPool );
        }
        void EndCommandBuffer(const NativeTypes::CommandBuffer& hCommandBuffer)
        {
            return Reinterpret()->EndCommandBufferImpl( hCommandBuffer );
        }

        void Reset(const NativeTypes::CommandBuffer& hCommandBuffer,
            const NativeTypes::CommandBufferPool& hCommandBufferPool)
        {
            return Reinterpret()->ResetImpl( hCommandBuffer, hCommandBufferPool );
        }

        void Barrier(const NativeTypes::CommandBuffer& hCommandBuffer, const SBarrierInfo& Info)
        {
            return Reinterpret()->BarrierImpl( hCommandBuffer, Info );
        }

        // Command Buffer
        void SetState(const NativeTypes::CommandBuffer& hCommandBuffer, const SViewportDesc& Desc)
        {
            return Reinterpret()->SetStateImpl( hCommandBuffer, Desc );
        }
        void SetState( const NativeTypes::CommandBuffer& hCommandBuffer, const SScissorDesc& Desc )
        {
            return Reinterpret()->SetStateImpl( hCommandBuffer, Desc );
        }

        void Draw(const NativeTypes::CommandBuffer& hCommandBuffer, const uint32_t& vertexCount,
            const uint32_t& instanceCount, const uint32_t& firstVertex, const uint32_t& firstInstance)
        {
            return Reinterpret()->DrawImpl( hCommandBuffer, vertexCount, instanceCount, firstVertex, firstInstance );
        }
        void DrawIndexed(const NativeTypes::CommandBuffer& hCommandBuffer, const SDrawParams& Params)
        {
            return Reinterpret()->DrawIndexedImpl( hCommandBuffer, Params );
        }
        void DrawMesh(const NativeTypes::CommandBuffer& hCommandBuffer, uint32_t width, uint32_t height,
            uint32_t depth)
        {
            return Reinterpret()->DrawMeshImpl( hCommandBuffer, width, height, depth );
        }

        // Dynamic rendering
        void BeginRenderPass(NativeTypes::CommandBuffer hCmdBuffer, const SBeginRenderPassInfo2& Info)
        {
            return Reinterpret()->BeginRenderPassImpl( hCmdBuffer, Info );
        }

        void BeginRenderPass( NativeTypes::CommandBuffer hCmdBuffer, const SBeginRenderPassInfo& Info )
        {
            return Reinterpret()->BeginRenderPassImpl( hCmdBuffer, Info );
        }

        void EndRenderPass( NativeTypes::CommandBuffer hCmdBuffer, NativeTypes::RenderPass hPass)
        {
            return Reinterpret()->EndRenderPassImpl( hCmdBuffer, hPass );
        }

        // Copy
        void Copy(const NativeTypes::CommandBuffer& hDDICmdBuffer, const SCopyTextureInfoEx& Info)
        {
            return Reinterpret()->CopyImpl( hDDICmdBuffer, Info );
        }
        void Copy(const NativeTypes::CommandBuffer& hCmdBuffer, const SCopyBufferInfo& Info)
        {
            return Reinterpret()->CopyImpl( hCmdBuffer, Info );
        }
        void Copy(const NativeTypes::CommandBuffer& hDDICmdBuffer, const SCopyBufferToTextureInfo& Info)
        {
            return Reinterpret()->CopyImpl( hDDICmdBuffer, Info );
        }
        void Blit(const NativeTypes::CommandBuffer& hAPICmdBuffer, const SBlitTextureInfo& Info)
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

        Result QueryPresentSurfaceCaps( const NativeTypes::PresentSurface& hSurface, SPresentSurfaceCaps* pOut )
        {
            return Reinterpret()->QueryPresentSurfaceCapsImpl( hSurface, pOut );
        }
        Result GetCurrentBackBufferIndex(const SDDISwapChain& SwapChain, const SDDIGetBackBufferInfo& Info,
            uint32_t* pOut)
        {
            return Reinterpret()->GetCurrentBackBufferIndexImpl( SwapChain, Info, pOut );
        }

        /*static void Convert(const SClearValue& In, NativeTypes::ClearValue* pOut)
        {
            typename RenderApiT::Convert( In, pOut );
        }*/

        // Debug
        void BeginDebugInfo(const NativeTypes::CommandBuffer& hDDICmdBuff, const SDebugInfo* pInfo)
        {
            return Reinterpret()->BeginDebugInfoImpl( hDDICmdBuff, pInfo );
        }
        void EndDebugInfo(const NativeTypes::CommandBuffer& hDDICmdBuff)
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

        bool   IsSignaled(const NativeTypes::CPUFence& hFence) const
        {
            return Reinterpret()->IsSignaledImpl( hFence );
        }

        bool IsSignaled( const NativeTypes::Fence& hFence ) const
        {
            return Reinterpret()->IsSignaledImpl( hFence );
        }
        NativeTypes::FenceValue GetCompletedValue(const NativeTypes::Fence& hFence) const
        {
            return Reinterpret()->GetCompletedValueImpl( hFence );
        }
        void   Reset(NativeTypes::CPUFence* phFence)
        {
            return Reinterpret()->ResetImpl( phFence );
        }

        void Reset( NativeTypes::Fence* phFence, NativeTypes::FenceValue value )
        {
            return Reinterpret()->ResetImpl( phFence, value );
        }

        Result WaitForFences( const NativeTypes::CPUFence& hFence, uint64_t timeout ) const
        {
            return Reinterpret()->WaitForFencesImpl( hFence, timeout );
        }

        Result WaitForFence( NativeTypes::Fence      Fence, NativeTypes::FenceValue value ) const
        {
            return Reinterpret()->WaitForFenceImpl( Fence, value );
        }

        Result WaitForQueue( const NativeTypes::Queue& hQueue )
        {
            return Reinterpret()->WaitForQueueImpl( hQueue );
        }

        Result WaitForDevice()
        {
            return Reinterpret()->WaitForDeviceImpl();
        }
    };

} // namespace VKE::RenderSystem
