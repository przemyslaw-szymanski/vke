#include "RenderSystem/CDDI.h"
#include "RenderSystem/CDDI.h"

#if VKE_RENDER_SYSTEM_D3D12

#include "Core/Managers/CFileManager.h"
#include "Core/Platform/CWindow.h"

#include "RenderSystem/CContextBase.h"
#include "RenderSystem/CDeviceContext.h"
#include "RenderSystem/CGraphicsContext.h"
#include "RenderSystem/CRenderPass.h"
#include "RenderSystem/Resources/CBuffer.h"
#include "RenderSystem/Resources/CTexture.h"

#include "RenderSystem/D3D12/dxgiFormats.h"

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
        Factory SImplementation::spFactory                          = NativeAPI::Null;
        bool    SImplementation::sDebugLayerEnabled                 = false;
        bool    SImplementation::SDeviceFeatures::sTearingSupported = false;
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

        DXGI_COLOR_SPACE_TYPE getDXGIColorSpace( COLOR_SPACE colorSpace )
        {
            static const DXGI_COLOR_SPACE_TYPE vMap[] = {
                DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709, // SRGB
                // DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709, // SRGB_LINEAR
            };
            return vMap[ static_cast< size_t >( colorSpace ) ];
        }

        D3D12_SRV_DIMENSION getSRVDimension( TEXTURE_VIEW_TYPE type )
        {
            static const D3D12_SRV_DIMENSION vMap[] = {
                D3D12_SRV_DIMENSION_TEXTURE1D,        // VIEW_1D
                D3D12_SRV_DIMENSION_TEXTURE2D,        // VIEW_2D
                D3D12_SRV_DIMENSION_TEXTURE3D,        // VIEW_3D
                D3D12_SRV_DIMENSION_TEXTURECUBE,      // VIEW_CUBE
                D3D12_SRV_DIMENSION_TEXTURE1DARRAY,   // VIEW_1D_ARRAY
                D3D12_SRV_DIMENSION_TEXTURE2DARRAY,   // VIEW_2D_ARRAY
                D3D12_SRV_DIMENSION_TEXTURECUBEARRAY, // VIEW_CUBE_ARRAY
            };
            return vMap[ static_cast< size_t >( type ) ];
        }

        D3D12_RESOURCE_DIMENSION getResourceDimension( TEXTURE_TYPE type )
        {
            // TODO(blturkot): Add support for TEXTURE_TYPE::TEX_2D_ARRAY and TEX_CUBE
            static const D3D12_RESOURCE_DIMENSION vMap[] = {
                D3D12_RESOURCE_DIMENSION_TEXTURE1D, // TEX_1D
                D3D12_RESOURCE_DIMENSION_TEXTURE2D, // TEX_2D
                D3D12_RESOURCE_DIMENSION_TEXTURE3D, // TEX_3D
                D3D12_RESOURCE_DIMENSION_UNKNOWN,
            };

            VKE_ASSERT2( _countof( vMap ) == TEXTURE_TYPE::_MAX_COUNT,
                         "Texture type map is not coherent with engine texture types" );

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

        DXGI_FORMAT getDXGIFormat( FORMAT engineFormat )
        {
            VKE_ASSERT2( _countof( VKE::RenderSystem::D3D12::g_aFormats ) == FORMAT::_MAX_COUNT,
                         "DX12 format map is not coherent with engine formats" );

            uint32_t formatIndex = static_cast< size_t >( engineFormat );
            return VKE::RenderSystem::D3D12::g_aFormats[ formatIndex ];
        }

        D3D12_RESOURCE_STATES AccessMaskToResourceState( MEMORY_ACCESS_TYPE mask )
        {
            D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON;

            if( mask & ( MemoryAccessTypes::CPU_MEMORY_READ | MemoryAccessTypes::CPU_MEMORY_WRITE |
                         MemoryAccessTypes::GPU_MEMORY_READ | MemoryAccessTypes::GPU_MEMORY_WRITE ) )
            {
                // DX12 requires CPU access to be in COMMON state.
                return state;
            }

            if( mask & MemoryAccessTypes::INDIRECT_BUFFER_READ )
            {
                state |= D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
            }

            if( mask & MemoryAccessTypes::INDEX_READ )
            {
                state |= D3D12_RESOURCE_STATE_INDEX_BUFFER;
            }

            if( mask & ( MemoryAccessTypes::VERTEX_ATTRIBUTE_READ | MemoryAccessTypes::VS_UNIFORM_READ |
                         MemoryAccessTypes::PS_UNIFORM_READ | MemoryAccessTypes::GS_UNIFORM_READ |
                         MemoryAccessTypes::TS_UNIFORM_READ | MemoryAccessTypes::CS_UNIFORM_READ |
                         MemoryAccessTypes::MS_UNIFORM_READ | MemoryAccessTypes::RT_UNIFORM_READ ) )
            {
                state |= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
            }

            if( mask & MemoryAccessTypes::INPUT_ATTACHMENT_READ )
            {
                state |= D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
            }

            // Is this actually UAV?
            if( mask & ( MemoryAccessTypes::VS_SHADER_READ | MemoryAccessTypes::GS_SHADER_READ |
                         MemoryAccessTypes::TS_SHADER_READ | MemoryAccessTypes::CS_SHADER_READ |
                         MemoryAccessTypes::MS_SHADER_READ | MemoryAccessTypes::RS_SHADER_READ ) )
            {
                state |= D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
            }
            // And this?
            if( mask & MemoryAccessTypes::PS_SHADER_READ )
            {
                state |= D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
            }

            if( mask & ( MemoryAccessTypes::VS_SHADER_WRITE | MemoryAccessTypes::GS_SHADER_WRITE |
                         MemoryAccessTypes::TS_SHADER_WRITE | MemoryAccessTypes::CS_SHADER_WRITE |
                         MemoryAccessTypes::MS_SHADER_WRITE | MemoryAccessTypes::RS_SHADER_WRITE |
                         MemoryAccessTypes::PS_SHADER_WRITE ) )
            {
                state |= D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
            }

            if( mask &
                ( MemoryAccessTypes::COLOR_RENDER_TARGET_READ | MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ ) )
            {
                state |= D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;
            }

            if( mask & ( MemoryAccessTypes::COLOR_RENDER_TARGET_WRITE |
                         MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_WRITE ) )
            {
                state |= D3D12_RESOURCE_STATE_RENDER_TARGET;
            }

            if( mask & ( MemoryAccessTypes::DATA_TRANSFER_READ ) )
            {
                state |= D3D12_RESOURCE_STATE_COPY_SOURCE;
            }

            if( mask & ( MemoryAccessTypes::DATA_TRANSFER_WRITE ) )
            {
                state |= D3D12_RESOURCE_STATE_COPY_DEST;
            }

            return state;
        }

#if 0
        // This is for enhanced barriers. Need to check capabilities and add another path for barriers.
        vke_force_inline D3D12_BARRIER_SUBRESOURCE_RANGE getSubresourceIndex( NativeAPI::Texture              hTexture,
                                                                              const STextureSubresourceRange& range )
        {
            D3D12_BARRIER_SUBRESOURCE_RANGE ddiRange;
            ddiRange.IndexOrFirstMipLevel = range.beginMipmapLevel;
            ddiRange.NumMipLevels         = range.mipmapLevelCount;
            ddiRange.FirstArraySlice      = range.beginArrayLayer;
            ddiRange.NumArraySlices       = range.layerCount;
            ddiRange.FirstPlane           = 0;
            ddiRange.NumPlanes            = 1;

            return ddiRange;
        }
#endif
        vke_force_inline UINT getSubresourceIndex( UINT mipLevel, UINT mipCount, UINT arraySlice, UINT arraySliceCount,
                                                   UINT plane )
        {
            return mipLevel + ( arraySlice * mipCount ) + ( plane * mipCount * arraySliceCount );
        }

        DXGI_SAMPLE_DESC getSampleDesc( SAMPLE_COUNT count )
        {
            DXGI_SAMPLE_DESC desc;
            desc.Count   = ( 1 << count );
            desc.Quality = 0;
            return desc;
        }

        D3D12_RESOURCE_FLAGS getResourceFlags( TEXTURE_USAGE usage )
        {
            D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;

            if( ( usage & TextureUsages::STORAGE ) )
            {
                flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
            }

            if( ( usage & TextureUsages::COLOR_RENDER_TARGET ) )
            {
                flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
            }

            if( ( usage & TextureUsages::DEPTH_STENCIL_RENDER_TARGET ) )
            {
                flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL | D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

                if( ( usage & TextureUsages::SAMPLED ) == 0 )
                {
                    flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
                }
            }

            return flags;
        }

        D3D12_HEAP_DESC GetMemoryHeapDesc( MEMORY_USAGE usage, bool HeapTier2 )
        {
            static const MEMORY_USAGE accessMaskBits = MemoryUsages::CPU_ACCESS | MemoryUsages::GPU_ACCESS |
                                                       MemoryUsages::CPU_CACHED | MemoryUsages::CPU_NO_FLUSH;

            static const MEMORY_USAGE vPreconfiguredHeaps[] = {
                // D3D12_HEAP_TYPE_DEFAULT
                MemoryUsages::GPU_ACCESS | !MemoryUsages::CPU_ACCESS,
                // D3D12_HEAP_TYPE_UPLOAD
                MemoryUsages::GPU_ACCESS | MemoryUsages::CPU_ACCESS | MemoryUsages::CPU_NO_FLUSH,
                // D3D12_HEAP_TYPE_READBACK
                MemoryUsages::GPU_ACCESS | MemoryUsages::CPU_ACCESS | MemoryUsages::CPU_CACHED,
            };

            bool AllowBuffers  = usage & MemoryUsages::BUFFER;
            bool AllowTextures = usage & MemoryUsages::TEXTURE;
            bool CPUAccess     = usage & MemoryUsages::CPU_ACCESS;
            bool GPUAccess     = usage & MemoryUsages::GPU_ACCESS;
            bool IsWriteback   = usage & MemoryUsages::CPU_CACHED;
            bool IsUpload      = usage & MemoryUsages::CPU_NO_FLUSH;

            D3D12_HEAP_DESC desc;
            desc.SizeInBytes = 0;                                          // To be filled later
            desc.Alignment   = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT; // Default alignment

            // Reset node mask
            desc.Properties.CreationNodeMask = 0;
            desc.Properties.VisibleNodeMask  = 0;

            if( GPUAccess && !CPUAccess )
            {
                // This heap type experiences the most bandwidth for the GPU, but cannot provide CPU access. The GPU can
                // read and write to the memory from this pool, and resource transition barriers may be changed. The
                // majority of heaps and resources are expected to be located here, and are typically populated through
                // resources in upload heaps.
                desc.Properties.Type = D3D12_HEAP_TYPE_DEFAULT;
            }
            else if( GPUAccess && CPUAccess )
            {
                if( usage & MemoryUsages::CPU_NO_FLUSH )
                {
                    // This heap type has CPU access optimized for uploading to the GPU, but does not experience the
                    // maximum amount of bandwidth for the GPU. This heap type is best for CPU-write-once, GPU-read-once
                    // data; but GPU-read-once is stricter than necessary. GPU-read-once-or-from-cache is an acceptable
                    // use-case for the data; but such usages are hard to judge due to differing GPU cache designs and
                    // sizes. If in doubt, stick to the GPU-read-once definition or profile the difference on many GPUs
                    // between copying the data to a _DEFAULT heap vs. reading the data from an _UPLOAD heap.

                    // Resources in this heap must be created with D3D12_RESOURCE_STATE_GENERIC_READ and cannot be
                    // changed away from this. The CPU address for such heaps is commonly not efficient for CPU reads.
                    desc.Properties.Type = D3D12_HEAP_TYPE_UPLOAD;
                }
                else if( usage & MemoryUsages::CPU_CACHED )
                {
                    // Specifies a heap used for reading back. This heap type has CPU access optimized for reading data
                    // back from the GPU, but does not experience the maximum amount of bandwidth for the GPU. This heap
                    // type is best for GPU-write-once, CPU-readable data. The CPU cache behavior is write-back, which
                    // is conducive for multiple sub-cache-line CPU reads.

                    // Resources in this heap must be created with D3D12_RESOURCE_STATE_COPY_DEST,
                    // and cannot be changed away from this.
                    desc.Properties.Type = D3D12_HEAP_TYPE_READBACK;
                }
                else
                {
                    // The application may specify the memory pool and CPU cache properties directly, which can be
                    // useful for UMA optimizations, multi-engine, multi-adapter, or other special cases. To do so, the
                    // application is expected to understand the adapter architecture to make the right choice.
                    desc.Properties.Type = D3D12_HEAP_TYPE_CUSTOM;
                }
            }
            else
            {
                desc.Properties.Type = D3D12_HEAP_TYPE_CUSTOM;
            }

            // Reset remaining fields from descriptor. Predefined heaps already have correct properties and API requires
            // to set UNKNOWN for them. Otherwise it will report runtime error.
            desc.Properties.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
            desc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

            // Custom heaps requires overriding.
            if( desc.Properties.Type == D3D12_HEAP_TYPE_CUSTOM )
            {
                if( !CPUAccess )
                {
                    // No CPU access, acts like DEFAULT heap.
                    desc.Properties.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_NOT_AVAILABLE;
                    desc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_L1;
                }
                else if( CPUAccess && IsWriteback )
                {
                    desc.Properties.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
                    desc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
                }
                else if( CPUAccess && IsUpload )
                {
                    desc.Properties.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_WRITE_COMBINE;
                    desc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
                }
            }

            // TODO(szymansk): I've added this flag assuming engine will handle not zeroed resources. Or should we allow
            // API to zero?
            // desc.Flags = D3D12_HEAP_FLAG_CREATE_NOT_ZEROED;
            desc.Flags = D3D12_HEAP_FLAG_NONE;

            if( ( AllowBuffers && AllowTextures ) || HeapTier2 )
            {
                // TODO(blturkot): This allows all buffers and textures but it is only required when HW supports Tier2
                // heaps.
                desc.Flags |= D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES;
            }
            else
            {
                if( !AllowBuffers )
                {
                    desc.Flags |= D3D12_HEAP_FLAG_DENY_BUFFERS;
                }

                if( !AllowTextures )
                {
                    desc.Flags |= D3D12_HEAP_FLAG_DENY_RT_DS_TEXTURES | D3D12_HEAP_FLAG_DENY_NON_RT_DS_TEXTURES;
                }
            }

            return desc;
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

            if( (UINT)adapterDesc.Flags & (UINT)DXGI_ADAPTER_FLAG_SOFTWARE )
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
            size_t infoSize;
            wcstombs_s( &infoSize, info.name, adapterDesc.Description, maxSize );

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

        NativeAPI::SImplementation::SDeviceFeatures::sTearingSupported = ( allowTearing == TRUE );

        return res;
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

        {
            D3D12_FEATURE_DATA_D3D12_OPTIONS options = GetFeatureOption< D3D12_FEATURE_DATA_D3D12_OPTIONS >(
                m_hDevice, D3D12_FEATURE::D3D12_FEATURE_D3D12_OPTIONS );
            m_Implementation.Features.BindlessResourceAccessSupported =
                options.ResourceBindingTier == D3D12_RESOURCE_BINDING_TIER_3;
            m_Implementation.Features.ResourceHeapTier = static_cast< uint8_t >( options.ResourceBindingTier );
        }

        {
            D3D12_FEATURE_DATA_D3D12_OPTIONS5 options =
                GetFeatureOption< D3D12_FEATURE_DATA_D3D12_OPTIONS5 >( m_hDevice, D3D12_FEATURE_D3D12_OPTIONS5 );
            m_Implementation.Features.RayTracingSupported =
                options.RaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED;
        }

        {
            D3D12_FEATURE_DATA_D3D12_OPTIONS7 options =
                GetFeatureOption< D3D12_FEATURE_DATA_D3D12_OPTIONS7 >( m_hDevice, D3D12_FEATURE_D3D12_OPTIONS7 );
            m_Implementation.Features.MeshShaderSupported =
                options.MeshShaderTier != D3D12_MESH_SHADER_TIER_NOT_SUPPORTED;
        }

        {
            D3D12_FEATURE_DATA_D3D12_OPTIONS12 options = GetFeatureOption< D3D12_FEATURE_DATA_D3D12_OPTIONS12 >(
                m_hDevice, D3D12_FEATURE::D3D12_FEATURE_D3D12_OPTIONS12 );

            m_Implementation.Features.EnhancedBarriersSupported = options.EnhancedBarriersSupported;
        }

        {
            D3D12_FEATURE_DATA_D3D12_OPTIONS16 options = GetFeatureOption< D3D12_FEATURE_DATA_D3D12_OPTIONS16 >(
                m_hDevice, D3D12_FEATURE::D3D12_FEATURE_D3D12_OPTIONS16 );

            m_Implementation.Features.UploadHeapSupported = options.GPUUploadHeapSupported;
        }

        for( UINT i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; i++ )
        {
            m_Implementation.Properties.Memory.DescriptorHeapSizes[ i ] =
                m_hDevice->GetDescriptorHandleIncrementSize( static_cast< D3D12_DESCRIPTOR_HEAP_TYPE >( i ) );
        }

        if( Info.Settings.Features.bindlessResourceAccess == FeatureEnableModes::ENABLE &&
            !m_Implementation.Features.BindlessResourceAccessSupported )
        {
            VKE_LOG_WARN( "CDDI::CreateDevice: Bindless Resource Access not fully supported on this device" );
        }

        if( Info.Settings.Features.meshShaders == FeatureEnableModes::ENABLE &&
            !m_Implementation.Features.MeshShaderSupported )
        {
            VKE_LOG_ERR( "CDDI::CreateDevice: Mesh Shaders not supported on this device" );
            return VKE_FAIL;
        }

        if( Info.Settings.Features.raytracing == FeatureEnableModes::ENABLE &&
            !m_Implementation.Features.RayTracingSupported )
        {
            VKE_LOG_ERR( "CDDI::CreateDevice: Raytracing not supported on this device" );
            return VKE_FAIL;
        }

        if( m_Implementation.Features.ResourceHeapTier < 2 )
        {
            VKE_LOG_WARN( "CDDI::CreateDevice: Hardware does not support Tier2 resource heaps." );
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
        VKE_ASSERT2( m_hDevice != NativeAPI::Null, "CDDI::CreateTexture: m_hDevice can't be null" );

        D3D12_RESOURCE_DESC texDesc;
        texDesc.Dimension = Map::getResourceDimension( Desc.type );
        texDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
        texDesc.Width     = static_cast< UINT64 >( Desc.Size.width );
        texDesc.Height    = static_cast< UINT >( Desc.Size.height );

        if( Desc.type == TEXTURE_TYPE::TEXTURE_3D )
        {
            texDesc.DepthOrArraySize = static_cast< UINT16 >( Desc.sliceCount );
        }
        else
        {
            texDesc.DepthOrArraySize = static_cast< UINT16 >( Desc.arrayElementCount );
        }

        texDesc.MipLevels  = static_cast< UINT16 >( Desc.mipmapCount );
        texDesc.Format     = Convert::getDXGIFormat( Desc.format );
        texDesc.SampleDesc = Convert::getSampleDesc( Desc.multisampling );
        texDesc.Layout     = D3D12_TEXTURE_LAYOUT_64KB_UNDEFINED_SWIZZLE; // Reserved resources must use this.
        texDesc.Flags      = Convert::getResourceFlags( Desc.usage );

        NativeAPI::Texture pTexture = NativeAPI::Null;

        // if( FAILED( m_hDevice->CreateReservedResource(
        //         &texDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS( &pTexture->pObject ) ) ) )
        //{
        //     VKE_LOG_ERR( "CDDI::CreateTexture: Create texture failure." );
        // }
        // else
        //{
        //     SetObjectDebugName( (const uint64_t)pTexture, ApiObjectTypes::TEXTURE, Desc.Name.GetData() );
        // }

        if( VKE_FAILED( Memory::CreateObject( &HeapAllocator, &pTexture ) ) )
        {
            VKE_LOG_ERR( "CDDI::CreateTexture: Failed to create texture object" );
        }
        else
        {
            // SetObjectDebugName( (const uint64_t)texture, ApiObjectTypes::TEXTURE, Desc.Name );
            // Desc.memoryUsage;
        }

        return pTexture;
    }

    void CDDI::DestroyTexture( NativeAPI::Texture* pInOut, const void* pAllocator )
    {
        UNIMPLEMENTED_D3D12_METHOD();
    }

    NativeAPI::TextureView CDDI::CreateTextureView( const STextureViewDesc& Desc, const void* pAllocator )
    {
        VKE_ASSERT2( m_hDevice != NativeAPI::Null, "m_hDevice can't be null" );

        // Initialize empty view on memory heap to defer creation.
        NativeAPI::TextureView srv = NativeAPI::Null;
        m_Implementation.CreateView( &srv );
        srv->CpuHandle.ptr = 0;

        // Fill descriptor params
        auto& srvDesc = srv->Desc;

        srvDesc.Format                  = Convert::getDXGIFormat( Desc.format );
        srvDesc.ViewDimension           = Map::getSRVDimension( Desc.type );
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

#define SET_COMMON_MIP_FIELDS( view )                                                                                  \
    view.MostDetailedMip     = Desc.SubresourceRange.beginMipmapLevel;                                                 \
    view.MipLevels           = Desc.SubresourceRange.mipmapLevelCount;                                                 \
    view.ResourceMinLODClamp = 0.0f;

        switch( srvDesc.Format )
        {
            case D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE1D:
                SET_COMMON_MIP_FIELDS( srvDesc.Texture1D );
                break;

            case D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2D:
                SET_COMMON_MIP_FIELDS( srvDesc.Texture2D );
                srvDesc.Texture2D.PlaneSlice = 0;
                break;

            case D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE3D:
                SET_COMMON_MIP_FIELDS( srvDesc.Texture3D );
                break;

            case D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURECUBE:
                SET_COMMON_MIP_FIELDS( srvDesc.TextureCube );
                break;

            case D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE1DARRAY:
                SET_COMMON_MIP_FIELDS( srvDesc.Texture1DArray );
                srvDesc.Texture1DArray.FirstArraySlice = Desc.SubresourceRange.beginArrayLayer;
                srvDesc.Texture1DArray.ArraySize       = Desc.SubresourceRange.layerCount;
                break;

            case D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2DARRAY:
                SET_COMMON_MIP_FIELDS( srvDesc.Texture2DArray );
                srvDesc.Texture2DArray.FirstArraySlice = Desc.SubresourceRange.beginArrayLayer;
                srvDesc.Texture2DArray.ArraySize       = Desc.SubresourceRange.layerCount;
                srvDesc.Texture2DArray.PlaneSlice      = 0;
                break;

            case D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURECUBEARRAY:
                SET_COMMON_MIP_FIELDS( srvDesc.TextureCubeArray );
                srvDesc.TextureCubeArray.First2DArrayFace = Desc.SubresourceRange.beginArrayLayer;
                srvDesc.TextureCubeArray.NumCubes         = Desc.SubresourceRange.layerCount;
                break;
        }

        return srv;
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
                VKE_LOG_WARN( "CDDI::CreateDescriptorPool: DESCRIPTOR_SET_TYPE::SAMPLER_AND_TEXTURE is not "
                              "supported in D3D12" );
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
            // We can prevent shader visibility set to all by create each range for a specific visibility. For
            // example range with visibility PIXEL, VERTEX. But it still won't match Vulkan as theres no possibility
            // for combining like PIXEL | VERTEX. D3D12_SHADER_VISIBILITY visibility = Convert::getShaderVisibility(
            // binding.stages );
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
        D3D12_RESOURCE_DESC            desc      = hTexture->Desc;
        D3D12_RESOURCE_ALLOCATION_INFO allocInfo = m_hDevice->GetResourceAllocationInfo( 0, 1, &desc );

        pOut->alignment = static_cast< uint32_t >( allocInfo.Alignment );
        pOut->size      = static_cast< uint32_t >( allocInfo.SizeInBytes );

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
        VKE_ASSERT2( m_hDevice != NativeAPI::Null, "CDDI::Allocate: m_hDevice can't be null" );

        D3D12_HEAP_DESC heapDesc =
            Convert::GetMemoryHeapDesc( Desc.usage, m_Implementation.Features.ResourceHeapTier >= 2 );
        heapDesc.SizeInBytes = Desc.size;

        // TODO(any): For multi-adapter systems, need to set proper masks.
        heapDesc.Properties.CreationNodeMask = getNodeMask();
        heapDesc.Properties.VisibleNodeMask  = getNodeMask();

        Result  res = Result::OK;
        HRESULT hr  = S_OK;
        if( FAILED( hr = m_hDevice->CreateHeap( &heapDesc, IID_PPV_ARGS( &pOut->hDDIMemory ) ) ) )
        {
            if( hr == E_OUTOFMEMORY )
            {
                res = Result::NO_MEMORY;
            }
            else
            {
                res = Result::FAIL;
            }
        }
        else
        {
            pOut->heapType = GetMemoryHeapType( Desc.usage );
            pOut->sizeLeft = Desc.size;
        }

        return res;
    }

    MEMORY_HEAP_TYPE CDDI::GetMemoryHeapType( MEMORY_USAGE usage ) const
    {
        /*
        MEMORY_HEAP_TYPE:
        - CPU,
        - GPU,
        - UPLOAD,
        - CPU_CACHED,
        - CPU_COHERENT,
        - OTHER,
        */

        static const MEMORY_USAGE cMask =
            MemoryUsages::CPU_ACCESS | MemoryUsages::GPU_ACCESS | MemoryUsages::CPU_NO_FLUSH | MemoryUsages::CPU_CACHED;

        static MEMORY_USAGE vMap[] = {
            MemoryUsages::CPU_ACCESS,                                                         // CPU
            MemoryUsages::GPU_ACCESS,                                                         // GPU
            MemoryUsages::CPU_ACCESS | MemoryUsages::GPU_ACCESS | MemoryUsages::CPU_NO_FLUSH, // UPLOAD
            MemoryUsages::CPU_ACCESS | MemoryUsages::GPU_ACCESS | MemoryUsages::CPU_CACHED,   // CPU_CACHED
            MemoryUsages::CPU_ACCESS | MemoryUsages::GPU_ACCESS,                              // CPU_COHERENT
            MemoryUsages::UNDEFINED,                                                          // OTHER
        };

        MEMORY_HEAP_TYPE result = MEMORY_HEAP_TYPE::_MAX_COUNT;

        for( uint32_t i = 0; i < MemoryHeapTypes::_MAX_COUNT; i++ )
        {
            if( vMap[ i ] == ( usage & cMask ) )
            {
                result = static_cast< MEMORY_HEAP_TYPE >( i );
                break;
            }
        }

        return result;
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
        // No-op
    }

    void CDDI::BeginCommandBuffer( const NativeAPI::CommandBuffer&     hCommandBuffer,
                                   const NativeAPI::CommandBufferPool& hCommandBufferPool )
    {
        // CommandAllocator and CommandList should be in reset state before calling this method, which is done by
        // Reset() method. D3D12 doesn't have any additional flags for beginning command buffer recording, just
        // reset()

        if( FAILED( hCommandBufferPool.Obj->Reset() ) )
        {
            VKE_LOG_ERR( "CDDI::Reset: Failed to reset command buffer pool" );
        }

        if( FAILED( hCommandBuffer->Reset( hCommandBufferPool.Obj, NativeAPI::Null ) ) )
        {
            VKE_LOG_ERR( "CDDI::Reset: Failed to reset command buffer" );
        }
    }

    void CDDI::EndCommandBuffer( const NativeAPI::CommandBuffer& hCommandBuffer )
    {
        if( FAILED( hCommandBuffer->Close() ) )
        {
            VKE_LOG_ERR( "CDDI::EndCommandBuffer: Failed to close command buffer" );
        }
    }

    typedef Utils::TCDynamicArray< D3D12_RESOURCE_BARRIER > DDIBarrierArray;

    void CreateLegacySubresourceBarriers( const STextureBarrierInfo& Info, DDIBarrierArray& OutArray )
    {
        if( Info.srcMemoryAccess == Info.dstMemoryAccess )
        {
            // TODO(szymansk): This assert should never be hit, engine must prevent transitioning same state.
            VKE_LOG_WARN( "CDDI::Barrier: Source and destination memory access masks are the same, DX12 doesn't "
                          "allow that." );
            return;
        }

        // Used when Texture was a custom struct.
        // const D3D12_RESOURCE_DESC& desc = Info.hDDITexture->Desc;
        D3D12_RESOURCE_DESC desc = Info.hDDITexture->Desc;

        UINT textureMipLevels = ( desc.MipLevels > 0 ) ? desc.MipLevels : 1;
        UINT textureArraySize = ( desc.DepthOrArraySize > 0 ) ? desc.DepthOrArraySize : 1;

        if( desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D )
        {
            textureArraySize = 1;
        }

        auto& range          = Info.SubresourceRange;
        bool  isFullResource = ( textureMipLevels == range.mipmapLevelCount ) &&
                              ( textureArraySize == range.layerCount ) && ( range.beginArrayLayer == 0 ) &&
                              ( range.beginMipmapLevel == 0 );

        if( isFullResource )
        {
            D3D12_RESOURCE_BARRIER barrier;
            barrier.Type  = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;

            auto& transition       = barrier.Transition;
            transition.pResource   = Info.hDDITexture->pObject;
            transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            transition.StateBefore = Convert::AccessMaskToResourceState( Info.srcMemoryAccess );
            transition.StateAfter  = Convert::AccessMaskToResourceState( Info.dstMemoryAccess );

            OutArray.PushBack( barrier );
        }
        else
        {
            for( uint32_t layer = 0; layer < Info.SubresourceRange.layerCount; layer++ )
            {
                for( uint32_t mip = 0; mip < Info.SubresourceRange.mipmapLevelCount; mip++ )
                {
                    D3D12_RESOURCE_BARRIER barrier;
                    barrier.Type  = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;

                    auto& transition     = barrier.Transition;
                    transition.pResource = Info.hDDITexture->pObject;
                    transition.Subresource =
                        Convert::getSubresourceIndex( Info.SubresourceRange.beginMipmapLevel + mip,
                                                      textureMipLevels,
                                                      Info.SubresourceRange.beginArrayLayer + layer,
                                                      textureArraySize,
                                                      1 );

                    transition.StateBefore = Convert::AccessMaskToResourceState( Info.srcMemoryAccess );
                    transition.StateAfter  = Convert::AccessMaskToResourceState( Info.dstMemoryAccess );

                    OutArray.PushBack( barrier );
                }
            }
        }
    }

    void CreateLegacySubresourceBarriers( const SBufferBarrierInfo& Info, DDIBarrierArray& OutArray )
    {
        VKE_ASSERT( !"CDDI::CreateLegacySubresourceBarriers: Buffer barriers are not supported in D3D12" );
    }

    void CDDI::Barrier( const NativeAPI::CommandBuffer& hCommandBuffer, const SBarrierInfo& Info )
    {
        DDIBarrierArray vBarriers( 0 );

        // Global memory barriers are not supported in D3D12
        // for( auto& barrier: Info.vMemoryBarriers )
        //{
        //    Convert::TransitionBarrier( vBarriers[ barrierIndex++ ], barrier );
        //}

        for( auto& barrier: Info.vTextureBarriers )
        {
            CreateLegacySubresourceBarriers( barrier, vBarriers );
        }

        for( auto& barrier: Info.vBufferBarriers )
        {
            CreateLegacySubresourceBarriers( barrier, vBarriers );
        }

        if( vBarriers.GetCount() > 0 )
        {
            hCommandBuffer->ResourceBarrier( vBarriers.GetCount(), vBarriers.GetData() );
        }
        else
        {
            VKE_LOG_WARN( "CDDI::Barrier: Requested barrier resulted in 0 actual barriers." );
        }
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
        ID3D12CommandList* const* ppCommandLists = (ID3D12CommandList* const*)&Info.pDDICommandBuffers[ 0 ];
        Info.hDDIQueue->ExecuteCommandLists( Info.commandBufferCount, ppCommandLists );
        return Result::OK;
    }

    Result CDDI::Present( const SPresentData& Info )
    {
        UNIMPLEMENTED_D3D12_METHOD();
        return Result::OK;
    }

    Result CDDI::CreateSwapChain( const SSwapChainDesc& Desc, const void*, SDDISwapChain* pOut )
    {
        VKE_ASSERT2( m_hDevice != NativeAPI::Null, "CDDI::CreateSwapChain: m_hDevice can't be null" );

        QueryPresentSurfaceCaps( nullptr, &pOut->Caps );

        UINT backBufferCount = Desc.backBufferCount;
        if( Constants::_SOptimal::IsOptimal( Desc.backBufferCount ) )
        {
            // Default to triple buffering
            backBufferCount = 2u;
        }
        else if( Desc.backBufferCount > DXGI_MAX_SWAP_CHAIN_BUFFERS )
        {
            VKE_LOG_ERR( "CDDI::CreateSwapChain: Unspecified number of backBufferCount or exceeds max supported." );
            return Result::NOT_SUPPORTED;
        }

        auto dxgiFormat = Convert::getDXGIFormat( Desc.format );
        if( dxgiFormat == DXGI_FORMAT_UNKNOWN )
        {
            VKE_LOG_ERR( "CDDI::CreateSwapChain: Unsupported swapchain format (no matching engine with DXGI format)." );
            return Result::NOT_SUPPORTED;
        }

        D3D12_FEATURE_DATA_FORMAT_SUPPORT formatSupport = { .Format = dxgiFormat };
        m_hDevice->CheckFeatureSupport( D3D12_FEATURE_FORMAT_SUPPORT, &formatSupport, sizeof( formatSupport ) );

        if( ( formatSupport.Support1 & D3D12_FORMAT_SUPPORT1_RENDER_TARGET ) == 0 )
        {
            VKE_LOG_ERR( "CDDI::CreateSwapChain: Format can't be used as render target (required)." );
            return Result::NOT_SUPPORTED;
        }

        if( ( formatSupport.Support1 & D3D12_FORMAT_SUPPORT1_SHADER_SAMPLE ) == 0 )
        {
            VKE_LOG_ERR( "CDDI::CreateSwapChain: Format can't be used as shader resource (required)." );
            return Result::NOT_SUPPORTED;
        }

        if( ( formatSupport.Support1 & D3D12_FORMAT_SUPPORT1_DISPLAY ) == 0 )
        {
            VKE_LOG_ERR( "CDDI::CreateSwapChain: Format can't be used to present() (required)." );
            return Result::NOT_SUPPORTED;
        }

        DXGI_SWAP_CHAIN_DESC1 swapChainDesc;
        swapChainDesc.Width  = Desc.Size.width;
        swapChainDesc.Height = Desc.Size.height;
        // TODO(blturkot): Validate for proper swapchain render target
        swapChainDesc.Format             = dxgiFormat;
        swapChainDesc.Stereo             = FALSE; // This is to enable stereoscopic rendering for 3D glasses.
        swapChainDesc.SampleDesc.Count   = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER | DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_SHADER_INPUT;
        swapChainDesc.BufferCount = backBufferCount;
        swapChainDesc.Scaling     = DXGI_SCALING_NONE;
        swapChainDesc.SwapEffect  = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
        swapChainDesc.AlphaMode   = DXGI_ALPHA_MODE_UNSPECIFIED;
        swapChainDesc.Flags       = 0;

        // Enable tearing if supported and allowed by application.
        // if( NativeAPI::SImplementation::sTearingSupported )
        //{
        //    swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
        //}

        // TODO(blturkot): Implement support for fullscreen.
        // TODO(blturkot): Support for vsync
        DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullscreenDesc;
        fullscreenDesc.RefreshRate.Numerator   = 0;
        fullscreenDesc.RefreshRate.Denominator = 0;
        fullscreenDesc.ScanlineOrdering        = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
        fullscreenDesc.Scaling                 = DXGI_MODE_SCALING_UNSPECIFIED;
        fullscreenDesc.Windowed                = TRUE;
        (void)fullscreenDesc;

        HWND  hWnd  = reinterpret_cast< HWND >( Desc.pWindow->GetDesc().hWnd );
        auto& queue = m_DeviceProperties.vQueueFamilies[ Desc.queueFamilyIndex ].vQueues[ 0 ];

        IDXGISwapChain1* pSwapChain1 = nullptr;
        if( FAILED( m_Implementation.spFactory->CreateSwapChainForHwnd(
                queue, hWnd, &swapChainDesc, NULL, NULL, &pSwapChain1 ) ) )
        {
            VKE_LOG_ERR( "CDDI::CreateSwapChain: Failed to create swap chain" );
            return Result::FAIL;
        }

        // Update out handle to higher SwapChain ptr. Similar to .As() on ComPtr
        pOut->hSwapChain = reinterpret_cast< NativeAPI::SwapChain >( pSwapChain1 );

        // DXGI has already bound the swapchain to the window.
        IDXGIOutput* pPresentOutput;
        pOut->hSwapChain->GetContainingOutput( &pPresentOutput );
        pOut->hSurface = reinterpret_cast< NativeAPI::PresentSurface >( pPresentOutput );

        DXGI_COLOR_SPACE_TYPE dxgiColorSpace        = Map::getDXGIColorSpace( Desc.colorSpace );
        UINT                  dxgiColorSpaceSupport = 0;

        pOut->hSwapChain->CheckColorSpaceSupport( dxgiColorSpace, &dxgiColorSpaceSupport );
        if( ( dxgiColorSpaceSupport & DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT ) == 0 )
        {
            VKE_LOG_WARN( "CDDI::CreateSwapChain: Color space not supported for swapchain." );
        }
        else
        {
            pOut->hSwapChain->SetColorSpace1( dxgiColorSpace );
        }

        for( UINT i = 0; i < backBufferCount; i++ )
        {
            NativeAPI::Texture bbTexture = NativeAPI::Null;
            // This is required when NativeAPI::Texture is a custom object.
            if( VKE_FAILED( Memory::CreateObject( &HeapAllocator, &bbTexture ) ) )
            {
                VKE_LOG_ERR( "CDDI::CreateSwapChain: Failed to create back buffer texture object." );
            }

            pOut->hSwapChain->GetBuffer( i, IID_PPV_ARGS( &bbTexture->pObject ) );

            // Create swapchain already creates required resources but doesn't have views.
            // To cheat engine, we can store the texture in vImages and create null views.
            pOut->vImages.PushBack( bbTexture );
            pOut->vImageViews.PushBack( nullptr );
        }

        // Update size from SwapChain after creation
        UINT width = 0, height = 0;
        pOut->hSwapChain->GetSourceSize( &width, &height );
        pOut->Size.width  = (VKE::RenderSystem::TextureSizeType)width;
        pOut->Size.height = (VKE::RenderSystem::TextureSizeType)height;

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
        // No enum or list for DXGI, need to loop and query supported formats.
        struct DXGI_ENGINE_FORMAT_PAIR
        {
            DXGI_FORMAT dxgiFormat;
            FORMAT      engineFormat;
        };

        static const DXGI_ENGINE_FORMAT_PAIR vApiAllowedFormats[] = {
            // Feature l1evel >= 9.1
            { DXGI_FORMAT_R8G8B8A8_UNORM, FORMAT::R8G8B8A8_UNORM },
            { DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, FORMAT::R8G8B8A8_SRGB },
            { DXGI_FORMAT_B8G8R8A8_UNORM, FORMAT::B8G8R8A8_UNORM },     // Except 10.x on Windows Vista
            { DXGI_FORMAT_B8G8R8A8_UNORM_SRGB, FORMAT::B8G8R8A8_SRGB }, // Except 10.x on Windows Vista
            // Feature level >= 10.0
            { DXGI_FORMAT_R16G16B16A16_FLOAT, FORMAT::R16G16B16A16_SFLOAT },
            { DXGI_FORMAT_R10G10B10A2_UNORM, FORMAT::R10G10B10A2_UNORM },
            // Feature level >= 11.0
            { DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM, FORMAT::R10G10B10_XR_BIAS_A2_UNORM },
        };

        D3D12_FEATURE_DATA_FORMAT_SUPPORT formatSupport = {};

        for( auto& format: vApiAllowedFormats )
        {
            formatSupport.Format = format.dxgiFormat;
            m_hDevice->CheckFeatureSupport( D3D12_FEATURE_FORMAT_SUPPORT, &formatSupport, sizeof( formatSupport ) );
            if( ( formatSupport.Support1 & D3D12_FORMAT_SUPPORT1_DISPLAY ) != 0 )
            {
                pOut->vFormats.PushBack( { format.engineFormat, COLOR_SPACE::SRGB } );
            }
        }

        // Present modes - only FIFO is guaranteed to be supported.
        pOut->vModes.PushBack( PRESENT_MODE::FIFO );

        // Mailbox is similar to FIFO but allows replacing the queued frames if the queue is full.
        // This is not supported in DXGI natively and requires emulation.
        // TODO(Any): Uncomment once emulation is implemented.
        // pOut->vModes.PushBack( PRESENT_MODE::MAILBOX );

        if( NativeAPI::SImplementation::SDeviceFeatures::sTearingSupported )
        {
            pOut->vModes.PushBack( PRESENT_MODE::IMMEDIATE );
        }

        pOut->canBeUsedAsRenderTarget = true;
        pOut->minImageCount           = 1;
        pOut->maxImageCount           = DXGI_MAX_SWAP_CHAIN_BUFFERS;

        // If size is 0, 0 CreateSwapChain will use the window's client area size.
        pOut->MinSize = { 1, 1 };
        pOut->MaxSize = { D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION, D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION };

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
        VKE_ASSERT2( handle != 0, "CDDI::SetObjectDebugName: Attempting to SetName on Null object." );

        wchar_t buffer[ 256 ];
        MultiByteToWideChar( CP_UTF8, 0, pName, -1, buffer, 256 );

        switch( objType )
        {
            case ApiObjectTypes::ADAPTER:
                ( (NativeAPI::Adapter)handle )->SetPrivateData( WKPDID_D3DDebugObjectName, sizeof( pName ), pName );
                break;
            case ApiObjectTypes::DEVICE:
                ( (NativeAPI::Device)handle )->SetName( buffer );
                break;
            case ApiObjectTypes::GPU_FENCE:
                ( (NativeAPI::GPUFence*)handle )->Obj->SetName( buffer );
                break;
            case ApiObjectTypes::COMMAND_BUFFER:
                ( (NativeAPI::CommandBuffer)handle )->SetName( buffer );
                break;
            case ApiObjectTypes::CPU_FENCE:
                ( (NativeAPI::CPUFence*)handle )->Obj->SetName( buffer );
                break;
            case ApiObjectTypes::BUFFER:
                ( (NativeAPI::Buffer)handle )->SetName( buffer );
                break;
            case ApiObjectTypes::TEXTURE:
                ( (NativeAPI::Texture)handle )->SetName( buffer );
                break;
            case ApiObjectTypes::BUFFER_VIEW:
                ( (NativeAPI::BufferView)handle )->SetName( buffer );
                break;
            case ApiObjectTypes::TEXTURE_VIEW:
                ( (NativeAPI::BufferView)handle )->SetName( buffer );
                break;
            case ApiObjectTypes::PIPELINE_LAYOUT:
                ( (NativeAPI::PipelineLayout)handle )->SetName( buffer );
                break;
            case ApiObjectTypes::PIPELINE:
                ( (NativeAPI::Pipeline)handle )->SetName( buffer );
                break;
            case ApiObjectTypes::DESCRIPTOR_SET:
                ( (NativeAPI::DescriptorSet)handle )->SetName( buffer );
                break;
            case ApiObjectTypes::FRAMEBUFFER:
                ( (NativeAPI::Framebuffer)handle )->SetName( buffer );
                break;
            case ApiObjectTypes::COMMAND_POOL:
                ( (NativeAPI::CommandBufferPool*)handle )->Obj->SetName( buffer );
                break;

            case ApiObjectTypes::CONTEXT:
            case ApiObjectTypes::DEVICE_MEMORY:
            case ApiObjectTypes::QUERY_POOL:
            case ApiObjectTypes::EVENT:
            case ApiObjectTypes::SHADER:
            case ApiObjectTypes::PIPELINE_CACHE:
            case ApiObjectTypes::RENDER_PASS:
            case ApiObjectTypes::DESCRIPTOR_SET_LAYOUT:
            case ApiObjectTypes::SAMPLER:
            case ApiObjectTypes::DESCRIPTOR_POOL:
                VKE_LOG_WARN( "CDDI::SetObjectDebugName: Unsupported objType" );
                break;

            default:
                VKE_LOG_ERR( "CDDI::SetObjectDebugName: Unhandled objType" );
                break;
        }
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
        phFence->Value = 0;
        if( FAILED( phFence->Obj->Signal( 0 ) ) )
        {
            VKE_LOG_ERR( "CDDI::Reset: Failed to reset fence" );
        }
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

#endif // VKE_RENDER_SYSTEM_D3D12
