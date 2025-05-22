#pragma once

#if VKE_VULKAN_RENDER_SYSTEM
#include "RenderSystem/Vulkan/Vulkan.h"
#include "RenderSystem/Common.h"
#include "RenderSystem/CDDITypes.h"
#include "Core/Memory/CMemoryPoolManager.h"
#include "Core/Memory/CFreeListPool.h"

namespace VKE
{
    namespace RenderSystem
    {
        class CDeviceContext;
        class CDDI;

        struct SMapMemoryInfo
        {
            NativeAPI::Memory   hMemory;
            uint32_t    offset;
            uint32_t    size;
        };

        struct SAllocateMemoryDesc
        {
            uint32_t        size;
            MEMORY_USAGE    usage;
        };

        struct SAllocationMemoryRequirementInfo
        {
            uint32_t    size;
            uint32_t    alignment;
        };

        struct SNativeAPIObjectMemoryAllocator
        {
            CMemoryPoolView     View;
        };

        template<typename VkObj, typename VkCreateInfo>
        struct TSVkObject
        {
            VkObj   handle;
            VKE_DEBUG_CODE( VkCreateInfo CreateInfo );
        };

        struct SCopyTextureInfo
        {
            //NativeAPI::Texture          hNativeAPISrcTexture;
            TexturePtr          pSrcTexture;
            TexturePtr          pDstTexture;
            TextureSize         Size;
            uint16_t            depth;
            TextureSize         SrcOffset;
            TextureSize         DstOffset;
        };

        struct SCopyTextureInfoEx
        {
            SCopyTextureInfo*           pBaseInfo;
            TEXTURE_STATE               srcTextureState;
            TEXTURE_STATE               dstTextureState;
            STextureSubresourceRange    SrcSubresource;
            STextureSubresourceRange    DstSubresource;
        };

        struct SCopyBufferInfo
        {
            struct SRegion
            {
                uint32_t    srcBufferOffset = UINT32_MAX;
                uint32_t    dstBufferOffset = UINT32_MAX;
                uint32_t    size;
            };
            using RegionArray = Utils::TCDynamicArray< SRegion >;

            //NativeAPI::Buffer           hNativeAPISrcBuffer;
            //NativeAPIBuffer           hNativeAPIDstBuffer;
            BufferPtr           pSrcBuffer;
            BufferPtr           pDstBuffer;
            SRegion             Region;
        };

        struct SBufferTextureRegion
        {
            uint32_t                    bufferOffset;
            uint32_t                    bufferRowLength;
            uint32_t                    bufferTextureHeight;
            STextureSubresourceRange    TextureSubresource;
            uint32_t                    textureOffsetX;
            uint32_t                    textureOffsetY;
            uint32_t                    textureOffsetZ;
            uint32_t                    textureWidth;
            uint32_t                    textureHeight;
            uint32_t                    textureDepth;
        };

        struct SCopyBufferToTextureInfo
        {
            using RegionArray = Utils::TCDynamicArray<SBufferTextureRegion>;
            //NativeAPI::Buffer           hNativeAPISrcBuffer;
            //NativeAPI::Texture          hNativeAPIDstTexture;
            BufferPtr           pSrcBuffer;
            TexturePtr          pDstTexture;
            TEXTURE_STATE       textureState;
            RegionArray         vRegions;
        };

        static const uint32_t DEFAULT_QUEUE_FAMILY_PROPERTY_COUNT = 16;

        using QueuePriorityArray = Utils::TCDynamicArray< float, DEFAULT_QUEUE_FAMILY_PROPERTY_COUNT >;
        using QueueFamilyPropertyArray = Utils::TCDynamicArray< VkQueueFamilyProperties, DEFAULT_QUEUE_FAMILY_PROPERTY_COUNT >;

        using UintArray = Utils::TCDynamicArray< uint32_t, DEFAULT_QUEUE_FAMILY_PROPERTY_COUNT >;
        using QueueTypeArray = UintArray[QueueTypes::_MAX_COUNT];
        using NativeAPIQueueArray = Utils::TCDynamicArray< NativeAPI::Queue >;

        struct SResourceBindingInfo
        {
            uint8_t     index;
            uint16_t    count;
            uint32_t    offset;
            uint32_t    range;
        };

        struct SUpdateBufferDescriptorSetInfo
        {
            struct SBufferInfo
            {
                NativeAPI::Buffer       hNativeAPIBuffer;
                NativeAPI::Size   offset;
                NativeAPI::Size   range;
            };
            using BufferInfoArray = Utils::TCDynamicArray< SBufferInfo, 4 >;
            uint32_t            binding;
            uint32_t            count;
            NativeAPI::DescriptorSet    hNativeAPISet;
            BufferInfoArray     vBufferInfos;
        };

        struct SUpdateTextureDescriptorSetInfo
        {
            struct STextureInfo
            {
                NativeAPI::Sampler      hNativeAPISampler;
                NativeAPI::TextureView  hNativeAPITextureView;
                TEXTURE_STATE   textureState;
            };

            using TextureInfoArray = Utils::TCDynamicArray< STextureInfo, 8 >;

            TextureInfoArray    vTextureInfos;
            NativeAPI::DescriptorSet    hNativeAPISet;
            uint8_t             binding;
            uint16_t            count;
        };

        struct SUpdateBindingInfo
        {
            struct SSamplerTextureInfo
            {
                NativeAPI::Sampler      hNativeAPISampler = NativeAPI::Null;
                NativeAPI::TextureView  hNativeAPITextureView = NativeAPI::Null;
                TEXTURE_STATE   textureState;
            };

            struct SBufferInfo
            {
                NativeAPI::Buffer       hNativeAPIBuffer;
                NativeAPI::Size   offset;
                NativeAPI::Size   range;
            };

            using BufferInfoArray = Utils::TCDynamicArray< SBufferInfo, 8 >;
            using TextureInfoArray = Utils::TCDynamicArray< SSamplerTextureInfo, 8 >;

            NativeAPI::DescriptorSet    hNativeAPISet;
            DESCRIPTOR_SET_TYPE type;
            uint8_t             binding;
            BufferInfoArray     vBuffers;
            TextureInfoArray    vTextures;
        };

        struct SQueueFamilyInfo
        {
            NativeAPIQueueArray       vQueues;
            QueuePriorityArray  vPriorities;
            uint32_t            index;
            QUEUE_TYPE          type;
        };

        using QueueFamilyInfoArray = Utils::TCDynamicArray< SQueueFamilyInfo >;

        struct SAdapterProperties
        {
            QueueFamilyInfoArray    vQueueInfos;
        };

        struct SVulkanDeviceFeatures
        {
            VkPhysicalDeviceFeatures2 Device;
            VkPhysicalDeviceVulkan11Features Device11;
            VkPhysicalDeviceVulkan12Features Device12;
            VkPhysicalDeviceMeshShaderFeaturesEXT MeshShaderEXT;
            VkPhysicalDeviceMeshShaderFeaturesNV MeshShaderNV;
            VkPhysicalDeviceRayTracingPipelineFeaturesKHR Raytracing10;
            VkPhysicalDeviceRayQueryFeaturesKHR Raytracing11;
            VkPhysicalDeviceRayTracingMotionBlurFeaturesNV Raytracing12;
            VkPhysicalDeviceDynamicRenderingFeaturesKHR DynamicRendering;
        };

        struct SDeviceProperties
        {
            QueueFamilyPropertyArray            vQueueFamilyProperties;
            QueueFamilyInfoArray                vQueueFamilies;

            struct
            {
                VkPhysicalDeviceProperties2             Device;
                VkPhysicalDeviceVulkan11Properties      Device11;
                VkPhysicalDeviceVulkan12Properties      Device12;
                VkPhysicalDeviceMemoryProperties2       Memory;
                VkPhysicalDeviceMeshShaderPropertiesEXT MeshShaderEXT;
                VkPhysicalDeviceMeshShaderPropertiesNV  MeshShaderNV;
                VkPhysicalDeviceRayTracingPipelinePropertiesKHR Raytracing10;
                VkPhysicalDeviceDescriptorIndexingProperties DescriptorIndexing;
                VkFormatProperties                      aFormatProperties[Formats::_MAX_COUNT];
            } Properties;

            SVulkanDeviceFeatures Features;

            VkPhysicalDeviceLimits&                     Limits = Properties.Device.properties.limits;

            void operator=( const SDeviceProperties& Rhs )
            {
                vQueueFamilyProperties = Rhs.vQueueFamilyProperties;
                vQueueFamilies = Rhs.vQueueFamilies;

                Memory::Copy<Formats::_MAX_COUNT>( Properties.aFormatProperties, Rhs.Properties.aFormatProperties );
                Memory::Copy( &Properties.Memory, &Rhs.Properties.Memory );
                Memory::Copy( &Properties.Device, &Rhs.Properties.Device );
                VKE_ASSERT2( false, "TODO: IMPLEMENT" );
            }
        };

        struct SDeviceInfo
        {
            struct
            {
                struct
                {
                    uint32_t    constantBufferOffset;
                    uint32_t    texelBufferOffset;
                    uint32_t    storageBufferOffset;
                    uint32_t    bufferCopyOffset;
                    uint32_t    bufferCopyRowPitch;
                    uint32_t    memoryMap;
                } Alignment;

                struct
                {
                    uint32_t maxAllocationCount;
                    uint32_t minMapAlignment;
                    uint32_t minTexelBufferOffsetAlignment;
                    uint32_t minConstantBufferOffsetAlignment;
                    uint32_t minStorageBufferOffsetAlignment;
                } Memory;

                struct
                {
                    int32_t     minTexel;
                    uint32_t    maxTexel;

                } Offset;

                struct
                {
                    uint32_t    maxDrawIndirect;
                    uint32_t    maxRenderTarget;
                } Count;

                struct
                {
                    uint32_t    maxRenderTargetWidth;
                    uint32_t    maxRenderTargetHeight;
                    uint32_t    maxTexture1DDimmension;
                    uint32_t    maxTexture2DDimmension;
                    uint32_t    maxTexture3DDimmension;
                    uint32_t    maxTextureCubeDimmension;
                } Texture;

                struct
                {

                } Buffer;

                struct
                {
                    struct
                    {
                        uint32_t    maxResourceCount;
                        uint32_t    maxStorageTextureCount;
                        uint32_t    maxTextureCount;
                        uint32_t    maxStorageBufferCount;
                        uint32_t    maxConstantBufferCount;
                        uint32_t    maxSamplerCount;
                    } Stage;

                    uint32_t    maxConstantBufferRange;
                    uint32_t    maxPushConstantsSize;
                } Binding;

                struct
                {
                    uint32_t    maxColorRenderTargetCount;
                } RenderPass;

                struct
                {
                    float       timestampPeriod;
                } Query;

                uint32_t        maxDrawIndexedIndexValue;

                uint32_t        maxClipDistance;

            } Limits;

            SDeviceFeatures Features;
        };

        struct SMemoryBarrierInfo
        {
            MEMORY_ACCESS_TYPE  srcMemoryAccess;
            MEMORY_ACCESS_TYPE  dstMemoryAccess;
        };

        struct STextureBarrierInfo : SMemoryBarrierInfo
        {
            NativeAPI::Texture                  hNativeAPITexture;
            TEXTURE_STATE               currentState;
            TEXTURE_STATE               newState;
            STextureSubresourceRange    SubresourceRange;
        };

        struct SBufferBarrierInfo : SMemoryBarrierInfo
        {
            NativeAPI::Buffer       hNativeAPIBuffer;
            uint32_t        size;
            uint32_t        offset;
        };

        struct SBarrierInfo
        {
            static const uint16_t MAX_BARRIER_COUNT = 16;
            using MemoryBarrierArray = Utils::TCDynamicArray< SMemoryBarrierInfo, MAX_BARRIER_COUNT >;
            using TextureBarrierArray = Utils::TCDynamicArray< STextureBarrierInfo, MAX_BARRIER_COUNT >;
            using BufferBarrierArray = Utils::TCDynamicArray< SBufferBarrierInfo, MAX_BARRIER_COUNT >;

            MemoryBarrierArray  vMemoryBarriers;
            TextureBarrierArray vTextureBarriers;
            BufferBarrierArray  vBufferBarriers;
        };

        struct VKE_API SNativeAPIExtension
        {
            /*SNativeAPIExtension() {}
            explicit SNativeAPIExtension( const vke_string& n ) : name{ n } {}
            explicit SNativeAPIExtension( vke_string&& n ) : name{ n } {}*/
            vke_string  name;
            bool        required = false;
            bool        supported = false;
            bool        enabled = false;
        };
        using NativeAPIExtArray = Utils::TCDynamicArray< SNativeAPIExtension, 1 >;
        using NativeAPIExtMap = vke_hash_map< vke_string, SNativeAPIExtension >;

        struct VKE_API SNativeAPIExtensionLayer
        {
            vke_string  name;
            bool        required = false;
            bool        supported = false;
            bool        enabled = false;
        };
        using NativeAPIExtLayerArray = Utils::TCDynamicArray< SNativeAPIExtensionLayer, 1 >;

        struct VKE_API SNativeAPIDrawInfo
        {
            NativeAPI::CommandBuffer    hCommandBuffer;
            uint32_t            vertexCount;
            uint32_t            instanceCount;
            uint32_t            firstVertex;
            uint32_t            firstInstance;
        };

        class VKE_API CDDI
        {
            friend class CDeviceContext;
            using AdapterArray = Utils::TCDynamicArray< NativeAPI::Adapter >;

            using GlobalICD = VkICD::Global;
            using InstanceICD = VkICD::Instance;
            using DeviceICD = VkICD::Device;

            public:

                struct AllocateDescs
                {
                    struct SDescSet
                    {
                        NativeAPI::DescriptorPool       hPool;
                        NativeAPI::DescriptorSetLayout* phLayouts;
                        uint32_t                count;
                        VKE_RENDER_SYSTEM_DEBUG_NAME;
                    };



                    struct SMemory
                    {
                        NativeAPI::Texture      hNativeAPITexture = NativeAPI::Null;
                        NativeAPI::Buffer       hNativeAPIBuffer = NativeAPI::Null;
                        uint32_t        size;
                        MEMORY_USAGE    memoryUsages;
                    };
                };

                struct FreeDescs
                {
                    struct SDescSet
                    {
                        NativeAPI::DescriptorPool       hPool;
                        NativeAPI::DescriptorSet*       phSets;
                        uint32_t                count;
                    };

                    struct SCommandBuffers
                    {
                        NativeAPI::CommandBufferPool    hPool;
                        NativeAPI::CommandBuffer*       pBuffers;
                        uint32_t                count;
                    };
                };

            public:

                static const GlobalICD&     GetGlobalICD() { return sGlobalICD; }
                static const InstanceICD&   GetInstantceICD() { return sInstanceICD; }

                static VkInstance&          GetInstance() { return sVkInstance; }

                Result              CreateDevice( const SCreateDeviceDesc& Info, CDeviceContext* pCtx );
                void                DestroyDevice();
                const DeviceICD&    GetDeviceICD() const { return m_ICD; }
                const DeviceICD&    GetICD() const { return m_ICD; }

                static Result       LoadICD(const SNativeAPILoadInfo& Info, SDriverInfo* pOut);
                static void         CloseICD();

                static
                const SNativeAPIExtension&    GetInstanceExtensionInfo( cstr_t pName );

                const NativeAPI::Device&        GetDevice() const { return m_hDevice; }
                const QueueFamilyInfoArray&   GetDeviceQueueInfos() const { return m_DeviceProperties.vQueueFamilies; }
                const NativeAPI::Adapter&       GetAdapter() const { return m_hAdapter; }

                static Result           QueryAdapters( AdapterInfoArray* pOut );

                void                    QueryDeviceInfo( SDeviceInfo* pOut );

                const SNativeAPIExtension&    GetExtensionInfo( cstr_t pName ) const;

                NativeAPI::Buffer               CreateBuffer( const SBufferDesc& Desc, const void* );
                void                    DestroyBuffer( NativeAPI::Buffer* phBuffer, const void* );
                NativeAPI::BufferView           CreateBufferView( const SBufferViewDesc& Desc, const void* );
                void                    DestroyBufferView( NativeAPI::BufferView* phBufferView, const void* );
                Result                  GetTextureFormatProperties( const STextureDesc&, STextureFormatProperties* );
                NativeAPI::Texture              CreateTexture( const STextureDesc& Desc, const void* );
                void                    DestroyTexture( NativeAPI::Texture* phImage, const void* );
                NativeAPI::TextureView          CreateTextureView( const STextureViewDesc& Desc, const void* );
                void                    DestroyTextureView( NativeAPI::TextureView* phImageView, const void* );
                NativeAPI::Framebuffer          CreateFramebuffer( const SFramebufferDesc& Desc, const void* );
                void                    DestroyFramebuffer( NativeAPI::Framebuffer* phFramebuffer, const void* );
                NativeAPI::CPUFence                CreateFence( const SFenceDesc& Desc, const void* );
                void                    DestroyFence( NativeAPI::CPUFence* phFence, const void* );
                NativeAPI::Fence            CreateFence( const SGPUFenceDesc& Desc, const void* );
                void                    DestroyFence( NativeAPI::Fence* phSemaphore, const void* );
                NativeAPI::RenderPass           CreateRenderPass( const SRenderPassDesc& Desc, const void* );
                void                    DestroyRenderPass( NativeAPI::RenderPass* phPass, const void* );
                NativeAPI::CommandBufferPool    CreateCommandBufferPool( const SCommandBufferPoolDesc& Desc, const void* );
                void                    DestroyCommandBufferPool( NativeAPI::CommandBufferPool* phPool, const void* );
                NativeAPI::DescriptorPool       CreateDescriptorPool( const SDescriptorPoolDesc& Desc, const void* );
                void                    DestroyDescriptorPool( NativeAPI::DescriptorPool* phPool, const void* );
                NativeAPI::DescriptorSetLayout  CreateDescriptorSetLayout( const SDescriptorSetLayoutDesc& Desc, const void* );
                void                    DestroyDescriptorSetLayout( NativeAPI::DescriptorSetLayout* phLayout, const void* );
                NativeAPI::Pipeline             CreatePipeline( const SPipelineDesc& Desc, const void* );
                void                    DestroyPipeline( NativeAPI::Pipeline* phPipeline, const void* );
                NativeAPI::PipelineLayout       CreatePipelineLayout( const SPipelineLayoutDesc& Desc, const void* );
                void                    DestroyPipelineLayout( NativeAPI::PipelineLayout* phLayout, const void* );
                NativeAPI::Shader               CreateShader( const SShaderData& Desc, const void* );
                void                    DestroyShader( NativeAPI::Shader* phShader, const void* );
                NativeAPI::Sampler              CreateSampler( const SSamplerDesc& Desc, const void* );
                void                    DestroySampler( NativeAPI::Sampler* phSampler, const void* );
                NativeAPI::Event                CreateEvent( const SEventDesc& Desc, const void* );
                void                    DestroyEvent( NativeAPI::Event* phEvent, const void* );

                Result          AllocateObjects(const AllocateDescs::SDescSet& Info, NativeAPI::DescriptorSet* pSets );
                void            FreeObjects( const FreeDescs::SDescSet& );
                Result          AllocateObjects( const SAllocateCommandBufferInfo& Info, NativeAPI::CommandBuffer* pBuffers );
                void            FreeObjects( const SFreeCommandBufferInfo& );

                Result          GetBufferMemoryRequirements( const NativeAPI::Buffer& hBuffer, SAllocationMemoryRequirementInfo* pOut );
                Result          GetTextureMemoryRequirements( const NativeAPI::Texture& hTexture, SAllocationMemoryRequirementInfo* pOut );
                void            UpdateDesc( SBufferDesc* pInOut );

                void            GetFormatFeatures( FORMAT fmt, STextureFormatFeatures* pOut ) const;            

                template<RESOURCE_TYPE Type>
                Result          Bind( const SBindMemoryInfo& Info );
                void            Bind( const SBindPipelineInfo& Info );
                void            UnbindPipeline( const NativeAPI::CommandBuffer&, const NativeAPI::Pipeline& );
                void            Bind( const SBindNativeAPIDescriptorSetsInfo& Info );
                void            Bind( const SBindRenderPassInfo& Info );
                void            UnbindRenderPass( const NativeAPI::CommandBuffer&, const NativeAPI::RenderPass& );
                void            Bind( const NativeAPI::CommandBuffer& hNativeAPICmdBuffer, const NativeAPI::Buffer& hNativeAPIBuffer, const uint32_t offset );
                void            Bind( const NativeAPI::CommandBuffer& hNativeAPICmdBuffer, const NativeAPI::Buffer& hNativeAPIBuffer, const uint32_t offset, const INDEX_TYPE& type );

                void            Free( NativeAPI::Memory* phMemory, const void* = nullptr );

                bool            IsSignaled( const NativeAPI::CPUFence& hFence ) const;
                void            Reset( NativeAPI::CPUFence* phFence );
                Result          WaitForFences( const NativeAPI::CPUFence& hFence, uint64_t timeout );
                NativeAPI::FenceValue GetGPUFenceValue( const NativeAPI::Fence& );
                Result          WaitForQueue( const NativeAPI::Queue& hQueue );
                Result          WaitForDevice();

                void            Update( const SUpdateBufferDescriptorSetInfo& Info );
                void            Update( const SUpdateTextureDescriptorSetInfo& Info );
                void            Update( const NativeAPI::DescriptorSet& hNativeAPISet, const SUpdateBindingsHelper& Info );
                void            Update( const NativeAPI::DescriptorSet& hNativeAPISrcSet, NativeAPI::DescriptorSet* phNativeAPIDstOut );

                Result          Allocate( const SAllocateMemoryDesc& Desc, SAllocateMemoryData* pOut );
                MEMORY_HEAP_TYPE GetMemoryHeapType( MEMORY_USAGE usage ) const;
                size_t GetMemoryHeapTotalSize( MEMORY_HEAP_TYPE ) const;
                size_t GetMemoryHeapCurrentSize( MEMORY_HEAP_TYPE ) const;
                void*           MapMemory( const SMapMemoryInfo& Info );
                void            UnmapMemory( const NativeAPI::Memory& hNativeAPIMemory );

                void            Reset( const NativeAPI::CommandBuffer& hCommandBuffer );
                void            BeginCommandBuffer( const NativeAPI::CommandBuffer& hCommandBuffer );
                void            EndCommandBuffer( const NativeAPI::CommandBuffer& hCommandBuffer );
                //void            BeginRenderPass( const NativeAPI::CommandBuffer& hCommandBuffer, const SBeginRenderPassInfo& Info );
                //void            EndRenderPass( const NativeAPI::CommandBuffer& hCommandBuffer );

                void            Barrier( const NativeAPI::CommandBuffer& hCommandBuffer, const SBarrierInfo& Info );

                // Command Buffer
                void            SetState( const NativeAPI::CommandBuffer& hCommandBuffer, const SViewportDesc& Desc );
                void            SetState( const NativeAPI::CommandBuffer& hCommandBuffer, const SScissorDesc& Desc );

                void            Draw( const NativeAPI::CommandBuffer& hCommandBuffer, const uint32_t& vertexCount,
                    const uint32_t& instanceCount, const uint32_t& firstVertex, const uint32_t& firstInstance );
                void            DrawIndexed( const NativeAPI::CommandBuffer& hCommandBuffer, const SDrawParams& Params );
                void DrawMesh( const NativeAPI::CommandBuffer& hCommandBuffer, uint32_t width, uint32_t height,
                               uint32_t depth );
                // Dynamic rendering
                void            BeginRenderPass( NativeAPI::CommandBuffer, const SBeginRenderPassInfo2& );
                void            EndRenderPass(NativeAPI::CommandBuffer);

                // Copy
                void            Copy( const NativeAPI::CommandBuffer& hNativeAPICmdBuffer, const SCopyTextureInfoEx& Info );
                void            Copy( const NativeAPI::CommandBuffer& hCmdBuffer, const SCopyBufferInfo& Info );
                void            Copy( const NativeAPI::CommandBuffer& hNativeAPICmdBuffer, const SCopyBufferToTextureInfo& Info );
                void            Blit( const NativeAPI::CommandBuffer& hAPICmdBuffer, const SBlitTextureInfo& Info );

                // Events
                void            SetEvent( const NativeAPI::Event& hNativeAPIEvent );
                void            SetEvent( const NativeAPI::CommandBuffer& hNativeAPICmdBuffer, const NativeAPI::Event& hNativeAPIEvent, const PIPELINE_STAGES& stages );
                void            Reset( const NativeAPI::Event& hNativeAPIInOut );
                void            Reset( const NativeAPI::CommandBuffer& hNativeAPICmdBuffer, const NativeAPI::Event& hNativeAPIEvent, const PIPELINE_STAGES& stages );
                bool            IsSet( const NativeAPI::Event& hNativeAPIEvent );

                Result          Submit( const SSubmitInfo& Info );
                Result          Submit( const SSubmitInfo& Info, const NativeAPI::Fence& hSignalFence, NativeAPI::FenceValue signalValue );
                Result          Present( const SPresentData& Info );

                Result          CreateSwapChain( const SSwapChainDesc& Desc, const void*, SNativeAPISwapChain* pInOut );
                void            DestroySwapChain( SNativeAPISwapChain* pInOut, const void* = nullptr );
                Result          ReCreateSwapChain( const SSwapChainDesc& Desc, SNativeAPISwapChain* pOut );
                Result          QueryPresentSurfaceCaps( const NativeAPI::PresentSurface& hSurface, SPresentSurfaceCaps* pOut );
                Result          GetCurrentBackBufferIndex( const SNativeAPISwapChain& SwapChain, const SNativeAPIGetBackBufferInfo& Info, uint32_t* pOut );

                static void     Convert( const SClearValue& In, NativeAPI::ClearValue* pOut );

                // Debug
                void            BeginDebugInfo( const NativeAPI::CommandBuffer& hNativeAPICmdBuff, const SDebugInfo* pInfo);
                void            EndDebugInfo( const NativeAPI::CommandBuffer& hNativeAPICmdBuff );
                void            SetObjectDebugName( const uint64_t& handle, const uint32_t& objType, cstr_t pName ) const;
                void            SetQueueDebugName( uint64_t, cstr_t ) const;

            protected:

                template<VkObjectType ObjectType, typename NativeAPIObjectT>
                VkResult        _CreateDebugInfo( const NativeAPIObjectT& hNativeAPIObject, cstr_t pName );

            protected:

                static GlobalICD                sGlobalICD;
                static InstanceICD              sInstanceICD;
                static handle_t                 shICD;
                static VkInstance               sVkInstance;
                static AdapterArray             svAdapters;
                static NativeAPIExtArray              svExtensions;
                static NativeAPIExtLayerArray         svLayers;
                static VkDebugReportCallbackEXT sVkDebugReportCallback;
                static VkDebugUtilsMessengerEXT sVkDebugMessengerCallback;

                DeviceICD                           m_ICD;
                
                NativeAPIExtMap                           m_mExtensions;
                NativeAPI::Device                           m_hDevice = NativeAPI::Null;
                NativeAPI::Adapter                          m_hAdapter = NativeAPI::Null;
                CDeviceContext*                     m_pCtx;
                SDeviceInfo                         m_DeviceInfo;
                SDeviceProperties                   m_DeviceProperties;
                VkDeviceSize                        m_aHeapSizes[ VK_MAX_MEMORY_HEAPS ];
                uint32_t m_aHeapTypeToHeapIndexMap[ MemoryHeapTypes::_MAX_COUNT ]; // MEMORY_HEAP_TYPE -> memory
                                                                                        // type index map
                MEMORY_HEAP_TYPE                    m_aHeapIndexToHeapTypeMap[ VK_MAX_MEMORY_HEAPS ];
                uint32_t m_instanceVersion = 0;
        };

        template<RESOURCE_TYPE Type>
        Result CDDI::Bind( const SBindMemoryInfo& Info )
        {
            VkResult res = VK_INCOMPLETE;
            if( Type == ResourceTypes::TEXTURE )
            {
                res = m_ICD.vkBindImageMemory( m_hDevice, Info.hNativeAPITexture, Info.hNativeAPIMemory, Info.offset );
            }
            else if( Type == ResourceTypes::BUFFER )
            {
                res = m_ICD.vkBindBufferMemory( m_hDevice, Info.hNativeAPIBuffer, Info.hNativeAPIMemory, Info.offset );
            }
            return res == VK_SUCCESS ? VKE_OK : VKE_FAIL;
        }

        template<VkObjectType ObjectType, typename NativeAPIObjectT>
        VkResult CDDI::_CreateDebugInfo( const NativeAPIObjectT& hNativeAPIObject, cstr_t pName )
        {
            VkResult ret = VK_SUCCESS;
#if VKE_RENDER_SYSTEM_DEBUG
            if( sInstanceICD.vkSetDebugUtilsObjectNameEXT )
            {
                VkDebugUtilsObjectNameInfoEXT ni = { VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT };
                ni.objectHandle = ( uint64_t )(hNativeAPIObject);
                ni.objectType = ObjectType;
                ni.pObjectName = pName;
                ret = sInstanceICD.vkSetDebugUtilsObjectNameEXT( m_hDevice, &ni );
            }
#endif // VKE_RENDER_SYSTEM_DEBUG
            VK_ERR( ret );
            return ret;
        }

    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDER_SYSTEM