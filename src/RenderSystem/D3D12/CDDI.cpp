#include "RenderSystem/CDDI.h"
#if VKE_D3D12_RENDER_SYSTEM
#include "RenderSystem/CDeviceContext.h"
#include "RenderSystem/CContextBase.h"
#include "RenderSystem/CGraphicsContext.h"
#include "RenderSystem/Resources/CTexture.h"
#include "RenderSystem/Resources/CBuffer.h"
#include "RenderSystem/CRenderPass.h"
#include "Core/Platform/CWindow.h"
#include "Core/Managers/CFileManager.h"



namespace VKE
{
#if 1

#define DDI_CREATE_OBJECT(_name, _CreateInfo, _pAllocator, _phObj) \
    m_ICD.vkCreate##_name( m_hDevice, &(_CreateInfo), static_cast<const VkAllocationCallbacks*>(_pAllocator), (_phObj) );

#define DDI_DESTROY_OBJECT(_name, _phObj, _pAllocator) \
    if( (_phObj) && (*_phObj) != NativeAPI::Null ) \
    { \
        m_ICD.vkDestroy##_name( m_hDevice, (*_phObj), static_cast<const VkAllocationCallbacks*>(_pAllocator) ); \
        (*_phObj) = NativeAPI::Null; \
    }

    namespace RenderSystem
    {
        
        NativeAPI::Instance CDDI::sInstance = NativeAPI::Null;
        NativeAPI::GlobalAPI CDDI::sGlobalICD = {};
        NativeAPI::InstanceAPI CDDI::sInstanceICD = {};
        handle_t               CDDI::shICD        = INVALID_HANDLE;

        namespace Map
        {
            Result NativeResult( HRESULT native )
            {
                Result ret = VKE_FAIL;

                return ret;
            }
            

        } // Map

        struct SNativeApiInternalData
        {
            QueueFamilyInfoArray vQueueFamilyInfos;
        };


        void CDDI::GetFormatFeatures(FORMAT fmt, STextureFormatFeatures* pOut) const
        {
            
        }

        using DDIExtNameArray = Utils::TCDynamicArray< cstr_t >;



        Result CDDI::LoadICD( const SDDILoadInfo& Info, SDriverInfo* pOut )
        {
            Result ret = VKE_OK;
            
            return ret;
        }

        void CDDI::CloseICD()
        {
    
        }

        const SDDIExtension& CDDI::GetExtensionInfo( cstr_t pName ) const
        {
            static const SDDIExtension sDummy;

            return sDummy;
        }

        const QueueFamilyInfoArray& CDDI::GetDeviceQueueInfos() const
        {
            return m_pInternal->vQueueFamilyInfos;
        }

        Result CDDI::CreateDevice( const SCreateDeviceDesc& Desc, CDeviceContext* pCtx )
        {
            if (m_pInternal == nullptr)
            {
                m_pInternal = VKE_NEW SNativeApiInternalData();
            }

            return VKE_OK;
        }

        void CDDI::DestroyDevice()
        {
            VKE_DELETE( m_pInternal );
        }

        Result CDDI::QueryAdapters( AdapterInfoArray* pOut )
        {
            Result ret = VKE_FAIL;
            
            return ret;
        }

        void CDDI::QueryDeviceInfo( SDeviceInfo* pOut )
        {
            
        }

        uint32_t CalcAlignedSize( uint32_t size, uint32_t alignment )
        {
            uint32_t ret = size;
            uint32_t remainder = size % alignment;
            if( remainder > 0 )
            {
                ret = size + alignment - remainder;
            }

            return ret;
        }

        /*void CDDI::UpdateDesc( SBufferDesc* pInOut )
        {
            if( pInOut->usage & BufferUsages::CONSTANT_BUFFER ||
                pInOut->usage & BufferUsages::UNIFORM_TEXEL_BUFFER )
            {
                pInOut->size = CalcAlignedSize( pInOut->size, static_cast<uint32_t>( m_DeviceProperties.Limits.minUniformBufferOffsetAlignment ) );
            }
        }*/

        NativeAPI::Buffer   CDDI::CreateBuffer( const SBufferDesc& Desc, const void* pAllocator )
        {
            return NativeAPI::Null;
        }

        void CDDI::DestroyBuffer( NativeAPI::Buffer* phBuffer, const void* pAllocator )
        {
 
        }

        NativeAPI::BufferView CDDI::CreateBufferView( const SBufferViewDesc& Desc, const void* pAllocator )
        {
            return NativeAPI::Null;
        }

        void CDDI::DestroyBufferView( NativeAPI::BufferView* phBufferView, const void* pAllocator )
        {
  
        }

        NativeAPI::Texture CDDI::CreateTexture( const STextureDesc& Desc, const void* pAllocator )
        {
            return NativeAPI::Null;
        }

        Result CDDI::GetTextureFormatProperties( const STextureDesc& Desc, STextureFormatProperties* pOut )
        {
            Result ret = VKE_OK;
            
            return ret;
        }

        void CDDI::DestroyTexture( NativeAPI::Texture* phImage, const void* pAllocator )
        {
  
        }

        NativeAPI::TextureView CDDI::CreateTextureView( const STextureViewDesc& Desc, const void* pAllocator )
        {
            return NativeAPI::Null;
        }

        void CDDI::DestroyTextureView( NativeAPI::TextureView* phImageView, const void* pAllocator )
        {
   
        }

        NativeAPI::Framebuffer CDDI::CreateFramebuffer( const SFramebufferDesc& Desc, const void* pAllocator )
        {
            return NativeAPI::Null;

        }

        void CDDI::DestroyFramebuffer( NativeAPI::Framebuffer* phFramebuffer, const void* pAllocator )
        {
          
        }

        NativeAPI::CPUFence CDDI::CreateFence( const SFenceDesc& Desc, const void* pAllocator )
        {
            return NativeAPI::Null;
        }

        void CDDI::DestroyFence( NativeAPI::CPUFence* phFence, const void* pAllocator )
        {
          
        }

        NativeAPI::GPUFence CDDI::CreateSemaphore( const SSemaphoreDesc& Desc, const void* pAllocator )
        {
            return NativeAPI::Null;
        }

        void CDDI::DestroySemaphore( NativeAPI::GPUFence* phSemaphore, const void* pAllocator )
        {
          
        }

        NativeAPI::CommandBufferPool CDDI::CreateCommandBufferPool( const SCommandBufferPoolDesc& Desc, const void* pAllocator )
        {
            return NativeAPI::Null;
        }

        void CDDI::DestroyCommandBufferPool( NativeAPI::CommandBufferPool* phPool, const void* pAllocator )
        {
        }

        NativeAPI::RenderPass CDDI::CreateRenderPass( const SRenderPassDesc& Desc, const void* )
        {
            return NativeAPI::Null;
        }

        void CDDI::DestroyRenderPass( NativeAPI::RenderPass* phRenderPass, const void* pAllocator )
        {
   
        }

        NativeAPI::DescriptorPool CDDI::CreateDescriptorPool( const SDescriptorPoolDesc& Desc, const void* pAllocator )
        {
            return NativeAPI::Null;
        }

        void CDDI::DestroyDescriptorPool( NativeAPI::DescriptorPool* phPool, const void* pAllocator )
        {

        }

        NativeAPI::Pipeline CDDI::CreatePipeline( const SPipelineDesc& Desc, const void* pAllocator)
        {
            return NativeAPI::Null;
        }

        void CDDI::DestroyPipeline( NativeAPI::Pipeline* phPipeline, const void* pAllocator )
        {
   
        }

        NativeAPI::DescriptorSetLayout CDDI::CreateDescriptorSetLayout( const SDescriptorSetLayoutDesc& Desc, const void* pAllocator )
        {
            return NativeAPI::Null;
        }

        void CDDI::Update( const SUpdateBufferDescriptorSetInfo& Info )
        {

        }

        void CDDI::Update( const SUpdateTextureDescriptorSetInfo& Info )
        {
   
        }

        void CDDI::Update( const NativeAPI::DescriptorSet& hDDISet, const SUpdateBindingsHelper& Info )
        {
            
        }

        void CDDI::Update( const NativeAPI::DescriptorSet& hDDISrcSet, NativeAPI::DescriptorSet* phDDIDstOut )
        {
            
        }

        void CDDI::DestroyDescriptorSetLayout( NativeAPI::DescriptorSetLayout* phLayout, const void* pAllocator )
        {
         
        }

        NativeAPI::PipelineLayout CDDI::CreatePipelineLayout( const SPipelineLayoutDesc& Desc, const void* pAllocator )
        {
            return NativeAPI::Null;
        }

        void CDDI::DestroyPipelineLayout( NativeAPI::PipelineLayout* phLayout, const void* pAllocator )
        {
           
        }

        NativeAPI::Shader CDDI::CreateShader( const SShaderData& Data, const void* pAllocator )
        {
            return NativeAPI::Null;
        }

        void CDDI::DestroyShader( NativeAPI::Shader* phShader, const void* pAllocator )
        {
   
        }

        NativeAPI::Sampler CDDI::CreateSampler( const SSamplerDesc& Desc, const void* pAllocator)
        {
            return NativeAPI::Null;
        }

        void CDDI::DestroySampler( NativeAPI::Sampler* phSampler, const void* pAllocator )
        {
         
        }

        NativeAPI::Event CDDI::CreateEvent( const SEventDesc&, const void* pAllocator )
        {
            return NativeAPI::Null;
        }

        void CDDI::DestroyEvent( NativeAPI::Event* phEvent, const void* pAllocator )
        {
    
        }

        Result CDDI::AllocateObjects(const AllocateDescs::SDescSet& Info, NativeAPI::DescriptorSet* pSets )
        {
            Result                      ret = VKE_FAIL;
            
            return ret;
        }

        void CDDI::FreeObjects( const FreeDescs::SDescSet& Desc )
        {

        }

        Result CDDI::AllocateObjects( const SAllocateCommandBufferInfo& Info, NativeAPI::CommandBuffer* pBuffers )
        {
            Result ret = VKE_FAIL;
           
            return ret;
        }

        void CDDI::FreeObjects( const SFreeCommandBufferInfo& Info )
        {
          
        }

        size_t CDDI::GetMemoryHeapTotalSize( MEMORY_HEAP_TYPE type ) const
        {
          
            return 0;
        }

        size_t CDDI::GetMemoryHeapCurrentSize( MEMORY_HEAP_TYPE type ) const
        {
            const auto idx = m_aHeapTypeToHeapIndexMap[ type ];
            return m_aHeapSizes[ idx ];
        }



        MEMORY_HEAP_TYPE CDDI::GetMemoryHeapType( MEMORY_USAGE usage ) const
        {
            MEMORY_HEAP_TYPE ret = MemoryHeapTypes::OTHER;
            
            return ret;
        }

        Result CDDI::Allocate( const SAllocateMemoryDesc& Desc, SAllocateMemoryData* pOut )
        {
            Result ret = VKE_FAIL;
           
            return ret;
        }

        Result CDDI::GetTextureMemoryRequirements( const NativeAPI::Texture& hTexture, SAllocationMemoryRequirementInfo* pOut )
        {
            
            return VKE_OK;
        }

        Result CDDI::GetBufferMemoryRequirements( const NativeAPI::Buffer& hBuffer, SAllocationMemoryRequirementInfo* pOut )
        {

            return VKE_OK;
        }

        void CDDI::Free( NativeAPI::Memory* phMemory, const void* pAllocator )
        {

        }

        bool CDDI::IsSignaled( const NativeAPI::CPUFence& hFence ) const
        {
            return false;
        }

        void CDDI::Reset( NativeAPI::CPUFence* phFence )
        {
           
        }

        Result CDDI::WaitForFences( const NativeAPI::CPUFence& hFence, uint64_t timeout )
        {
            return VKE_FAIL;
        }

        Result CDDI::WaitForQueue( const NativeAPI::Queue& hQueue )
        {
            return VKE_FAIL;
        }

        Result CDDI::WaitForDevice()
        {
            return VKE_FAIL;
        }

        void* CDDI::MapMemory(const SMapMemoryInfo& Info)
        {
            void* pData = nullptr;
            return pData;
        }

        void CDDI::UnmapMemory( const NativeAPI::Memory& hDDIMemory )
        {

        }

        void CDDI::Draw( const NativeAPI::CommandBuffer& hCommandBuffer, const uint32_t& vertexCount,
            const uint32_t& instanceCount, const uint32_t& firstVertex, const uint32_t& firstInstance )
        {

        }

        void CDDI::DrawIndexed( const NativeAPI::CommandBuffer& hCommandBuffer, const SDrawParams& Params )
        {

        }

        void CDDI::DrawMesh(const NativeAPI::CommandBuffer& hCommandBuffer, uint32_t width, uint32_t height, uint32_t depth)
        {

        }

        void CDDI::BeginRenderPass( NativeAPI::CommandBuffer hCommandBuffer, const SBeginRenderPassInfo2& Info )
        {
            
        }

        void CDDI::EndRenderPass(NativeAPI::CommandBuffer hDDICommandBuffer)
        {

        }

        void CDDI::Copy( const NativeAPI::CommandBuffer& hCmdBuffer, const SCopyBufferToTextureInfo& Info )
        {
          
        }

        void CDDI::Copy( const NativeAPI::CommandBuffer& hDDICmdBuffer, const SCopyBufferInfo& Info )
        {
          
        }



        void CDDI::Copy( const NativeAPI::CommandBuffer& hDDICmdBuffer, const SCopyTextureInfoEx& Info )
        {
           
        }

        void CDDI::Blit( const NativeAPI::CommandBuffer& hAPICmdBuffer, const SBlitTextureInfo& Info )
        {
           
        }

        void CDDI::SetEvent( const NativeAPI::Event& hDDIEvent )
        {
          
        }

        void CDDI::SetEvent( const NativeAPI::CommandBuffer& hDDICmdBuffer, const NativeAPI::Event& hDDIEvent, const PIPELINE_STAGES& stages )
        {
         
        }

        void CDDI::Reset( const NativeAPI::Event& hDDIInOut )
        {
       
        }

        void CDDI::Reset( const NativeAPI::CommandBuffer& hDDICmdBuffer, const NativeAPI::Event& hDDIEvent, const PIPELINE_STAGES& stages )
        {
         
        }

        bool CDDI::IsSet( const NativeAPI::Event& hDDIEvent )
        {
            return false;
        }

        Result CDDI::Submit( const SSubmitInfo& Info )
        {
            Result ret = VKE_FAIL;

          
            return ret;
        }

        Result CDDI::Present( const SPresentData& Info )
        {
            return VKE_FAIL;
        }


        Result CDDI::CreateSwapChain( const SSwapChainDesc& Desc, const void*, SDDISwapChain* pOut )
        {
            Result ret = VKE_FAIL;
          
            return ret;
        }

        Result CDDI::ReCreateSwapChain( const SSwapChainDesc& Desc, SDDISwapChain* pOut )
        {
            Result ret = VKE_FAIL;
           
            return ret;
        }

        Result CDDI::QueryPresentSurfaceCaps( const NativeAPI::PresentSurface& hSurface, SPresentSurfaceCaps* pOut )
        {
            Result ret = VKE_FAIL;
         
            return ret;
        }

        void CDDI::DestroySwapChain( SDDISwapChain* pInOut, const void* )
        {
          
        }

        Result CDDI::GetCurrentBackBufferIndex( const SDDISwapChain& SwapChain, const SDDIGetBackBufferInfo& Info,
            uint32_t* pOut )
        {
            Result ret = VKE_FAIL;

          
            return ret;
        }

        void CDDI::Reset( const NativeAPI::CommandBuffer& hCommandBuffer )
        {
         
        }

        void CDDI::BeginCommandBuffer( const NativeAPI::CommandBuffer& hCommandBuffer )
        {
          
        }

        void CDDI::EndCommandBuffer( const NativeAPI::CommandBuffer& hCommandBuffer )
        {
         
        }

        Result CDDI::Bind( const SBindMemoryInfo& Info )
        {
            return VKE_FAIL;
        }

        void CDDI::Bind( const SBindPipelineInfo& Info )
        {
         
        }

        void CDDI::UnbindPipeline( const NativeAPI::CommandBuffer&, const NativeAPI::Pipeline& )
        {

        }

        void CDDI::Bind( const SBindRenderPassInfo& Info )
        {
            
        }

        void CDDI::UnbindRenderPass( const NativeAPI::CommandBuffer& hCb, const NativeAPI::RenderPass& )
        {
        
        }

        void CDDI::Bind( const SBindDDIDescriptorSetsInfo& Info )
        {
        
        }

        void CDDI::Bind( const NativeAPI::CommandBuffer& hDDICmdBuffer, const NativeAPI::Buffer& hDDIBuffer, const uint32_t offset )
        {
        
        }

        void CDDI::Bind( const NativeAPI::CommandBuffer& hDDICmdBuffer, const NativeAPI::Buffer& hDDIBuffer, const uint32_t offset,
            const INDEX_TYPE& type )
        {
          
        }

        void CDDI::SetState( const NativeAPI::CommandBuffer& hCommandBuffer, const SViewportDesc& Desc )
        {
           
        }

        void CDDI::SetState( const NativeAPI::CommandBuffer& hCommandBuffer, const SScissorDesc& Desc )
        {
          
        }

        void CDDI::Barrier( const NativeAPI::CommandBuffer& hCommandBuffer, const SBarrierInfo& Info )
        {
           
        }


        void CDDI::Convert( const SClearValue& In, NativeAPI::ClearValue* pOut )
        {
  
        }

        void CDDI::BeginDebugInfo( const NativeAPI::CommandBuffer& hDDICmdBuff, const SDebugInfo* pInfo )
        {
          
        }

        void CDDI::EndDebugInfo( const NativeAPI::CommandBuffer& hDDICmdBuff )
        {
        
        }

        void CDDI::SetObjectDebugName( const uint64_t& handle, const uint32_t& objType, cstr_t pName ) const
        {

        }

        void CDDI::SetQueueDebugName( uint64_t handle, cstr_t pName ) const
        {

        }

     

    } // RenderSystem
#endif 0
} // VKE
#endif // VKE_VULKAN_RENDER_SYSTEM