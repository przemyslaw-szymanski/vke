#pragma once

#include "RenderSystem/Common.h"

#include "Core/Memory/CFreeListPool.h"
#include "Core/Memory/CMemoryPoolManager.h"

namespace VKE::RenderSystem
{
    // Forward declarations
    class CDeviceContext;

    // CDDI: Common Device Driver Interface for any render system
    class VKE_API CDDI
    {
        friend class CDeviceContext;
        using AdapterArray = Utils::TCDynamicArray< NativeAPI::Adapter >;

    public:
        struct AllocateDescs
        {
            struct SDescSet
            {
                NativeAPI::DescriptorPool       hPool;
                NativeAPI::DescriptorSetLayout* phLayouts;
                uint32_t                        count;
                VKE_RENDER_SYSTEM_DEBUG_NAME;
            };

            struct SMemory
            {
                NativeAPI::Texture hDDITexture = NativeAPI::Null;
                NativeAPI::Buffer  hDDIBuffer  = NativeAPI::Null;
                uint32_t           size;
                MEMORY_USAGE       memoryUsages;
            };
        };

        struct FreeDescs
        {
            struct SDescSet
            {
                NativeAPI::DescriptorPool hPool;
                NativeAPI::DescriptorSet* phSets;
                uint32_t                  count;
            };

            struct SCommandBuffers
            {
                NativeAPI::CommandBufferPool hPool;
                NativeAPI::CommandBuffer*    pBuffers;
                uint32_t                     count;
            };
        };

        // Static methods
        static Result QueryAdapters( AdapterInfoArray* pOut );
        static Result Load( const SDDILoadInfo& Info, SDriverInfo* pOut );

        // Object methods
        Result CreateDevice( const SCreateDeviceDesc& Info, CDeviceContext* pCtx );
        void   DestroyDevice();

        const NativeAPI::Device& GetDevice() const
        {
            return m_hDevice;
        }

        const NativeAPI::Adapter& GetAdapter() const
        {
            return m_hAdapter;
        }

        const QueueFamilyInfoArray& GetDeviceQueueInfos() const
        {
            return m_DeviceProperties.vQueueFamilies;
        }

        void QueryDeviceInfo( SDeviceInfo* pOut );

        NativeAPI::Buffer              CreateBuffer( const SBufferDesc& Desc, const void* );
        void                           DestroyBuffer( NativeAPI::Buffer* phBuffer, const void* );
        NativeAPI::BufferView          CreateBufferView( const SBufferViewDesc& Desc, const void* );
        void                           DestroyBufferView( NativeAPI::BufferView* phBufferView, const void* );
        Result                         GetTextureFormatProperties( const STextureDesc&, STextureFormatProperties* );
        NativeAPI::Texture             CreateTexture( const STextureDesc& Desc, const void* );
        void                           DestroyTexture( NativeAPI::Texture* phImage, const void* );
        NativeAPI::TextureView         CreateTextureView( const STextureViewDesc& Desc, const void* );
        void                           DestroyTextureView( NativeAPI::TextureView* phImageView, const void* );
        NativeAPI::Framebuffer         CreateFramebuffer( const SFramebufferDesc& Desc, const void* );
        void                           DestroyFramebuffer( NativeAPI::Framebuffer* phFramebuffer, const void* );
        NativeAPI::CPUFence            CreateFence( const SFenceDesc& Desc, const void* );
        void                           DestroyFence( NativeAPI::CPUFence* phFence, const void* );
        NativeAPI::GPUFence            CreateSemaphore( const SSemaphoreDesc& Desc, const void* );
        void                           DestroySemaphore( NativeAPI::GPUFence* phSemaphore, const void* );
        NativeAPI::RenderPass          CreateRenderPass( const SRenderPassDesc& Desc, const void* );
        void                           DestroyRenderPass( NativeAPI::RenderPass* phPass, const void* );
        NativeAPI::CommandBufferPool   CreateCommandBufferPool( const SCommandBufferPoolDesc& Desc, const void* );
        void                           DestroyCommandBufferPool( NativeAPI::CommandBufferPool* phPool, const void* );
        NativeAPI::DescriptorPool      CreateDescriptorPool( const SDescriptorPoolDesc& Desc, const void* );
        void                           DestroyDescriptorPool( NativeAPI::DescriptorPool* phPool, const void* );
        NativeAPI::DescriptorSetLayout CreateDescriptorSetLayout( const SDescriptorSetLayoutDesc& Desc, const void* );
        void                      DestroyDescriptorSetLayout( NativeAPI::DescriptorSetLayout* phLayout, const void* );
        NativeAPI::Pipeline       CreatePipeline( const SPipelineDesc& Desc, const void* );
        void                      DestroyPipeline( NativeAPI::Pipeline* phPipeline, const void* );
        NativeAPI::PipelineLayout CreatePipelineLayout( const SPipelineLayoutDesc& Desc, const void* );
        void                      DestroyPipelineLayout( NativeAPI::PipelineLayout* phLayout, const void* );
        NativeAPI::Shader         CreateShader( const SShaderData& Desc, const void* );
        void                      DestroyShader( NativeAPI::Shader* phShader, const void* );
        NativeAPI::Sampler        CreateSampler( const SSamplerDesc& Desc, const void* );
        void                      DestroySampler( NativeAPI::Sampler* phSampler, const void* );
        NativeAPI::Event          CreateEvent( const SEventDesc& Desc, const void* );
        void                      DestroyEvent( NativeAPI::Event* phEvent, const void* );

        Result AllocateObjects( const AllocateDescs::SDescSet& Info, NativeAPI::DescriptorSet* pSets );
        void   FreeObjects( const FreeDescs::SDescSet& );
        Result AllocateObjects( const SAllocateCommandBufferInfo& Info, NativeAPI::CommandBuffer* pBuffers );
        void   FreeObjects( const SFreeCommandBufferInfo& );

        Result GetBufferMemoryRequirements( const NativeAPI::Buffer& hBuffer, SAllocationMemoryRequirementInfo* pOut );
        Result GetTextureMemoryRequirements( const NativeAPI::Texture&         hTexture,
                                             SAllocationMemoryRequirementInfo* pOut );
        void   UpdateDesc( SBufferDesc* pInOut );

        void GetFormatFeatures( FORMAT fmt, STextureFormatFeatures* pOut ) const;

        Result Bind( RESOURCE_TYPE Type, const SBindMemoryInfo& Info );
        void   Bind( const SBindPipelineInfo& Info );
        void   Bind( const SBindDDIDescriptorSetsInfo& Info );
        void   Bind( const SBindRenderPassInfo& Info );
        void   Bind( const NativeAPI::CommandBuffer& hDDICmdBuffer, const NativeAPI::Buffer& hDDIBuffer,
                     const uint32_t offset );
        void   Bind( const NativeAPI::CommandBuffer& hDDICmdBuffer, const NativeAPI::Buffer& hDDIBuffer,
                     const uint32_t offset, const INDEX_TYPE& type );
        void   UnbindPipeline( const NativeAPI::CommandBuffer&, const NativeAPI::Pipeline& );
        void   UnbindRenderPass( const NativeAPI::CommandBuffer&, const NativeAPI::RenderPass& );

        void Free( NativeAPI::Memory* phMemory, const void* = nullptr );

        void Update( const SUpdateBufferDescriptorSetInfo& Info );
        void Update( const SUpdateTextureDescriptorSetInfo& Info );
        void Update( const NativeAPI::DescriptorSet& hDDISet, const SUpdateBindingsHelper& Info );
        void Update( const NativeAPI::DescriptorSet& hDDISrcSet, NativeAPI::DescriptorSet* phDDIDstOut );

        Result           Allocate( const SAllocateMemoryDesc& Desc, SAllocateMemoryData* pOut );
        MEMORY_HEAP_TYPE GetMemoryHeapType( MEMORY_USAGE usage ) const;
        size_t           GetMemoryHeapTotalSize( MEMORY_HEAP_TYPE ) const;
        size_t           GetMemoryHeapCurrentSize( MEMORY_HEAP_TYPE ) const;
        void*            MapMemory( const SMapMemoryInfo& Info );
        void             UnmapMemory( const NativeAPI::Memory& hDDIMemory );

        void Reset( const NativeAPI::CommandBuffer&     hCommandBuffer,
                    const NativeAPI::CommandBufferPool& hCommandBufferPool );
        void BeginCommandBuffer( const NativeAPI::CommandBuffer& hCommandBuffer, const NativeAPI::CommandBufferPool& hCommandBufferPool );
        void EndCommandBuffer( const NativeAPI::CommandBuffer& hCommandBuffer );

        void Barrier( const NativeAPI::CommandBuffer& hCommandBuffer, const SBarrierInfo& Info );

        // Command Buffer
        void SetState( const NativeAPI::CommandBuffer& hCommandBuffer, const SViewportDesc& Desc );
        void SetState( const NativeAPI::CommandBuffer& hCommandBuffer, const SScissorDesc& Desc );

        void Draw( const NativeAPI::CommandBuffer& hCommandBuffer, const uint32_t& vertexCount,
                   const uint32_t& instanceCount, const uint32_t& firstVertex, const uint32_t& firstInstance );
        void DrawIndexed( const NativeAPI::CommandBuffer& hCommandBuffer, const SDrawParams& Params );
        void DrawMesh( const NativeAPI::CommandBuffer& hCommandBuffer, uint32_t width, uint32_t height,
                       uint32_t depth );

        // Dynamic rendering
        void BeginRenderPass( NativeAPI::CommandBuffer, const SBeginRenderPassInfo2& );
        void EndRenderPass( NativeAPI::CommandBuffer );

        // Copy
        void Copy( const NativeAPI::CommandBuffer& hDDICmdBuffer, const SCopyTextureInfoEx& Info );
        void Copy( const NativeAPI::CommandBuffer& hCmdBuffer, const SCopyBufferInfo& Info );
        void Copy( const NativeAPI::CommandBuffer& hDDICmdBuffer, const SCopyBufferToTextureInfo& Info );
        void Blit( const NativeAPI::CommandBuffer& hAPICmdBuffer, const SBlitTextureInfo& Info );

        // Events
        void SetEvent( const NativeAPI::Event& hDDIEvent );
        void SetEvent( const NativeAPI::CommandBuffer& hDDICmdBuffer, const NativeAPI::Event& hDDIEvent,
                       const PIPELINE_STAGES& stages );
        void Reset( const NativeAPI::Event& hDDIInOut );
        void Reset( const NativeAPI::CommandBuffer& hDDICmdBuffer, const NativeAPI::Event& hDDIEvent,
                    const PIPELINE_STAGES& stages );
        bool IsSet( const NativeAPI::Event& hDDIEvent );

        Result Submit( const SSubmitInfo& Info );
        Result Present( const SPresentData& Info );

        Result CreateSwapChain( const SSwapChainDesc& Desc, const void*, SDDISwapChain* pInOut );
        void   DestroySwapChain( SDDISwapChain* pInOut, const void* = nullptr );
        Result ReCreateSwapChain( const SSwapChainDesc& Desc, SDDISwapChain* pOut );
        Result QueryPresentSurfaceCaps( const NativeAPI::PresentSurface& hSurface, SPresentSurfaceCaps* pOut );
        Result GetCurrentBackBufferIndex( const SDDISwapChain& SwapChain, const SDDIGetBackBufferInfo& Info,
                                          uint32_t* pOut );

        static void Convert( const SClearValue& In, NativeAPI::ClearValue* pOut );

        // Debug
        void BeginDebugInfo( const NativeAPI::CommandBuffer& hDDICmdBuff, const SDebugInfo* pInfo );
        void EndDebugInfo( const NativeAPI::CommandBuffer& hDDICmdBuff );
        void SetObjectDebugName( const uint64_t& handle, const uint32_t& objType, cstr_t pName ) const;
        void SetQueueDebugName( uint64_t, cstr_t ) const;

        bool   IsSignaled( const NativeAPI::CPUFence& hFence ) const;
        void   Reset( NativeAPI::CPUFence* phFence );
        Result WaitForFences( const NativeAPI::CPUFence& hFence, uint64_t timeout );
        Result WaitForQueue( const NativeAPI::Queue& hQueue );
        Result WaitForDevice();

        NativeAPI::SImplementation& getImplementation()
        {
            return m_Implementation;
        };

    protected:
        static AdapterArray svAdapters;

        NativeAPI::SImplementation m_Implementation;
        NativeAPI::Device          m_hDevice  = NativeAPI::Null;
        NativeAPI::Adapter         m_hAdapter = NativeAPI::Null;

        CDeviceContext*   m_pCtx;
        SDeviceInfo       m_DeviceInfo;
        SDeviceProperties m_DeviceProperties;

        struct
        {
            uint32_t         TypeToIndex[ MemoryHeapTypes::_MAX_COUNT ];
            MEMORY_HEAP_TYPE IndexToType[ NativeAPI::SImplementation::MAX_MEMORY_HEAPS ];
        } HeapMap;
    };

} // namespace VKE::RenderSystem
