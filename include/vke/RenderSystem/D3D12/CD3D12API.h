#pragma once

#include "RenderSystem/Common.h"
#include <RenderSystem/TCAPI.h>
#include "Core/Memory/CFreeListPool.h"
#include "Core/Memory/CMemoryPoolManager.h"
#include "RenderSystem/D3D12/CDDITypes.h"

namespace VKE::RenderSystem
{
    // Forward declarations
    class CDeviceContext;
}

namespace VKE::RenderSystem::D3D12
{
    // CAPI: Common Device Driver Interface for any render system
    class VKE_API CD3D12API : public TCAPI< CD3D12API >
    {
        friend class CDeviceContext;
        using AdapterArray = Utils::TCDynamicArray< NativeTypes::Adapter >;

    public:

        // Static methods
        static Result QueryAdapters( AdapterInfoArray* pOut );
        static Result Load( const SDDILoadInfo& Info, SDriverInfo* pOut );

        // Object methods
        Result CreateDevice( const SCreateDeviceDesc& Info, CDeviceContext* pCtx );
        void   DestroyDevice();

        const NativeTypes::Device GetDevice() const
        {
            return NativeTypes::Device{ reinterpret_cast<handle_t>( m_hDevice ) };
        }

        const NativeTypes::Adapter GetAdapter() const
        {
            return NativeTypes::Adapter{ reinterpret_cast<handle_t>( m_hAdapter ) };
        }

        const QueueFamilyInfoArray& GetDeviceQueueInfos() const
        {
            return m_DeviceProperties.vQueueFamilies;
        }

        void QueryDeviceInfo( SDeviceInfo* pOut );

        NativeTypes::Buffer              CreateBuffer( const SBufferDesc& Desc, const SBindMemoryInfo& MemInfo );
        void                           DestroyBuffer( NativeTypes::Buffer* phBuffer, const void* );
        NativeTypes::BufferView          CreateBufferView( const SBufferViewDesc& Desc, const void* );
        void                           DestroyBufferView( NativeTypes::BufferView* phBufferView, const void* );
        Result                         GetTextureFormatProperties( const STextureDesc&, STextureFormatProperties* );
        NativeTypes::Texture             CreateTexture( const STextureDesc& Desc, const SBindMemoryInfo& MemInfo );
        void                           DestroyTexture( NativeTypes::Texture* phImage, const void* );
        NativeTypes::TextureView         CreateTextureView( const STextureViewDesc& Desc, const void* );
        void                           DestroyTextureView( NativeTypes::TextureView* phImageView, const void* );
        NativeTypes::Framebuffer         CreateFramebuffer( const SFramebufferDesc& Desc, const void* );
        void                           DestroyFramebuffer( NativeTypes::Framebuffer* phFramebuffer, const void* );
        NativeTypes::CPUFence            CreateFence( const SFenceDesc& Desc, const void* ) const;
        NativeTypes::Fence               CreateFence2( const SFenceDesc& Desc ) const;
        void                           DestroyFence( NativeTypes::CPUFence* phFence, const void* );
        void                           DestroyFence( NativeTypes::Fence* phFence );
        NativeTypes::GPUFence            CreateSemaphore( const SSemaphoreDesc& Desc, const void* ) const;
        void                           DestroySemaphore( NativeTypes::GPUFence* phSemaphore, const void* );
        NativeTypes::RenderPass          CreateRenderPass( const SRenderPassDesc& Desc, const void* );
        void                           DestroyRenderPass( NativeTypes::RenderPass* phPass, const void* );
        NativeTypes::CommandBufferPool   CreateCommandBufferPool( const SCommandBufferPoolDesc& Desc, const void* );
        void                           DestroyCommandBufferPool( NativeTypes::CommandBufferPool* phPool, const void* );
        NativeTypes::DescriptorPool      CreateDescriptorPool( const SDescriptorPoolDesc& Desc, const void* );
        void                           DestroyDescriptorPool( NativeTypes::DescriptorPool* phPool, const void* );
        NativeTypes::DescriptorSetLayout CreateDescriptorSetLayout( const SDescriptorSetLayoutDesc& Desc, const void* );
        void                      DestroyDescriptorSetLayout( NativeTypes::DescriptorSetLayout* phLayout, const void* );
        NativeTypes::Pipeline       CreatePipeline( const SPipelineDesc& Desc, const void* );
        void                      DestroyPipeline( NativeTypes::Pipeline* phPipeline, const void* );
        NativeTypes::PipelineLayout CreatePipelineLayout( const SPipelineLayoutDesc& Desc, const void* );
        void                      DestroyPipelineLayout( NativeTypes::PipelineLayout* phLayout, const void* );
        NativeTypes::Shader         CreateShader( const SShaderData& Desc, const void* );
        void                      DestroyShader( NativeTypes::Shader* phShader, const void* );
        NativeTypes::Sampler        CreateSampler( const SSamplerDesc& Desc, const void* );
        void                      DestroySampler( NativeTypes::Sampler* phSampler, const void* );
        NativeTypes::Event          CreateEvent( const SEventDesc& Desc, const void* );
        void                      DestroyEvent( NativeTypes::Event* phEvent, const void* );

        Result CreateDescriptorSets( const AllocateDescs::SDescSet& Info, NativeTypes::DescriptorSet* pSets );
        void   FreeObjects( const FreeDescs::SDescSet& );
        void   Update( const SUpdateBufferDescriptorSetInfo& Info );
        void   Update( const SUpdateTextureDescriptorSetInfo& Info );
        void   Update( const NativeTypes::DescriptorSet& hDDISet, const SUpdateBindingsHelper& Info );
        void   Update( const NativeTypes::DescriptorSet& hDDISrcSet, NativeTypes::DescriptorSet* phDDIDstOut );

        Result CreateCommandBuffers( const SAllocateCommandBufferInfo& Info, NativeTypes::CommandBuffer* pBuffers );
        void   FreeObjects( const SFreeCommandBufferInfo& );

        Result GetBufferMemoryRequirements( const SBufferDesc& Desc, SAllocationMemoryRequirementInfo* pOut );
        Result GetTextureMemoryRequirements( const STextureDesc& Desc, SAllocationMemoryRequirementInfo* pOut );
        void   UpdateDesc( SBufferDesc* pInOut );

        void GetFormatFeatures( FORMAT fmt, STextureFormatFeatures* pOut ) const;

        void Bind( const SBindPipelineInfo& Info );
        void Bind( const SBindDDIDescriptorSetsInfo& Info );
        void Bind( const NativeTypes::CommandBuffer& hDDICmdBuffer, const NativeTypes::Buffer& hDDIBuffer,
                   const uint32_t offset );
        void Bind( const NativeTypes::CommandBuffer& hDDICmdBuffer, const NativeTypes::Buffer& hDDIBuffer,
                   const uint32_t offset, const INDEX_TYPE& type );
        void UnbindPipeline( const NativeTypes::CommandBuffer&, const NativeTypes::Pipeline& );
        void UnbindRenderPass( const NativeTypes::CommandBuffer&, const NativeTypes::RenderPass& );

        void Free( NativeTypes::MemoryHeap* phMemory, const void* = nullptr );

        Result           Allocate( const SAllocateMemoryDesc& Desc, SAllocateMemoryData* pOut );
        MEMORY_HEAP_TYPE GetMemoryHeapType( MEMORY_USAGE usage ) const;
        size_t           GetMemoryHeapTotalSize( MEMORY_HEAP_TYPE ) const;
        size_t           GetMemoryHeapCurrentSize( MEMORY_HEAP_TYPE ) const;
        void*            MapMemory( const SMapMemoryInfo& Info );
        void             UnmapMemory( const SMapMemoryInfo& Info );

        void Reset( const NativeTypes::CommandBuffer&     hCommandBuffer,
                    const NativeTypes::CommandBufferPool& hCommandBufferPool );
        void BeginCommandBuffer( const NativeTypes::CommandBuffer&     hCommandBuffer,
                                 const NativeTypes::CommandBufferPool& hCommandBufferPool );
        void EndCommandBuffer( const NativeTypes::CommandBuffer& hCommandBuffer );

        void Barrier( const NativeTypes::CommandBuffer& hCommandBuffer, const SBarrierInfo& Info );

        // Command Buffer
        void SetState( const NativeTypes::CommandBuffer& hCommandBuffer, const SViewportDesc& Desc );
        void SetState( const NativeTypes::CommandBuffer& hCommandBuffer, const SScissorDesc& Desc );

        void Draw( const NativeTypes::CommandBuffer& hCommandBuffer, const uint32_t& vertexCount,
                   const uint32_t& instanceCount, const uint32_t& firstVertex, const uint32_t& firstInstance );
        void DrawIndexed( const NativeTypes::CommandBuffer& hCommandBuffer, const SDrawParams& Params );
        void DrawMesh( const NativeTypes::CommandBuffer& hCommandBuffer, uint32_t width, uint32_t height,
                       uint32_t depth );

        // Dynamic rendering
        void BeginRenderPass( NativeTypes::CommandBuffer, const SBeginRenderPassInfo2& );
        void BeginRenderPass( NativeTypes::CommandBuffer, const SBeginRenderPassInfo& );
        // void EndRenderPass( NativeTypes::CommandBuffer );
        void EndRenderPass( NativeTypes::CommandBuffer, NativeTypes::RenderPass );

        // Copy
        void Copy( const NativeTypes::CommandBuffer& hDDICmdBuffer, const SCopyTextureInfoEx& Info );
        void Copy( const NativeTypes::CommandBuffer& hCmdBuffer, const SCopyBufferInfo& Info );
        void Copy( const NativeTypes::CommandBuffer& hDDICmdBuffer, const SCopyBufferToTextureInfo& Info );
        void Blit( const NativeTypes::CommandBuffer& hAPICmdBuffer, const SBlitTextureInfo& Info );

        // Events
        void SetEvent( const NativeTypes::Event& hDDIEvent );
        void SetEvent( const NativeTypes::CommandBuffer& hDDICmdBuffer, const NativeTypes::Event& hDDIEvent,
                       const PIPELINE_STAGES& stages );
        void Reset( const NativeTypes::Event& hDDIInOut );
        void Reset( const NativeTypes::CommandBuffer& hDDICmdBuffer, const NativeTypes::Event& hDDIEvent,
                    const PIPELINE_STAGES& stages );
        bool IsSet( const NativeTypes::Event& hDDIEvent );

        Result Submit( const SSubmitInfo& Info );
        Result Present( const SPresentData& Info );

        Result CreateSwapChain( const SSwapChainDesc& Desc, const void*, SDDISwapChain* pInOut );
        void   DestroySwapChain( SDDISwapChain* pInOut, const void* = nullptr );
        Result ReCreateSwapChain( const SSwapChainDesc& Desc, SDDISwapChain* pOut );
        Result QueryPresentSurfaceCaps( const NativeTypes::PresentSurface& hSurface, SPresentSurfaceCaps* pOut );
        Result GetCurrentBackBufferIndex( const SDDISwapChain& SwapChain, const SDDIGetBackBufferInfo& Info,
                                          uint32_t* pOut );

        //static void Convert( const SClearValue& In, NativeTypes::ClearValue* pOut );

        // Debug
        void BeginDebugInfo( const NativeTypes::CommandBuffer& hDDICmdBuff, const SDebugInfo* pInfo );
        void EndDebugInfo( const NativeTypes::CommandBuffer& hDDICmdBuff );
        void SetObjectDebugName( const uint64_t& handle, const uint32_t& objType, cstr_t pName ) const;
        void SetQueueDebugName( uint64_t, cstr_t ) const;

        bool                  IsSignaled( const NativeTypes::CPUFence& hFence ) const;
        bool                  IsSignaled( const NativeTypes::Fence& hFence ) const;
        NativeTypes::FenceValue GetCompletedValue( const NativeTypes::Fence& hFence ) const;
        void                  Reset( NativeTypes::CPUFence* phFence );
        void                  Reset( NativeTypes::Fence* phFence, NativeTypes::FenceValue value );
        Result                WaitForFences( const NativeTypes::CPUFence& hFence, uint64_t timeout ) const;
        Result                WaitForFence( NativeTypes::Fence Fence, NativeTypes::FenceValue value ) const;
        Result                WaitForQueue( const NativeTypes::Queue& hQueue );
        Result                WaitForDevice();

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
