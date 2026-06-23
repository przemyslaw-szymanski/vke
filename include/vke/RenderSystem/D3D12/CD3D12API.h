#pragma once

#include "RenderSystem/Common.h"
#include "RenderSystem/TCRHI.h"
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
        using AdapterArray = Utils::TCDynamicArray< RHI::Adapter >;

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

        const RHI::Device GetDeviceImpl() const;

        const RHI::Adapter GetAdapterImpl() const;

        const QueueFamilyInfoArray& GetDeviceQueueInfosImpl() const
        {
            return m_DeviceProperties.vQueueFamilies;
        }

        void QueryDeviceInfoImpl( SDeviceInfo* pOut );

        RHI::Buffer              CreateBufferImpl( const SBufferDesc& Desc, const SBindMemoryInfo& MemInfo );
        void                             DestroyBufferImpl( RHI::Buffer* phBuffer, const void* );
        RHI::BufferView          CreateBufferViewImpl( const SBufferViewDesc& Desc, const void* );
        void                             DestroyBufferViewImpl( RHI::BufferView* phBufferView, const void* );
        Result                         GetTextureFormatPropertiesImpl( const STextureDesc&, STextureFormatProperties* );
        RHI::Texture CreateTextureImpl( const STextureDesc& Desc, const SBindMemoryInfo& MemInfo );
        void                             DestroyTextureImpl( RHI::Texture* phImage, const void* );
        RHI::TextureView         CreateTextureViewImpl( const STextureViewDesc& Desc, const void* );
        void                             DestroyTextureViewImpl( RHI::TextureView* phImageView, const void* );
        RHI::Framebuffer         CreateFramebufferImpl( const SFramebufferDesc& Desc, const void* );
        void                             DestroyFramebufferImpl( RHI::Framebuffer* phFramebuffer, const void* );
        RHI::CPUFence            CreateFenceImpl( const SFenceDesc& Desc, const void* ) const;
        RHI::Fence               CreateFence2Impl( const SFenceDesc& Desc ) const;
        void                             DestroyFenceImpl( RHI::CPUFence* phFence, const void* );
        void                             DestroyFenceImpl( RHI::Fence* phFence );
        RHI::GPUFence            CreateSemaphoreImpl( const SSemaphoreDesc& Desc, const void* ) const;
        void                             DestroySemaphoreImpl( RHI::GPUFence* phSemaphore, const void* );
        RHI::RenderPass          CreateRenderPassImpl( const SRenderPassDesc& Desc, const void* );
        void                             DestroyRenderPassImpl( RHI::RenderPass* phPass, const void* );
        RHI::CommandBufferPool   CreateCommandBufferPoolImpl( const SCommandBufferPoolDesc& Desc, const void* );
        void                        DestroyCommandBufferPoolImpl( RHI::CommandBufferPool* phPool, const void* );
        RHI::DescriptorPool CreateDescriptorPoolImpl( const SDescriptorPoolDesc& Desc, const void* );
        void                             DestroyDescriptorPoolImpl( RHI::DescriptorPool* phPool, const void* );
        RHI::DescriptorSetLayout CreateDescriptorSetLayoutImpl( const SDescriptorSetLayoutDesc& Desc,
                                                                        const void* );
        void                  DestroyDescriptorSetLayoutImpl( RHI::DescriptorSetLayout* phLayout, const void* );
        RHI::Pipeline CreatePipelineImpl( const SPipelineDesc& Desc, const void* );
        void                  DestroyPipelineImpl( RHI::Pipeline* phPipeline, const void* );
        RHI::PipelineLayout CreatePipelineLayoutImpl( const SPipelineLayoutDesc& Desc, const void* );
        void                        DestroyPipelineLayoutImpl( RHI::PipelineLayout* phLayout, const void* );
        RHI::Shader         CreateShaderImpl( const SShaderData& Desc, const void* );
        void                        DestroyShaderImpl( RHI::Shader* phShader, const void* );
        RHI::Sampler        CreateSamplerImpl( const SSamplerDesc& Desc, const void* );
        void                        DestroySamplerImpl( RHI::Sampler* phSampler, const void* );
        RHI::Event          CreateEventImpl( const SEventDesc& Desc, const void* );
        void                        DestroyEventImpl( RHI::Event* phEvent, const void* );

        Result CreateDescriptorSetsImpl( const AllocateDescs::SDescSet& Info, RHI::DescriptorSet* pSets );
        void   FreeObjectsImpl( const FreeDescs::SDescSet& );
        void   UpdateImpl( const SUpdateBufferDescriptorSetInfo& Info );
        void   UpdateImpl( const SUpdateTextureDescriptorSetInfo& Info );
        void   UpdateImpl( const RHI::DescriptorSet& hDDISet, const SUpdateBindingsHelper& Info );
        void   UpdateImpl( const RHI::DescriptorSet& hDDISrcSet, RHI::DescriptorSet* phDDIDstOut );

        Result CreateCommandBuffersImpl( const SAllocateCommandBufferInfo& Info, RHI::CommandBuffer* pBuffers );
        void   FreeObjectsImpl( const SFreeCommandBufferInfo& );

        Result GetBufferMemoryRequirementsImpl( const SBufferDesc& Desc, SAllocationMemoryRequirementInfo* pOut );
        Result GetTextureMemoryRequirementsImpl( const STextureDesc& Desc, SAllocationMemoryRequirementInfo* pOut );
        void   UpdateDescImpl( SBufferDesc* pInOut );

        void GetFormatFeaturesImpl( FORMAT fmt, STextureFormatFeatures* pOut ) const;

        void BindImpl( const SBindPipelineInfo& Info );
        void BindImpl( const SBindDDIDescriptorSetsInfo& Info );
        void BindImpl( const RHI::CommandBuffer& hDDICmdBuffer, const RHI::Buffer& hDDIBuffer,
                   const uint32_t offset );
        void BindImpl( const RHI::CommandBuffer& hDDICmdBuffer, const RHI::Buffer& hDDIBuffer,
                   const uint32_t offset, const INDEX_TYPE& type );
        void UnbindPipelineImpl( const RHI::CommandBuffer&, const RHI::Pipeline& );
        void UnbindRenderPassImpl( const RHI::CommandBuffer&, const RHI::RenderPass& );

        void FreeImpl( RHI::MemoryHeap* phMemory, const void* = nullptr );

        Result           AllocateImpl( const SAllocateMemoryDesc& Desc, SAllocateMemoryData* pOut );
        MEMORY_HEAP_TYPE GetMemoryHeapTypeImpl( MEMORY_USAGE usage ) const;
        size_t           GetMemoryHeapTotalSizeImpl( MEMORY_HEAP_TYPE ) const;
        size_t           GetMemoryHeapCurrentSizeImpl( MEMORY_HEAP_TYPE ) const;
        void*            MapMemoryImpl( const SMapMemoryInfo& Info );
        void             UnmapMemoryImpl( const SMapMemoryInfo& Info );

        void ResetImpl( const RHI::CommandBuffer&     hCommandBuffer,
                    const RHI::CommandBufferPool& hCommandBufferPool );
        void BeginCommandBufferImpl( const RHI::CommandBuffer&     hCommandBuffer,
                                 const RHI::CommandBufferPool& hCommandBufferPool );
        void EndCommandBufferImpl( const RHI::CommandBuffer& hCommandBuffer );

        void BarrierImpl( const RHI::CommandBuffer& hCommandBuffer, const SBarrierInfo& Info );

        // Command Buffer
        void SetStateImpl( const RHI::CommandBuffer& hCommandBuffer, const SViewportDesc& Desc );
        void SetStateImpl( const RHI::CommandBuffer& hCommandBuffer, const SScissorDesc& Desc );

        void DrawImpl( const RHI::CommandBuffer& hCommandBuffer, const uint32_t& vertexCount,
                   const uint32_t& instanceCount, const uint32_t& firstVertex, const uint32_t& firstInstance );
        void DrawIndexedImpl( const RHI::CommandBuffer& hCommandBuffer, const SDrawParams& Params );
        void DrawMeshImpl( const RHI::CommandBuffer& hCommandBuffer, uint32_t width, uint32_t height,
                       uint32_t depth );

        // Dynamic rendering
        void BeginRenderPassImpl( RHI::CommandBuffer, const SBeginRenderPassInfo2& );
        void BeginRenderPassImpl( RHI::CommandBuffer, const SBeginRenderPassInfo& );
        // void EndRenderPass( RHI::CommandBuffer );
        void EndRenderPassImpl( RHI::CommandBuffer, RHI::RenderPass );

        // Copy
        void CopyImpl( const RHI::CommandBuffer& hDDICmdBuffer, const SCopyTextureInfoEx& Info );
        void CopyImpl( const RHI::CommandBuffer& hCmdBuffer, const SCopyBufferInfo& Info );
        void CopyImpl( const RHI::CommandBuffer& hDDICmdBuffer, const SCopyBufferToTextureInfo& Info );
        void BlitImpl( const RHI::CommandBuffer& hAPICmdBuffer, const SBlitTextureInfo& Info );

        // Events
        void SetEventImpl( const RHI::Event& hDDIEvent );
        void SetEventImpl( const RHI::CommandBuffer& hDDICmdBuffer, const RHI::Event& hDDIEvent,
                       const PIPELINE_STAGES& stages );
        void ResetImpl( const RHI::Event& hDDIInOut );
        void ResetImpl( const RHI::CommandBuffer& hDDICmdBuffer, const RHI::Event& hDDIEvent,
                    const PIPELINE_STAGES& stages );
        bool IsSetImpl( const RHI::Event& hDDIEvent );

        Result SubmitImpl( const SSubmitInfo& Info );
        Result PresentImpl( const SPresentData& Info );

        Result CreateSwapChainImpl( const SSwapChainDesc& Desc, const void*, SDDISwapChain* pInOut );
        void   DestroySwapChainImpl( SDDISwapChain* pInOut, const void* = nullptr );
        Result ReCreateSwapChainImpl( const SSwapChainDesc& Desc, SDDISwapChain* pOut );
        Result QueryPresentSurfaceCapsImpl( const RHI::PresentSurface& hSurface, SPresentSurfaceCaps* pOut );
        Result GetCurrentBackBufferIndexImpl( const SDDISwapChain& SwapChain, const SDDIGetBackBufferInfo& Info,
                                          uint32_t* pOut );

        //static void Convert( const SClearValue& In, RHI::ClearValue* pOut );

        // Debug
        void BeginDebugInfoImpl( const RHI::CommandBuffer& hDDICmdBuff, const SDebugInfo* pInfo );
        void EndDebugInfoImpl( const RHI::CommandBuffer& hDDICmdBuff );
        void SetObjectDebugNameImpl( const uint64_t& handle, const uint32_t& objType, cstr_t pName ) const;
        void SetQueueDebugNameImpl( uint64_t, cstr_t ) const;

        bool                    IsSignaledImpl( const RHI::CPUFence& hFence ) const;
        bool                    IsSignaledImpl( const RHI::Fence& hFence ) const;
        RHI::FenceValue GetCompletedValueImpl( const RHI::Fence& hFence ) const;
        void                    ResetImpl( RHI::CPUFence* phFence );
        void                    ResetImpl( RHI::Fence* phFence, RHI::FenceValue value );
        Result                  WaitForFencesImpl( const RHI::CPUFence& hFence, uint64_t timeout ) const;
        Result                  WaitForFenceImpl( RHI::Fence Fence, RHI::FenceValue value ) const;
        Result                  WaitForQueueImpl( const RHI::Queue& hQueue );
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
