#pragma once

#include "RenderSystem/Common.h"
#include "RenderSystem/CRuntimeConfig.h"

#include "Core/Memory/CFreeListPool.h"
#include "Core/Memory/CMemoryPoolManager.h"

#include "RenderSystem/RHI/RHIPreprocessor.h"

namespace VKE::RenderSystem::RHI
{

    struct SRHILogLevels
    {
        enum LEVEL : uint8_t
        {
            RHI_LOG_LEVEL_INFO,
            RHI_LOG_LEVEL_WARNING,
            RHI_LOG_LEVEL_ERROR,
            _MAX_COUNT
        };

        static const char* GetText( LEVEL level )
        {
            static constexpr const char* apTypes[ LEVEL::_MAX_COUNT ] = { "[INFO]", "[WARNING]", "[ERROR]" };
            return apTypes[ level ];
        }
    };

    using RHI_LOG_LEVEL = SRHILogLevels::LEVEL;

    class CRHI
    {
    public:
        static Result Load( const SRHILoadInfo& Info, SDriverInfo* pOut );
        static Result QueryAdapters( AdapterInfoArray* pOut );

        SDeviceInfo       m_DeviceInfo;
        SDeviceProperties m_DeviceProperties;

        struct
        {
            uint32_t         TypeToIndex[ MemoryHeapTypes::_MAX_COUNT ];
            MEMORY_HEAP_TYPE IndexToType[ 16 ];
        } HeapMap;

#if VKE_RHI_USE_VIRTUAL
        CRHI()          = default;
        virtual ~CRHI() = default;
#else
        SRHIImplementation* m_pImplementation;

        CRHI();
        ~CRHI();
#endif

        VKE_RHI_VIRTUAL const RHI::Device GetDevice() const VKE_RHI_PURE;
        VKE_RHI_VIRTUAL const RHI::Adapter          GetAdapter() const VKE_RHI_PURE;
        VKE_RHI_VIRTUAL const QueueFamilyInfoArray& GetDeviceQueueInfos() const VKE_RHI_PURE;
        VKE_RHI_VIRTUAL void                        QueryDeviceInfo( SDeviceInfo* pOut ) VKE_RHI_PURE;

        VKE_RHI_VIRTUAL RHI::Buffer CreateBuffer( const SBufferDesc&     Desc,
                                                  const SBindMemoryInfo& MemInfo ) VKE_RHI_PURE;
        VKE_RHI_VIRTUAL RHI::BufferView CreateBufferView( const SBufferViewDesc& Desc,
                                                          const void*            pAllocator = nullptr ) VKE_RHI_PURE;
        VKE_RHI_VIRTUAL RHI::CommandBufferPool CreateCommandBufferPool( const SCommandBufferPoolDesc& Desc,
                                                                        const void* pAllocator = nullptr ) VKE_RHI_PURE;
        VKE_RHI_VIRTUAL RHI::DescriptorPool CreateDescriptorPool( const SDescriptorPoolDesc& Desc,
                                                                  const void* pAllocator = nullptr ) VKE_RHI_PURE;
        VKE_RHI_VIRTUAL                     RHI::DescriptorSetLayout
                                            CreateDescriptorSetLayout( const SDescriptorSetLayoutDesc& Desc,
                                                                       const void*                     pAllocator = nullptr ) VKE_RHI_PURE;
        VKE_RHI_VIRTUAL RHI::Event CreateEvent( const SEventDesc& Desc, const void* pAllocator = nullptr ) VKE_RHI_PURE;
        VKE_RHI_VIRTUAL RHI::CPUFence CreateFence( const SFenceDesc& Desc,
                                                   const void*       pAllocator = nullptr ) const VKE_RHI_PURE;
        VKE_RHI_VIRTUAL RHI::Fence CreateFence2( const SFenceDesc& Desc ) const VKE_RHI_PURE;
        VKE_RHI_VIRTUAL RHI::Framebuffer CreateFramebuffer( const SFramebufferDesc& Desc,
                                                            const void*             pAllocator = nullptr ) VKE_RHI_PURE;
        VKE_RHI_VIRTUAL RHI::GPUFence CreateGPUFence( const SSemaphoreDesc& Desc,
                                                      const void*           pAllocator = nullptr ) const VKE_RHI_PURE;
        VKE_RHI_VIRTUAL RHI::Pipeline CreatePipeline( const SPipelineDesc& Desc,
                                                      const void*          pAllocator = nullptr ) VKE_RHI_PURE;
        VKE_RHI_VIRTUAL RHI::PipelineLayout CreatePipelineLayout( const SPipelineLayoutDesc& Desc,
                                                                  const void* pAllocator = nullptr ) VKE_RHI_PURE;
        VKE_RHI_VIRTUAL RHI::RenderPass CreateRenderPass( const SRenderPassDesc& Desc,
                                                          const void*            pAllocator = nullptr ) VKE_RHI_PURE;
        VKE_RHI_VIRTUAL RHI::Sampler CreateSampler( const SSamplerDesc& Desc,
                                                    const void*         pAllocator = nullptr ) VKE_RHI_PURE;
        VKE_RHI_VIRTUAL RHI::Shader CreateShader( const SShaderData& Desc,
                                                  const void*        pAllocator = nullptr ) VKE_RHI_PURE;
        VKE_RHI_VIRTUAL RHI::Texture CreateTexture( const STextureDesc&    Desc,
                                                    const SBindMemoryInfo& MemInfo ) VKE_RHI_PURE;
        VKE_RHI_VIRTUAL RHI::TextureView CreateTextureView( const STextureViewDesc& Desc,
                                                            const void*             pAllocator = nullptr ) VKE_RHI_PURE;

        VKE_RHI_VIRTUAL void DestroyBuffer( RHI::Buffer* phBuffer, const void* pAllocator = nullptr ) VKE_RHI_PURE;
        VKE_RHI_VIRTUAL void DestroyBufferView( RHI::BufferView* phBufferView,
                                                const void*      pAllocator = nullptr ) VKE_RHI_PURE;
        VKE_RHI_VIRTUAL void DestroyCommandBufferPool( RHI::CommandBufferPool* phPool,
                                                       const void*             pAllocator = nullptr ) VKE_RHI_PURE;
        VKE_RHI_VIRTUAL void DestroyDescriptorPool( RHI::DescriptorPool* phPool,
                                                    const void*          pAllocator = nullptr ) VKE_RHI_PURE;
        VKE_RHI_VIRTUAL void DestroyDescriptorSetLayout( RHI::DescriptorSetLayout* phLayout,
                                                         const void*               pAllocator = nullptr ) VKE_RHI_PURE;
        VKE_RHI_VIRTUAL void DestroyDevice() VKE_RHI_PURE;
        VKE_RHI_VIRTUAL void DestroyEvent( RHI::Event* phEvent, const void* pAllocator = nullptr ) VKE_RHI_PURE;
        VKE_RHI_VIRTUAL void DestroyFence( RHI::CPUFence* phFence, const void* pAllocator = nullptr ) VKE_RHI_PURE;
        VKE_RHI_VIRTUAL void DestroyFence( RHI::Fence* phFence ) VKE_RHI_PURE;
        VKE_RHI_VIRTUAL void DestroyFramebuffer( RHI::Framebuffer* phFramebuffer,
                                                 const void*       pAllocator = nullptr ) VKE_RHI_PURE;
        VKE_RHI_VIRTUAL void DestroyGPUFence( RHI::GPUFence* phSemaphore,
                                              const void*    pAllocator = nullptr ) VKE_RHI_PURE;
        VKE_RHI_VIRTUAL void DestroyPipeline( RHI::Pipeline* phPipeline,
                                              const void*    pAllocator = nullptr ) VKE_RHI_PURE;
        VKE_RHI_VIRTUAL void DestroyPipelineLayout( RHI::PipelineLayout* phLayout,
                                                    const void*          pAllocator = nullptr ) VKE_RHI_PURE;
        VKE_RHI_VIRTUAL void DestroyRenderPass( RHI::RenderPass* phPass,
                                                const void*      pAllocator = nullptr ) VKE_RHI_PURE;
        VKE_RHI_VIRTUAL void DestroySampler( RHI::Sampler* phSampler, const void* pAllocator = nullptr ) VKE_RHI_PURE;
        VKE_RHI_VIRTUAL void DestroyShader( RHI::Shader* phShader, const void* pAllocator = nullptr ) VKE_RHI_PURE;
        VKE_RHI_VIRTUAL void DestroyTexture( RHI::Texture* phImage, const void* pAllocator = nullptr ) VKE_RHI_PURE;
        VKE_RHI_VIRTUAL void DestroyTextureView( RHI::TextureView* phImageView,
                                                 const void*       pAllocator = nullptr ) VKE_RHI_PURE;

        VKE_RHI_VIRTUAL Result Allocate( const SAllocateMemoryDesc& Desc, SAllocateMemoryData* pOut ) VKE_RHI_PURE;
        VKE_RHI_VIRTUAL void Barrier( const RHI::CommandBuffer& hCommandBuffer, const SBarrierInfo& Info ) VKE_RHI_PURE;
        VKE_RHI_VIRTUAL void BeginCommandBuffer( const RHI::CommandBuffer&     hCommandBuffer,
                                                 const RHI::CommandBufferPool& hCommandBufferPool ) VKE_RHI_PURE;
        VKE_RHI_VIRTUAL Result Bind( RESOURCE_TYPE type, const SBindMemoryInfo& Info ) VKE_RHI_PURE;
        VKE_RHI_VIRTUAL void   Bind( const SBindPipelineInfo& Info ) VKE_RHI_PURE;
        VKE_RHI_VIRTUAL void   Bind( const SBindRHIDescriptorSetsInfo& Info ) VKE_RHI_PURE;
        VKE_RHI_VIRTUAL void   Bind( const SBindRenderPassInfo& Info ) VKE_RHI_PURE;
        VKE_RHI_VIRTUAL void   Bind( const SBindVertexBufferInfo& Info ) VKE_RHI_PURE;
        VKE_RHI_VIRTUAL void   Bind( const RHI::CommandBuffer& hRHICmdBuffer, const RHI::Buffer& hRHIBuffer,
                                     const uint32_t offset, const INDEX_TYPE& type ) VKE_RHI_PURE;
        VKE_RHI_VIRTUAL Result CreateCommandBuffers( const SAllocateCommandBufferInfo& Info,
                                                     RHI::CommandBuffer*               pBuffers ) VKE_RHI_PURE;
        VKE_RHI_VIRTUAL Result CreateDescriptorSets( const AllocateDescs::SDescSet& Info,
                                                     RHI::DescriptorSet*            pSets ) VKE_RHI_PURE;
        VKE_RHI_VIRTUAL Result CreateDevice( SCreateDeviceDesc& Info, SSettings* pFeaturesOut ) VKE_RHI_PURE;
        VKE_RHI_VIRTUAL Result CreateSwapChain( const SSwapChainDesc& Desc, SRHISwapChain* pInOut,
                                                const void* pAllocator = nullptr ) VKE_RHI_PURE;
        VKE_RHI_VIRTUAL void   DestroySwapChain( SRHISwapChain* pInOut     = nullptr,
                                                 const void*    pAllocator = nullptr ) VKE_RHI_PURE;
        VKE_RHI_VIRTUAL void   EndCommandBuffer( const RHI::CommandBuffer& hCommandBuffer ) VKE_RHI_PURE;
        VKE_RHI_VIRTUAL void Free( RHI::MemoryHeap* phMemory = nullptr, const void* pAllocator = nullptr ) VKE_RHI_PURE;
        VKE_RHI_VIRTUAL void FreeObjects( const FreeDescs::SDescSet& Sets ) VKE_RHI_PURE;
        VKE_RHI_VIRTUAL void FreeObjects( const SFreeCommandBufferInfo& Info ) VKE_RHI_PURE;
        VKE_RHI_VIRTUAL Result GetBufferMemoryRequirements( const SBufferDesc&                Desc,
                                                            SAllocationMemoryRequirementInfo* pOut ) VKE_RHI_PURE;
        VKE_RHI_VIRTUAL void   GetFormatFeatures( FORMAT fmt, STextureFormatFeatures* pOut ) const VKE_RHI_PURE;
        VKE_RHI_VIRTUAL size_t GetMemoryHeapCurrentSize( MEMORY_HEAP_TYPE type ) const VKE_RHI_PURE;
        VKE_RHI_VIRTUAL size_t GetMemoryHeapTotalSize( MEMORY_HEAP_TYPE type ) const VKE_RHI_PURE;
        VKE_RHI_VIRTUAL MEMORY_HEAP_TYPE GetMemoryHeapType( MEMORY_USAGE usage ) const VKE_RHI_PURE;
        VKE_RHI_VIRTUAL Result           GetTextureFormatProperties( const STextureDesc&       Desc,
                                                                     STextureFormatProperties* pOut ) VKE_RHI_PURE;
        VKE_RHI_VIRTUAL Result GetTextureMemoryRequirements( const STextureDesc&               Desc,
                                                             SAllocationMemoryRequirementInfo* pOut ) VKE_RHI_PURE;
        VKE_RHI_VIRTUAL void*  MapMemory( const SMapMemoryInfo& Info ) VKE_RHI_PURE;
        VKE_RHI_VIRTUAL void   Reset( const RHI::CommandBuffer& hCommandBuffer ) VKE_RHI_PURE;
        VKE_RHI_VIRTUAL void   Reset( const RHI::CommandBuffer&     hCommandBuffer,
                                      const RHI::CommandBufferPool& hCommandBufferPool ) VKE_RHI_PURE;
        VKE_RHI_VIRTUAL void   SetState( const RHI::CommandBuffer& hCommandBuffer,
                                         const SViewportDesc&      Desc ) VKE_RHI_PURE;
        VKE_RHI_VIRTUAL void   SetState( const RHI::CommandBuffer& hCommandBuffer,
                                         const SScissorDesc&       Desc ) VKE_RHI_PURE;
        VKE_RHI_VIRTUAL void   UnbindPipeline( const RHI::CommandBuffer& hCmdBuffer,
                                               const RHI::Pipeline&      hPipeline ) VKE_RHI_PURE;
        VKE_RHI_VIRTUAL void   UnbindRenderPass( const RHI::CommandBuffer& hCmdBuffer,
                                                 const RHI::RenderPass&    hRenderPass ) VKE_RHI_PURE;
        VKE_RHI_VIRTUAL void   UnmapMemory( const SMapMemoryInfo& Info ) VKE_RHI_PURE;
        VKE_RHI_VIRTUAL void   Update( const SUpdateTextureDescriptorSetInfo& Info ) VKE_RHI_PURE;
        VKE_RHI_VIRTUAL void   Update( const RHI::DescriptorSet&    hRHISet,
                                       const SUpdateBindingsHelper& Info ) VKE_RHI_PURE;
        VKE_RHI_VIRTUAL void   Update( const RHI::DescriptorSet& hRHISrcSet,
                                       RHI::DescriptorSet*       phRHIDstOut ) VKE_RHI_PURE;
        VKE_RHI_VIRTUAL void   Update( const SUpdateBufferDescriptorSetInfo& Info ) VKE_RHI_PURE;
        VKE_RHI_VIRTUAL void   UpdateDesc( SBufferDesc* pInOut ) VKE_RHI_PURE;

        // Command Buffer
        VKE_RHI_VIRTUAL void Draw( const RHI::CommandBuffer& hCommandBuffer, const uint32_t& vertexCount,
                                   const uint32_t& instanceCount, const uint32_t& firstVertex,
                                   const uint32_t& firstInstance ) VKE_RHI_PURE;
        VKE_RHI_VIRTUAL void DrawIndexed( const RHI::CommandBuffer& hCommandBuffer,
                                          const SDrawParams&        Params ) VKE_RHI_PURE;
        VKE_RHI_VIRTUAL void DrawMesh( const RHI::CommandBuffer& hCommandBuffer, uint32_t width, uint32_t height,
                                       uint32_t depth ) VKE_RHI_PURE;

        // Dynamic rendering
        VKE_RHI_VIRTUAL void BeginRenderPass( RHI::CommandBuffer           hCmdBuffer,
                                              const SBeginRenderPassInfo2& Info ) VKE_RHI_PURE;
        VKE_RHI_VIRTUAL void BeginRenderPass( RHI::CommandBuffer          hCmdBuffer,
                                              const SBeginRenderPassInfo& Info ) VKE_RHI_PURE;
        VKE_RHI_VIRTUAL void EndRenderPass( RHI::CommandBuffer hCmdBuffer, RHI::RenderPass hPass ) VKE_RHI_PURE;

        // Copy
        VKE_RHI_VIRTUAL void Copy( const RHI::CommandBuffer& hRHICmdBuffer,
                                   const SCopyTextureInfoEx& Info ) VKE_RHI_PURE;
        VKE_RHI_VIRTUAL void Copy( const RHI::CommandBuffer& hCmdBuffer, const SCopyBufferInfo& Info ) VKE_RHI_PURE;
        VKE_RHI_VIRTUAL void Copy( const RHI::CommandBuffer&       hRHICmdBuffer,
                                   const SCopyBufferToTextureInfo& Info ) VKE_RHI_PURE;
        VKE_RHI_VIRTUAL void Blit( const RHI::CommandBuffer& hAPICmdBuffer, const SBlitTextureInfo& Info ) VKE_RHI_PURE;

        VKE_RHI_VIRTUAL Result Submit( const SSubmitInfo& Info ) VKE_RHI_PURE;
        VKE_RHI_VIRTUAL Result Present( const SPresentData& Info ) VKE_RHI_PURE;
        VKE_RHI_VIRTUAL Result ReCreateSwapChain( const SSwapChainDesc& Desc, SRHISwapChain* pOut ) VKE_RHI_PURE;
        VKE_RHI_VIRTUAL Result QueryPresentSurfaceCaps( const RHI::PresentSurface& hSurface,
                                                        SPresentSurfaceCaps*       pOut ) VKE_RHI_PURE;
        VKE_RHI_VIRTUAL Result GetCurrentBackBufferIndex( const SRHISwapChain&         SwapChain,
                                                          const SRHIGetBackBufferInfo& Info,
                                                          uint32_t*                    pOut ) VKE_RHI_PURE;

        // Debug
        VKE_RHI_VIRTUAL void BeginDebugInfo( const RHI::CommandBuffer& hRHICmdBuff,
                                             const SDebugInfo*         pInfo ) VKE_RHI_PURE;
        VKE_RHI_VIRTUAL void EndDebugInfo( const RHI::CommandBuffer& hRHICmdBuff ) VKE_RHI_PURE;
        VKE_RHI_VIRTUAL void SetObjectDebugName( const uint64_t& handle, const uint32_t& objType,
                                                 cstr_t pName ) const VKE_RHI_PURE;
        VKE_RHI_VIRTUAL void SetQueueDebugName( uint64_t handle, cstr_t pName ) const VKE_RHI_PURE;
        VKE_RHI_VIRTUAL bool IsSignaled( const RHI::CPUFence& hFence ) const VKE_RHI_PURE;
        VKE_RHI_VIRTUAL bool IsSignaled( const RHI::Fence& hFence ) const VKE_RHI_PURE;
        VKE_RHI_VIRTUAL void Reset( RHI::CPUFence* phFence ) VKE_RHI_PURE;
        VKE_RHI_VIRTUAL void Reset( RHI::Fence* phFence, RHI::FenceValue value ) VKE_RHI_PURE;
        VKE_RHI_VIRTUAL void Reset( const RHI::Event& hRHIInOut ) VKE_RHI_PURE;
        VKE_RHI_VIRTUAL void Reset( const RHI::CommandBuffer& hRHICmdBuffer, const RHI::Event& hRHIEvent,
                                    const PIPELINE_STAGES& stages ) VKE_RHI_PURE;

        VKE_RHI_VIRTUAL RHI::FenceValue GetCompletedValue( const RHI::Fence& hFence ) const VKE_RHI_PURE;

        VKE_RHI_VIRTUAL Result WaitForFences( const RHI::CPUFence& hFence, uint64_t timeout ) const VKE_RHI_PURE;
        VKE_RHI_VIRTUAL Result WaitForFence( RHI::Fence Fence, RHI::FenceValue value ) const VKE_RHI_PURE;
        VKE_RHI_VIRTUAL Result WaitForQueue( const RHI::Queue& hQueue ) VKE_RHI_PURE;
        VKE_RHI_VIRTUAL Result WaitForDevice() VKE_RHI_PURE;

        VKE_RHI_VIRTUAL void SetEvent( const RHI::Event& hRHIEvent ) VKE_RHI_PURE;
        VKE_RHI_VIRTUAL void SetEvent( const RHI::CommandBuffer& hRHICmdBuffer, const RHI::Event& hRHIEvent,
                                       const PIPELINE_STAGES& stages ) VKE_RHI_PURE;

        VKE_RHI_VIRTUAL bool IsSet( const RHI::Event& hRHIEvent ) VKE_RHI_PURE;
    };

} // namespace VKE::RenderSystem::RHI

#include "RenderSystem/RHI/D3D12/CD3D12RHI.h"
#include "RenderSystem/RHI/Vulkan/CVulkanRHI.h"

namespace VKE::RenderSystem::RHI
{
    static vke_force_inline CRHI* CreateRHI()
    {
        CRHI* pRHI = nullptr;

#if VKE_RHI_USE_VIRTUAL
        RHI_API api = GetRuntimeRHI();
        if( VKE_RHI_CHECK_VULKAN( api ) )
        {
            VKE_RHI_VULKAN_STATIC_CLASS* pVulkanRHI;
            Memory::CreateObject( &HeapAllocator, &pVulkanRHI );
            pRHI = pVulkanRHI;
        }
        else if( VKE_RHI_CHECK_D3D12( api ) )
        {
            VKE_RHI_D3D12_STATIC_CLASS* pD3D12RHI;
            Memory::CreateObject( &HeapAllocator, &pD3D12RHI );
            pRHI = pD3D12RHI;
        }
#else
        Memory::CreateObject( &HeapAllocator, &pRHI );
#endif
        return pRHI;
    }

    static vke_force_inline Result Load( const SRHILoadInfo& Info, SDriverInfo* pOut )
    {
        return VKE_RHI_CALL_STATIC( Load, Info, pOut );
    }

    static vke_force_inline Result QueryAdapters( AdapterInfoArray* pOut )
    {
        return VKE_RHI_CALL_STATIC( QueryAdapters, pOut );
    }
}; // namespace VKE::RenderSystem::RHI