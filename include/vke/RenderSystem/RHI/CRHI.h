#pragma once

#include "RenderSystem/Common.h"

#include "Core/Memory/CFreeListPool.h"
#include "Core/Memory/CMemoryPoolManager.h"

namespace VKE::RenderSystem::RHI
{
    // Forward declaration
    struct SImplementation;

    class VKE_API CRHI
    {
    private:
        SImplementation* m_pImplementation;

        // TODO(blturkot): Remove this after refactor of each RHI
        CDeviceContext*   m_pCtx;
        SDeviceInfo       m_DeviceInfo;
        SDeviceProperties m_DeviceProperties;

        struct
        {
            uint32_t         TypeToIndex[ MemoryHeapTypes::_MAX_COUNT ];
            MEMORY_HEAP_TYPE IndexToType[ 16 ];
        } HeapMap;

    public:
        CRHI();
        ~CRHI();

        static Result Load( const SRHILoadInfo& Info, SDriverInfo* pOut );

        Result CreateDevice( const SCreateDeviceDesc& Info, CDeviceContext* pCtx );

        void DestroyDevice();

        const RHI::Device GetDevice() const;

        const RHI::Adapter GetAdapter() const;

        const QueueFamilyInfoArray& GetDeviceQueueInfos() const;

        static Result QueryAdapters( AdapterInfoArray* pOut );

        void QueryDeviceInfo( SDeviceInfo* pOut );

        RHI::Buffer            CreateBuffer( const SBufferDesc& Desc, const SBindMemoryInfo& MemInfo );
        RHI::BufferView        CreateBufferView( const SBufferViewDesc& Desc, const void* pAllocator = nullptr );
        RHI::CommandBufferPool CreateCommandBufferPool( const SCommandBufferPoolDesc& Desc,
                                                        const void*                   pAllocator = nullptr );
        RHI::DescriptorPool CreateDescriptorPool( const SDescriptorPoolDesc& Desc, const void* pAllocator = nullptr );
        RHI::DescriptorSetLayout CreateDescriptorSetLayout( const SDescriptorSetLayoutDesc& Desc,
                                                            const void*                     pAllocator = nullptr );
        RHI::Event               CreateEvent( const SEventDesc& Desc, const void* pAllocator = nullptr );
        RHI::CPUFence            CreateFence( const SFenceDesc& Desc, const void* pAllocator = nullptr ) const;
        RHI::Fence               CreateFence2( const SFenceDesc& Desc ) const;
        RHI::Framebuffer         CreateFramebuffer( const SFramebufferDesc& Desc, const void* pAllocator = nullptr );
        RHI::GPUFence            CreateGPUFence( const SSemaphoreDesc& Desc, const void* pAllocator = nullptr ) const;
        RHI::Pipeline            CreatePipeline( const SPipelineDesc& Desc, const void* pAllocator = nullptr );
        RHI::PipelineLayout CreatePipelineLayout( const SPipelineLayoutDesc& Desc, const void* pAllocator = nullptr );
        RHI::RenderPass     CreateRenderPass( const SRenderPassDesc& Desc, const void* pAllocator = nullptr );
        RHI::Sampler        CreateSampler( const SSamplerDesc& Desc, const void* pAllocator = nullptr );
        RHI::Shader         CreateShader( const SShaderData& Desc, const void* pAllocator = nullptr );
        RHI::Texture        CreateTexture( const STextureDesc& Desc, const SBindMemoryInfo& MemInfo );
        RHI::TextureView    CreateTextureView( const STextureViewDesc& Desc, const void* pAllocator = nullptr );

        void DestroyBuffer( RHI::Buffer* phBuffer, const void* pAllocator = nullptr );
        void DestroyBufferView( RHI::BufferView* phBufferView, const void* pAllocator = nullptr );
        void DestroyCommandBufferPool( RHI::CommandBufferPool* phPool, const void* pAllocator = nullptr );
        void DestroyDescriptorPool( RHI::DescriptorPool* phPool, const void* pAllocator = nullptr );
        void DestroyDescriptorSetLayout( RHI::DescriptorSetLayout* phLayout, const void* pAllocator = nullptr );
        void DestroyEvent( RHI::Event* phEvent, const void* pAllocator = nullptr );
        void DestroyFence( RHI::CPUFence* phFence, const void* pAllocator = nullptr );
        void DestroyFence( RHI::Fence* phFence );
        void DestroyFramebuffer( RHI::Framebuffer* phFramebuffer, const void* pAllocator = nullptr );
        void DestroyGPUFence( RHI::GPUFence* phSemaphore, const void* pAllocator = nullptr );
        void DestroyPipeline( RHI::Pipeline* phPipeline, const void* pAllocator = nullptr );
        void DestroyPipelineLayout( RHI::PipelineLayout* phLayout, const void* pAllocator = nullptr );
        void DestroyRenderPass( RHI::RenderPass* phPass, const void* pAllocator = nullptr );
        void DestroySampler( RHI::Sampler* phSampler, const void* pAllocator = nullptr );
        void DestroyShader( RHI::Shader* phShader, const void* pAllocator = nullptr );
        void DestroyTexture( RHI::Texture* phImage, const void* pAllocator = nullptr );
        void DestroyTextureView( RHI::TextureView* phImageView, const void* pAllocator = nullptr );

        Result Bind( RESOURCE_TYPE type, const SBindMemoryInfo& Info );
        void   Bind( const SBindPipelineInfo& Info );
        void   Bind( const SBindRHIDescriptorSetsInfo& Info );
        void   Bind( const SBindRenderPassInfo& Info );
        void   Bind( const SBindVertexBufferInfo& Info );
        void   Bind( const RHI::CommandBuffer& hRHICmdBuffer, const RHI::Buffer& hRHIBuffer, const uint32_t offset,
                     const INDEX_TYPE& type );
        Result CreateCommandBuffers( const SAllocateCommandBufferInfo& Info, RHI::CommandBuffer* pBuffers );
        Result CreateDescriptorSets( const AllocateDescs::SDescSet& Info, RHI::DescriptorSet* pSets );
        Result CreateSwapChain( const SSwapChainDesc& Desc, SRHISwapChain* pInOut, const void* pAllocator = nullptr );
        void   DestroySwapChain( SRHISwapChain* pInOut = nullptr, const void* pAllocator = nullptr );
        Result GetBufferMemoryRequirements( const SBufferDesc& Desc, SAllocationMemoryRequirementInfo* pOut );
        Result GetTextureFormatProperties( const STextureDesc& Desc, STextureFormatProperties* pOut );
        Result GetTextureMemoryRequirements( const STextureDesc& Desc, SAllocationMemoryRequirementInfo* pOut );

        void FreeObjects( const FreeDescs::SDescSet& Sets );
        void FreeObjects( const SFreeCommandBufferInfo& Info );

        void UpdateDesc( SBufferDesc* pInOut );

        void GetFormatFeatures( FORMAT fmt, STextureFormatFeatures* pOut ) const;

        void UnbindPipeline( const RHI::CommandBuffer& hCmdBuffer, const RHI::Pipeline& hPipeline );

        void UnbindRenderPass( const RHI::CommandBuffer& hCmdBuffer, const RHI::RenderPass& hRenderPass );

        void Free( RHI::MemoryHeap* phMemory = nullptr, const void* pAllocator = nullptr );

        void Update( const SUpdateBufferDescriptorSetInfo& Info );

        void Update( const SUpdateTextureDescriptorSetInfo& Info );

        void Update( const RHI::DescriptorSet& hRHISet, const SUpdateBindingsHelper& Info );

        void Update( const RHI::DescriptorSet& hRHISrcSet, RHI::DescriptorSet* phRHIDstOut );

        Result Allocate( const SAllocateMemoryDesc& Desc, SAllocateMemoryData* pOut );

        MEMORY_HEAP_TYPE GetMemoryHeapType( MEMORY_USAGE usage ) const;

        size_t GetMemoryHeapTotalSize( MEMORY_HEAP_TYPE type ) const;

        size_t GetMemoryHeapCurrentSize( MEMORY_HEAP_TYPE type ) const;

        void* MapMemory( const SMapMemoryInfo& Info );

        void UnmapMemory( const SMapMemoryInfo& Info );

        void Reset( const RHI::CommandBuffer& hCommandBuffer );

        void BeginCommandBuffer( const RHI::CommandBuffer&     hCommandBuffer,
                                 const RHI::CommandBufferPool& hCommandBufferPool );

        void EndCommandBuffer( const RHI::CommandBuffer& hCommandBuffer );

        void Reset( const RHI::CommandBuffer& hCommandBuffer, const RHI::CommandBufferPool& hCommandBufferPool );

        void Barrier( const RHI::CommandBuffer& hCommandBuffer, const SBarrierInfo& Info );

        // Command Buffer
        void SetState( const RHI::CommandBuffer& hCommandBuffer, const SViewportDesc& Desc );

        void SetState( const RHI::CommandBuffer& hCommandBuffer, const SScissorDesc& Desc );

        void Draw( const RHI::CommandBuffer& hCommandBuffer, const uint32_t& vertexCount, const uint32_t& instanceCount,
                   const uint32_t& firstVertex, const uint32_t& firstInstance );

        void DrawIndexed( const RHI::CommandBuffer& hCommandBuffer, const SDrawParams& Params );

        void DrawMesh( const RHI::CommandBuffer& hCommandBuffer, uint32_t width, uint32_t height, uint32_t depth );

        // Dynamic rendering
        void BeginRenderPass( RHI::CommandBuffer hCmdBuffer, const SBeginRenderPassInfo2& Info );

        void BeginRenderPass( RHI::CommandBuffer hCmdBuffer, const SBeginRenderPassInfo& Info );

        void EndRenderPass( RHI::CommandBuffer hCmdBuffer, RHI::RenderPass hPass );

        // Copy
        void Copy( const RHI::CommandBuffer& hRHICmdBuffer, const SCopyTextureInfoEx& Info );

        void Copy( const RHI::CommandBuffer& hCmdBuffer, const SCopyBufferInfo& Info );

        void Copy( const RHI::CommandBuffer& hRHICmdBuffer, const SCopyBufferToTextureInfo& Info );

        void Blit( const RHI::CommandBuffer& hAPICmdBuffer, const SBlitTextureInfo& Info );

        Result Submit( const SSubmitInfo& Info );

        Result Present( const SPresentData& Info );

        Result ReCreateSwapChain( const SSwapChainDesc& Desc, SRHISwapChain* pOut );

        Result QueryPresentSurfaceCaps( const RHI::PresentSurface& hSurface, SPresentSurfaceCaps* pOut );

        Result GetCurrentBackBufferIndex( const SRHISwapChain& SwapChain, const SRHIGetBackBufferInfo& Info,
                                          uint32_t* pOut );

        // Debug
        void BeginDebugInfo( const RHI::CommandBuffer& hRHICmdBuff, const SDebugInfo* pInfo );

        void EndDebugInfo( const RHI::CommandBuffer& hRHICmdBuff );

        void SetObjectDebugName( const uint64_t& handle, const uint32_t& objType, cstr_t pName ) const;

        void SetQueueDebugName( uint64_t handle, cstr_t pName ) const;

        bool IsSignaled( const RHI::CPUFence& hFence ) const;

        bool IsSignaled( const RHI::Fence& hFence ) const;

        RHI::FenceValue GetCompletedValue( const RHI::Fence& hFence ) const;

        void Reset( RHI::CPUFence* phFence );

        void Reset( RHI::Fence* phFence, RHI::FenceValue value );

        void Reset( const RHI::Event& hRHIInOut );
        void Reset( const RHI::CommandBuffer& hRHICmdBuffer, const RHI::Event& hRHIEvent,
                    const PIPELINE_STAGES& stages );

        Result WaitForFences( const RHI::CPUFence& hFence, uint64_t timeout ) const;

        Result WaitForFence( RHI::Fence Fence, RHI::FenceValue value ) const;

        Result WaitForQueue( const RHI::Queue& hQueue );

        Result WaitForDevice();

        void SetEvent( const RHI::Event& hRHIEvent );
        void SetEvent( const RHI::CommandBuffer& hRHICmdBuffer, const RHI::Event& hRHIEvent,
                       const PIPELINE_STAGES& stages );

        bool IsSet( const RHI::Event& hRHIEvent );
    };

} // namespace VKE::RenderSystem::RHI
