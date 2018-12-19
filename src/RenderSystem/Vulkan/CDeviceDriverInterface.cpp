#include "RenderSystem/CDeviceDriverInterface.h"
#if VKE_VULKAN_RENDERER
#include "RenderSystem/CDeviceContext.h"
#include "RenderSystem/Resources/CTexture.h"
#include "RenderSystem/Resources/CBuffer.h"
#include "RenderSystem/CRenderPass.h"

namespace VKE
{

#define DDI_CREATE_OBJECT(_name, _CreateInfo, _pAllocator, _phObj) \
    m_ICD.vkCreate##_name( m_hDevice, &(_CreateInfo), static_cast<const VkAllocationCallbacks*>(_pAllocator), (_phObj) );

#define DDI_DESTROY_OBJECT(_name, _phObj, _pAllocator) \
    if( (_phObj) && (*_phObj) != DDI_NULL_HANDLE ) \
    { \
        m_ICD.vkDestroy##_name( m_hDevice, (*_phObj), static_cast<const VkAllocationCallbacks*>(_pAllocator) ); \
        (*_phObj) = DDI_NULL_HANDLE; \
    }

    namespace RenderSystem
    {
        VkICD::Global CDDI::sGlobalICD;
        VkICD::Instance CDDI::sInstanceICD;
        handle_t CDDI::shICD = 0;
        VkInstance CDDI::sVkInstance = VK_NULL_HANDLE;
        CDDI::PhysicalDeviceArray CDDI::sPhysicalDevices;



        Result GetInstanceValidationLayers( bool bEnable, const VkICD::Global& Global,
            vke_vector<VkLayerProperties>* pvProps, CStrVec* pvNames )
        {
            if( !bEnable )
                return VKE_OK;

            static const char* apNames[] =
            {
                "VK_LAYER_LUNARG_parameter_validation",
                "VK_LAYER_LUNARG_device_limits",
                "VK_LAYER_LUNARG_object_tracker",
                "VK_LAYER_LUNARG_core_validation",
                "VK_LAYER_LUNARG_swapchain"
            };
            /*vNames.push_back("VK_LAYER_GOOGLE_threading");
            vNames.push_back("VK_LAYER_LUNARG_parameter_validation");
            vNames.push_back("VK_LAYER_LUNARG_device_limits");
            vNames.push_back("VK_LAYER_LUNARG_object_tracker");
            vNames.push_back("VK_LAYER_LUNARG_image");
            vNames.push_back("VK_LAYER_LUNARG_core_validation");
            vNames.push_back("VK_LAYER_LUNARG_swapchain");
            vNames.push_back("VK_LAYER_GOOGLE_unique_objects");*/

            uint32_t count = 0;
            auto& vProps = *pvProps;
            VK_ERR( Global.vkEnumerateInstanceLayerProperties( &count, nullptr ) );
            vProps.resize( count );
            VK_ERR( Global.vkEnumerateInstanceLayerProperties( &count, &vProps[0] ) );

            for( uint32_t i = 0; i < ARRAYSIZE( apNames ); ++i )
            {
                auto pName = apNames[i];
                for( auto& Prop : vProps )
                {
                    if( strcmp( Prop.layerName, pName ) == 0 )
                    {
                        pvNames->push_back( pName );
                    }
                }
            }
            return VKE_OK;
        }

        CStrVec GetInstanceExtensionNames( const VkICD::Global& Global )
        {
            CStrVec vNames;
            vke_vector< VkExtensionProperties > vProps;
            uint32_t count = 0;
            VK_ERR( Global.vkEnumerateInstanceExtensionProperties( "", &count, nullptr ) );
            vProps.resize( count );
            VK_ERR( Global.vkEnumerateInstanceExtensionProperties( "", &count, &vProps[0] ) );

            // Required extensions
            vNames.push_back( VK_EXT_DEBUG_REPORT_EXTENSION_NAME );
            vNames.push_back( VK_KHR_SURFACE_EXTENSION_NAME );
#if VKE_WINDOWS
            vNames.push_back( VK_KHR_WIN32_SURFACE_EXTENSION_NAME );
#elif VKE_LINUX
            vNames.push_back( VK_KHR_XCB_SURFACE_EXTENSION_NAME );
#elif VKE_ANDROID
            vNames.push_back( VK_KHR_ANDROID_SURFACE_EXTENSION_NAME );
#endif

            // Check extension availability
            for( auto& pExt : vNames )
            {
                bool bAvailable = false;
                for( auto& Prop : vProps )
                {
                    if( strcmp( Prop.extensionName, pExt ) == 0 )
                    {
                        bAvailable = true;
                        break;
                    }
                }
                if( !bAvailable )
                {
                    VKE_LOG_ERR( "There is no required extension: " << pExt << " supported by this GPU" );
                    vNames.clear();
                    return vNames;
                }
            }
            return vNames;
        }

        Result EnableInstanceExtensions( bool bEnable )
        {
            (void)bEnable;
            return VKE_OK;
        }

        Result EnableInstanceLayers( bool bEnable )
        {
            (void)bEnable;
            return VKE_OK;
        }

        Result GetDeviceProperties( const VkPhysicalDevice& vkPhysicalDevice, SDeviceProperties* pOut )
        {
            auto& InstanceICD = CDDI::GetInstantceICD();

            uint32_t propCount = 0;
            InstanceICD.vkGetPhysicalDeviceQueueFamilyProperties( vkPhysicalDevice, &propCount, nullptr );
            if( propCount == 0 )
            {
                VKE_LOG_ERR( "No device queue family properties" );
                return VKE_FAIL;
            }

            pOut->vQueueFamilyProperties.Resize( propCount );
            auto& aProperties = pOut->vQueueFamilyProperties;
            auto& vQueueFamilies = pOut->vQueueFamilies;

            InstanceICD.vkGetPhysicalDeviceQueueFamilyProperties( vkPhysicalDevice, &propCount, &aProperties[0] );
            // Choose a family index
            for( uint32_t i = 0; i < propCount; ++i )
            {
                //uint32_t isGraphics = aProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT;
                uint32_t isCompute = aProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT;
                uint32_t isTransfer = aProperties[i].queueFlags & VK_QUEUE_TRANSFER_BIT;
                uint32_t isSparse = aProperties[i].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT;
                uint32_t isGraphics = aProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT;
                VkBool32 isPresent = VK_FALSE;
#if VKE_USE_VULKAN_WINDOWS
                isPresent = InstanceICD.vkGetPhysicalDeviceWin32PresentationSupportKHR( vkPhysicalDevice, i );
#elif VKE_USE_VULKAN_LINUX
                isPresent = InstanceICD.vkGetPhysicalDeviceXcbPresentationSupportKHR( s_physical_device, i,
                    xcb_connection, visual_id );
#elif VKE_USE_VULKAN_ANDROID
#error "implement"
#endif

                SQueueFamily Family;
                Family.vQueues.Resize( aProperties[i].queueCount );
                Family.vPriorities.Resize( aProperties[i].queueCount, 1.0f );
                Family.index = i;
                Family.isGraphics = isGraphics != 0;
                Family.isCompute = isCompute != 0;
                Family.isTransfer = isTransfer != 0;
                Family.isSparse = isSparse != 0;
                Family.isPresent = isPresent == VK_TRUE;

                vQueueFamilies.PushBack( Family );
            }

            InstanceICD.vkGetPhysicalDeviceMemoryProperties( vkPhysicalDevice, &pOut->vkMemProperties );
            InstanceICD.vkGetPhysicalDeviceFeatures( vkPhysicalDevice, &pOut->vkFeatures );

            for( uint32_t i = 0; i < RenderSystem::Formats::_MAX_COUNT; ++i )
            {
                const auto& fmt = RenderSystem::g_aFormats[i];
                InstanceICD.vkGetPhysicalDeviceFormatProperties( vkPhysicalDevice, fmt,
                    &pOut->aFormatProperties[i] );
            }

            return VKE_OK;
        }

        Result CheckDeviceExtensions( VkPhysicalDevice vkPhysicalDevice,
            const Utils::TCDynamicArray<const char *>& vExtensions )
        {
            auto& InstanceICD = CDDI::GetInstantceICD();
            uint32_t count = 0;
            VK_ERR( InstanceICD.vkEnumerateDeviceExtensionProperties( vkPhysicalDevice, nullptr, &count, nullptr ) );

            Utils::TCDynamicArray< VkExtensionProperties > vProperties( count );

            VK_ERR( InstanceICD.vkEnumerateDeviceExtensionProperties( vkPhysicalDevice, nullptr, &count,
                &vProperties[0] ) );

            std::string ext;
            Result err = VKE_OK;

            for( uint32_t e = 0; e < vExtensions.GetCount(); ++e )
            {
                ext = vExtensions[e];
                bool found = false;
                for( uint32_t p = 0; p < count; ++p )
                {
                    if( ext == vProperties[p].extensionName )
                    {
                        found = true;
                        break;
                    }
                }
                if( !found )
                {
                    VKE_LOG_ERR( "Extension: " << ext << " is not supported by the device." );
                    err = VKE_ENOTFOUND;
                }
            }

            return err;
        }


        Result CDDI::CreateDevice( CDeviceContext* pCtx )
        {
            m_pCtx = pCtx;
            Utils::TCDynamicArray< const char* > vExtensions =
            {
                VK_KHR_SWAPCHAIN_EXTENSION_NAME
            };

            auto hAdapter = m_pCtx->m_Desc.pAdapterInfo->handle;
            VkPhysicalDevice vkPhysicalDevice = reinterpret_cast<VkPhysicalDevice>( hAdapter );
            //VkInstance vkInstance = reinterpret_cast<VkInstance>(Desc.hAPIInstance);

            {
                sInstanceICD.vkGetPhysicalDeviceFeatures( vkPhysicalDevice, &m_DeviceInfo.Features );
                //ICD.Instance.vkGetPhysicalDeviceFormatProperties( vkPhysicalDevice, &m_DeviceInfo.FormatProperties );
                sInstanceICD.vkGetPhysicalDeviceMemoryProperties( vkPhysicalDevice, &m_DeviceInfo.MemoryProperties );
                sInstanceICD.vkGetPhysicalDeviceProperties( vkPhysicalDevice, &m_DeviceInfo.Properties );
            }

            VKE_RETURN_IF_FAILED( GetDeviceProperties( vkPhysicalDevice, &m_DeviceProperties ) );
            VKE_RETURN_IF_FAILED( CheckDeviceExtensions( vkPhysicalDevice, vExtensions ) );

            Utils::TCDynamicArray<VkDeviceQueueCreateInfo> vQis;
            for( auto& Family : m_DeviceProperties.vQueueFamilies )
            {
                if( !Family.vQueues.IsEmpty() )
                {
                    VkDeviceQueueCreateInfo qi;
                    Vulkan::InitInfo( &qi, VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO );
                    qi.flags = 0;
                    qi.pQueuePriorities = &Family.vPriorities[0];
                    qi.queueFamilyIndex = Family.index;
                    qi.queueCount = static_cast<uint32_t>(Family.vQueues.GetCount());
                    vQis.PushBack( qi );
                }
            }

            //VkPhysicalDeviceFeatures df = {};

            VkDeviceCreateInfo di;
            Vulkan::InitInfo( &di, VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO );
            di.enabledExtensionCount = vExtensions.GetCount();
            di.enabledLayerCount = 0;
            di.pEnabledFeatures = nullptr;
            di.ppEnabledExtensionNames = &vExtensions[0];
            di.ppEnabledLayerNames = nullptr;
            di.pQueueCreateInfos = &vQis[0];
            di.queueCreateInfoCount = static_cast<uint32_t>(vQis.GetCount());
            di.flags = 0;

            VK_ERR( sInstanceICD.vkCreateDevice( vkPhysicalDevice, &di, nullptr, &m_hDevice ) );

            VKE_RETURN_IF_FAILED( Vulkan::LoadDeviceFunctions( m_hDevice, sInstanceICD, &m_ICD ) );

            for( auto& Family : m_DeviceProperties.vQueueFamilies )
            {
                for( uint32_t q = 0; q < Family.vQueues.GetCount(); ++q )
                {
                    VkQueue vkQueue;
                    m_ICD.vkGetDeviceQueue( m_hDevice, Family.index, q, &vkQueue );
                    Family.vQueues[q].vkQueue = vkQueue;
                    Family.vQueues[q].familyIndex = Family.index;
                }
            }
        
            return VKE_OK;
        }

        void CDDI::DestroyDevice()
        {
            if( m_hDevice != DDI_NULL_HANDLE )
            {
                sInstanceICD.vkDestroyDevice( m_hDevice, nullptr );
            }
            m_hDevice = DDI_NULL_HANDLE;
            m_pCtx = nullptr;
        }

        Result CDDI::LoadICD( const SDDILoadInfo& Info )
        {
            Result ret = VKE_OK;
            shICD = Platform::DynamicLibrary::Load( "vulkan-1.dll" );
            if( shICD != 0 )
            {
                ret = Vulkan::LoadGlobalFunctions( shICD, &sGlobalICD );
                if( VKE_SUCCEEDED( ret ) )
                {
                    bool bEnabled = true;
                    auto vExtNames = GetInstanceExtensionNames( sGlobalICD );
                    if( vExtNames.empty() )
                    {
                        return VKE_FAIL;
                    }

                    CStrVec vLayerNames;
                    vke_vector< VkLayerProperties > vLayerProps;
                    ret = GetInstanceValidationLayers( bEnabled, sGlobalICD, &vLayerProps, &vLayerNames );
                    if( VKE_SUCCEEDED( ret ) )
                    {
                        VkApplicationInfo vkAppInfo;
                        vkAppInfo.apiVersion = VK_API_VERSION_1_0;
                        vkAppInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
                        vkAppInfo.pNext = nullptr;
                        vkAppInfo.applicationVersion = Info.AppInfo.applicationVersion;
                        vkAppInfo.engineVersion = Info.AppInfo.engineVersion;
                        vkAppInfo.pApplicationName = Info.AppInfo.pApplicationName;
                        vkAppInfo.pEngineName = Info.AppInfo.pEngineName;

                        VkInstanceCreateInfo InstInfo;
                        Vulkan::InitInfo( &InstInfo, VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO );
                        InstInfo.enabledExtensionCount = static_cast<uint32_t>(vExtNames.size());
                        InstInfo.enabledLayerCount = static_cast<uint32_t>(vLayerNames.size());
                        InstInfo.flags = 0;
                        InstInfo.pApplicationInfo = &vkAppInfo;
                        InstInfo.ppEnabledExtensionNames = vExtNames.data();
                        InstInfo.ppEnabledLayerNames = vLayerNames.data();

                        VkResult vkRes = sGlobalICD.vkCreateInstance( &InstInfo, nullptr, &sVkInstance );
                        VK_ERR( vkRes );
                        if( vkRes == VK_SUCCESS )
                        {
                            ret = Vulkan::LoadInstanceFunctions( sVkInstance, sGlobalICD, &sInstanceICD );
                        }
                        else
                        {
                            ret = VKE_FAIL;
                            VKE_LOG_ERR( "Unable to create Vulkan instance: " << vkRes );
                        }
                    }
                    else
                    {
                        VKE_LOG_ERR( "Unable to get Vulkan instance validation layers." );
                    }
                }
                else
                {
                    VKE_LOG_ERR("Unable to load Vulkan global function pointers.");
                }
            }
            else
            {
                VKE_LOG_ERR( "Unable to load library: vulkan-1.dll" );
            }
            return ret;
        }

        void CDDI::CloseICD()
        {
            //sGlobalICD.vkDestroyInstance( sVkInstance, nullptr );
            sInstanceICD.vkDestroyInstance( sVkInstance, nullptr );
            sVkInstance = VK_NULL_HANDLE;
            Platform::DynamicLibrary::Close( shICD );
        }

        Result CDDI::QueryAdapters( AdapterInfoArray* pOut )
        {
            Result ret = VKE_FAIL;
            uint32_t count = 0;
            VkResult vkRes = sInstanceICD.vkEnumeratePhysicalDevices( sVkInstance, &count, nullptr );
            VK_ERR( vkRes );
            if( vkRes == VK_SUCCESS )
            {
                if( count > 0 )
                {
                    sPhysicalDevices.Resize( count );
                    vkRes = sInstanceICD.vkEnumeratePhysicalDevices( sVkInstance, &count, &sPhysicalDevices[ 0 ] );
                    VK_ERR( vkRes );
                    if( vkRes == VK_SUCCESS )
                    {
                        const uint32_t nameLen = Min( VK_MAX_PHYSICAL_DEVICE_NAME_SIZE, Constants::MAX_NAME_LENGTH );

                        for( size_t i = 0; i < sPhysicalDevices.GetCount(); ++i )
                        {
                            const auto& vkPhysicalDevice = sPhysicalDevices[ i ];

                            VkPhysicalDeviceProperties Props;
                            sInstanceICD.vkGetPhysicalDeviceProperties( vkPhysicalDevice, &Props );
                            RenderSystem::SAdapterInfo Info = {};
                            Info.apiVersion = Props.apiVersion;
                            Info.deviceID = Props.deviceID;
                            Info.driverVersion = Props.driverVersion;
                            Info.type = static_cast<RenderSystem::ADAPTER_TYPE>(Props.deviceType);
                            Info.vendorID = Props.vendorID;
                            Info.handle = reinterpret_cast<handle_t>(vkPhysicalDevice);
                            Memory::Copy( Info.name, sizeof( Info.name ), Props.deviceName, nameLen );

                            pOut->PushBack( Info );
                        }
                        ret = VKE_OK;
                    }
                    else
                    {
                        VKE_LOG_ERR( "No physical device available for this machine" );
                    }
                }
                else
                {
                    VKE_LOG_ERR( "No physical device available for this machine" );
                    VKE_LOG_ERR( "Vulkan is not supported for this GPU" );
                }
            }
            else
            {
                VKE_LOG_ERR( "Unable to enumerate Vulkan physical devices: " << vkRes );
            }
            return ret;
        }

        DDIBuffer   CDDI::CreateObject( const SBufferDesc& Desc, const void* pAllocator )
        {
            VkBufferCreateInfo ci;
            DDIBuffer hBuffer = VK_NULL_HANDLE;
            Vulkan::InitInfo( &ci, VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO );
            {
                ci.flags = 0;
                ci.pQueueFamilyIndices = nullptr;
                ci.queueFamilyIndexCount = 0;
                ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                ci.size = Desc.size;
                ci.usage = Vulkan::Convert::BufferUsage( Desc.usage );
                VkResult vkRes = DDI_CREATE_OBJECT( Buffer, ci, pAllocator, &hBuffer );
                VK_ERR( vkRes );
            }
            return hBuffer;
        }

        void CDDI::DestroyObject( DDIBuffer* phBuffer, const void* pAllocator )
        {
            DDI_DESTROY_OBJECT( Buffer, phBuffer, pAllocator );
        }

        DDIBufferView CDDI::CreateObject( const SBufferViewDesc& Desc, const void* pAllocator )
        {
            DDIBufferView hView = DDI_NULL_HANDLE;
            VkBufferViewCreateInfo ci;
            {
                ci.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
                ci.pNext = nullptr;
                ci.flags = 0;
                ci.format = Vulkan::Map::Format( Desc.format );
                ci.buffer = m_pCtx->GetBuffer( Desc.hBuffer )->GetDDIObject();
                ci.offset = Desc.offset;
            }
            VkResult vkRes = m_ICD.vkCreateBufferView( m_hDevice, &ci, nullptr, &hView );
            VK_ERR( vkRes );
            return hView;
        }

        void CDDI::DestroyObject( DDIBufferView* phBufferView, const void* pAllocator )
        {
            DDI_DESTROY_OBJECT( BufferView, phBufferView, pAllocator );
        }

        DDIImage CDDI::CreateObject( const STextureDesc& Desc, const void* pAllocator )
        {
            DDIImage hImage = DDI_NULL_HANDLE;
            VkImageCreateInfo ci;
            {
                ci.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
                ci.pNext = nullptr;
                ci.flags = 0;
                ci.format = Vulkan::Map::Format( Desc.format );
                ci.imageType = Vulkan::Map::ImageType( Desc.type );
                ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                ci.mipLevels = Desc.mipLevelCount;
                ci.samples = Vulkan::Map::SampleCount( Desc.multisampling );
                ci.pQueueFamilyIndices = nullptr;
                ci.queueFamilyIndexCount = 0;
                ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                ci.tiling = Vulkan::Convert::ImageUsageToTiling( Desc.usage );
                ci.arrayLayers = 1;
                ci.extent.width = Desc.Size.width;
                ci.extent.height = Desc.Size.height;
                ci.extent.depth = 1;
                ci.usage = Vulkan::Map::ImageUsage( Desc.usage );
            }
            VkResult vkRes = m_ICD.vkCreateImage( m_hDevice, &ci, nullptr, &hImage );
            VK_ERR( vkRes );
            return hImage;
        }

        void CDDI::DestroyObject( DDIImage* phImage, const void* pAllocator )
        {
            DDI_DESTROY_OBJECT( Image, phImage, pAllocator );
        }

        DDIImageView CDDI::CreateObject( const STextureViewDesc& Desc, const void* pAllocator )
        {
            static const VkComponentMapping DefaultMapping = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
            DDIImageView hView = DDI_NULL_HANDLE;
            VkImageViewCreateInfo ci;
            {
                TextureRefPtr pTex = m_pCtx->GetTexture( Desc.hTexture );
                ci.components = DefaultMapping;
                ci.flags = 0;
                ci.format = Vulkan::Map::Format( Desc.format );
                ci.image = pTex->GetDDIObject();
                ci.pNext = nullptr;
                ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                ci.subresourceRange.aspectMask = Vulkan::Map::ImageAspect( Desc.aspect );
                ci.subresourceRange.baseArrayLayer = 0;
                ci.subresourceRange.baseMipLevel = Desc.beginMipmapLevel;
                ci.subresourceRange.layerCount = 1;
                ci.subresourceRange.levelCount = Desc.endMipmapLevel;
                ci.viewType = Vulkan::Map::ImageViewType( Desc.type );
            }
            VkResult vkRes = m_ICD.vkCreateImageView( m_hDevice, &ci, static_cast<const VkAllocationCallbacks*>( pAllocator ),
                &hView );
            VK_ERR( vkRes );
            return hView;
        }

        void CDDI::DestroyObject( DDIImageView* phImageView, const void* pAllocator )
        {
            DDI_DESTROY_OBJECT( ImageView, phImageView, pAllocator );
        }

        DDIFramebuffer CDDI::CreateObject( const SFramebufferDesc& Desc, const void* pAllocator )
        {
            Utils::TCDynamicArray< DDIImageView > vAttachments;
            const uint32_t attachmentCount = Desc.vAttachments.GetCount();
            vAttachments.Resize( attachmentCount );
            
            for( uint32_t i = 0; i < attachmentCount; ++i )
            {
                vAttachments[i] = m_pCtx->GetTextureView( Desc.vAttachments[i] )->GetDDIObject();
            }

            VkFramebufferCreateInfo ci;
            ci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            ci.pNext = nullptr;
            ci.flags = 0;
            ci.width = Desc.Size.width;
            ci.height = Desc.Size.height;
            ci.layers = 1;
            ci.attachmentCount = Desc.vAttachments.GetCount();
            ci.pAttachments = &vAttachments[0];
            ci.renderPass = m_pCtx->GetRenderPass( Desc.hRenderPass )->GetDDIObject();
        
        }

        void CDDI::DestroyObject( DDIFramebuffer* phFramebuffer, const void* pAllocator )
        {
            DDI_DESTROY_OBJECT( Framebuffer, phFramebuffer, pAllocator );
        }

        DDIFence CDDI::CreateObject( const SFenceDesc& Desc, const void* pAllocator )
        {
            VkFenceCreateInfo ci;
            ci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            ci.pNext = nullptr;
            ci.flags = Desc.isSignaled;
            DDIFence hObj = DDI_NULL_HANDLE;
            VkResult res = DDI_CREATE_OBJECT( Fence, ci, pAllocator, &hObj );
            VK_ERR( res );
            return hObj;
        }

        DDIRenderPass CDDI::CreateObject( const SRenderPassDesc& Desc, const void* pAllocator )
        {
            DDIRenderPass hPass = DDI_NULL_HANDLE;

            return hPass;
        }

        void CDDI::DestroyObject( DDIRenderPass* phPass, const void* pAllocator )
        {
            DDI_DESTROY_OBJECT( RenderPass, phPass, pAllocator );
        }

        DDICommandBufferPool CDDI::CreateObject( const SCommandBufferPoolDesc& Desc, const void* pAllocator )
        {
            DDICommandBufferPool hPool = DDI_NULL_HANDLE;
            VkCommandPoolCreateInfo ci;
            ci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            ci.pNext = nullptr;
            ci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            ci.queueFamilyIndex = Desc.pQueue->GetFamilyIndex();
            VkResult res = DDI_CREATE_OBJECT( CommandPool, ci, pAllocator, &hPool );
            VK_ERR( res );
            return hPool;
        }

        Result CDDI::AllocateObjects(const AllocateDescs::SDescSet& Info, DDIDescriptorSet* pSets )
        {
            Result ret = VKE_FAIL;
            VkDescriptorSetAllocateInfo ai;
            ai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            ai.pNext = nullptr;
            ai.descriptorPool = Info.hPool;
            ai.descriptorSetCount = Info.count;
            ai.pSetLayouts = Info.phLayouts;
            VkResult res = m_ICD.vkAllocateDescriptorSets( m_hDevice, &ai, pSets );
            VK_ERR( res );
            if( res == VK_SUCCESS )
            {
                ret = VKE_OK;
            }
            return ret;
        }

        void CDDI::FreeObjects( const FreeDescs::SDescSet& Desc )
        {
            m_ICD.vkFreeDescriptorSets( m_hDevice, Desc.hPool, Desc.count, Desc.phSets );
        }

        DDIMemory CDDI::AllocateMemory( const SMemoryDesc& Desc, const void* pAllocator )
        {
            DDIMemory hMemory = DDI_NULL_HANDLE;
            VkMemoryAllocateInfo ai;
            ai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            ai.pNext = nullptr;
            ai.allocationSize = Desc.size;
            ai.memoryTypeIndex = Desc.typeIdx;
            VkResult res = m_ICD.vkAllocateMemory( m_hDevice, &ai, reinterpret_cast< const VkAllocationCallbacks* >( pAllocator ), &hMemory );
            VK_ERR( res );
            return hMemory;
        }

        Result CDDI::Wait( const DDIFence& hFence, uint64_t timeout )
        {
            VkResult res = m_ICD.vkWaitForFences( m_hDevice, 1, &hFence, VK_TRUE, timeout );
            VK_ERR( res );
            Result ret = VKE_FAIL;
            switch( res )
            {
                case VK_SUCCESS:
                    ret = VKE_OK;
                    break;
                case VK_TIMEOUT:
                    ret = VKE_TIMEOUT;
                    break;
            
                default:
                    break;
            };
            return ret;
        }

        Result CDDI::Wait( const DDIQueue& hQueue )
        {
            VkResult res = m_ICD.vkQueueWaitIdle( hQueue );
            VK_ERR( res );
            return res == VK_SUCCESS ? VKE_OK : VKE_FAIL;
        }

        Result CDDI::Wait()
        {
            VkResult res = m_ICD.vkDeviceWaitIdle( m_hDevice );
            VK_ERR( res );
            return res == VK_SUCCESS ? VKE_OK : VKE_FAIL;
        }

        Result CDDI::Submit( const SSubmitInfo& Info )
        {
            Result ret = VKE_FAIL;

            static const VkPipelineStageFlags vkWaitMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            VkSubmitInfo si;
            Vulkan::InitInfo( &si, VK_STRUCTURE_TYPE_SUBMIT_INFO );
            si.pSignalSemaphores = Info.pSignalSemaphores;
            si.signalSemaphoreCount = Info.signalSemaphoreCount;
            si.pWaitSemaphores = Info.pWaitSemaphores;
            si.waitSemaphoreCount = Info.waitSemaphoreCount;
            si.pWaitDstStageMask = &vkWaitMask;
            si.commandBufferCount = Info.commandBufferCount;
            si.pCommandBuffers = Info.pCommandBuffers;
            //VK_ERR( m_pQueue->Submit( ICD, si, pSubmit->m_hDDIFence ) );
            VkResult res = m_ICD.vkQueueSubmit( Info.hQueue, 1, &si, Info.hFence );
            VK_ERR( res );
            ret = res == VK_SUCCESS ? VKE_OK : VKE_FAIL;
            return ret;
        }

        Result CDDI::Present( const SPresentInfo& Info )
        {
            VkPresentInfoKHR pi;
            pi.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
            pi.pNext = nullptr;
            pi.pImageIndices = &Info.vImageIndices[0];
            pi.pSwapchains = &Info.vSwapchains[0];
            pi.pWaitSemaphores = &Info.vWaitSemaphores[0];
            pi.pResults = nullptr;
            pi.swapchainCount = Info.vSwapchains.GetCount();
            pi.waitSemaphoreCount = Info.vWaitSemaphores.GetCount();
            
            VkResult res = m_ICD.vkQueuePresentKHR( Info.hQueue, &pi );
            VK_ERR( res );
            return res == VK_SUCCESS ? VKE_OK : VKE_FAIL;
        }

    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER