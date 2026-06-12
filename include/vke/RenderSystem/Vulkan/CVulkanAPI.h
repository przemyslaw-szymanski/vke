#pragma once

#include "RenderSystem/Common.h"
#include <RenderSystem/TCAPI.h>
#include "Core/Memory/CFreeListPool.h"
#include "Core/Memory/CMemoryPoolManager.h"

namespace VKE::RenderSystem
{
    // Forward declarations
    class CDeviceContext;
} // VKE::RenderSystem

namespace VKE::RenderSystem::Vulkan
{
    // CAPI: Common Device Driver Interface for any render system
    class VKE_API CVulkanAPI : public TCAPI<CVulkanAPI>
    {
        friend class CDeviceContext;
        using AdapterArray = Utils::TCDynamicArray< NativeAPI::Adapter >;

    public:

        CVulkanAPI()
        {

        }

        Result CreateDeviceImpl( const SCreateDeviceDesc& Info, CDeviceContext* pCtx );
        void   DestroyDeviceImpl();

        static Result Load( const SDDILoadInfo& Info, SDriverInfo* pOut );

        const NativeAPI::Device& GetDeviceImpl() const
        {
            return m_hDevice;
        }

        const NativeAPI::Adapter& GetAdapterImpl() const
        {
            return m_hAdapter;
        }

        const QueueFamilyInfoArray& GetDeviceQueueInfosImpl() const
        {
            return m_DeviceProperties.vQueueFamilies;
        }

        static Result QueryAdaptersImpl( AdapterInfoArray* pOut );

        void QueryDeviceInfoImpl( SDeviceInfo* pOut );

        NativeAPI::Buffer              CreateBufferImpl( const SBufferDesc& Desc, const SBindMemoryInfo& MemInfo );
        void                           DestroyBufferImpl( NativeAPI::Buffer* phBuffer, const void* );
        NativeAPI::BufferView          CreateBufferViewImpl( const SBufferViewDesc& Desc, const void* );
        void                           DestroyBufferViewImpl( NativeAPI::BufferView* phBufferView, const void* );
        Result                         GetTextureFormatPropertiesImpl( const STextureDesc&, STextureFormatProperties* );
        NativeAPI::Texture             CreateTextureImpl( const STextureDesc& Desc, const SBindMemoryInfo& MemInfo );
        void                           DestroyTextureImpl( NativeAPI::Texture* phImage, const void* );
        NativeAPI::TextureView         CreateTextureViewImpl( const STextureViewDesc& Desc, const void* );
        void                           DestroyTextureViewImpl( NativeAPI::TextureView* phImageView, const void* );
        NativeAPI::Framebuffer         CreateFramebufferImpl( const SFramebufferDesc& Desc, const void* );
        void                           DestroyFramebufferImpl( NativeAPI::Framebuffer* phFramebuffer, const void* );
        NativeAPI::CPUFence            CreateFenceImpl( const SFenceDesc& Desc, const void* ) const;
        NativeAPI::Fence               CreateFence2Impl( const SFenceDesc& Desc ) const;
        void                           DestroyFenceImpl( NativeAPI::CPUFence* phFence, const void* );
        void                           DestroyFenceImpl( NativeAPI::Fence* phFence );
        NativeAPI::GPUFence            CreateSemaphoreImpl( const SSemaphoreDesc& Desc, const void* ) const;
        void                           DestroySemaphoreImpl( NativeAPI::GPUFence* phSemaphore, const void* );
        NativeAPI::RenderPass          CreateRenderPassImpl( const SRenderPassDesc& Desc, const void* );
        void                           DestroyRenderPassImpl( NativeAPI::RenderPass* phPass, const void* );
        NativeAPI::CommandBufferPool   CreateCommandBufferPoolImpl( const SCommandBufferPoolDesc& Desc, const void* );
        void                           DestroyCommandBufferPoolImpl( NativeAPI::CommandBufferPool* phPool, const void* );
        NativeAPI::DescriptorPool      CreateDescriptorPoolImpl( const SDescriptorPoolDesc& Desc, const void* );
        void                           DestroyDescriptorPoolImpl( NativeAPI::DescriptorPool* phPool, const void* );
        NativeAPI::DescriptorSetLayout CreateDescriptorSetLayoutImpl( const SDescriptorSetLayoutDesc& Desc,
                                                                      const void* );
        void                      DestroyDescriptorSetLayoutImpl( NativeAPI::DescriptorSetLayout* phLayout, const void* );
        NativeAPI::Pipeline       CreatePipelineImpl( const SPipelineDesc& Desc, const void* );
        void                      DestroyPipelineImpl( NativeAPI::Pipeline* phPipeline, const void* );
        NativeAPI::PipelineLayout CreatePipelineLayoutImpl( const SPipelineLayoutDesc& Desc, const void* );
        void                      DestroyPipelineLayoutImpl( NativeAPI::PipelineLayout* phLayout, const void* );
        NativeAPI::Shader         CreateShaderImpl( const SShaderData& Desc, const void* );
        void                      DestroyShaderImpl( NativeAPI::Shader* phShader, const void* );
        NativeAPI::Sampler        CreateSamplerImpl( const SSamplerDesc& Desc, const void* );
        void                      DestroySamplerImpl( NativeAPI::Sampler* phSampler, const void* );
        NativeAPI::Event          CreateEventImpl( const SEventDesc& Desc, const void* );
        void                      DestroyEventImpl( NativeAPI::Event* phEvent, const void* );

        Result CreateDescriptorSetsImpl( const AllocateDescs::SDescSet& Info, NativeAPI::DescriptorSet* pSets );
        void   FreeObjectsImpl( const FreeDescs::SDescSet& );
        Result CreateCommandBuffersImpl( const SAllocateCommandBufferInfo& Info, NativeAPI::CommandBuffer* pBuffers );
        void   FreeObjectsImpl( const SFreeCommandBufferInfo& );

        Result GetBufferMemoryRequirementsImpl( const SBufferDesc& Desc, SAllocationMemoryRequirementInfo* pOut );
        Result GetTextureMemoryRequirementsImpl( const STextureDesc& Desc, SAllocationMemoryRequirementInfo* pOut );
        void   UpdateDescImpl( SBufferDesc* pInOut );

        void GetFormatFeaturesImpl( FORMAT fmt, STextureFormatFeatures* pOut ) const;

        Result BindImpl( RESOURCE_TYPE Type, const SBindMemoryInfo& Info );
        void   BindImpl( const SBindPipelineInfo& Info );
        void   BindImpl( const SBindDDIDescriptorSetsInfo& Info );
        void   BindImpl( const SBindRenderPassInfo& Info );
        void   BindImpl( const NativeAPI::CommandBuffer& hDDICmdBuffer, const NativeAPI::Buffer& hDDIBuffer,
                     const uint32_t offset );
        void   BindImpl( const NativeAPI::CommandBuffer& hDDICmdBuffer, const NativeAPI::Buffer& hDDIBuffer,
                     const uint32_t offset, const INDEX_TYPE& type );
        void   UnbindPipelineImpl( const NativeAPI::CommandBuffer&, const NativeAPI::Pipeline& );
        void   UnbindRenderPassImpl( const NativeAPI::CommandBuffer&, const NativeAPI::RenderPass& );

        void FreeImpl( NativeAPI::Memory* phMemory, const void* = nullptr );

        void UpdateImpl( const SUpdateBufferDescriptorSetInfo& Info );
        void UpdateImpl( const SUpdateTextureDescriptorSetInfo& Info );
        void UpdateImpl( const NativeAPI::DescriptorSet& hDDISet, const SUpdateBindingsHelper& Info );
        void UpdateImpl( const NativeAPI::DescriptorSet& hDDISrcSet, NativeAPI::DescriptorSet* phDDIDstOut );

        Result           AllocateImpl( const SAllocateMemoryDesc& Desc, SAllocateMemoryData* pOut );
        MEMORY_HEAP_TYPE GetMemoryHeapTypeImpl( MEMORY_USAGE usage ) const;
        size_t           GetMemoryHeapTotalSizeImpl( MEMORY_HEAP_TYPE ) const;
        size_t           GetMemoryHeapCurrentSizeImpl( MEMORY_HEAP_TYPE ) const;
        void*            MapMemoryImpl( const SMapMemoryInfo& Info );
        void             UnmapMemoryImpl( const SMapMemoryInfo& Info );

        void ResetImpl( const NativeAPI::CommandBuffer& hCommandBuffer );
        void BeginCommandBufferImpl( const NativeAPI::CommandBuffer& hCommandBuffer );
        void BeginCommandBufferImpl( const NativeAPI::CommandBuffer&     hCommandBuffer,
                                 const NativeAPI::CommandBufferPool& hCommandBufferPool );
        void EndCommandBufferImpl( const NativeAPI::CommandBuffer& hCommandBuffer );
        void ResetImpl( const NativeAPI::CommandBuffer&     hCommandBuffer,
                    const NativeAPI::CommandBufferPool& hCommandBufferPool );

        void BarrierImpl( const NativeAPI::CommandBuffer& hCommandBuffer, const SBarrierInfo& Info );

        // Command Buffer
        void SetStateImpl( const NativeAPI::CommandBuffer& hCommandBuffer, const SViewportDesc& Desc );
        void SetStateImpl( const NativeAPI::CommandBuffer& hCommandBuffer, const SScissorDesc& Desc );

        void DrawImpl( const NativeAPI::CommandBuffer& hCommandBuffer, const uint32_t& vertexCount,
                   const uint32_t& instanceCount, const uint32_t& firstVertex, const uint32_t& firstInstance );
        void DrawIndexedImpl( const NativeAPI::CommandBuffer& hCommandBuffer, const SDrawParams& Params );
        void DrawMeshImpl( const NativeAPI::CommandBuffer& hCommandBuffer, uint32_t width, uint32_t height,
                       uint32_t depth );

        // Dynamic rendering
        void BeginRenderPassImpl( NativeAPI::CommandBuffer, const SBeginRenderPassInfo2& );
        void BeginRenderPassImpl( NativeAPI::CommandBuffer, const SBeginRenderPassInfo& );
        void EndRenderPassImpl( NativeAPI::CommandBuffer, NativeAPI::RenderPass );

        // Copy
        void CopyImpl( const NativeAPI::CommandBuffer& hDDICmdBuffer, const SCopyTextureInfoEx& Info );
        void CopyImpl( const NativeAPI::CommandBuffer& hCmdBuffer, const SCopyBufferInfo& Info );
        void CopyImpl( const NativeAPI::CommandBuffer& hDDICmdBuffer, const SCopyBufferToTextureInfo& Info );
        void BlitImpl( const NativeAPI::CommandBuffer& hAPICmdBuffer, const SBlitTextureInfo& Info );

        // Events
        void SetEventImpl( const NativeAPI::Event& hDDIEvent );
        void SetEventImpl( const NativeAPI::CommandBuffer& hDDICmdBuffer, const NativeAPI::Event& hDDIEvent,
                       const PIPELINE_STAGES& stages );
        void ResetImpl( const NativeAPI::Event& hDDIInOut );
        void ResetImpl( const NativeAPI::CommandBuffer& hDDICmdBuffer, const NativeAPI::Event& hDDIEvent,
                    const PIPELINE_STAGES& stages );
        bool IsSetImpl( const NativeAPI::Event& hDDIEvent );

        Result SubmitImpl( const SSubmitInfo& Info );
        Result PresentImpl( const SPresentData& Info );

        Result CreateSwapChainImpl( const SSwapChainDesc& Desc, const void*, SDDISwapChain* pInOut );
        void   DestroySwapChainImpl( SDDISwapChain* pInOut, const void* = nullptr );
        Result ReCreateSwapChainImpl( const SSwapChainDesc& Desc, SDDISwapChain* pOut );
        Result QueryPresentSurfaceCapsImpl( const NativeAPI::PresentSurface& hSurface, SPresentSurfaceCaps* pOut );
        Result GetCurrentBackBufferIndexImpl( const SDDISwapChain& SwapChain, const SDDIGetBackBufferInfo& Info,
                                          uint32_t* pOut );

        static void ConvertImpl( const SClearValue& In, NativeAPI::ClearValue* pOut );

        // Debug
        void BeginDebugInfoImpl( const NativeAPI::CommandBuffer& hDDICmdBuff, const SDebugInfo* pInfo );
        void EndDebugInfoImpl( const NativeAPI::CommandBuffer& hDDICmdBuff );
        void SetObjectDebugNameImpl( const uint64_t& handle, const uint32_t& objType, cstr_t pName ) const;
        void SetQueueDebugNameImpl( uint64_t, cstr_t ) const;

        bool                  IsSignaledImpl( const NativeAPI::CPUFence& hFence ) const;
        bool                  IsSignaledImpl( const NativeAPI::Fence& hFence ) const;
        NativeAPI::FenceValue GetCompletedValueImpl( const NativeAPI::Fence& hFence ) const;
        void                  ResetImpl( NativeAPI::CPUFence* phFence );
        void                  ResetImpl( NativeAPI::Fence* phFence, NativeAPI::FenceValue value );
        Result                WaitForFencesImpl( const NativeAPI::CPUFence& hFence, uint64_t timeout ) const;
        Result                WaitForFenceImpl( NativeAPI::Fence Fence, NativeAPI::FenceValue value ) const;
        Result                WaitForQueueImpl( const NativeAPI::Queue& hQueue );
        Result                WaitForDeviceImpl();

        NativeAPI::SImplementation& getImplementation()
        {
            return m_Implementation;
        };

    protected:
        static AdapterArray svAdapters;
        int                        m_a = 3;
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
