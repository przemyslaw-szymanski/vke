#pragma once

#if VKE_VULKAN_RENDERER
#include "RenderSystem/Vulkan/Vulkan.h"
#include "RenderSystem/Common.h"
#include "RenderSystem/CDDITypes.h"



namespace VKE
{
    namespace RenderSystem
    {
        class CDeviceContext;
        class CDDI;

        

        template<typename VkObj, typename VkCreateInfo>
        struct TSVkObject
        {
            VkObj   handle;
            VKE_DEBUG_CODE( VkCreateInfo CreateInfo );
        };

        static const uint32_t DEFAULT_QUEUE_FAMILY_PROPERTY_COUNT = 16;
        
        using QueuePriorityArray = Utils::TCDynamicArray< float, DEFAULT_QUEUE_FAMILY_PROPERTY_COUNT >;
        using QueueFamilyPropertyArray = Utils::TCDynamicArray< VkQueueFamilyProperties, DEFAULT_QUEUE_FAMILY_PROPERTY_COUNT >;
        
        using UintArray = Utils::TCDynamicArray< uint32_t, DEFAULT_QUEUE_FAMILY_PROPERTY_COUNT >;
        using QueueTypeArray = UintArray[QueueTypes::_MAX_COUNT];
        using DDIQueueArray = Utils::TCDynamicArray< DDIQueue >;

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
            VkFormatProperties                  aFormatProperties[Formats::_MAX_COUNT];
            VkPhysicalDeviceMemoryProperties    vkMemProperties;
            VkPhysicalDeviceProperties          vkProperties;
            VkPhysicalDeviceFeatures            vkFeatures;

            void operator=( const SDeviceProperties& Rhs )
            {
                vQueueFamilyProperties = Rhs.vQueueFamilyProperties;
                vQueueFamilies = Rhs.vQueueFamilies;

                Memory::Copy<Formats::_MAX_COUNT>( aFormatProperties, Rhs.aFormatProperties );
                Memory::Copy( &vkMemProperties, &Rhs.vkMemProperties );
                Memory::Copy( &vkProperties, &Rhs.vkProperties );
                Memory::Copy( &vkFeatures, &Rhs.vkFeatures );
            }
        };

        struct SDeviceInfo
        {
            VkPhysicalDeviceProperties          Properties;
            VkPhysicalDeviceMemoryProperties    MemoryProperties;
            VkPhysicalDeviceFeatures            Features;
            VkPhysicalDeviceLimits              Limits;
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
                        DDITexture      hImage = DDI_NULL_HANDLE;
                        DDIBuffer       hBuffer = DDI_NULL_HANDLE;
                        uint32_t        size;
                        MEMORY_USAGES   memoryUsages;
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

                const DDIDevice&        GetDevice() const { return m_hDevice; }
                const QueueFamilyInfoArray&   GetDeviceQueueInfos() const { return m_DeviceProperties.vQueueFamilies; }
                const DDIAdapter&       GetAdapter() const { return m_hAdapter; }

                static Result           QueryAdapters( AdapterInfoArray* pOut );

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

                template<RESOURCE_TYPE Type>
                Result          Allocate( const AllocateDescs::SMemory& Desc, const void*, SMemoryAllocateData* pOut );
                template<RESOURCE_TYPE Type>
                Result          Bind( const SBindMemoryInfo& Info );
                void            Bind( const SBindPipelineInfo& Info );
                void            Bind( const SBindDescriptorSetsInfo& Info );
                void            Bind( const SBindRenderPassInfo& Info );
                void            Bind( const SBindVertexBufferInfo& Info );
                void            Bind( const SBindIndexBufferInfo& Info );
                void            Free( DDIMemory* phMemory, const void* = nullptr );

                bool            IsReady( const DDIFence& hFence );
                void            Reset( DDIFence* phFence );
                Result          WaitForFences( const DDIFence& hFence, uint64_t timeout );
                Result          WaitForQueue( const DDIQueue& hQueue );
                Result          WaitForDevice();

                void            BeginCommandBuffer( const DDICommandBuffer& hCommandBuffer );
                void            EndCommandBuffer( const DDICommandBuffer& hCommandBuffer );
                void            BeginRenderPass( const DDICommandBuffer& hCommandBuffer, const SRenderPassInfo& Info );
                void            EndRenderPass( const DDICommandBuffer& hCommandBuffer );

                Result          Submit( const SSubmitInfo& Info );
                Result          Present( const SPresentInfo& Info );

                Result          CreateSwapChain( const SSwapChainDesc& Desc, const void*, SDDISwapChain* pInOut );
                void            DestroySwapChain( SDDISwapChain* pInOut, const void* = nullptr );
                Result          QueryPresentSurfaceCaps( const DDIPresentSurface& hSurface, SPresentSurfaceCaps* pOut );
                uint32_t        GetCurrentBackBufferIndex( const SDDISwapChain& SwapChain, const SDDIGetBackBufferInfo& Info );

                Result          CompileShader( const SCompileShaderInfo& Info, SCompileShaderData* pOut );

            protected:

                Result          _Allocate( const AllocateDescs::SMemory& Desc, const VkMemoryRequirements& vkRequirements,
                    const void*, SMemoryAllocateData* pData );

            protected:

                static GlobalICD            sGlobalICD;
                static InstanceICD          sInstanceICD;
                static handle_t             shICD;
                static VkInstance           sVkInstance;
                static AdapterArray         svAdapters;

                DeviceICD                           m_ICD;
                DDIDevice                           m_hDevice = DDI_NULL_HANDLE;
                DDIAdapter                          m_hAdapter = DDI_NULL_HANDLE;
                CDeviceContext*                     m_pCtx;
                SDeviceInfo                         m_DeviceInfo;
                SDeviceProperties                   m_DeviceProperties;
                VkPhysicalDeviceMemoryProperties    m_vkMemoryProperties;
        };

        template<RESOURCE_TYPE Type>
        Result CDDI::Allocate( const AllocateDescs::SMemory& Desc, const void* pAllocator,
            SMemoryAllocateData* pData )
        {
            VkMemoryRequirements vkRequirements;
            if( Type == ResourceTypes::TEXTURE )
            {
                VKE_ASSERT( Desc.hImage != DDI_NULL_HANDLE, "" );
                m_ICD.vkGetImageMemoryRequirements( m_hDevice, Desc.hImage, &vkRequirements );
            }
            else if( Type == ResourceTypes::BUFFER )
            {
                VKE_ASSERT( Desc.hBuffer != DDI_NULL_HANDLE, "" );
                m_ICD.vkGetBufferMemoryRequirements( m_hDevice, Desc.hBuffer, &vkRequirements );
            }
            else
            {
                VKE_ASSERT( nullptr, "Bad template parameter: RESOURCE_TYPE" );
            }
            return _Allocate( Desc, vkRequirements, pAllocator, pData );
        }

        template<RESOURCE_TYPE Type>
        Result CDDI::Bind( const SBindMemoryInfo& Info )
        {
            VkResult res = VK_INCOMPLETE;
            if( Type == ResourceTypes::TEXTURE )
            {
                res = m_ICD.vkBindImageMemory( m_hDevice, Info.hImage, Info.hMemory, Info.offset );
            }
            else if( Type == ResourceTypes::BUFFER )
            {
                res = m_ICD.vkBindBufferMemory( m_hDevice, Info.hBuffer, Info.hMemory, Info.offset );
            }
            return res == VK_SUCCESS ? VKE_OK : VKE_FAIL;
        }

    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER