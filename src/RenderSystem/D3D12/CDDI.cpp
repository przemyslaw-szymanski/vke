#include "RenderSystem/CDDI.h"

#if VKE_D3D12_RENDER_SYSTEM

#include "Core/Managers/CFileManager.h"
#include "Core/Platform/CWindow.h"

#include "RenderSystem/CContextBase.h"
#include "RenderSystem/CDeviceContext.h"
#include "RenderSystem/CGraphicsContext.h"
#include "RenderSystem/CRenderPass.h"
#include "RenderSystem/Resources/CBuffer.h"
#include "RenderSystem/Resources/CTexture.h"

namespace VKE::RenderSystem
{
#define TRACK_CALL_ONCE( msg )                                                                                         \
    static bool s_called = false;                                                                                      \
    if( s_called )                                                                                                     \
    {                                                                                                                  \
        VKE_LOG_ERR( "D3D12 Render System: " + std::string( msg ) + " can only be called once!" );                     \
    }                                                                                                                  \
    s_called = true;

#define UNIMPLEMENTED_D3D12_METHOD() VKE_ASSERT2( false, "D3D12 Render System: Unimplemented method" )

    namespace NativeAPI
    {
        // Init static members
        Factory SImplementation::spFactory          = NativeAPI::Null;
        bool    SImplementation::sTearingSupported  = false;
        bool    SImplementation::sDebugLayerEnabled = false;
    }; // namespace NativeAPI

    CDDI::AdapterArray CDDI::svAdapters;

    namespace Map
    {
        D3D12_COMMAND_LIST_TYPE getCommandListType( QUEUE_TYPE type )
        {
            VKE_ASSERT2( type < QUEUE_TYPE::_MAX_COUNT, "Invalid queue type" );

            static const D3D12_COMMAND_LIST_TYPE vFamilyMap[] = {
                D3D12_COMMAND_LIST_TYPE_DIRECT,  // GENERAL
                D3D12_COMMAND_LIST_TYPE_COMPUTE, // COMPUTE
                D3D12_COMMAND_LIST_TYPE_COPY,    // TRANSFER
                D3D12_COMMAND_LIST_TYPE_NONE,    // SPARSE - not supported in DX12
                D3D12_COMMAND_LIST_TYPE_NONE,    // PRESENT - not supported in DX12
            };

            return vFamilyMap[ static_cast< size_t >( type ) ];
        }

        D3D12_DESCRIPTOR_HEAP_TYPE getDescriptorHeapType( DESCRIPTOR_SET_TYPE type )
        {
            static const D3D12_DESCRIPTOR_HEAP_TYPE vMap[] = {
                D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,     // SAMPLER
                D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, // TEXTURE
                D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, // SAMPLER_AND_TEXTURE     : not supported in DX12
                D3D12_DESCRIPTOR_HEAP_TYPE_RTV,         // STORAGE_TEXTURE
                D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, // READ_ONLY_TEXEL_BUFFER  : basically read only UAV
                D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, // READ_WRITE_TEXEL_BUFFER : basically R/W UAV
                D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, // CONSTANT_BUFFER
                D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, // BUFFER
                D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, // DYNAMIC_CONSTANT_BUFFER
                D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, // DYNAMIC_BUFFER
                D3D12_DESCRIPTOR_HEAP_TYPE_RTV,         // RENDER_TARGET
                D3D12_DESCRIPTOR_HEAP_TYPE_DSV,         // DEPTH_STENCIL
            };

            return vMap[ static_cast< size_t >( type ) ];
        }

    }; // namespace Map

    namespace Convert
    {
        D3D12_SHADER_VISIBILITY getShaderVisibility( uint16_t type )
        {
            switch( type )
            {
                case PipelineStages::TYPE::VERTEX:
                    return D3D12_SHADER_VISIBILITY_VERTEX;

                case PipelineStages::TYPE::TS_DOMAIN:
                    return D3D12_SHADER_VISIBILITY_DOMAIN;

                case PipelineStages::TYPE::TS_HULL:
                    return D3D12_SHADER_VISIBILITY_HULL;

                case PipelineStages::TYPE::GEOMETRY:
                    return D3D12_SHADER_VISIBILITY_GEOMETRY;

                case PipelineStages::TYPE::PIXEL:
                    return D3D12_SHADER_VISIBILITY_PIXEL;

                case PipelineStages::TYPE::MS_TASK:
                    return D3D12_SHADER_VISIBILITY_AMPLIFICATION;

                case PipelineStages::TYPE::MS_MESH:
                    return D3D12_SHADER_VISIBILITY_MESH;

                default:
                    return D3D12_SHADER_VISIBILITY_ALL;
            }
        }

        D3D12_DESCRIPTOR_RANGE_TYPE getDescriptorRangeType( BINDING_TYPE type )
        {
            static const D3D12_DESCRIPTOR_RANGE_TYPE vMap[] = {
                D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, // SAMPLER
                D3D12_DESCRIPTOR_RANGE_TYPE_SRV,     // TEXTURE
                D3D12_DESCRIPTOR_RANGE_TYPE_SRV,     // SAMPLER_AND_TEXTURE     : not supported in DX12
                D3D12_DESCRIPTOR_RANGE_TYPE_UAV,     // STORAGE_TEXTURE
                D3D12_DESCRIPTOR_RANGE_TYPE_UAV,     // READ_ONLY_TEXEL_BUFFER  : basically read only UAV
                D3D12_DESCRIPTOR_RANGE_TYPE_UAV,     // READ_WRITE_TEXEL_BUFFER : basically R/W UAV
                D3D12_DESCRIPTOR_RANGE_TYPE_CBV,     // CONSTANT_BUFFER
                D3D12_DESCRIPTOR_RANGE_TYPE_UAV,     // BUFFER
                D3D12_DESCRIPTOR_RANGE_TYPE_CBV,     // DYNAMIC_CONSTANT_BUFFER
                D3D12_DESCRIPTOR_RANGE_TYPE_UAV,     // DYNAMIC_BUFFER
                D3D12_DESCRIPTOR_RANGE_TYPE_SRV,     // RENDER_TARGET
                D3D12_DESCRIPTOR_RANGE_TYPE_SRV,     // DEPTH_STENCIL
            };

            return vMap[ type ];
        }
    }; // namespace Convert

    // Static methods

    UINT getNodeMask()
    {
        // TODO(blturkot): Implement node mask when engine enable multi adapter rendering.
        return 0;
    }

    D3D_FEATURE_LEVEL getMaxFeatureLevel( IDXGIAdapter1* pAdapter )
    {
        static const D3D_FEATURE_LEVEL featureLevels[] = {
            D3D_FEATURE_LEVEL_12_2, D3D_FEATURE_LEVEL_12_1, D3D_FEATURE_LEVEL_12_0,
            D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0,
        };

        D3D_FEATURE_LEVEL maxLevel = featureLevels[ _countof( featureLevels ) - 1 ];

        ID3D12Device* device = NativeAPI::Null;
        HRESULT       result = S_OK;

        for( auto level: featureLevels )
        {
            result = D3D12CreateDevice( pAdapter, level, IID_PPV_ARGS( &device ) );

            if( SUCCEEDED( result ) )
            {
                maxLevel = level;
                device->Release();
                break;
            }
        }

        return maxLevel;
    }

    Result CDDI::QueryAdapters( AdapterInfoArray* pOut )
    {
        static const size_t MAX_ADAPTERS = 5;
        auto                factory      = NativeAPI::SImplementation::spFactory;

        if( factory == NativeAPI::Null )
        {
            VKE_LOG_ERR( "CDDI::QueryAdapters: DXGI Factory is null" );
            return VKE_FAIL;
        }

        UINT    index  = 0;
        HRESULT result = S_OK;

        // Limit adapters by high performance preference
        while( index < MAX_ADAPTERS )
        {
            IDXGIAdapter1*     pAdapter1 = NativeAPI::Null;
            NativeAPI::Adapter pAdapter  = NativeAPI::Null;

            result = factory->EnumAdapterByGpuPreference(
                index++, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS( &pAdapter1 ) );

            if( result == DXGI_ERROR_NOT_FOUND )
            {
                break;
            }

            if( FAILED( pAdapter1->QueryInterface( IID_PPV_ARGS( &pAdapter ) ) ) )
            {
                VKE_LOG_ERR( "CDDI::QueryAdapters: Query NativeAPI::Adapter failed" );
            }

            DXGI_ADAPTER_DESC3 adapterDesc;
            if( FAILED( pAdapter->GetDesc3( &adapterDesc ) ) )
            {
                VKE_LOG_ERR( "CDDI::QueryAdapters: Fail getting descriptor" );
            }

            VKE::RenderSystem::SAdapterInfo info = {};

            info.deviceID   = static_cast< uint32_t >( adapterDesc.DeviceId );           // from: UINT
            info.vendorID   = static_cast< uint32_t >( adapterDesc.VendorId );           // from: UINT
            info.apiVersion = static_cast< uint32_t >( getMaxFeatureLevel( pAdapter ) ); // from: D3D_FEATURE_LEVEL

            info.hDDIAdapter = reinterpret_cast< handle_t >( pAdapter );

            LARGE_INTEGER driverVersion = {};
            if( SUCCEEDED( pAdapter->CheckInterfaceSupport( __uuidof( IDXGIDevice ), &driverVersion ) ) )
            {
                // Intel UHD eg: 30.0.101.1273
                // Nvidia eg: 31.0.15.3742
                WORD major = driverVersion.QuadPart >> 48;
                WORD minor = ( driverVersion.QuadPart >> 32 ) & 0xFFFF;
                WORD patch = ( driverVersion.QuadPart >> 16 ) & 0xFFFF;
                WORD build = driverVersion.QuadPart & 0xFFFF;

                char output[ 128 ];
                sprintf_s( &output[ 0 ], 128, "%u.%u.%u.%u", major, minor, patch, build );
                VKE_LOG( output );

                info.driverVersion = ( major << 16 ) | patch;
            }

            if( adapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE )
            {
                info.type = VKE::RenderSystem::ADAPTER_TYPE::VIRTUAL;
            }
            else if( adapterDesc.DedicatedVideoMemory == 0 )
            {
                info.type = VKE::RenderSystem::ADAPTER_TYPE::INTEGRATED;
            }
            else
            {
                info.type = VKE::RenderSystem::ADAPTER_TYPE::DISCRETE;
            }

            // Discrepancy between Vulkan and DX12 - Vk reports char[] while DX12 wchar[] in unicode.
            // To store info.name, conversion is needed.
            size_t maxSize = std::min( _countof( info.name ), _countof( adapterDesc.Description ) );
            wcstombs( info.name, adapterDesc.Description, maxSize );

            pOut->PushBack( info );
        }

        pOut->Resize( index - 1 );

        return Result::OK;
    }

    Result CDDI::Load( const SDDILoadInfo& Info, SDriverInfo* pOut )
    {
        if( Info.enableDebugMode )
        {
            ID3D12Debug* pDebug;
            if( FAILED( D3D12GetDebugInterface( IID_PPV_ARGS( &pDebug ) ) ) )
            {
                VKE_LOG_ERR( "CDDI::Load: Error while getting debug interface." );
            }
            else
            {
                pDebug->EnableDebugLayer();
                NativeAPI::SImplementation::sDebugLayerEnabled = true;
            }
        }

        // DX12 doesn't actually have a driver to load, just create the DXGI Factory via DXGI.
        Result            res = VKE_OK;
        NativeAPI::Result nativeRes;

        UINT flags = Info.enableDebugMode ? DXGI_CREATE_FACTORY_DEBUG : 0;
        nativeRes  = CreateDXGIFactory2( flags, IID_PPV_ARGS( &NativeAPI::SImplementation::spFactory ) );

        if( FAILED( nativeRes ) )
        {
            VKE_LOG_ERR( "CDDI::Load: Failed to create DXGI Factory" );
            return VKE_FAIL;
        }

        BOOL allowTearing = FALSE;
        if( FAILED( NativeAPI::SImplementation::spFactory->CheckFeatureSupport(
                DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof( allowTearing ) ) ) )
        {
            VKE_LOG_ERR( "CDDI::QueryAdapters: Check tearing support failed" );
            return VKE_FAIL;
        }

        NativeAPI::SImplementation::sTearingSupported = ( allowTearing == TRUE );

        return VKE_OK;
    }

    template< typename FeatureOption >
    FeatureOption GetFeatureOption( NativeAPI::Device pDevice, D3D12_FEATURE Feature )
    {
        FeatureOption option = {};

        if( FAILED( pDevice->CheckFeatureSupport( Feature, &option, sizeof( option ) ) ) )
        {
            VKE_LOG_ERR( "CDDI::CheckForFeature: CheckFeatureSupport failed" );
        }

        return option;
    }

    Result QueryAdapterProperties( const NativeAPI::Adapter& hAdapter, SDeviceProperties* pOut )
    {
        Memory::Zero( &pOut->Features );
        Memory::Zero( &pOut->Limits );
        Memory::Zero( &pOut->Properties );

        // Query memory properties
        DXGI_QUERY_VIDEO_MEMORY_INFO videoMemoryInfo = {};

        HRESULT hr;
        if( FAILED( hr = hAdapter->QueryVideoMemoryInfo( 0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &videoMemoryInfo ) ) )
        {
            VKE_LOG_ERR( "CDDI::QueryDeviceInfo: QueryVideoMemoryInfo failed with error code " + std::to_string( hr ) );
        }

        pOut->Properties.Memory.localBudget = videoMemoryInfo.Budget;

        if( FAILED( hr = hAdapter->QueryVideoMemoryInfo( 0, DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL, &videoMemoryInfo ) ) )
        {
            VKE_LOG_ERR( "CDDI::QueryDeviceInfo: QueryVideoMemoryInfo failed with error code " + std::to_string( hr ) );
        }

        pOut->Properties.Memory.hostBudget = videoMemoryInfo.Budget;

        return Result::OK;
    }

    // Object methods

    Result CDDI::CreateDevice( const SCreateDeviceDesc& Info, CDeviceContext* pCtx )
    {
        m_hAdapter = reinterpret_cast< NativeAPI::Adapter >( pCtx->m_Desc.pAdapterInfo->hDDIAdapter );
        m_pCtx     = pCtx;

        VKE_ASSERT2( m_hAdapter != NativeAPI::Null, "CDDI::CreateDevice: Adapter is null" );
        VKE_RETURN_IF_FAILED( QueryAdapterProperties( m_hAdapter, &m_DeviceProperties ) );

        // Compare pCtx->m_Desc.pAdapterInfo->apiVersion with Info.Settings.Features

        HRESULT result = D3D12CreateDevice( m_hAdapter,
                                            static_cast< D3D_FEATURE_LEVEL >( pCtx->m_Desc.pAdapterInfo->apiVersion ),
                                            IID_PPV_ARGS( &m_hDevice ) );
        if( FAILED( result ) )
        {
            VKE_LOG_ERR( "CDDI::CreateDevice: D3D12CreateDevice failed" );
            return VKE_FAIL;
        }

        if( NativeAPI::SImplementation::sDebugLayerEnabled )
        {
            ID3D12InfoQueue* pInfoQueue = NativeAPI::Null;
            if( FAILED( m_hDevice->QueryInterface( &pInfoQueue ) ) )
            {
                VKE_LOG_ERR( "CDDI::CreateDevice: QueryInterface for ID3D12InfoQueue failed" );
            }
            else
            {
                // Suppress messages about unrecognized format support queries
                D3D12_MESSAGE_SEVERITY severities[] = {
                    D3D12_MESSAGE_SEVERITY_INFO,
                };

                D3D12_MESSAGE_ID denyIds[] = {
                    D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
                    D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,
                };

                D3D12_INFO_QUEUE_FILTER filter = {};
                filter.DenyList.NumSeverities  = _countof( severities );
                filter.DenyList.pSeverityList  = severities;
                filter.DenyList.NumIDs         = _countof( denyIds );
                filter.DenyList.pIDList        = denyIds;

                if( FAILED( pInfoQueue->PushStorageFilter( &filter ) ) )
                {
                    VKE_LOG_ERR( "CDDI::CreateDevice: PushStorageFilter failed" );
                }

                pInfoQueue->Release();
            }
        }

        if( Info.Settings.Features.raytracing == FeatureEnableModes::ENABLE &&
            GetFeatureOption< D3D12_FEATURE_DATA_D3D12_OPTIONS5 >( m_hDevice,
                                                                   D3D12_FEATURE::D3D12_FEATURE_D3D12_OPTIONS5 )
                    .RaytracingTier == D3D12_RAYTRACING_TIER_NOT_SUPPORTED )
        {
            VKE_LOG_ERR( "CDDI::CreateDevice: Raytracing not supported on this device" );
            return VKE_FAIL;
        }

        if( Info.Settings.Features.meshShaders == FeatureEnableModes::ENABLE &&
            GetFeatureOption< D3D12_FEATURE_DATA_D3D12_OPTIONS7 >( m_hDevice, D3D12_FEATURE_D3D12_OPTIONS7 )
                    .MeshShaderTier == D3D12_MESH_SHADER_TIER_NOT_SUPPORTED )
        {
            VKE_LOG_ERR( "CDDI::CreateDevice: Mesh Shaders not supported on this device" );
            return VKE_FAIL;
        }

        if( Info.Settings.Features.bindlessResourceAccess == FeatureEnableModes::ENABLE &&
            GetFeatureOption< D3D12_FEATURE_DATA_D3D12_OPTIONS >( m_hDevice, D3D12_FEATURE_D3D12_OPTIONS )
                    .ResourceBindingTier != D3D12_RESOURCE_BINDING_TIER_3 )
        {
            VKE_LOG_WARN( "CDDI::CreateDevice: Bindless Resource Access not fully supported on this device" );
        }

        D3D12_FEATURE_DATA_D3D12_OPTIONS16 options16 = GetFeatureOption< D3D12_FEATURE_DATA_D3D12_OPTIONS16 >(
            m_hDevice, D3D12_FEATURE::D3D12_FEATURE_D3D12_OPTIONS16 );

        m_Implementation.Properties.Memory.UploadHeapSupported = options16.GPUUploadHeapSupported;

        for( UINT i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; i++ )
        {
            m_Implementation.Properties.Memory.DescriptorHeapSizes[ i ] =
                m_hDevice->GetDescriptorHandleIncrementSize( static_cast< D3D12_DESCRIPTOR_HEAP_TYPE >( i ) );
        }

        return Result::OK;
    }

    void CDDI::DestroyDevice()
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    NativeAPI::Queue CreateCommandQueue( NativeAPI::Device pDevice, D3D12_COMMAND_LIST_TYPE type,
                                         bool required = false )
    {
        D3D12_COMMAND_QUEUE_DESC desc = {};
        desc.Type                     = type;
        desc.Priority                 = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        desc.Flags                    = D3D12_COMMAND_QUEUE_FLAG_NONE;
        desc.NodeMask                 = getNodeMask();

        NativeAPI::Queue pQueue = NativeAPI::Null;
        if( FAILED( pDevice->CreateCommandQueue( &desc, IID_PPV_ARGS( &pQueue ) ) ) && required )
        {
            VKE_LOG_ERR( "CDDI::CreateCommandQueue: Failed to create command queue" );
        }

        return pQueue;
    }

    void CDDI::QueryDeviceInfo( SDeviceInfo* pOut )
    {
        TRACK_CALL_ONCE( "CDDI::QueryDeviceInfo" );

        auto& Limits = pOut->Limits;

        auto& Alignment                = Limits.Alignment;
        Alignment.constantBufferOffset = 256;
        Alignment.bufferCopyOffset     = 0;
        Alignment.bufferCopyRowPitch   = 256;
        Alignment.memoryMap =
            0; // DX12 doesn't require alignments for memory mapping but resources needs to be aligned
               // appropriately, eg.:  64KB for default buffers/textures, 4KB for upload/readback buffers/textures.
        Alignment.texelBufferOffset   = 0;
        Alignment.storageBufferOffset = 0;

        auto& Binding                        = Limits.Binding;
        Binding.maxConstantBufferRange       = D3D12_REQ_CONSTANT_BUFFER_ELEMENT_COUNT * 16; // 16 bytes per element
        Binding.maxPushConstantsSize         = 0; // DX12 doesn't have push constants
        Binding.Stage.maxConstantBufferCount = D3D12_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT;
        Binding.Stage.maxSamplerCount        = D3D12_COMMONSHADER_SAMPLER_SLOT_COUNT;
        Binding.Stage.maxStorageBufferCount  = 0; // DX12 doesn't have storage buffers
        Binding.Stage.maxStorageTextureCount = 0; // DX12 doesn't have storage textures
        Binding.Stage.maxResourceCount       = D3D12_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT;
        Binding.Stage.maxTextureCount        = D3D12_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT;

        auto& Memory                            = Limits.Memory;
        Memory.maxAllocationCount               = UINT32_MAX; // No documented limit
        Memory.minMapAlignment                  = 0;          // No documented limit
        Memory.minTexelBufferOffsetAlignment    = 0;          // No documented limit
        Memory.minConstantBufferOffsetAlignment = 256;        // Must be 256-byte aligned
        Memory.minStorageBufferOffsetAlignment  = 0;          // No documented limit

        auto& l1Budget = m_Implementation.Properties.Memory.localBudget;
        auto& l0Budget = m_Implementation.Properties.Memory.hostBudget;

        // Right now just assume budgets for heaps for each level.
        auto& Heaps                            = m_Implementation.Properties.Memory.HeapProperties;
        Heaps[ MemoryHeapTypes::CPU ]          = { D3D12_HEAP_TYPE_DEFAULT, D3D12_MEMORY_POOL_L0, l0Budget };
        Heaps[ MemoryHeapTypes::GPU ]          = { D3D12_HEAP_TYPE_DEFAULT, D3D12_MEMORY_POOL_L1, l1Budget };
        Heaps[ MemoryHeapTypes::UPLOAD ]       = { D3D12_HEAP_TYPE_UPLOAD, D3D12_MEMORY_POOL_UNKNOWN, l1Budget };
        Heaps[ MemoryHeapTypes::CPU_CACHED ]   = { D3D12_HEAP_TYPE_READBACK, D3D12_MEMORY_POOL_L0, l0Budget };
        Heaps[ MemoryHeapTypes::CPU_COHERENT ] = { D3D12_HEAP_TYPE_READBACK, D3D12_MEMORY_POOL_L0, l0Budget };
        Heaps[ MemoryHeapTypes::OTHER ]        = { D3D12_HEAP_TYPE_CUSTOM, D3D12_MEMORY_POOL_UNKNOWN, l1Budget };

        auto& RenderPass                     = Limits.RenderPass;
        RenderPass.maxColorRenderTargetCount = D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT;

        auto& Query           = Limits.Query;
        Query.timestampPeriod = 0.0; // This should in nanoseconds to match other API.
                                     // DX12 needs to query the timestamp frequency from the command queue.

        VKE_ASSERT2( m_hDevice != NativeAPI::Null, "Device cannot be Null when calling QueryDeviceIndo" );

        // TODO(any): Think about queue priorities and multiple queues per family.
        for( int i = 0; i < QUEUE_TYPE::_MAX_COUNT; i++ )
        {
            NativeAPI::Queue pQueue = NativeAPI::Null;

            SQueueFamilyInfo queueInfo = {};

            queueInfo.index = i;
            queueInfo.type  = static_cast< QUEUE_TYPE >( i );

            D3D12_COMMAND_LIST_TYPE type = Map::getCommandListType( queueInfo.type );

            if( type != D3D12_COMMAND_LIST_TYPE_NONE )
            {
                pQueue = CreateCommandQueue( m_hDevice, type, type == D3D12_COMMAND_LIST_TYPE_DIRECT );
            }

            if( pQueue != NativeAPI::Null )
            {
                queueInfo.vQueues.PushBack( pQueue );
                queueInfo.vPriorities.PushBack( 1.0f );
            }
            else
            {
                queueInfo.vQueues.Resize( 0 );
                queueInfo.vPriorities.Resize( 0 );
            }

            m_DeviceProperties.vQueueFamilies.PushBack( queueInfo );
        }
    }

    NativeAPI::Buffer CDDI::CreateBuffer( const SBufferDesc& Desc, const void* pAllocator )
    {
        UNIMPLEMENTED_D3D12_METHOD();
        return NativeAPI::Null;
    }

    void CDDI::DestroyBuffer( NativeAPI::Buffer* pInOut, const void* pAllocator )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    NativeAPI::BufferView CDDI::CreateBufferView( const SBufferViewDesc& Desc, const void* pAllocator )
    {
        UNIMPLEMENTED_D3D12_METHOD();
        return NativeAPI::Null;
    }

    void CDDI::DestroyBufferView( NativeAPI::BufferView* pInOut, const void* pAllocator )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    Result CDDI::GetTextureFormatProperties( const STextureDesc& Desc, STextureFormatProperties* pOut )
    {
        UNIMPLEMENTED_D3D12_METHOD();
        return Result::OK;
    }

    NativeAPI::Texture CDDI::CreateTexture( const STextureDesc& Desc, const void* pAllocator )
    {
        UNIMPLEMENTED_D3D12_METHOD();
        return NativeAPI::Null;
    }

    void CDDI::DestroyTexture( NativeAPI::Texture* pInOut, const void* pAllocator )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    NativeAPI::TextureView CDDI::CreateTextureView( const STextureViewDesc& Desc, const void* pAllocator )
    {
        UNIMPLEMENTED_D3D12_METHOD();
        return NativeAPI::Null;
    }

    void CDDI::DestroyTextureView( NativeAPI::TextureView* pInOut, const void* pAllocator )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    NativeAPI::Framebuffer CDDI::CreateFramebuffer( const SFramebufferDesc& Desc, const void* pAllocator )
    {
        UNIMPLEMENTED_D3D12_METHOD();
        return NativeAPI::Null;
    }

    void CDDI::DestroyFramebuffer( NativeAPI::Framebuffer* pInOut, const void* pAllocator )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    NativeAPI::CPUFence CDDI::CreateFence( const SFenceDesc& Desc, const void* pAllocator )
    {
        VKE_ASSERT2( m_hDevice != NativeAPI::Null, "m_hDevice can't be null" );
        NativeAPI::CPUFence fence;

        D3D12_FENCE_FLAGS flags = D3D12_FENCE_FLAG_NONE;

        if( FAILED( m_hDevice->CreateFence( 0, flags, IID_PPV_ARGS( &fence.Obj ) ) ) )
        {
            VKE_LOG_ERR( "CDDI::CreateFence: Failed to create fence" );
        }

        if( Desc.isSignaled )
        {
            fence.Obj->Signal( 1 );
            fence.Value = 1;
        }
        else
        {
            fence.Value = 0;
        }

        return fence;
    }

    void CDDI::DestroyFence( NativeAPI::CPUFence* pInOut, const void* pAllocator )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    NativeAPI::GPUFence CDDI::CreateSemaphore( const SSemaphoreDesc& Desc, const void* pAllocator )
    {
        VKE_ASSERT2( m_hDevice != NativeAPI::Null, "m_hDevice can't be null" );
        NativeAPI::GPUFence fence;

        D3D12_FENCE_FLAGS flags = D3D12_FENCE_FLAG_NONE;

        if( FAILED( m_hDevice->CreateFence( 0, flags, IID_PPV_ARGS( &fence.Obj ) ) ) )
        {
            VKE_LOG_ERR( "CDDI::CreateFence: Failed to create fence" );
        }

        fence.Value = 0;
        return fence;
    }

    void CDDI::DestroySemaphore( NativeAPI::GPUFence* pInOut, const void* pAllocator )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    NativeAPI::RenderPass CDDI::CreateRenderPass( const SRenderPassDesc& Desc, const void* pAllocator )
    {
        UNIMPLEMENTED_D3D12_METHOD();
        return NativeAPI::Null;
    }

    void CDDI::DestroyRenderPass( NativeAPI::RenderPass* pInOut, const void* pAllocator )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    NativeAPI::CommandBufferPool CDDI::CreateCommandBufferPool( const SCommandBufferPoolDesc& Desc,
                                                                const void*                   pAllocator )
    {
        VKE_ASSERT2( m_hDevice != NativeAPI::Null, "CDDI::CreateCommandBufferPool: m_hDevice can't be null" );
        NativeAPI::CommandBufferPool pCommandAllocator = NativeAPI::Null;

        D3D12_COMMAND_LIST_TYPE type = Map::getCommandListType( Desc.pContext->m_pQueue->GetType() );
        if( type == D3D12_COMMAND_LIST_TYPE_NONE )
        {
            VKE_LOG_ERR( "CDDI::CreateCommandBufferPool: Unsupported command list type" );
            return NativeAPI::Null;
        }

        if( FAILED( m_hDevice->CreateCommandAllocator( type, IID_PPV_ARGS( &pCommandAllocator.Obj ) ) ) )
        {
            VKE_LOG_ERR( "CDDI::CreateCommandBufferPool: Failed to create command allocator" );
        }

        pCommandAllocator.EngineType = Desc.pContext->m_pQueue->GetType();
        pCommandAllocator.NativeType = type;

        return pCommandAllocator;
    }

    void CDDI::DestroyCommandBufferPool( NativeAPI::CommandBufferPool* pInOut, const void* pAllocator )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    NativeAPI::DescriptorPool CDDI::CreateDescriptorPool( const SDescriptorPoolDesc& Desc, const void* pAllocator )
    {
        VKE_ASSERT2( m_hDevice != NativeAPI::Null, "CDDI::CreateDescriptorPool: m_hDevice can't be null" );

        uint32_t descriptorHeapSizes[ D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES ] = { 0 };

        NativeAPI::DescriptorPool  pool     = NativeAPI::Null;
        D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
        D3D12_DESCRIPTOR_HEAP_TYPE heapType = {};

        for( auto& poolSize: Desc.vPoolSizes )
        {
            if( poolSize.type == DESCRIPTOR_SET_TYPE::SAMPLER_AND_TEXTURE )
            {
                VKE_LOG_WARN(
                    "CDDI::CreateDescriptorPool: DESCRIPTOR_SET_TYPE::SAMPLER_AND_TEXTURE is not supported in D3D12" );
            }
            heapType                         = Map::getDescriptorHeapType( poolSize.type );
            descriptorHeapSizes[ heapType ] += poolSize.count;
        }

        for( uint32_t i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; i++ )
        {
            if( descriptorHeapSizes[ i ] )
            {

                heapDesc.Type           = static_cast< D3D12_DESCRIPTOR_HEAP_TYPE >( i );
                heapDesc.NumDescriptors = descriptorHeapSizes[ i ];
                heapDesc.NodeMask       = getNodeMask();
                heapDesc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

                if( heapDesc.Type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV ||
                    heapDesc.Type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER )
                {
                    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
                }

                if( FAILED( m_hDevice->CreateDescriptorHeap( &heapDesc, IID_PPV_ARGS( &pool.Heaps[ i ] ) ) ) )
                {
                    VKE_LOG_ERR( "CDDI::CreateDescriptorPool: Failed to create descriptor heap" );
                }
            }
        }

        return pool;
    }

    void CDDI::DestroyDescriptorPool( NativeAPI::DescriptorPool* pInOut, const void* pAllocator )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    NativeAPI::DescriptorSetLayout CDDI::CreateDescriptorSetLayout( const SDescriptorSetLayoutDesc& Desc,
                                                                    const void*                     pAllocator )
    {
        VKE_ASSERT2( m_hDevice != NativeAPI::Null, "CDDI::CreateDescriptorSetLayout: m_hDevice can't be null" );
        NativeAPI::DescriptorSetLayout descriptorSetLayout = NativeAPI::Null;

        NativeAPI::CustomTypes::DDIDescriptorSetLayout& rootParameter = descriptorSetLayout.Obj;

        // Assume shader visibility all, see notes below.
        rootParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        rootParameter.ParameterType    = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;

        for( auto& binding: Desc.vBindings )
        {
            // We can prevent shader visibility set to all by create each range for a specific visibility. For example
            // range with visibility PIXEL, VERTEX. But it still won't match Vulkan as theres no possibility for
            // combining like PIXEL | VERTEX.
            // D3D12_SHADER_VISIBILITY visibility = Convert::getShaderVisibility( binding.stages );
            NativeAPI::CustomTypes::DDIDescriptorSetRange range;

            range.RangeType          = Convert::getDescriptorRangeType( binding.type );
            range.NumDescriptors     = binding.count;
            range.BaseShaderRegister = binding.idx;
            range.RegisterSpace      = 0;

            /// This flag allows us to update descriptors in descriptor table on command list execution.
            /// Same as in vulkan.
            range.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;

            range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
            descriptorSetLayout.vDescriptorRanges.PushBack( range );
        }

        rootParameter.DescriptorTable.NumDescriptorRanges = descriptorSetLayout.vDescriptorRanges.GetCount();
        rootParameter.DescriptorTable.pDescriptorRanges   = descriptorSetLayout.vDescriptorRanges.GetData();

        return descriptorSetLayout;
    }

    void CDDI::DestroyDescriptorSetLayout( NativeAPI::DescriptorSetLayout* pInOut, const void* pAllocator )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    NativeAPI::Pipeline CDDI::CreatePipeline( const SPipelineDesc& Desc, const void* pAllocator )
    {
        UNIMPLEMENTED_D3D12_METHOD();
        return NativeAPI::Null;
    }

    void CDDI::DestroyPipeline( NativeAPI::Pipeline* pInOut, const void* pAllocator )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    NativeAPI::PipelineLayout CDDI::CreatePipelineLayout( const SPipelineLayoutDesc& Desc, const void* pAllocator )
    {
        VKE_ASSERT2( m_hDevice != NativeAPI::Null, "CDDI::CreatePipelineLayout: m_hDevice can't be null" );
        VKE_ASSERT2( m_pCtx != nullptr, "CDDI::CreatePipelineLayout: m_pCtx can't be null" );

        D3D12_VERSIONED_ROOT_SIGNATURE_DESC versionedRootSignature;
        versionedRootSignature.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;

        D3D12_ROOT_SIGNATURE_DESC1& rootSignatureDesc = versionedRootSignature.Desc_1_1;

        rootSignatureDesc.NumStaticSamplers = 0;
        rootSignatureDesc.pStaticSamplers   = nullptr;
        rootSignatureDesc.NumParameters     = 0;
        rootSignatureDesc.Flags             = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

        Utils::TCDynamicArray< D3D12_ROOT_PARAMETER1, 32 > vRootParameters;
        for( auto& layout: Desc.vDescriptorSetLayouts )
        {
            const NativeAPI::DescriptorSetLayout& hDescriptorSetLayout = m_pCtx->GetDescriptorSetLayout( layout );
            vRootParameters.PushBack( hDescriptorSetLayout.Obj );
        }

        // Right now push constants are not being used in Engine. Root signature includes information about push
        // constants should be enabled. Enable this once completed.
        /* for( auto& pushConst: Desc.vPushConstants )
        {
            D3D12_ROOT_PARAMETER1 param;
            param.ParameterType    = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
            param.ShaderVisibility = Convert::getShaderVisibility( pushConst.stages );

            // stages, size, offset
            param.Constants.Num32BitValues = pushConst.size;
            param.Constants.RegisterSpace  = 0;
            param.Constants.ShaderRegister;
        } */

        rootSignatureDesc.NumParameters = vRootParameters.GetCount();
        rootSignatureDesc.pParameters   = vRootParameters.GetData();

        ID3DBlob* pSignatureBlob = nullptr;
        ID3DBlob* pErrorBlob     = nullptr;

        if( FAILED( D3D12SerializeVersionedRootSignature( &versionedRootSignature, &pSignatureBlob, &pErrorBlob ) ) )
        {
            VKE_LOG_ERR( "CDDI::CreatePipelineLayout: Unable to serialize root signature." );
        }

        ID3D12RootSignature* pRootSignature;

        if( FAILED( m_hDevice->CreateRootSignature( getNodeMask(),
                                                    pSignatureBlob->GetBufferPointer(),
                                                    pSignatureBlob->GetBufferSize(),
                                                    IID_PPV_ARGS( &pRootSignature ) ) ) )
        {
            VKE_LOG_ERR( "CDDI::CreatePipelineLayout: Unable to create root signature." );
        }

        return pRootSignature;
    }

    void CDDI::DestroyPipelineLayout( NativeAPI::PipelineLayout* pInOut, const void* pAllocator )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    NativeAPI::Shader CDDI::CreateShader( const SShaderData& Desc, const void* pAllocator )
    {
        NativeAPI::Shader shader;
        shader.Obj.pShaderBytecode = (BYTE*)Desc.pCode;
        shader.Obj.BytecodeLength  = Desc.codeSize;
        return shader;
    }

    void CDDI::DestroyShader( NativeAPI::Shader* pInOut, const void* pAllocator )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    NativeAPI::Sampler CDDI::CreateSampler( const SSamplerDesc& Desc, const void* pAllocator )
    {
        UNIMPLEMENTED_D3D12_METHOD();
        return NativeAPI::Null;
    }

    void CDDI::DestroySampler( NativeAPI::Sampler* pInOut, const void* pAllocator )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    NativeAPI::Event CDDI::CreateEvent( const SEventDesc& Desc, const void* pAllocator )
    {
        UNIMPLEMENTED_D3D12_METHOD();
        return NativeAPI::Null;
    }

    void CDDI::DestroyEvent( NativeAPI::Event* pInOut, const void* pAllocator )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    Result CDDI::AllocateObjects( const AllocateDescs::SDescSet& Info, NativeAPI::DescriptorSet* pSets )
    {
        UNIMPLEMENTED_D3D12_METHOD();
        return Result::OK;
    }

    void CDDI::FreeObjects( const FreeDescs::SDescSet& )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    Result CDDI::AllocateObjects( const SAllocateCommandBufferInfo& Info, NativeAPI::CommandBuffer* pBuffers )
    {
        Result result = VKE_OK;
        VKE_ASSERT2( m_hDevice != NativeAPI::Null, "CDDI::AllocateObjects: m_hDevice can't be null" );

        D3D12_COMMAND_LIST_TYPE type =
            ( Info.level == COMMAND_BUFFER_LEVEL::PRIMARY ) ? Info.hDDIPool.NativeType : D3D12_COMMAND_LIST_TYPE_BUNDLE;

        if( type == D3D12_COMMAND_LIST_TYPE_NONE )
        {
            VKE_LOG_WARN( "CDDI::AllocateObjects: Unsupported command list type" );
            return result;
        }

        for( uint32_t i = 0; i < Info.count; i++ )
        {
            NativeAPI::CommandBuffer pCommandList = NativeAPI::Null;
            if( FAILED( m_hDevice->CreateCommandList(
                    0, type, Info.hDDIPool.Obj, NativeAPI::Null, IID_PPV_ARGS( &pCommandList ) ) ) )
            {
                VKE_LOG_ERR( "CDDI::AllocateObjects: Failed to create command list" );
                result = VKE_FAIL;
                break;
            }

            // CreateCommandList always create command list in open state. In order to allocate more objects
            // every command list needs to be in closed state.
            pCommandList->Close();
            pBuffers[ i ] = pCommandList;
        }

        return result;
    }

    void CDDI::FreeObjects( const SFreeCommandBufferInfo& )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    Result CDDI::GetBufferMemoryRequirements( const NativeAPI::Buffer& hBuffer, SAllocationMemoryRequirementInfo* pOut )
    {
        UNIMPLEMENTED_D3D12_METHOD();
        return Result::OK;
    }

    Result CDDI::GetTextureMemoryRequirements( const NativeAPI::Texture&         hTexture,
                                               SAllocationMemoryRequirementInfo* pOut )
    {
        UNIMPLEMENTED_D3D12_METHOD();
        return Result::OK;
    }

    void CDDI::UpdateDesc( SBufferDesc* pInOut )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    void CDDI::GetFormatFeatures( FORMAT fmt, STextureFormatFeatures* pOut ) const
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    Result CDDI::Bind( RESOURCE_TYPE Type, const SBindMemoryInfo& Info )
    {
        UNIMPLEMENTED_D3D12_METHOD();
        return Result::OK;
    }

    void CDDI::Bind( const SBindPipelineInfo& Info )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    void CDDI::Bind( const SBindDDIDescriptorSetsInfo& Info )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    void CDDI::Bind( const SBindRenderPassInfo& Info )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    void CDDI::Bind( const NativeAPI::CommandBuffer& hDDICmdBuffer, const NativeAPI::Buffer& hDDIBuffer,
                     const uint32_t offset )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    void CDDI::Bind( const NativeAPI::CommandBuffer& hDDICmdBuffer, const NativeAPI::Buffer& hDDIBuffer,
                     const uint32_t offset, const INDEX_TYPE& type )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    void CDDI::UnbindPipeline( const NativeAPI::CommandBuffer&, const NativeAPI::Pipeline& )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    void CDDI::UnbindRenderPass( const NativeAPI::CommandBuffer&, const NativeAPI::RenderPass& )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    void CDDI::Free( NativeAPI::Memory* phMemory, const void* )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    void CDDI::Update( const SUpdateBufferDescriptorSetInfo& Info )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    void CDDI::Update( const SUpdateTextureDescriptorSetInfo& Info )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    void CDDI::Update( const NativeAPI::DescriptorSet& hDDISet, const SUpdateBindingsHelper& Info )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    void CDDI::Update( const NativeAPI::DescriptorSet& hDDISrcSet, NativeAPI::DescriptorSet* phDDIDstOut )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    Result CDDI::Allocate( const SAllocateMemoryDesc& Desc, SAllocateMemoryData* pOut )
    {
        UNIMPLEMENTED_D3D12_METHOD();
        return Result::OK;
    }

    MEMORY_HEAP_TYPE CDDI::GetMemoryHeapType( MEMORY_USAGE usage ) const
    {
        UNIMPLEMENTED_D3D12_METHOD();
        return MEMORY_HEAP_TYPE::GPU;
    }

    size_t CDDI::GetMemoryHeapTotalSize( MEMORY_HEAP_TYPE type ) const
    {
        VKE_ASSERT2( type < MemoryHeapTypes::_MAX_COUNT, "Incorrect MEMORY_HEAP_TYPE" );
        return m_Implementation.Properties.Memory.HeapProperties[ type ].SizeInBytes;
    }

    size_t CDDI::GetMemoryHeapCurrentSize( MEMORY_HEAP_TYPE ) const
    {
        UNIMPLEMENTED_D3D12_METHOD();
        return 0;
    }

    void* CDDI::MapMemory( const SMapMemoryInfo& Info )
    {
        UNIMPLEMENTED_D3D12_METHOD();
        return NativeAPI::Null;
    }

    void CDDI::UnmapMemory( const NativeAPI::Memory& hDDIMemory )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    void CDDI::Reset( const NativeAPI::CommandBuffer&     hCommandBuffer,
                      const NativeAPI::CommandBufferPool& hCommandBufferPool )
    {
        if( FAILED( hCommandBufferPool.Obj->Reset() ) )
        {
            VKE_LOG_ERR( "CDDI::Reset: Failed to reset command buffer pool" );
        }

        if( FAILED( hCommandBuffer->Reset( hCommandBufferPool.Obj, NativeAPI::Null ) ) )
        {
            VKE_LOG_ERR( "CDDI::Reset: Failed to reset command buffer" );
        }
    }

    void CDDI::BeginCommandBuffer( const NativeAPI::CommandBuffer& hCommandBuffer )
    {
        // CommandAllocator and CommandList should be in reset state before calling this method, which is done by
        // Reset() method. D3D12 doesn't have any additional flags for beginning command buffer recording.
    }

    void CDDI::EndCommandBuffer( const NativeAPI::CommandBuffer& hCommandBuffer )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    void CDDI::Barrier( const NativeAPI::CommandBuffer& hCommandBuffer, const SBarrierInfo& Info )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    void CDDI::SetState( const NativeAPI::CommandBuffer& hCommandBuffer, const SViewportDesc& Desc )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    void CDDI::SetState( const NativeAPI::CommandBuffer& hCommandBuffer, const SScissorDesc& Desc )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    void CDDI::Draw( const NativeAPI::CommandBuffer& hCommandBuffer, const uint32_t& vertexCount,
                     const uint32_t& instanceCount, const uint32_t& firstVertex, const uint32_t& firstInstance )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    void CDDI::DrawIndexed( const NativeAPI::CommandBuffer& hCommandBuffer, const SDrawParams& Params )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    void CDDI::DrawMesh( const NativeAPI::CommandBuffer& hCommandBuffer, uint32_t width, uint32_t height,
                         uint32_t depth )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    void CDDI::BeginRenderPass( NativeAPI::CommandBuffer, const SBeginRenderPassInfo2& )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    void CDDI::EndRenderPass( NativeAPI::CommandBuffer )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    void CDDI::Copy( const NativeAPI::CommandBuffer& hDDICmdBuffer, const SCopyTextureInfoEx& Info )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    void CDDI::Copy( const NativeAPI::CommandBuffer& hCmdBuffer, const SCopyBufferInfo& Info )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    void CDDI::Copy( const NativeAPI::CommandBuffer& hDDICmdBuffer, const SCopyBufferToTextureInfo& Info )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    void CDDI::Blit( const NativeAPI::CommandBuffer& hAPICmdBuffer, const SBlitTextureInfo& Info )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    void CDDI::SetEvent( const NativeAPI::Event& hDDIEvent )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    void CDDI::SetEvent( const NativeAPI::CommandBuffer& hDDICmdBuffer, const NativeAPI::Event& hDDIEvent,
                         const PIPELINE_STAGES& stages )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    void CDDI::Reset( const NativeAPI::Event& hDDIInOut )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    void CDDI::Reset( const NativeAPI::CommandBuffer& hDDICmdBuffer, const NativeAPI::Event& hDDIEvent,
                      const PIPELINE_STAGES& stages )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    bool CDDI::IsSet( const NativeAPI::Event& hDDIEvent )
    {
        UNIMPLEMENTED_D3D12_METHOD();
        return false;
    }

    Result CDDI::Submit( const SSubmitInfo& Info )
    {
        UNIMPLEMENTED_D3D12_METHOD();
        return Result::OK;
    }

    Result CDDI::Present( const SPresentData& Info )
    {
        UNIMPLEMENTED_D3D12_METHOD();
        return Result::OK;
    }

    Result CDDI::CreateSwapChain( const SSwapChainDesc& Desc, const void*, SDDISwapChain* pInOut )
    {
        UNIMPLEMENTED_D3D12_METHOD();
        return Result::OK;
    }

    void CDDI::DestroySwapChain( SDDISwapChain* pInOut, const void* )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    Result CDDI::ReCreateSwapChain( const SSwapChainDesc& Desc, SDDISwapChain* pOut )
    {
        UNIMPLEMENTED_D3D12_METHOD();
        return Result::OK;
    }

    Result CDDI::QueryPresentSurfaceCaps( const NativeAPI::PresentSurface& hSurface, SPresentSurfaceCaps* pOut )
    {
        UNIMPLEMENTED_D3D12_METHOD();
        return Result::OK;
    }

    Result CDDI::GetCurrentBackBufferIndex( const SDDISwapChain& SwapChain, const SDDIGetBackBufferInfo& Info,
                                            uint32_t* pOut )
    {
        UNIMPLEMENTED_D3D12_METHOD();
        return Result::OK;
    }

    void CDDI::Convert( const SClearValue& In, NativeAPI::ClearValue* pOut )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    // Debug

    void CDDI::BeginDebugInfo( const NativeAPI::CommandBuffer& hDDICmdBuff, const SDebugInfo* pInfo )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    void CDDI::EndDebugInfo( const NativeAPI::CommandBuffer& hDDICmdBuff )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    void CDDI::SetObjectDebugName( const uint64_t& handle, const uint32_t& objType, cstr_t pName ) const
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    void CDDI::SetQueueDebugName( uint64_t handle, cstr_t pName ) const
    {
        NativeAPI::Queue pQueue = (NativeAPI::Queue)handle;
        VKE_ASSERT2( pQueue != NativeAPI::Null, "CDDI::SetQueueDebugName: Queue is null" );
        pQueue->SetName( (LPCWSTR)pName );
    }

    bool CDDI::IsSignaled( const NativeAPI::CPUFence& hFence ) const
    {
        VKE_ASSERT2( m_hDevice != NativeAPI::Null, "CDDI::IsSignaled: m_hDevice is null" );
        return hFence.Obj->GetCompletedValue() >= hFence.Value;
    }

    void CDDI::Reset( NativeAPI::CPUFence* phFence )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    Result CDDI::WaitForFences( const NativeAPI::CPUFence& hFence, uint64_t timeout )
    {
        // TODO(blturkot): Wait for fence implementation.
        return Result::OK;
    }

    Result CDDI::WaitForQueue( const NativeAPI::Queue& hQueue )
    {
        // TODO(blturkot): Each queue needs to have its own fence for synchronization.
        return Result::OK;
    }

    Result CDDI::WaitForDevice()
    {
        // TODO(blturkot): Get all queues and wait for them.
        return Result::OK;
    }

} // namespace VKE::RenderSystem

#endif // VKE_D3D12_RENDER_SYSTEM
