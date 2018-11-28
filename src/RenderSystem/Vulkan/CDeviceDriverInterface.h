#pragma once

#if VKE_VULKAN_RENDERER
#include "RenderSystem/Vulkan/Vulkan.h"

namespace VKE
{
    namespace RenderSystem
    {
        class CDeviceContext;

        static const auto DDINullHandle = VK_NULL_HANDLE;
        using DDIBuffer = VkBuffer;
        using DDIPipeline = VkPipeline;
        using DDIImage = VkImage;
        using DDISampler = VkSampler;
        using DDIRenderPass = VkRenderPass;
        using DDICommandBuffer = VkCommandBuffer;
        using DDIImageView = VkImageView;
        using DDIBufferView = VkBufferView;
        using DDIFence = VkFence;
        using DDISemaphore = VkSemaphore;
        using DDIDevice = VkDevice;
        using DDIDescriptorPool = VkDescriptorPool;
        using DDIDescriptorSet = VkDescriptorSet;
        using DDIDescriptorSetLayout = VkDescriptorSetLayout;
        using DDICommandBufferPool = VkCommandPool;
        using DDIFramebuffer = VkFramebuffer;
        using DDIClearValue = VkClearValue;
        
        struct SDDILoadInfo
        {
            SAPIAppInfo     AppInfo;
        };

        using AdapterInfoArray = Utils::TCDynamicArray< RenderSystem::SAdapterInfo >;

        struct SDeviceInfo
        {
            VkPhysicalDeviceProperties          Properties;
            VkPhysicalDeviceMemoryProperties    MemoryProperties;
            VkPhysicalDeviceFeatures            Features;
            VkPhysicalDeviceLimits              Limits;
        };

        template<typename VkObj, typename VkCreateInfo>
        struct TSVkObject
        {
            VkObj   handle;
            VKE_DEBUG_CODE( VkCreateInfo CreateInfo );
        };

        struct QueueTypes
        {
            enum TYPE
            {
                GRAPHICS,
                COMPUTE,
                TRANSFER,
                SPARSE,
                _MAX_COUNT
            };
        };

        using QUEUE_TYPE = QueueTypes::TYPE;

        static const uint32_t DEFAULT_QUEUE_FAMILY_PROPERTY_COUNT = 16;
        using QueueArray = Utils::TCDynamicArray< Vulkan::SQueue, DEFAULT_QUEUE_FAMILY_PROPERTY_COUNT >;
        using QueuePriorityArray = Utils::TCDynamicArray< float, DEFAULT_QUEUE_FAMILY_PROPERTY_COUNT >;
        using QueueFamilyPropertyArray = Utils::TCDynamicArray< VkQueueFamilyProperties, DEFAULT_QUEUE_FAMILY_PROPERTY_COUNT >;
        
        using UintArray = Utils::TCDynamicArray< uint32_t, DEFAULT_QUEUE_FAMILY_PROPERTY_COUNT >;
        using QueueTypeArray = UintArray[QueueTypes::_MAX_COUNT];

        struct SQueueFamily
        {
            QueueArray          vQueues;
            QueuePriorityArray  vPriorities;
            uint32_t            index;
            bool                isGraphics;
            bool                isCompute;
            bool                isTransfer;
            bool                isSparse;
            bool                isPresent;
        };

        using QueueFamilyArray = Utils::TCDynamicArray< SQueueFamily >;

        struct SDeviceProperties
        {
            QueueFamilyPropertyArray            vQueueFamilyProperties;
            QueueFamilyArray                    vQueueFamilies;
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

        

        class VKE_API CDDI
        {
            friend class CDeviceContext;
            using PhysicalDeviceArray = Utils::TCDynamicArray< VkPhysicalDevice >;

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
                };

                struct FreeDescs
                {
                    struct SDescSet
                    {
                        DDIDescriptorPool       hPool;
                        DDIDescriptorSet*       phSets;
                        uint32_t                count;
                    };
                };

            public:

                static const GlobalICD&     GetGlobalICD() { return sGlobalICD; }
                static const InstanceICD&   GetInstantceICD() { return sInstanceICD; }

                Result              CreateDevice( CDeviceContext* pCtx );
                void                DestroyDevice();
                const DeviceICD&    GetDeviceICD() const { return m_ICD; }
                const DeviceICD&    GetICD() const { return m_ICD; }

                static Result       LoadICD(const SDDILoadInfo& Info);
                static void         CloseICD();

                const DDIDevice&    GetDevice() const { return m_hDevice; }

                static Result       QueryAdapters( AdapterInfoArray* pOut );

                DDIBuffer           CreateObject( const SBufferDesc& Desc, const void* = nullptr );
                void            DestroyObject( DDIBuffer* phBuffer, const void* = nullptr );
                DDIBufferView   CreateObject( const SBufferViewDesc& Desc, const void* = nullptr );
                void            DestroyObject( DDIBufferView* phBufferView, const void* = nullptr );
                DDIImage        CreateObject( const STextureDesc& Desc, const void* = nullptr );
                void            DestroyObject( DDIImage* phImage, const void* = nullptr );
                DDIImageView    CreateObject( const STextureViewDesc& Desc, const void* = nullptr );
                void            DestroyObject( DDIImageView* phImageView, const void* = nullptr );

                Result          AllocateObjects(const AllocateDescs::SDescSet& Info, DDIDescriptorSet* pSets );
                void            FreeObjects( const FreeDescs::SDescSet& );

            protected:

                static GlobalICD            sGlobalICD;
                static InstanceICD          sInstanceICD;
                static handle_t             shICD;
                static VkInstance           sVkInstance;
                static PhysicalDeviceArray  sPhysicalDevices;

                DeviceICD               m_ICD;
                DDIDevice               m_hDevice = DDINullHandle;
                CDeviceContext*         m_pCtx;
                SDeviceInfo             m_DeviceInfo;
                SDeviceProperties       m_DeviceProperties;
        };
    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER