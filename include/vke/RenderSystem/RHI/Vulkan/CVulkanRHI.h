#pragma once

#if VKE_RHI_USE_VIRTUAL

namespace VKE::RenderSystem::RHI
{
    class CVulkanRHI final : public CRHI
    {
    public:
        static Result Load( const SRHILoadInfo& Info, SDriverInfo* pOut );
        static Result QueryAdapters( AdapterInfoArray* pOut );

        VulkanAPI::SImplementation* m_pImplementation = nullptr;

        CVulkanRHI();
        virtual ~CVulkanRHI();

        virtual const RHI::Device           GetDevice() const;
        virtual const RHI::Adapter          GetAdapter() const;
        virtual const QueueFamilyInfoArray& GetDeviceQueueInfos() const;
        virtual void                        QueryDeviceInfo( SDeviceInfo* pOut );

        virtual RHI::Buffer     CreateBuffer( const SBufferDesc& Desc, const SBindMemoryInfo& MemInfo );
        virtual RHI::BufferView CreateBufferView( const SBufferViewDesc& Desc, const void* pAllocator = nullptr );
        virtual RHI::CommandBufferPool   CreateCommandBufferPool( const SCommandBufferPoolDesc& Desc,
                                                                  const void*                   pAllocator = nullptr );
        virtual RHI::DescriptorPool      CreateDescriptorPool( const SDescriptorPoolDesc& Desc,
                                                               const void*                pAllocator = nullptr );
        virtual RHI::DescriptorSetLayout CreateDescriptorSetLayout( const SDescriptorSetLayoutDesc& Desc,
                                                                    const void* pAllocator = nullptr );
        virtual RHI::Event               CreateEvent( const SEventDesc& Desc, const void* pAllocator = nullptr );
        virtual RHI::CPUFence            CreateFence( const SFenceDesc& Desc, const void* pAllocator = nullptr ) const;
        virtual RHI::Fence               CreateFence2( const SFenceDesc& Desc ) const;
        virtual RHI::Framebuffer CreateFramebuffer( const SFramebufferDesc& Desc, const void* pAllocator = nullptr );
        virtual RHI::GPUFence    CreateGPUFence( const SSemaphoreDesc& Desc, const void* pAllocator = nullptr ) const;
        virtual RHI::Pipeline    CreatePipeline( const SPipelineDesc& Desc, const void* pAllocator = nullptr );
        virtual RHI::PipelineLayout CreatePipelineLayout( const SPipelineLayoutDesc& Desc,
                                                          const void*                pAllocator = nullptr );
        virtual RHI::RenderPass     CreateRenderPass( const SRenderPassDesc& Desc, const void* pAllocator = nullptr );
        virtual RHI::Sampler        CreateSampler( const SSamplerDesc& Desc, const void* pAllocator = nullptr );
        virtual RHI::Shader         CreateShader( const SShaderData& Desc, const void* pAllocator = nullptr );
        virtual RHI::Texture        CreateTexture( const STextureDesc& Desc, const SBindMemoryInfo& MemInfo );
        virtual RHI::TextureView    CreateTextureView( const STextureViewDesc& Desc, const void* pAllocator = nullptr );

        virtual void DestroyBuffer( RHI::Buffer* phBuffer, const void* pAllocator = nullptr );
        virtual void DestroyBufferView( RHI::BufferView* phBufferView, const void* pAllocator = nullptr );
        virtual void DestroyCommandBufferPool( RHI::CommandBufferPool* phPool, const void* pAllocator = nullptr );
        virtual void DestroyDescriptorPool( RHI::DescriptorPool* phPool, const void* pAllocator = nullptr );
        virtual void DestroyDescriptorSetLayout( RHI::DescriptorSetLayout* phLayout, const void* pAllocator = nullptr );
        virtual void DestroyDevice();
        virtual void DestroyEvent( RHI::Event* phEvent, const void* pAllocator = nullptr );
        virtual void DestroyFence( RHI::CPUFence* phFence, const void* pAllocator = nullptr );
        virtual void DestroyFence( RHI::Fence* phFence );
        virtual void DestroyFramebuffer( RHI::Framebuffer* phFramebuffer, const void* pAllocator = nullptr );
        virtual void DestroyGPUFence( RHI::GPUFence* phSemaphore, const void* pAllocator = nullptr );
        virtual void DestroyPipeline( RHI::Pipeline* phPipeline, const void* pAllocator = nullptr );
        virtual void DestroyPipelineLayout( RHI::PipelineLayout* phLayout, const void* pAllocator = nullptr );
        virtual void DestroyRenderPass( RHI::RenderPass* phPass, const void* pAllocator = nullptr );
        virtual void DestroySampler( RHI::Sampler* phSampler, const void* pAllocator = nullptr );
        virtual void DestroyShader( RHI::Shader* phShader, const void* pAllocator = nullptr );
        virtual void DestroyTexture( RHI::Texture* phImage, const void* pAllocator = nullptr );
        virtual void DestroyTextureView( RHI::TextureView* phImageView, const void* pAllocator = nullptr );

        virtual Result Allocate( const SAllocateMemoryDesc& Desc, SAllocateMemoryData* pOut );
        virtual void   Barrier( const RHI::CommandBuffer& hCommandBuffer, const SBarrierInfo& Info );
        virtual void   BeginCommandBuffer( const RHI::CommandBuffer&     hCommandBuffer,
                                           const RHI::CommandBufferPool& hCommandBufferPool );
        virtual Result Bind( RESOURCE_TYPE type, const SBindMemoryInfo& Info );
        virtual void   Bind( const SBindPipelineInfo& Info );
        virtual void   Bind( const SBindRHIDescriptorSetsInfo& Info );
        virtual void   Bind( const SBindRenderPassInfo& Info );
        virtual void   Bind( const SBindVertexBufferInfo& Info );
        virtual void   Bind( const RHI::CommandBuffer& hRHICmdBuffer, const RHI::Buffer& hRHIBuffer,
                             const uint32_t offset, const INDEX_TYPE& type );
        virtual Result CreateCommandBuffers( const SAllocateCommandBufferInfo& Info, RHI::CommandBuffer* pBuffers );
        virtual Result CreateDescriptorSets( const AllocateDescs::SDescSet& Info, RHI::DescriptorSet* pSets );
        virtual Result CreateDevice( SCreateDeviceDesc& Info, SSettings* pFeaturesOut );
        virtual Result CreateSwapChain( const SSwapChainDesc& Desc, SRHISwapChain* pInOut,
                                        const void* pAllocator = nullptr );
        virtual void   DestroySwapChain( SRHISwapChain* pInOut = nullptr, const void* pAllocator = nullptr );
        virtual void   EndCommandBuffer( const RHI::CommandBuffer& hCommandBuffer );
        virtual void   Free( RHI::MemoryHeap* phMemory = nullptr, const void* pAllocator = nullptr );
        virtual void   FreeObjects( const FreeDescs::SDescSet& Sets );
        virtual void   FreeObjects( const SFreeCommandBufferInfo& Info );
        virtual Result GetBufferMemoryRequirements( const SBufferDesc& Desc, SAllocationMemoryRequirementInfo* pOut );
        virtual void   GetFormatFeatures( FORMAT fmt, STextureFormatFeatures* pOut ) const;
        virtual size_t GetMemoryHeapCurrentSize( MEMORY_HEAP_TYPE type ) const;
        virtual size_t GetMemoryHeapTotalSize( MEMORY_HEAP_TYPE type ) const;
        virtual MEMORY_HEAP_TYPE GetMemoryHeapType( MEMORY_USAGE usage ) const;
        virtual Result           GetTextureFormatProperties( const STextureDesc& Desc, STextureFormatProperties* pOut );
        virtual Result GetTextureMemoryRequirements( const STextureDesc& Desc, SAllocationMemoryRequirementInfo* pOut );
        virtual void*  MapMemory( const SMapMemoryInfo& Info );
        virtual void   Reset( const RHI::CommandBuffer& hCommandBuffer );
        virtual void   Reset( const RHI::CommandBuffer&     hCommandBuffer,
                              const RHI::CommandBufferPool& hCommandBufferPool );
        virtual void   SetState( const RHI::CommandBuffer& hCommandBuffer, const SViewportDesc& Desc );
        virtual void   SetState( const RHI::CommandBuffer& hCommandBuffer, const SScissorDesc& Desc );
        virtual void   UnbindPipeline( const RHI::CommandBuffer& hCmdBuffer, const RHI::Pipeline& hPipeline );
        virtual void   UnbindRenderPass( const RHI::CommandBuffer& hCmdBuffer, const RHI::RenderPass& hRenderPass );
        virtual void   UnmapMemory( const SMapMemoryInfo& Info );
        virtual void   Update( const SUpdateTextureDescriptorSetInfo& Info );
        virtual void   Update( const RHI::DescriptorSet& hRHISet, const SUpdateBindingsHelper& Info );
        virtual void   Update( const RHI::DescriptorSet& hRHISrcSet, RHI::DescriptorSet* phRHIDstOut );
        virtual void   Update( const SUpdateBufferDescriptorSetInfo& Info );
        virtual void   UpdateDesc( SBufferDesc* pInOut );

        // Command Buffer
        virtual void Draw( const RHI::CommandBuffer& hCommandBuffer, const uint32_t& vertexCount,
                           const uint32_t& instanceCount, const uint32_t& firstVertex, const uint32_t& firstInstance );
        virtual void DrawIndexed( const RHI::CommandBuffer& hCommandBuffer, const SDrawParams& Params );
        virtual void DrawMesh( const RHI::CommandBuffer& hCommandBuffer, uint32_t width, uint32_t height,
                               uint32_t depth );

        // Dynamic rendering
        virtual void BeginRenderPass( RHI::CommandBuffer hCmdBuffer, const SBeginRenderPassInfo2& Info );
        virtual void BeginRenderPass( RHI::CommandBuffer hCmdBuffer, const SBeginRenderPassInfo& Info );
        virtual void EndRenderPass( RHI::CommandBuffer hCmdBuffer, RHI::RenderPass hPass );

        // Copy
        virtual void Copy( const RHI::CommandBuffer& hRHICmdBuffer, const SCopyTextureInfoEx& Info );
        virtual void Copy( const RHI::CommandBuffer& hCmdBuffer, const SCopyBufferInfo& Info );
        virtual void Copy( const RHI::CommandBuffer& hRHICmdBuffer, const SCopyBufferToTextureInfo& Info );
        virtual void Blit( const RHI::CommandBuffer& hAPICmdBuffer, const SBlitTextureInfo& Info );

        virtual Result Submit( const SSubmitInfo& Info );
        virtual Result Present( const SPresentData& Info );
        virtual Result ReCreateSwapChain( const SSwapChainDesc& Desc, SRHISwapChain* pOut );
        virtual Result QueryPresentSurfaceCaps( const RHI::PresentSurface& hSurface, SPresentSurfaceCaps* pOut );
        virtual Result GetCurrentBackBufferIndex( const SRHISwapChain& SwapChain, const SRHIGetBackBufferInfo& Info,
                                                  uint32_t* pOut );

        // Debug
        virtual void BeginDebugInfo( const RHI::CommandBuffer& hRHICmdBuff, const SDebugInfo* pInfo );
        virtual void EndDebugInfo( const RHI::CommandBuffer& hRHICmdBuff );
        virtual void SetObjectDebugName( const uint64_t& handle, const uint32_t& objType, cstr_t pName ) const;
        virtual void SetQueueDebugName( uint64_t handle, cstr_t pName ) const;
        virtual bool IsSignaled( const RHI::CPUFence& hFence ) const;
        virtual bool IsSignaled( const RHI::Fence& hFence ) const;
        virtual void Reset( RHI::CPUFence* phFence );
        virtual void Reset( RHI::Fence* phFence, RHI::FenceValue value );
        virtual void Reset( const RHI::Event& hRHIInOut );
        virtual void Reset( const RHI::CommandBuffer& hRHICmdBuffer, const RHI::Event& hRHIEvent,
                            const PIPELINE_STAGES& stages );

        virtual RHI::FenceValue GetCompletedValue( const RHI::Fence& hFence ) const;

        virtual Result WaitForFences( const RHI::CPUFence& hFence, uint64_t timeout ) const;
        virtual Result WaitForFence( RHI::Fence Fence, RHI::FenceValue value ) const;
        virtual Result WaitForQueue( const RHI::Queue& hQueue );
        virtual Result WaitForDevice();

        virtual void SetEvent( const RHI::Event& hRHIEvent );
        virtual void SetEvent( const RHI::CommandBuffer& hRHICmdBuffer, const RHI::Event& hRHIEvent,
                               const PIPELINE_STAGES& stages );

        virtual bool IsSet( const RHI::Event& hRHIEvent );
    };
}; // namespace VKE::RenderSystem::RHI

#endif