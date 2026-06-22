#pragma once

#include "RenderSystem/Common.h"
#include <RenderSystem/TCAPI.h>
#include "Core/Memory/CFreeListPool.h"
#include "Core/Memory/CMemoryPoolManager.h"


namespace VKE::RenderSystem
{
    // Forward declarations
    class CDeviceContext;
}

namespace VKE::RenderSystem::D3D12
{
    struct NativeAPI;
    struct SImplementation;
    // CAPI: Common Device Driver Interface for any render system
    class VKE_API CD3D12API : public TCRHI< CD3D12API >
    {
        friend class TCRHI< CD3D12API >;
        friend class CDeviceContext;
        using AdapterArray = Utils::TCDynamicArray< NativeTypes::Adapter >;

    public:

        CD3D12API();
        ~CD3D12API();

    private:

        // Static methods
        static Result QueryAdaptersImpl( AdapterInfoArray* pOut );
        static Result LoadImpl( const SDDILoadInfo& Info, SDriverInfo* pOut );

        // Object methods
        Result CreateDeviceImpl( const SCreateDeviceDesc& Info, CDeviceContext* pCtx );
        void   DestroyDeviceImpl();

        const NativeTypes::Device GetDeviceImpl() const;

        const NativeTypes::Adapter GetAdapterImpl() const;

        const QueueFamilyInfoArray& GetDeviceQueueInfosImpl() const
        {
            return m_DeviceProperties.vQueueFamilies;
        }

        void QueryDeviceInfoImpl( SDeviceInfo* pOut );

        NativeTypes::Buffer              CreateBufferImpl( const SBufferDesc& Desc, const SBindMemoryInfo& MemInfo );
        void                             DestroyBufferImpl( NativeTypes::Buffer* phBuffer, const void* );
        NativeTypes::BufferView          CreateBufferViewImpl( const SBufferViewDesc& Desc, const void* );
        void                             DestroyBufferViewImpl( NativeTypes::BufferView* phBufferView, const void* );
        Result                         GetTextureFormatPropertiesImpl( const STextureDesc&, STextureFormatProperties* );
        NativeTypes::Texture CreateTextureImpl( const STextureDesc& Desc, const SBindMemoryInfo& MemInfo );
        void                             DestroyTextureImpl( NativeTypes::Texture* phImage, const void* );
        NativeTypes::TextureView         CreateTextureViewImpl( const STextureViewDesc& Desc, const void* );
        void                             DestroyTextureViewImpl( NativeTypes::TextureView* phImageView, const void* );
        NativeTypes::Framebuffer         CreateFramebufferImpl( const SFramebufferDesc& Desc, const void* );
        void                             DestroyFramebufferImpl( NativeTypes::Framebuffer* phFramebuffer, const void* );
        NativeTypes::CPUFence            CreateFenceImpl( const SFenceDesc& Desc, const void* ) const;
        NativeTypes::Fence               CreateFence2Impl( const SFenceDesc& Desc ) const;
        void                             DestroyFenceImpl( NativeTypes::CPUFence* phFence, const void* );
        void                             DestroyFenceImpl( NativeTypes::Fence* phFence );
        NativeTypes::GPUFence            CreateSemaphoreImpl( const SSemaphoreDesc& Desc, const void* ) const;
        void                             DestroySemaphoreImpl( NativeTypes::GPUFence* phSemaphore, const void* );
        NativeTypes::RenderPass          CreateRenderPassImpl( const SRenderPassDesc& Desc, const void* );
        void                             DestroyRenderPassImpl( NativeTypes::RenderPass* phPass, const void* );
        NativeTypes::CommandBufferPool   CreateCommandBufferPoolImpl( const SCommandBufferPoolDesc& Desc, const void* );
        void                        DestroyCommandBufferPoolImpl( NativeTypes::CommandBufferPool* phPool, const void* );
        NativeTypes::DescriptorPool CreateDescriptorPoolImpl( const SDescriptorPoolDesc& Desc, const void* );
        void                             DestroyDescriptorPoolImpl( NativeTypes::DescriptorPool* phPool, const void* );
        NativeTypes::DescriptorSetLayout CreateDescriptorSetLayoutImpl( const SDescriptorSetLayoutDesc& Desc,
                                                                        const void* );
        void                  DestroyDescriptorSetLayoutImpl( NativeTypes::DescriptorSetLayout* phLayout, const void* );
        NativeTypes::Pipeline CreatePipelineImpl( const SPipelineDesc& Desc, const void* );
        void                  DestroyPipelineImpl( NativeTypes::Pipeline* phPipeline, const void* );
        NativeTypes::PipelineLayout CreatePipelineLayoutImpl( const SPipelineLayoutDesc& Desc, const void* );
        void                        DestroyPipelineLayoutImpl( NativeTypes::PipelineLayout* phLayout, const void* );
        NativeTypes::Shader         CreateShaderImpl( const SShaderData& Desc, const void* );
        void                        DestroyShaderImpl( NativeTypes::Shader* phShader, const void* );
        NativeTypes::Sampler        CreateSamplerImpl( const SSamplerDesc& Desc, const void* );
        void                        DestroySamplerImpl( NativeTypes::Sampler* phSampler, const void* );
        NativeTypes::Event          CreateEventImpl( const SEventDesc& Desc, const void* );
        void                        DestroyEventImpl( NativeTypes::Event* phEvent, const void* );

        Result CreateDescriptorSetsImpl( const AllocateDescs::SDescSet& Info, NativeTypes::DescriptorSet* pSets );
        void   FreeObjectsImpl( const FreeDescs::SDescSet& );
        void   UpdateImpl( const SUpdateBufferDescriptorSetInfo& Info );
        void   UpdateImpl( const SUpdateTextureDescriptorSetInfo& Info );
        void   UpdateImpl( const NativeTypes::DescriptorSet& hDDISet, const SUpdateBindingsHelper& Info );
        void   UpdateImpl( const NativeTypes::DescriptorSet& hDDISrcSet, NativeTypes::DescriptorSet* phDDIDstOut );

        Result CreateCommandBuffersImpl( const SAllocateCommandBufferInfo& Info, NativeTypes::CommandBuffer* pBuffers );
        void   FreeObjectsImpl( const SFreeCommandBufferInfo& );

        Result GetBufferMemoryRequirementsImpl( const SBufferDesc& Desc, SAllocationMemoryRequirementInfo* pOut );
        Result GetTextureMemoryRequirementsImpl( const STextureDesc& Desc, SAllocationMemoryRequirementInfo* pOut );
        void   UpdateDescImpl( SBufferDesc* pInOut );

        void GetFormatFeaturesImpl( FORMAT fmt, STextureFormatFeatures* pOut ) const;

        void BindImpl( const SBindPipelineInfo& Info );
        void BindImpl( const SBindDDIDescriptorSetsInfo& Info );
        void BindImpl( const NativeTypes::CommandBuffer& hDDICmdBuffer, const NativeTypes::Buffer& hDDIBuffer,
                   const uint32_t offset );
        void BindImpl( const NativeTypes::CommandBuffer& hDDICmdBuffer, const NativeTypes::Buffer& hDDIBuffer,
                   const uint32_t offset, const INDEX_TYPE& type );
        void UnbindPipelineImpl( const NativeTypes::CommandBuffer&, const NativeTypes::Pipeline& );
        void UnbindRenderPassImpl( const NativeTypes::CommandBuffer&, const NativeTypes::RenderPass& );

        void FreeImpl( NativeTypes::MemoryHeap* phMemory, const void* = nullptr );

        Result           AllocateImpl( const SAllocateMemoryDesc& Desc, SAllocateMemoryData* pOut );
        MEMORY_HEAP_TYPE GetMemoryHeapTypeImpl( MEMORY_USAGE usage ) const;
        size_t           GetMemoryHeapTotalSizeImpl( MEMORY_HEAP_TYPE ) const;
        size_t           GetMemoryHeapCurrentSizeImpl( MEMORY_HEAP_TYPE ) const;
        void*            MapMemoryImpl( const SMapMemoryInfo& Info );
        void             UnmapMemoryImpl( const SMapMemoryInfo& Info );

        void ResetImpl( const NativeTypes::CommandBuffer&     hCommandBuffer,
                    const NativeTypes::CommandBufferPool& hCommandBufferPool );
        void BeginCommandBufferImpl( const NativeTypes::CommandBuffer&     hCommandBuffer,
                                 const NativeTypes::CommandBufferPool& hCommandBufferPool );
        void EndCommandBufferImpl( const NativeTypes::CommandBuffer& hCommandBuffer );

        void BarrierImpl( const NativeTypes::CommandBuffer& hCommandBuffer, const SBarrierInfo& Info );

        // Command Buffer
        void SetStateImpl( const NativeTypes::CommandBuffer& hCommandBuffer, const SViewportDesc& Desc );
        void SetStateImpl( const NativeTypes::CommandBuffer& hCommandBuffer, const SScissorDesc& Desc );

        void DrawImpl( const NativeTypes::CommandBuffer& hCommandBuffer, const uint32_t& vertexCount,
                   const uint32_t& instanceCount, const uint32_t& firstVertex, const uint32_t& firstInstance );
        void DrawIndexedImpl( const NativeTypes::CommandBuffer& hCommandBuffer, const SDrawParams& Params );
        void DrawMeshImpl( const NativeTypes::CommandBuffer& hCommandBuffer, uint32_t width, uint32_t height,
                       uint32_t depth );

        // Dynamic rendering
        void BeginRenderPassImpl( NativeTypes::CommandBuffer, const SBeginRenderPassInfo2& );
        void BeginRenderPassImpl( NativeTypes::CommandBuffer, const SBeginRenderPassInfo& );
        // void EndRenderPass( NativeTypes::CommandBuffer );
        void EndRenderPassImpl( NativeTypes::CommandBuffer, NativeTypes::RenderPass );

        // Copy
        void CopyImpl( const NativeTypes::CommandBuffer& hDDICmdBuffer, const SCopyTextureInfoEx& Info );
        void CopyImpl( const NativeTypes::CommandBuffer& hCmdBuffer, const SCopyBufferInfo& Info );
        void CopyImpl( const NativeTypes::CommandBuffer& hDDICmdBuffer, const SCopyBufferToTextureInfo& Info );
        void BlitImpl( const NativeTypes::CommandBuffer& hAPICmdBuffer, const SBlitTextureInfo& Info );

        // Events
        void SetEventImpl( const NativeTypes::Event& hDDIEvent );
        void SetEventImpl( const NativeTypes::CommandBuffer& hDDICmdBuffer, const NativeTypes::Event& hDDIEvent,
                       const PIPELINE_STAGES& stages );
        void ResetImpl( const NativeTypes::Event& hDDIInOut );
        void ResetImpl( const NativeTypes::CommandBuffer& hDDICmdBuffer, const NativeTypes::Event& hDDIEvent,
                    const PIPELINE_STAGES& stages );
        bool IsSetImpl( const NativeTypes::Event& hDDIEvent );

        Result SubmitImpl( const SSubmitInfo& Info );
        Result PresentImpl( const SPresentData& Info );

        Result CreateSwapChainImpl( const SSwapChainDesc& Desc, const void*, SDDISwapChain* pInOut );
        void   DestroySwapChainImpl( SDDISwapChain* pInOut, const void* = nullptr );
        Result ReCreateSwapChainImpl( const SSwapChainDesc& Desc, SDDISwapChain* pOut );
        Result QueryPresentSurfaceCapsImpl( const NativeTypes::PresentSurface& hSurface, SPresentSurfaceCaps* pOut );
        Result GetCurrentBackBufferIndexImpl( const SDDISwapChain& SwapChain, const SDDIGetBackBufferInfo& Info,
                                          uint32_t* pOut );

        //static void Convert( const SClearValue& In, NativeTypes::ClearValue* pOut );

        // Debug
        void BeginDebugInfoImpl( const NativeTypes::CommandBuffer& hDDICmdBuff, const SDebugInfo* pInfo );
        void EndDebugInfoImpl( const NativeTypes::CommandBuffer& hDDICmdBuff );
        void SetObjectDebugNameImpl( const uint64_t& handle, const uint32_t& objType, cstr_t pName ) const;
        void SetQueueDebugNameImpl( uint64_t, cstr_t ) const;

        bool                    IsSignaledImpl( const NativeTypes::CPUFence& hFence ) const;
        bool                    IsSignaledImpl( const NativeTypes::Fence& hFence ) const;
        NativeTypes::FenceValue GetCompletedValueImpl( const NativeTypes::Fence& hFence ) const;
        void                    ResetImpl( NativeTypes::CPUFence* phFence );
        void                    ResetImpl( NativeTypes::Fence* phFence, NativeTypes::FenceValue value );
        Result                  WaitForFencesImpl( const NativeTypes::CPUFence& hFence, uint64_t timeout ) const;
        Result                  WaitForFenceImpl( NativeTypes::Fence Fence, NativeTypes::FenceValue value ) const;
        Result                  WaitForQueueImpl( const NativeTypes::Queue& hQueue );
        Result                  WaitForDeviceImpl();

    protected:
        static AdapterArray svAdapters;

        SImplementation* m_pImplementation;

        CDeviceContext*   m_pCtx;
        SDeviceInfo       m_DeviceInfo;
        SDeviceProperties m_DeviceProperties;

        struct
        {
            uint32_t         TypeToIndex[ MemoryHeapTypes::_MAX_COUNT ];
            MEMORY_HEAP_TYPE IndexToType[ 16 ];
        } HeapMap;
    };

} // namespace VKE::RenderSystem
