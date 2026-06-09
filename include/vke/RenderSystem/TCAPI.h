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

    public:

        RenderApiT* Reinterpret()
        {
            return reinterpret_cast< RenderApiT* >( this );
        }

        const RenderApiT* Reinterpret() const
        {
            return reinterpret_cast< const RenderApiT* >( this );
        }

        Result CreateDevice(const SCreateDeviceDesc& Info, CDeviceContext* pCtx)
        {
            return Reinterpret()->CreateDevice( Info, pCtx );
        }
        void   DestroyDevice()
        {
            Reinterpret()->DestroyDeviceImpl();
        }

        static Result Load(const SDDILoadInfo& Info, SDriverInfo* pOut)
        {
            return RenderApiT::LoadImpl( Info, pOut );
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

        NativeAPI::Buffer              CreateBuffer(const SBufferDesc& Desc)
        {
            return Reinterpret()->CreateBuffer( Desc, nullptr );
        }
        void                           DestroyBuffer(NativeAPI::Buffer* phBuffer)
        {
            return Reinterpret()->DestroyBuffer( phBuffer, nullptr );
        }
        NativeAPI::BufferView          CreateBufferView(const SBufferViewDesc& Desc)
        {
            return Reinterpret()->CreateBufferView( Desc, nullptr );
        }
        void                           DestroyBufferView(NativeAPI::BufferView* phBufferView)
        {
            return Reinterpret()->DestroyBufferView( phBufferView, nullptr );
        }
        Result                         GetTextureFormatProperties(const STextureDesc& Desc, STextureFormatProperties* pOut)
        {
            return Reinterpret()->GetTextureFormatProperties( Desc, pOut );
        }
        NativeAPI::Texture             CreateTexture(const STextureDesc& Desc)
        {
            return Reinterpret()->CreateTexture( Desc, nullptr );
        }

        void DestroyTexture( NativeAPI::Texture* phImage )
        {
            return Reinterpret()->DestroyTexture( phImage, nullptr );
        }
        NativeAPI::TextureView         CreateTextureView(const STextureViewDesc& Desc)
        {
            return Reinterpret()->CreateTextureView( Desc, nullptr );
        }

        void DestroyTextureView( NativeAPI::TextureView* phImageView )
        {
            return Reinterpret()->DestroyTextureView( phImageView, nullptr );
        }
        NativeAPI::Framebuffer         CreateFramebuffer(const SFramebufferDesc& Desc)
        {
            return Reinterpret()->CreateFramebuffer( Desc, nullptr );
        }

        void DestroyFramebuffer( NativeAPI::Framebuffer* phFramebuffer )
        {
            return Reinterpret()->DestroyFramebuffer( phFramebuffer, nullptr );
        }

        NativeAPI::CPUFence CreateFence( const SFenceDesc& Desc ) const
        {
            return Reinterpret()->CreateFence( Desc, nullptr );
        }
        NativeAPI::Fence               CreateFence2(const SFenceDesc& Desc) const
        {
            return Reinterpret()->CreateFence2( Desc );
        }
        void                           DestroyFence(NativeAPI::CPUFence* phFence)
        {
            return Reinterpret()->DestroyFence( phFence, nullptr );
        }

        void DestroyFence( NativeAPI::Fence* phFence )
        {
            return Reinterpret()->DestroyFence( phFence );
        }
        NativeAPI::GPUFence            CreateSemaphore(const SSemaphoreDesc& Desc) const
        {
            return Reinterpret()->CreateSemaphore( Desc, nullptr );
        }

        void DestroySemaphore( NativeAPI::GPUFence* phSemaphore )
        {
            return Reinterpret()->DestroySemaphore( phSemaphore, nullptr );
        }
        NativeAPI::RenderPass          CreateRenderPass(const SRenderPassDesc& Desc)
        {
            return Reinterpret()->CreateRenderPass( Desc, nullptr );
        }

        void DestroyRenderPass( NativeAPI::RenderPass* phPass )
        {
            return Reinterpret()->DestroyRenderPass( phPass, nullptr );
        }
        NativeAPI::CommandBufferPool   CreateCommandBufferPool(const SCommandBufferPoolDesc& Desc)
        {
            return Reinterpret()->CreateCommandBufferPool( Desc, nullptr );
        }

        void DestroyCommandBufferPool( NativeAPI::CommandBufferPool* phPool )
        {
            return Reinterpret()->DestroyCommandBufferPool( phPool, nullptr );
        }
        NativeAPI::DescriptorPool      CreateDescriptorPool(const SDescriptorPoolDesc& Desc)
        {
            return Reinterpret()->CreateDescriptorPool( Desc, nullptr );
        }

        void DestroyDescriptorPool( NativeAPI::DescriptorPool* phPool )
        {
            return Reinterpret()->DestroyDescriptorPool( phPool, nullptr );
        }
        NativeAPI::DescriptorSetLayout CreateDescriptorSetLayout(const SDescriptorSetLayoutDesc& Desc)
        {
            return Reinterpret()->CreateDescriptorSetLayout( Desc, nullptr );
        }

        void DestroyDescriptorSetLayout( NativeAPI::DescriptorSetLayout* phLayout )
        {
            return Reinterpret()->DestroyDescriptorSetLayout( phLayout, nullptr );
        }
        NativeAPI::Pipeline       CreatePipeline(const SPipelineDesc& Desc)
        {
            return Reinterpret()->CreatePipeline( Desc, nullptr );
        }

        void DestroyPipeline( NativeAPI::Pipeline* phPipeline )
        {
            return Reinterpret()->DestroyPipeline( phPipeline, nullptr );
        }
        NativeAPI::PipelineLayout CreatePipelineLayout(const SPipelineLayoutDesc& Desc)
        {
            return Reinterpret()->CreatePipelineLayout( Desc, nullptr );
        }

        void DestroyPipelineLayout( NativeAPI::PipelineLayout* phLayout )
        {
            return Reinterpret()->DestroyPipelineLayout( phLayout, nullptr );
        }
        NativeAPI::Shader         CreateShader(const SShaderData& Desc)
        {
            return Reinterpret()->CreateShader( Desc, nullptr );
        }

        void DestroyShader( NativeAPI::Shader* phShader )
        {
            return Reinterpret()->DestroyShader( phShader, nullptr );
        }
        NativeAPI::Sampler        CreateSampler(const SSamplerDesc& Desc)
        {
            return Reinterpret()->CreateSampler(Desc, nullptr);
        }

        void DestroySampler( NativeAPI::Sampler* phSampler )
        {
            return Reinterpret()->DestroySampler( phSampler, nullptr );
        }
        NativeAPI::Event          CreateEvent(const SEventDesc& Desc)
        {
            return Reinterpret()->CreateEvent( Desc, nullptr );
        }

        void DestroyEvent( NativeAPI::Event* phEvent )
        {
            return Reinterpret()->DestroyEvent( phEvent, nullptr );
        }

        Result AllocateObjects(const AllocateDescs::SDescSet& Info, NativeAPI::DescriptorSet* pSets)
        {
            return Reinterpret()->AllocateObjects( Info, pSets );
        }

        void FreeObjects( const FreeDescs::SDescSet& Sets )
        {
            return Reinterpret()->FreeObjects( Sets );
        }
        Result AllocateObjects(const SAllocateCommandBufferInfo& Info, NativeAPI::CommandBuffer* pBuffers)
        {
            return Reinterpret()->AllocateObjects( Info, pBuffers );
        }

        void FreeObjects( const SFreeCommandBufferInfo& Info )
        {
            return Reinterpret()->FreeObjects( Info );
        }

        Result GetBufferMemoryRequirements( const NativeAPI::Buffer& hBuffer, SAllocationMemoryRequirementInfo* pOut )
        {
            return Reinterpret()->GetBufferMemoryRequirements( hBuffer, pOut );
        }
        Result GetTextureMemoryRequirements(const NativeAPI::Texture& hTexture,
            SAllocationMemoryRequirementInfo* pOut)
        {
            return Reinterpret()->GetTextureMemoryRequirements( hTexture, pOut );
        }
        void   UpdateDesc(SBufferDesc* pInOut)
        {
            return Reinterpret()->UpdateDesc( pInOut );
        }

        void GetFormatFeatures( FORMAT fmt, STextureFormatFeatures* pOut ) const
        {
            return Reinterpret()->GetFormatFeatures( fmt, pOut );
        }

        Result Bind(RESOURCE_TYPE type, const SBindMemoryInfo& Info)
        {
            return Reinterpret()->Bind( type, Info );
        }
        void   Bind(const SBindPipelineInfo& Info)
        {
            return Reinterpret()->Bind( Info );
        }
        void   Bind(const SBindDDIDescriptorSetsInfo& Info)
        {
            return Reinterpret()->Bind( Info );
        }
        void   Bind(const SBindRenderPassInfo& Info)
        {
            return Reinterpret()->Bind( Info );
        }
        void   Bind(const NativeAPI::CommandBuffer& hDDICmdBuffer, const NativeAPI::Buffer& hDDIBuffer,
            const uint32_t offset)
        {
            return Reinterpret()->Bind( hDDICmdBuffer, hDDIBuffer, offset );
        }
        void   Bind(const NativeAPI::CommandBuffer& hDDICmdBuffer, const NativeAPI::Buffer& hDDIBuffer,
            const uint32_t offset, const INDEX_TYPE& type)
        {
            return Reinterpret()->Bind( hDDICmdBuffer, hDDIBuffer, offset, type );
        }
        void   UnbindPipeline(const NativeAPI::CommandBuffer& hCmdBuffer, const NativeAPI::Pipeline& hPipeline)
        {
            return Reinterpret()->UnbindPipeline( hCmdBuffer, hPipeline );
        }
        void   UnbindRenderPass(const NativeAPI::CommandBuffer& hCmdBuffer, const NativeAPI::RenderPass& hRenderPass)
        {
            return Reinterpret()->UnbindRenderPass( hCmdBuffer, hRenderPass );
        }

        void Free(NativeAPI::Memory* phMemory = nullptr)
        {
            return Reinterpret()->Free( phMemory );
        }

        void Update(const SUpdateBufferDescriptorSetInfo& Info)
        {
            return Reinterpret()->Update( Info );
        }
        void Update(const SUpdateTextureDescriptorSetInfo& Info)
        {
            return Reinterpret()->Update( Info );
        }
        void Update(const NativeAPI::DescriptorSet& hDDISet, const SUpdateBindingsHelper& Info)
        {
            return Reinterpret()->Update( hDDISet, Info );
        }

        void Update( const NativeAPI::DescriptorSet& hDDISrcSet, NativeAPI::DescriptorSet* phDDIDstOut )
        {
            return Reinterpret()->Update( hDDISrcSet, phDDIDstOut );
        }

        Result           Allocate(const SAllocateMemoryDesc& Desc, SAllocateMemoryData* pOut)
        {
            return Reinterpret()->Allocate( Desc, pOut );
        }

        MEMORY_HEAP_TYPE GetMemoryHeapType( MEMORY_USAGE usage ) const
        {
            return Reinterpret()->GetMemoryHeapType( usage );
        }

        size_t GetMemoryHeapTotalSize( MEMORY_HEAP_TYPE type ) const
        {
            return Reinterpret()->GetMemoryHeapTotalSize( type );
        }
        size_t           GetMemoryHeapCurrentSize(MEMORY_HEAP_TYPE type) const
        {
            return Reinterpret()->GetMemoryHeapCurrentSize( type );
        }
        void* MapMemory(const SMapMemoryInfo& Info)
        {
            return Reinterpret()->MapMemory( Info );
        }
        void             UnmapMemory(const NativeAPI::Memory& hDDIMemory)
        {
            return Reinterpret()->UnmapMemory( hDDIMemory );
        }

        void Reset(const NativeAPI::CommandBuffer& hCommandBuffer)
        {
            return Reinterpret()->Reset( hCommandBuffer );
        }
        void BeginCommandBuffer(const NativeAPI::CommandBuffer& hCommandBuffer)
        {
            return Reinterpret()->BeginCommandBuffer( hCommandBuffer );
        }
        void EndCommandBuffer(const NativeAPI::CommandBuffer& hCommandBuffer)
        {
            return Reinterpret()->EndCommandBuffer( hCommandBuffer );
        }

        void Barrier(const NativeAPI::CommandBuffer& hCommandBuffer, const SBarrierInfo& Info)
        {
            return Reinterpret()->Barrier( hCommandBuffer, Info );
        }

        // Command Buffer
        void SetState(const NativeAPI::CommandBuffer& hCommandBuffer, const SViewportDesc& Desc)
        {
            return Reinterpret()->SetState( hCommandBuffer, Desc );
        }
        void SetState( const NativeAPI::CommandBuffer& hCommandBuffer, const SScissorDesc& Desc )
        {
            return Reinterpret()->SetState( hCommandBuffer, Desc );
        }

        void Draw(const NativeAPI::CommandBuffer& hCommandBuffer, const uint32_t& vertexCount,
            const uint32_t& instanceCount, const uint32_t& firstVertex, const uint32_t& firstInstance)
        {
            return Reinterpret()->Draw( hCommandBuffer, vertexCount, instanceCount, firstVertex, firstInstance );
        }
        void DrawIndexed(const NativeAPI::CommandBuffer& hCommandBuffer, const SDrawParams& Params)
        {
            return Reinterpret()->DrawIndexed( hCommandBuffer, Params );
        }
        void DrawMesh(const NativeAPI::CommandBuffer& hCommandBuffer, uint32_t width, uint32_t height,
            uint32_t depth)
        {
            return Reinterpret()->DrawMesh( hCommandBuffer, width, height, depth );
        }

        // Dynamic rendering
        void BeginRenderPass(NativeAPI::CommandBuffer hCmdBuffer, const SBeginRenderPassInfo2& Info)
        {
            return Reinterpret()->BeginRenderPass( hCmdBuffer, Info );
        }
        void EndRenderPass(NativeAPI::CommandBuffer hCmdBuffer)
        {
            return Reinterpret()->EndRenderPass( hCmdBuffer );
        }

        // Copy
        void Copy(const NativeAPI::CommandBuffer& hDDICmdBuffer, const SCopyTextureInfoEx& Info)
        {
            return Reinterpret()->Copy( hDDICmdBuffer, Info );
        }
        void Copy(const NativeAPI::CommandBuffer& hCmdBuffer, const SCopyBufferInfo& Info)
        {
            return Reinterpret()->Copy( hCmdBuffer, Info );
        }
        void Copy(const NativeAPI::CommandBuffer& hDDICmdBuffer, const SCopyBufferToTextureInfo& Info)
        {
            return Reinterpret()->Copy( hDDICmdBuffer, Info );
        }
        void Blit(const NativeAPI::CommandBuffer& hAPICmdBuffer, const SBlitTextureInfo& Info)
        {
            return Reinterpret()->Blit( hAPICmdBuffer, Info );
        }

        Result Submit( const SSubmitInfo& Info )
        {
            return Reinterpret()->Submit( Info );
        }

        Result Present( const SPresentData& Info )
        {
            return Reinterpret()->Present( Info );
        }

        Result CreateSwapChain(const SSwapChainDesc& Desc, SDDISwapChain* pInOut)
        {
            return Reinterpret()->CreateSwapChain( Desc, pInOut, nullptr );
        }

        void DestroySwapChain( SDDISwapChain* pInOut = nullptr )
        {
            return Reinterpret()->DestroySwapChain( pInOut );
        }

        Result ReCreateSwapChain( const SSwapChainDesc& Desc, SDDISwapChain* pOut )
        {
            return Reinterpret()->ReCreateSwapChain( Desc, pOut );
        }

        Result QueryPresentSurfaceCaps( const NativeAPI::PresentSurface& hSurface, SPresentSurfaceCaps* pOut )
        {
            return Reinterpret()->QueryPresentSurfaceCaps( hSurface, pOut );
        }
        Result GetCurrentBackBufferIndex(const SDDISwapChain& SwapChain, const SDDIGetBackBufferInfo& Info,
            uint32_t* pOut)
        {
            return Reinterpret()->GetCurrentBackBufferIndex( SwapChain, Info, pOut );
        }

        static void Convert(const SClearValue& In, NativeAPI::ClearValue* pOut)
        {
            typename RenderApiT::Convert( In, pOut );
        }

        // Debug
        void BeginDebugInfo(const NativeAPI::CommandBuffer& hDDICmdBuff, const SDebugInfo* pInfo)
        {
            return Reinterpret()->BeginDebugInfo( hDDICmdBuff, pInfo );
        }
        void EndDebugInfo(const NativeAPI::CommandBuffer& hDDICmdBuff)
        {
            return Reinterpret()->EndDebugInfo( hDDICmdBuff );
        }
        void SetObjectDebugName(const uint64_t& handle, const uint32_t& objType, cstr_t pName) const
        {
            return Reinterpret()->SetObjectDebugName( handle, objType, pName );
        }
        void SetQueueDebugName(uint64_t handle, cstr_t pName) const
        {
            return Reinterpret()->SetQueueDebugName( handle, pName );
        }

        bool   IsSignaled(const NativeAPI::CPUFence& hFence) const
        {
            return Reinterpret()->IsSignaled( hFence );
        }

        bool IsSignaled( const NativeAPI::Fence& hFence ) const
        {
            return Reinterpret()->IsSignaled( hFence );
        }
        NativeAPI::FenceValue GetCompletedValue(const NativeAPI::Fence& hFence) const
        {
            return Reinterpret()->GetCompletedValue( hFence );
        }
        void   Reset(NativeAPI::CPUFence* phFence)
        {
            return Reinterpret()->Reset( phFence );
        }

        void Reset( NativeAPI::Fence* phFence, NativeAPI::FenceValue value )
        {
            return Reinterpret()->Reset( phFence, value );
        }

        Result WaitForFences( const NativeAPI::CPUFence& hFence, uint64_t timeout ) const
        {
            return Reinterpret()->WaitForFences( hFence, timeout );
        }

        Result WaitForFence( NativeAPI::Fence      Fence, NativeAPI::FenceValue value ) const
        {
            return Reinterpret()->WaitForFence( Fence, value );
        }

        Result WaitForQueue( const NativeAPI::Queue& hQueue )
        {
            return Reinterpret()->WaitForQueue( hQueue );
        }

        Result WaitForDevice()
        {
            return Reinterpret()->WaitForDevice();
        }
    };

} // namespace VKE::RenderSystem
