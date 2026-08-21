#pragma once

#if VKE_WINDOWS

#include "Core/Memory/CFreeListPool.h"
#include "Core/Utils/TCBitPool.h"

#include <directx/d3d12.h>

// NOTE: We intentionally do NOT include the umbrella <directx/d3dx12.h> here.
// That umbrella unconditionally pulls in <directx/d3dx12_property_format_table.h>,
// which references internal D3D enums (D3D_FORMAT_LAYOUT, D3D_FORMAT_TYPE_LEVEL, ...)
// that the vcpkg DirectX-Headers package does not define under MinGW/GCC.
// We also avoid <directx/d3dx12_resource_helpers.h> for the same reason: it
// transitively includes that broken table header. Only the CD3DX12 helpers we
// actually use are included below (the project only uses CD3DX12FeatureSupport).
#include <directx/d3dx12_core.h>
#include <directx/d3dx12_barriers.h>
#include <directx/d3dx12_pipeline_state_stream.h>
#include <directx/d3dx12_root_signature.h>

// The vcpkg DirectX-Headers advertise a recent D3D12_SDK_VERSION, so
// d3dx12_check_feature_support.h references newer feature-level enumerators
// (e.g. D3D_FEATURE_LEVEL_1_0_GENERIC) that MinGW's d3d12.h does not define.
// Provide a guarded fallback matching the official Windows SDK value.
#ifndef D3D_FEATURE_LEVEL_1_0_GENERIC
#define D3D_FEATURE_LEVEL_1_0_GENERIC ( (D3D_FEATURE_LEVEL)0x100 )
#endif

// MinGW's d3dcommon.h may not define newer primitive-topology enumerators.
// Provide guarded fallbacks matching the official Windows SDK values.
#ifndef D3D_PRIMITIVE_TOPOLOGY_TRIANGLEFAN
#define D3D_PRIMITIVE_TOPOLOGY_TRIANGLEFAN ( (D3D_PRIMITIVE_TOPOLOGY)18 )
#endif

#include <directx/d3dx12_check_feature_support.h>
#include <dxgi1_6.h>
#include <pix3.h>

// ---------------------------------------------------------------------------
// MinGW __uuidof support for the MSVC-style DirectX-Headers.
//
// GCC/MinGW does not support MSVC's __declspec(uuid())/__uuidof. Instead,
// mingw-w64 implements __uuidof(T) via the __mingw_uuidof<T>() template, which
// requires every interface to be registered with __CRT_UUID_DECL(T, ...).
// The vcpkg DirectX-Headers are MIDL-generated for MSVC and declare interfaces
// with MIDL_INTERFACE("guid") but never emit __CRT_UUID_DECL, so IID_PPV_ARGS
// and __uuidof fail to link with "undefined reference to __mingw_uuidof<T>()".
//
// Provide the declarations here for every D3D12/DXGI interface the engine uses
// with IID_PPV_ARGS/__uuidof. GUID values mirror the DEFINE_GUID(IID_*) entries
// in the corresponding headers. MSVC is unaffected.
// ---------------------------------------------------------------------------
#if defined( __MINGW32__ ) && defined( __CRT_UUID_DECL )
__CRT_UUID_DECL( ID3D12Device, 0x189819f1, 0x1db6, 0x4b57, 0xbe, 0x54, 0x18, 0x21, 0x33, 0x9b, 0x85, 0xf7 )
__CRT_UUID_DECL( ID3D12Device10, 0x517f8718, 0xaa66, 0x49f9, 0xb0, 0x2b, 0xa7, 0xab, 0x89, 0xc0, 0x60, 0x31 )
__CRT_UUID_DECL( ID3D12DescriptorHeap, 0x8efb471d, 0x616c, 0x4f49, 0x90, 0xf7, 0x12, 0x7b, 0xb7, 0x63, 0xfa, 0x51 )
__CRT_UUID_DECL( ID3D12DeviceRemovedExtendedData1, 0x9727A022, 0xCF1D, 0x4DDA, 0x9E, 0xBA, 0xEF, 0xFA, 0x65, 0x3F, 0xC5, 0x06 )
__CRT_UUID_DECL( ID3D12DeviceRemovedExtendedDataSettings1, 0xDBD5AE51, 0x3317, 0x4F0A, 0xAD, 0xF9, 0x1D, 0x7C, 0xED, 0xCA, 0xAE, 0x0B )
__CRT_UUID_DECL( ID3D12Debug1, 0xaffaa4ca, 0x63fe, 0x4d8e, 0xb8, 0xad, 0x15, 0x90, 0x00, 0xaf, 0x43, 0x04 )
__CRT_UUID_DECL( ID3D12CommandQueue, 0x0ec870a6, 0x5d7e, 0x4c22, 0x8c, 0xfc, 0x5b, 0xaa, 0xe0, 0x76, 0x16, 0xed )
__CRT_UUID_DECL( ID3D12Resource, 0x696442be, 0xa72e, 0x4059, 0xbc, 0x79, 0x5b, 0x5c, 0x98, 0x04, 0x0f, 0xad )
__CRT_UUID_DECL( ID3D12Fence, 0x0a753dcf, 0xc4d8, 0x4b91, 0xad, 0xf6, 0xbe, 0x5a, 0x60, 0xd9, 0x5a, 0x76 )
__CRT_UUID_DECL( ID3D12Fence1, 0x433685fe, 0xe22b, 0x4ca0, 0xa8, 0xdb, 0xb5, 0xb4, 0xf4, 0xdd, 0x0e, 0x4a )
__CRT_UUID_DECL( ID3D12Heap, 0x6b3b2502, 0x6e51, 0x45b3, 0x90, 0xee, 0x98, 0x84, 0x26, 0x5e, 0x8d, 0xf3 )
__CRT_UUID_DECL( ID3D12PipelineState, 0x765a30f3, 0xf624, 0x4c6f, 0xa8, 0x28, 0xac, 0xe9, 0x48, 0x62, 0x24, 0x45 )
__CRT_UUID_DECL( ID3D12RootSignature, 0xc54a6b66, 0x72df, 0x4ee8, 0x8b, 0xe5, 0xa9, 0x46, 0xa1, 0x42, 0x92, 0x14 )
__CRT_UUID_DECL( ID3D12CommandAllocator, 0x6102dee4, 0xaf59, 0x4b09, 0xb9, 0x99, 0xb4, 0x4d, 0x73, 0xf0, 0x9b, 0x24 )
__CRT_UUID_DECL( ID3D12GraphicsCommandList6, 0xc3827890, 0xe548, 0x4cfa, 0x96, 0xcf, 0x56, 0x89, 0xa9, 0x37, 0x0f, 0x80 )
__CRT_UUID_DECL( ID3D12InfoQueue, 0x0742a90b, 0xc387, 0x483f, 0xb9, 0x46, 0x30, 0xa7, 0xe4, 0xe6, 0x14, 0x58 )
//__CRT_UUID_DECL( IDXGIFactory7, 0xa4966eed, 0x76db, 0x44da, 0x84, 0xc1, 0xee, 0x9a, 0x7a, 0xfb, 0x20, 0xa8 )
//__CRT_UUID_DECL( IDXGIAdapter1, 0x29038f61, 0x3839, 0x4626, 0x91, 0xfd, 0x08, 0x68, 0x79, 0x01, 0x1a, 0x05 )
//__CRT_UUID_DECL( IDXGIAdapter4, 0x3c8d99d1, 0x4fbf, 0x4181, 0xa8, 0x2c, 0xaf, 0x66, 0xbf, 0x7b, 0xd2, 0x4e )
//__CRT_UUID_DECL( IDXGIDebug1, 0xc5a05f0c, 0x16f2, 0x4adf, 0x9f, 0x4d, 0xa8, 0xc4, 0xd5, 0x8a, 0xc5, 0x50 )
//__CRT_UUID_DECL( IDXGIDevice, 0x54ec77fa, 0x1377, 0x44e6, 0x8c, 0x32, 0x88, 0xfd, 0x5f, 0x44, 0xc8, 0x4c )
#endif // __MINGW32__ && __CRT_UUID_DECL

namespace VKE::RenderSystem::D3D12
{
    template< class ObjT >
    concept Nullable = std::is_pointer_v< ObjT >;

    // ------------------------------------------------------------------------
    // Portable COM "by-value return" call.
    //
    // Several D3D12 methods return a struct by value in the MSVC/Windows-SDK ABI
    // (e.g. GetCPUDescriptorHandleForHeapStart, GetDesc, GetResourceAllocationInfo).
    // The IDL-generated d3d12.h selects the signature with:
    //     #if defined(_MSC_VER) || !defined(_WIN32)   // aggregate returned by value
    //     #else                                       // aggregate via hidden first out-param
    // i.e. the out-param form is used only for a non-MSVC compiler targeting Win32
    // (mingw-w64). We gate on the exact same condition so this tracks the header
    // ABI rather than guessing from the compiler identity.
    //
    //     VKE_D3D12_CALL_RET( OutVar, pObj, Method, args... );
    // expands to:
    //     by-value ABI : (OutVar) = pObj->Method( args... )
    //     out-param ABI: pObj->Method( &(OutVar), args... )
    // ------------------------------------------------------------------------
#if defined( _MSC_VER ) || !defined( _WIN32 )
#define VKE_D3D12_CALL_RET( OutVar, Obj, Method, ... ) ( ( OutVar ) = ( Obj )->Method( __VA_ARGS__ ) )
#else
#define VKE_D3D12_CALL_RET( OutVar, Obj, Method, ... ) ( Obj )->Method( &(OutVar)__VA_OPT__(, ) __VA_ARGS__ )
#endif

    struct NativeAPI
    {
        // DirectX 12 have multiple structures for the same thing but with different feature sets. To prevent huge pain
        // in the butt when refactoring code due to higher struct / pointer number, we'll have one place to refactor
        // whole CDDI.
        using D3D12Fence               = ID3D12Fence1;
        using D3D12CommandAllocator    = ID3D12CommandAllocator;
        using D3D12CommandList         = ID3D12CommandList;
        using D3D12GraphicsCommandList = ID3D12GraphicsCommandList6;
        using D3D12Shader              = D3D12_SHADER_BYTECODE;
        using D3D12RootParameter       = D3D12_ROOT_PARAMETER1;
        using D3D12DescriptorRange     = D3D12_DESCRIPTOR_RANGE1;
        using D3D12Resource            = ID3D12Resource;
        using D3D12PipelineState       = ID3D12PipelineState;
        using D3D12Device              = ID3D12Device10;
        using D3D12CommandQueue        = ID3D12CommandQueue;
        using D3D12Heap                = ID3D12Heap;
        using D3D12Output              = IDXGIOutput6;
        using D3D12SwapChain           = IDXGISwapChain4;
        using D3D12Adapter             = IDXGIAdapter4;
        using D3D12RootSignature       = ID3D12RootSignature;
        using D3D12Factory             = IDXGIFactory7;
        using D3D12DescriptorHeap      = ID3D12DescriptorHeap;
        using D3D12ResourceDesc        = D3D12_RESOURCE_DESC;

        static const uint32_t                   DEFAULT_QUEUE_FAMILY_PROPERTY_COUNT = 16;
        inline static const decltype( nullptr ) Null                                = nullptr;

        enum struct ResourceViewTypes : uint32_t
        {
            SRV = VKE_BIT( 1 ),
            UAV = VKE_BIT( 2 ),
            RTV = VKE_BIT( 3 ),
            DSV = VKE_BIT( 4 ),
        };

        struct CustomTypes
        {
            struct SFence
            {
                NativeAPI::D3D12Fence* pObject = nullptr;
                HANDLE                 hEvent  = nullptr;
                UINT64                 Value   = 0;

                void        Signal( UINT64 Value );
                VKE::Result Wait( UINT64 Value, DWORD timeout = INFINITE );
                void        Signal( NativeAPI::D3D12CommandQueue* pQueue, UINT64 Value );
                void        Wait( NativeAPI::D3D12CommandQueue* pQueue, UINT64 Value );

                UINT64 GetCompletedValue();
                UINT64 GetSignaledValue();

                vke_force_inline const char* GetFenceName()
                {
                    if( aFenceName[ 0 ] == '\0' )
                    {
                        snprintf( aFenceName, sizeof( aFenceName ), "Fence_%llx", (uint64_t)pObject );
                    }
                    return aFenceName;
                }

            private:
                char aFenceName[ 32 ] = {};
            };

            struct SCPUFence : public SFence
            {
            };

            struct SGPUFence : public SFence
            {
            };

            struct SCommandBufferPool
            {
                struct SCommandListWithAllocator
                {
                    NativeAPI::D3D12CommandAllocator*    pAllocator = nullptr;
                    NativeAPI::D3D12GraphicsCommandList* pCmdList   = nullptr;
                };

                Utils::TCDynamicArray< SCommandListWithAllocator, 32 > vNativeCommandListsWithAllocators;

                D3D12_COMMAND_LIST_TYPE NativeType = D3D12_COMMAND_LIST_TYPE_DIRECT;
                uint32_t                EngineType = 0;

            protected:
                wstr_t m_name;

            public:
                NativeAPI::D3D12CommandAllocator*
                GetAllocator( NativeAPI::D3D12GraphicsCommandList* pNativeCommandList )
                {
                    NativeAPI::D3D12CommandAllocator* out = nullptr;
                    for( auto& Pair: vNativeCommandListsWithAllocators )
                    {
                        if( Pair.pCmdList == pNativeCommandList )
                        {
                            out = Pair.pAllocator;
                            break;
                        }
                    }
                    return out;
                }

                void SetName( cwstr_t name )
                {
                    m_name = name;
                }
            };

            struct SDescriptorSetLayout
            {
                Utils::TCDynamicArray< D3D12_DESCRIPTOR_RANGE1, 32 > vDescriptorRanges;
                NativeAPI::D3D12RootParameter                        RootParameter;
                D3D12_DESCRIPTOR_HEAP_TYPE                           type;
                uint32_t                                             numSlots;
            };

            struct SDescriptorPool
            {
                using SlotPool = Utils::TCBitPool< uint8_t >;
                NativeAPI::D3D12DescriptorHeap* pHeap;
                SlotPool                        SlotMgr;
                uint32_t                        descriptorSize;
                Memory::CFreeListPool           DescriptorSetMemMgr;
                D3D12_DESCRIPTOR_HEAP_TYPE      type;
            };

            struct SDescriptorSet
            {
                uint64_t         descTableCPUStartAddr;
                uint64_t         descTableGPUStartAddr;
                SDescriptorPool* pPool;
                // ExtentU32        aUsedSlots[ D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES ] = { { UNDEFINED_U32 } };
                ExtentU32             PoolSlots = { UNDEFINED_U32 };
                SDescriptorSetLayout* pLayout;

                D3D12_CPU_DESCRIPTOR_HANDLE GetCpuDescriptorHandle( uint32_t slotIndexOffset ) const
                {
                    D3D12_CPU_DESCRIPTOR_HANDLE Start{};
                    VKE_D3D12_CALL_RET( Start, pPool->pHeap, GetCPUDescriptorHandleForHeapStart );
                    return { Start.ptr + ( PoolSlots.begin + slotIndexOffset ) * pPool->descriptorSize };
                }

                D3D12_GPU_DESCRIPTOR_HANDLE GetGpuDescriptorHandle( uint32_t slotIndexOffset ) const
                {
                    D3D12_GPU_DESCRIPTOR_HANDLE Start{};
                    VKE_D3D12_CALL_RET( Start, pPool->pHeap, GetGPUDescriptorHandleForHeapStart );
                    return { Start.ptr + ( PoolSlots.begin + slotIndexOffset ) * pPool->descriptorSize };
                }
            };

            struct SCommandQueue
            {
                static Utils::TCDynamicArray< SCommandQueue > s_QueuePool;

                NativeAPI::D3D12CommandQueue* pQueue = nullptr;
                SGPUFence                     GpuFence;
            };

            struct SResourceView
            {
                D3D12_SHADER_RESOURCE_VIEW_DESC  ShaderResourceViewDesc;
                D3D12_RENDER_TARGET_VIEW_DESC    RenderTargetViewDesc;
                D3D12_UNORDERED_ACCESS_VIEW_DESC UnorderedAccessViewDesc;
                D3D12_DEPTH_STENCIL_VIEW_DESC    DepthStencilViewDesc;

                D3D12Resource* pResource;

                void Enable( ResourceViewTypes DescType )
                {
                    ResourceViewTypesMask |= static_cast< uint32_t >( DescType );
                }

                bool IsEnabled( ResourceViewTypes DescType ) const
                {
                    return ( ResourceViewTypesMask & static_cast< uint32_t >( DescType ) ) != 0;
                }

            private:
                uint32_t ResourceViewTypesMask = 0;
            };

            template< typename T, UINT numElements >
            struct TSStaticArray
            {
                std::array< T, numElements > Data = {};

                union
                {
                    UINT count = 0;
                };

                T& Reserve()
                {
                    VKE_ASSERT2( count < numElements, "Out of bounds array access" );
                    return Data[ count++ ];
                }

                bool HasAny()
                {
                    return count > 0;
                }

                UINT GetCount()
                {
                    return count;
                }
            };

            struct SRenderPass
            {
                static const uint32_t MAX_RENDER_TARGETS = Config::RenderSystem::RenderTarget::MAX_COUNT_IN_RENDER_PASS;

                using SRenderPassBarriers = TSStaticArray< D3D12_RESOURCE_BARRIER, MAX_RENDER_TARGETS >;

                struct
                {
                    D3D12_CPU_DESCRIPTOR_HANDLE hCPUDescriptor;
                } DepthStencilView;

                struct SClearArgs
                {
                    enum ClearType
                    {
                        RENDER_TARGET,
                        DEPTH_STENCIL_VIEW,
                    } Type;

                    struct SClearRenderTargetViewArgs
                    {
                        D3D12_CPU_DESCRIPTOR_HANDLE hRenderTargetView;
                        float                       aColorRGBA[ 4 ];
                        D3D12_RECT                  Rect;
                    };

                    struct SClearDepthStencilViewArgs
                    {
                        D3D12_CPU_DESCRIPTOR_HANDLE hDepthStencilView;
                        D3D12_CLEAR_FLAGS           ClearFlags;
                        FLOAT                       depth;
                        UINT8                       stencil;
                        D3D12_RECT                  Rect;
                    };

                    union
                    {
                        SClearRenderTargetViewArgs RenderTargetView;
                        SClearDepthStencilViewArgs DepthStencilView;
                    };
                };

                struct SClear : TSStaticArray< SClearArgs, MAX_RENDER_TARGETS >
                {
                    SRenderPassBarriers Barriers;
                };

                struct SSubpass
                {
                    SRenderPassBarriers BeginBarriers;

                    struct SSubpassRenderTarget
                    {
                        D3D12_CPU_DESCRIPTOR_HANDLE hCPUDescriptor;
                        D3D12_RESOURCE_STATES       resourceState;
                    };

                    TSStaticArray< SSubpassRenderTarget, D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT > RenderTargetViews;
                };

                using SSubpassArray = Utils::TCDynamicArray< SSubpass, 1 >;

                TSStaticArray< D3D12_CPU_DESCRIPTOR_HANDLE, MAX_RENDER_TARGETS > RenderTargetViews;
                TSStaticArray< NativeAPI::D3D12Resource*, MAX_RENDER_TARGETS >   DiscardResources;
                SClear                                                           Clear;

                // For pipeline states created with render passes
                UINT        NumRenderTargetViews                          = 0;
                DXGI_FORMAT RenderTargetViewFormats[ MAX_RENDER_TARGETS ] = { DXGI_FORMAT_UNKNOWN };
                DXGI_FORMAT DepthStencilRenderTargetFormat;

                SSubpassArray vSubpasses          = {};
                uint32_t      currentSubpassIndex = 0;

                SRenderPassBarriers EndBarriers;

                char aName[ 256 ] = { 0 };

                const char* GetName() const
                {
                    return aName;
                }

                void SetName( const char* pInName )
                {
                    if( pInName != nullptr )
                    {
                        ::strncpy_s( aName, pInName, _TRUNCATE );
                    }
                    else
                    {
                        aName[ 0 ] = 0;
                    }
                }

                void Reset()
                {
                    currentSubpassIndex = 0;
                }

                SSubpass& CurrentSubpass()
                {
                    return vSubpasses[ currentSubpassIndex ];
                }

                SSubpass& NextSubpass()
                {
                    return vSubpasses[ ++currentSubpassIndex ];
                }
            };

            struct SPipelineStateObject
            {
                enum PipelineStateObjectType : uint16_t
                {
                    GRAPHICS,
                    COMPUTE,
                    MESH,
                    RAYTRACING,
                };

                union
                {
                    NativeAPI::D3D12PipelineState* Graphics;
                    NativeAPI::D3D12PipelineState* Compute;
                    NativeAPI::D3D12PipelineState* Mesh;
                    ID3D12StateObject*             Raytracing;
                };

                D3D_PRIMITIVE_TOPOLOGY  PrimitiveTopology;
                PipelineStateObjectType Type;
                uint16_t                VertexBufferStride;
            };
        }; // struct CustomTypes

        struct SClearValue : D3D12_CLEAR_VALUE
        {
            SClearValue()
            {
            }

            SClearValue( DXGI_FORMAT fmt, float r, float g, float b, float a ) :
                D3D12_CLEAR_VALUE{ fmt, { r, g, b, a } }
            {
            }

            SClearValue( DXGI_FORMAT fmt, float d, uint8_t s )
            {
                this->Format               = fmt;
                this->DepthStencil.Depth   = d;
                this->DepthStencil.Stencil = s;
            }

            SClearValue( float r, float g, float b, float a ) : SClearValue( DXGI_FORMAT_UNKNOWN, r, g, b, a )
            {
                /*this->Color[ 0 ] = r;
                this->Color[ 1 ] = g;
                this->Color[ 2 ] = b;
                this->Color[ 3 ] = a;*/
            }

            SClearValue( float d, uint8_t s ) : SClearValue( DXGI_FORMAT_UNKNOWN, d, s )
            {
            }

            SClearValue& operator=( const SClearValue& Other )
            {
                this->Format     = Other.Format;
                this->Color[ 0 ] = Other.Color[ 0 ];
                this->Color[ 1 ] = Other.Color[ 1 ];
                this->Color[ 2 ] = Other.Color[ 2 ];
                this->Color[ 3 ] = Other.Color[ 3 ];
                return *this;
            }
        };

        struct SDeviceLimits
        {
            // TODO(blturkot): Fill with limits
        };

        using Buffer                = D3D12Resource*;
        using Pipeline              = CustomTypes::SPipelineStateObject*;
        using Texture               = D3D12Resource*;
        using Sampler               = D3D12_SAMPLER_DESC*;
        using RenderPass            = CustomTypes::SRenderPass*;
        using CommandBuffer         = D3D12GraphicsCommandList*;
        using TextureView           = CustomTypes::SResourceView*;
        using BufferView            = CustomTypes::SResourceView*;
        using CPUFence              = CustomTypes::SCPUFence*;
        using GPUFence              = CustomTypes::SGPUFence*;
        using Fence                 = CustomTypes::SFence*;
        using FenceValue            = UINT64;
        using Device                = D3D12Device*;
        using DescriptorPool        = CustomTypes::SDescriptorPool*;
        using DescriptorSet         = CustomTypes::SDescriptorSet*;
        using DescriptorSetLayout   = CustomTypes::SDescriptorSetLayout*;
        using CommandBufferPool     = CustomTypes::SCommandBufferPool*;
        using Framebuffer           = D3D12Resource*;
        using ClearValue            = SClearValue;
        using Queue                 = D3D12CommandQueue*;
        using Format                = DXGI_FORMAT;
        using ImageType             = D3D12_RESOURCE_DIMENSION;
        using ImageLayout           = D3D12_RESOURCE_FLAGS;
        using ImageUsageFlags       = D3D12_RESOURCE_FLAGS;
        using MemoryHeap            = D3D12Heap*;
        using PresentSurface        = D3D12Output*;
        using SwapChain             = D3D12SwapChain*;
        using Adapter               = D3D12Adapter*;
        using Shader                = D3D12Shader*;
        using PipelineLayout        = D3D12RootSignature*;
        using DeviceSize            = UINT64;
        using Event                 = HANDLE;
        using QueueFamilyProperties = void*;
        using DeviceLimits          = SDeviceLimits;

        using Result = HRESULT;

        enum ImageViewType
        {
            D3D12_VIEW_TYPE_BUFFER,
            D3D12_VIEW_TYPE_TEX1D,
            D3D12_VIEW_TYPE_TEX1D_ARRAY,
            D3D12_VIEW_TYPE_TEX2D,
            D3D12_VIEW_TYPE_TEX2D_ARRAY,
            D3D12_VIEW_TYPE_TEX2DMS,
            D3D12_VIEW_TYPE_TEX2DMS_ARRAY,
            D3D12_VIEW_TYPE_TEX3D,
            D3D12_VIEW_TYPE_TEXCUBE,
            D3D12_VIEW_TYPE_TEXCUBE_ARRAY,
            D3D12_VIEW_TYPE_RT_ACC_STRUCT,
        };

    }; // struct RHI

    struct SImplementation
    {
        static const uint32_t MAX_MEMORY_HEAPS = 16;

        static NativeAPI::D3D12Factory* spFactory;
        static bool                     sDebugLayerEnabled;
        NativeAPI::Device               m_hDevice;
        NativeAPI::Adapter              m_hAdapter;

        struct SMemoryHeapProperties
        {
            D3D12_HEAP_TYPE   Type;
            D3D12_MEMORY_POOL Pool;
            size_t            SizeInBytes;
        };

        // TODO(blturkot): Fill with private data
        struct SDeviceProperties
        {
            struct
            {
                struct
                {
                    NativeAPI::DeviceLimits limits;
                } properties;

            } Device;

            struct
            {
                UINT                  DescriptorHeapSizes[ D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES ];
                SMemoryHeapProperties HeapProperties[ MAX_MEMORY_HEAPS ];
                UINT64                localBudget;
                UINT64                hostBudget;
            } Memory;

            void* aFormatProperties[ Formats::_MAX_COUNT ];
        } Properties; // struct SDeviceProperties

        struct SDeviceFeatures
        {
            static bool sTearingSupported;

            uint8_t ResourceHeapTier;

            bool BindlessResourceAccessSupported;
            bool EnhancedBarriersSupported;
            bool UploadHeapSupported;
            bool MeshShaderSupported;
            bool RayTracingSupported;
            bool TightAlignmentSupported;
        } Features; // struct SDeviceFeatures

        struct SDescriptorHeapInfo
        {
            static const size_t scMaxDescriptorsInHeap = 1000;

            NativeAPI::D3D12DescriptorHeap* pDescriptorHeap;
            D3D12_DESCRIPTOR_HEAP_TYPE      Type;
            D3D12_DESCRIPTOR_HEAP_FLAGS     Flags;
            SIZE_T                          NumDescriptors = 0;
            SIZE_T                          DescriptorSize;
            Utils::TCBitPool< uint8_t >     SlotPool;

            bool IsFull() const
            {
                return NumDescriptors >= scMaxDescriptorsInHeap;
            }

            bool HasSpace( uint32_t Count = 0 ) const
            {
                return ( NumDescriptors + Count ) <= scMaxDescriptorsInHeap;
            }

            bool Matches( D3D12_DESCRIPTOR_HEAP_TYPE InType, D3D12_DESCRIPTOR_HEAP_FLAGS InFlags ) const
            {
                return this->Type == InType && this->Flags == InFlags;
            }

            D3D12_CPU_DESCRIPTOR_HANDLE Allocate( uint32_t numDescriptors )
            {
                uint32_t                    firstSlotIndex = SlotPool.AllocateSlots( numDescriptors );
                D3D12_CPU_DESCRIPTOR_HANDLE Handle         = {};

                if( firstSlotIndex != UNDEFINED_U32 )
                {
                    VKE_D3D12_CALL_RET( Handle, pDescriptorHeap, GetCPUDescriptorHandleForHeapStart );
                    Handle.ptr     += firstSlotIndex * DescriptorSize;
                    NumDescriptors += numDescriptors;
                }

                return Handle;
            }

            void Free( size_t firstSlotPtr, uint32_t numDescriptors )
            {
                uint32_t firstSlotIndex = static_cast< uint32_t >( firstSlotPtr / DescriptorSize );
                SlotPool.FreeSlots( firstSlotIndex, numDescriptors );
            }
        };

        SDescriptorHeapInfo* CreateDescriptorHeap( const NativeAPI::Device& pDevice, D3D12_DESCRIPTOR_HEAP_TYPE Type,
                                                   D3D12_DESCRIPTOR_HEAP_FLAGS Flags );

        SDescriptorHeapInfo* GetDescriptorHeap( const NativeAPI::Device& pDevice, D3D12_DESCRIPTOR_HEAP_TYPE Type,
                                                D3D12_DESCRIPTOR_HEAP_FLAGS Flags );

        NativeAPI::Fence m_pGlobalFence = nullptr;

        void ReleaseDescriptorHeaps();

    private:
        Utils::TCDynamicArray< SDescriptorHeapInfo, 4 > m_vDescriptorHeapPool;
    };

} // namespace VKE::RenderSystem::D3D12

#endif // VKE_COMPILE_D3D12_RHI