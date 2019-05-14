#pragma once

#if VKE_VULKAN_RENDERER
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
            DDIMemory   hMemory;
            uint32_t    offset;
            uint32_t    size;
        };

        struct SAllocateMemoryDesc
        {
            uint32_t        size;
            MEMORY_USAGE    usage;
        };

        struct SAllocationMemoryRequirements
        {
            uint32_t    size;
            uint32_t    alignment;
        };

        struct SDDIObjectMemoryAllocator
        {
            CMemoryPoolView     View;
        };

        template<typename VkObj, typename VkCreateInfo>
        struct TSVkObject
        {
            VkObj   handle;
            VKE_DEBUG_CODE( VkCreateInfo CreateInfo );
        };

        struct SCopyImageInfo
        {
            DDITexture          hDDISrcTexture;
            DDITexture          hDDIDstTexture;
            TEXTURE_STATE       hSrcTextureState;
            TEXTURE_STATE       hDstTextureState;
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

            DDIBuffer           hDDISrcBuffer;
            DDIBuffer           hDDIDstBuffer;
            SRegion             Region;
        };

        struct SCopyBufferToImageInfo
        {

        };

        static const uint32_t DEFAULT_QUEUE_FAMILY_PROPERTY_COUNT = 16;
        
        using QueuePriorityArray = Utils::TCDynamicArray< float, DEFAULT_QUEUE_FAMILY_PROPERTY_COUNT >;
        using QueueFamilyPropertyArray = Utils::TCDynamicArray< VkQueueFamilyProperties, DEFAULT_QUEUE_FAMILY_PROPERTY_COUNT >;
        
        using UintArray = Utils::TCDynamicArray< uint32_t, DEFAULT_QUEUE_FAMILY_PROPERTY_COUNT >;
        using QueueTypeArray = UintArray[QueueTypes::_MAX_COUNT];
        using DDIQueueArray = Utils::TCDynamicArray< DDIQueue >;

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
                DDIBuffer       hDDIBuffer;
                DDIDeviceSize   offset;
                DDIDeviceSize   range;
            };
            using BufferInfoArray = Utils::TCDynamicArray< SBufferInfo, 4 >;
            uint32_t            binding;
            uint32_t            count;
            DDIDescriptorSet    hDDISet;
            BufferInfoArray     vBufferInfos;
        };

        struct SQueueFamilyInfo
        {
            DDIQueueArray       vQueues;
            QueuePriorityArray  vPriorities;
            uint32_t            index;
            QUEUE_TYPE          type;
        };

        using QueueFamilyInfoArray = Utils::TCDynamicArray< SQueueFamilyInfo >;

        struct SAdapterProperties
        {
            QueueFamilyInfoArray    vQueueInfos;
        };

        struct SDeviceProperties
        {
            QueueFamilyPropertyArray            vQueueFamilyProperties;
            QueueFamilyInfoArray                vQueueFamilies;
            
            struct
            {
                VkPhysicalDeviceProperties2             Device;
                VkPhysicalDeviceMemoryProperties2       Memory;
                VkPhysicalDeviceMeshShaderPropertiesNV  MeshShaderNV;
                VkFormatProperties                      aFormatProperties[Formats::_MAX_COUNT];
            } Properties;

            struct
            {
                VkPhysicalDeviceFeatures2               Device;
                VkPhysicalDeviceMeshShaderFeaturesNV    MeshShaderNV;
            } Features;

            VkPhysicalDeviceLimits&                     Limits = Properties.Device.properties.limits;

            void operator=( const SDeviceProperties& Rhs )
            {
                vQueueFamilyProperties = Rhs.vQueueFamilyProperties;
                vQueueFamilies = Rhs.vQueueFamilies;

                Memory::Copy<Formats::_MAX_COUNT>( Properties.aFormatProperties, Rhs.Properties.aFormatProperties );
                Memory::Copy( &Properties.Memory, &Rhs.Properties.Memory );
                Memory::Copy( &Properties.Device, &Rhs.Properties.Device );
                VKE_ASSERT( false, "TODO: IMPLEMENT" );
            }
        };

        struct SDeviceInfo
        {
            
        };

        struct SMemoryBarrierInfo
        {
            MEMORY_ACCESS_TYPE  srcMemoryAccess;
            MEMORY_ACCESS_TYPE  dstMemoryAccess;
        };

        struct STextureBarrierInfo : SMemoryBarrierInfo
        {
            DDITexture                  hDDITexture;
            TEXTURE_STATE               currentState;
            TEXTURE_STATE               newState;
            STextureSubresourceRange    SubresourceRange;
        };

        struct SBufferBarrierInfo : SMemoryBarrierInfo
        {
            DDIBuffer       hDDIBuffer;
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

        struct VKE_API SDDIExtension
        {
            /*SDDIExtension() {}
            explicit SDDIExtension( const vke_string& n ) : name{ n } {}
            explicit SDDIExtension( vke_string&& n ) : name{ n } {}*/
            vke_string  name;
            bool        required = false;
            bool        supported = false;
            bool        enabled = false;
        };
        using DDIExtArray = Utils::TCDynamicArray< SDDIExtension, 1 >;
        using DDIExtMap = vke_hash_map< vke_string, SDDIExtension >;

        struct VKE_API SDDIExtensionLayer
        {
            vke_string  name;
            bool        required = false;
            bool        supported = false;
            bool        enabled = false;
        };
        using DDIExtLayerArray = Utils::TCDynamicArray< SDDIExtensionLayer, 1 >;

        struct VKE_API SDDIDrawInfo
        {
            DDICommandBuffer    hCommandBuffer;
            uint32_t            vertexCount;
            uint32_t            instanceCount;
            uint32_t            firstVertex;
            uint32_t            firstInstance;
        };

        class VKE_API CDDI
        {
            friend class CDeviceContext;
            using AdapterArray = Utils::TCDynamicArray< DDIAdapter >;

            using GlobalICD = VkICD::Global;
            using InstanceICD = VkICD::Instance;
            using DeviceICD = VkICD::Device;   

            public:

                struct AllocateDescs
                {
                    struct SDescSet
                    {
                        DDIDescriptorPool       hPool;
                        DDIDescriptorSetLayout* phLayouts;
                        uint32_t                count;
                    };

                    

                    struct SMemory
                    {
                        DDITexture      hDDITexture = DDI_NULL_HANDLE;
                        DDIBuffer       hDDIBuffer = DDI_NULL_HANDLE;
                        uint32_t        size;
                        MEMORY_USAGE    memoryUsages;
                    };
                };

                struct FreeDescs
                {
                    struct SDescSet
                    {
                        DDIDescriptorPool       hPool;
                        DDIDescriptorSet*       phSets;
                        uint32_t                count;
                    };

                    struct SCommandBuffers
                    {
                        DDICommandBufferPool    hPool;
                        DDICommandBuffer*       pBuffers;
                        uint32_t                count;
                    };
                };

            public:

                static const GlobalICD&     GetGlobalICD() { return sGlobalICD; }
                static const InstanceICD&   GetInstantceICD() { return sInstanceICD; }

                static VkInstance&          GetInstance() { return sVkInstance; }

                Result              CreateDevice( CDeviceContext* pCtx );
                void                DestroyDevice();
                const DeviceICD&    GetDeviceICD() const { return m_ICD; }
                const DeviceICD&    GetICD() const { return m_ICD; }

                static Result       LoadICD(const SDDILoadInfo& Info);
                static void         CloseICD();

                static
                const SDDIExtension&    GetInstanceExtensionInfo( cstr_t pName );

                const DDIDevice&        GetDevice() const { return m_hDevice; }
                const QueueFamilyInfoArray&   GetDeviceQueueInfos() const { return m_DeviceProperties.vQueueFamilies; }
                const DDIAdapter&       GetAdapter() const { return m_hAdapter; }

                static Result           QueryAdapters( AdapterInfoArray* pOut );

                const SDDIExtension&    GetExtensionInfo( cstr_t pName ) const;

                DDIBuffer               CreateObject( const SBufferDesc& Desc, const void* );
                void                    DestroyObject( DDIBuffer* phBuffer, const void* );
                DDIBufferView           CreateObject( const SBufferViewDesc& Desc, const void* );
                void                    DestroyObject( DDIBufferView* phBufferView, const void* );
                DDITexture              CreateObject( const STextureDesc& Desc, const void* );
                void                    DestroyObject( DDITexture* phImage, const void* );
                DDITextureView          CreateObject( const STextureViewDesc& Desc, const void* );
                void                    DestroyObject( DDITextureView* phImageView, const void* );
                DDIFramebuffer          CreateObject( const SFramebufferDesc& Desc, const void* );
                void                    DestroyObject( DDIFramebuffer* phFramebuffer, const void* );
                DDIFence                CreateObject( const SFenceDesc& Desc, const void* );
                void                    DestroyObject( DDIFence* phFence, const void* );
                DDISemaphore            CreateObject( const SSemaphoreDesc& Desc, const void* );
                void                    DestroyObject( DDISemaphore* phSemaphore, const void* );
                DDIRenderPass           CreateObject( const SRenderPassDesc& Desc, const void* );
                void                    DestroyObject( DDIRenderPass* phPass, const void* );
                DDICommandBufferPool    CreateObject( const SCommandBufferPoolDesc& Desc, const void* );
                void                    DestroyObject( DDICommandBufferPool* phPool, const void* );
                DDIDescriptorPool       CreateObject( const SDescriptorPoolDesc& Desc, const void* );
                void                    DestroyObject( DDIDescriptorPool* phPool, const void* );
                DDIDescriptorSetLayout  CreateObject( const SDescriptorSetLayoutDesc& Desc, const void* );
                void                    DestroyObject( DDIDescriptorSetLayout* phLayout, const void* );
                DDIPipeline             CreateObject( const SPipelineDesc& Desc, const void* );
                void                    DestroyObject( DDIPipeline* phPipeline, const void* );
                DDIPipelineLayout       CreateObject( const SPipelineLayoutDesc& Desc, const void* );
                void                    DestroyObject( DDIPipelineLayout* phLayout, const void* );
                DDIShader               CreateObject( const SShaderData& Desc, const void* );
                void                    DestroyObject( DDIShader* phShader, const void* );

                Result          AllocateObjects(const AllocateDescs::SDescSet& Info, DDIDescriptorSet* pSets );
                void            FreeObjects( const FreeDescs::SDescSet& );
                Result          AllocateObjects( const SAllocateCommandBufferInfo& Info, DDICommandBuffer* pBuffers );
                void            FreeObjects( const SFreeCommandBufferInfo& );

                Result          GetMemoryRequirements( const DDIBuffer& hBuffer, SAllocationMemoryRequirements* pOut );
                Result          GetMemoryRequirements( const DDITexture& hTexture, SAllocationMemoryRequirements* pOut );
                void            UpdateDesc( SBufferDesc* pInOut );

                template<RESOURCE_TYPE Type>
                Result          Bind( const SBindMemoryInfo& Info );
                void            Bind( const SBindPipelineInfo& Info );
                void            Unbind( const DDICommandBuffer&, const DDIPipeline& );
                void            Bind( const SBindDescriptorSetsInfo& Info );
                void            Bind( const SBindRenderPassInfo& Info );
                void            Unbind( const DDICommandBuffer&, const DDIRenderPass& );
                void            Bind( const SBindVertexBufferInfo& Info );
                void            Bind( const SBindIndexBufferInfo& Info );

                void            Free( DDIMemory* phMemory, const void* = nullptr );

                bool            IsReady( const DDIFence& hFence );
                void            Reset( DDIFence* phFence );
                Result          WaitForFences( const DDIFence& hFence, uint64_t timeout );
                Result          WaitForQueue( const DDIQueue& hQueue );
                Result          WaitForDevice();

                void            Update( const SUpdateBufferDescriptorSetInfo& Info );

                Result          Allocate( const SAllocateMemoryDesc& Desc, SAllocateMemoryData* pOut );
                void*           MapMemory( const SMapMemoryInfo& Info );
                void            UnmapMemory( const DDIMemory& hDDIMemory );

                void            Reset( const DDICommandBuffer& hCommandBuffer );
                void            BeginCommandBuffer( const DDICommandBuffer& hCommandBuffer );
                void            EndCommandBuffer( const DDICommandBuffer& hCommandBuffer );
                //void            BeginRenderPass( const DDICommandBuffer& hCommandBuffer, const SBeginRenderPassInfo& Info );
                //void            EndRenderPass( const DDICommandBuffer& hCommandBuffer );

                void            Barrier( const DDICommandBuffer& hCommandBuffer, const SBarrierInfo& Info );

                // Command Buffer
                void            SetState( const DDICommandBuffer& hCommandBuffer, const SViewportDesc& Desc );
                void            SetState( const DDICommandBuffer& hCommandBuffer, const SScissorDesc& Desc );

                void            Draw( const DDICommandBuffer& hCommandBuffer, const uint32_t& vertexCount,
                    const uint32_t& instanceCount, const uint32_t& firstVertex, const uint32_t& firstInstance );


                // Copy
                void            Copy( const SCopyImageInfo& Info );
                void            Copy( const DDICommandBuffer& hCmdBuffer, const SCopyBufferInfo& Info );
                void            Copy( const SCopyBufferToImageInfo& Info );

                Result          Submit( const SSubmitInfo& Info );
                Result          Present( const SPresentData& Info );

                Result          CreateSwapChain( const SSwapChainDesc& Desc, const void*, SDDISwapChain* pInOut );
                void            DestroySwapChain( SDDISwapChain* pInOut, const void* = nullptr );
                Result          ReCreateSwapChain( const SSwapChainDesc& Desc, SDDISwapChain* pOut );
                Result          QueryPresentSurfaceCaps( const DDIPresentSurface& hSurface, SPresentSurfaceCaps* pOut );
                Result          GetCurrentBackBufferIndex( const SDDISwapChain& SwapChain, const SDDIGetBackBufferInfo& Info, uint32_t* pOut );

                Result          CompileShader( const SCompileShaderInfo& Info, SCompileShaderData* pOut );

                static void     Convert( const SClearValue& In, DDIClearValue* pOut );

            protected:

                template<VkObjectType ObjectType, typename DDIObjectT>
                VkResult        _CreateDebugInfo( const DDIObjectT& hDDIObject, cstr_t pName );

            protected:

                static GlobalICD                sGlobalICD;
                static InstanceICD              sInstanceICD;
                static handle_t                 shICD;
                static VkInstance               sVkInstance;
                static AdapterArray             svAdapters;
                static DDIExtArray              svExtensions;
                static DDIExtLayerArray         svLayers;
                static VkDebugReportCallbackEXT sVkDebugReportCallback;
                static VkDebugUtilsMessengerEXT sVkDebugMessengerCallback;

                DeviceICD                           m_ICD;
                DDIExtMap                           m_mExtensions;
                DDIDevice                           m_hDevice = DDI_NULL_HANDLE;
                DDIAdapter                          m_hAdapter = DDI_NULL_HANDLE;
                CDeviceContext*                     m_pCtx;
                SDeviceInfo                         m_DeviceInfo;
                SDeviceProperties                   m_DeviceProperties;
                VkDeviceSize                        m_aHeapSizes[ VK_MAX_MEMORY_HEAPS ];
        };

        template<RESOURCE_TYPE Type>
        Result CDDI::Bind( const SBindMemoryInfo& Info )
        {
            VkResult res = VK_INCOMPLETE;
            if( Type == ResourceTypes::TEXTURE )
            {
                res = m_ICD.vkBindImageMemory( m_hDevice, Info.hDDITexture, Info.hDDIMemory, Info.offset );
            }
            else if( Type == ResourceTypes::BUFFER )
            {
                res = m_ICD.vkBindBufferMemory( m_hDevice, Info.hDDIBuffer, Info.hDDIMemory, Info.offset );
            }
            return res == VK_SUCCESS ? VKE_OK : VKE_FAIL;
        }

        template<VkObjectType ObjectType, typename DDIObjectT>
        VkResult CDDI::_CreateDebugInfo( const DDIObjectT& hDDIObject, cstr_t pName )
        {
            VkResult ret = VK_SUCCESS;
#if VKE_RENDERER_DEBUG
            if( sInstanceICD.vkSetDebugUtilsObjectNameEXT )
            {
                VkDebugUtilsObjectNameInfoEXT ni = { VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT };
                ni.objectHandle = reinterpret_cast<uint64_t>(hDDIObject);
                ni.objectType = ObjectType;
                ni.pObjectName = pName;
                ret = sInstanceICD.vkSetDebugUtilsObjectNameEXT( m_hDevice, &ni ); 
            }
#endif // VKE_RENDERER_DEBUG
            VK_ERR( ret );
            return ret;
        }

    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER