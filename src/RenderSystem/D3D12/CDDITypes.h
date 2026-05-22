#pragma once

#if VKE_RENDER_SYSTEM_D3D12
#include "Core/Memory/CFreeListPool.h"
#include "Core/Utils/TCBitPool.h"

#include <directx/d3d12.h>
#include <directx/d3dx12.h>
#include <dxgi1_6.h>
#include <pix3.h>

namespace VKE::RenderSystem
{
    static const uint32_t DEFAULT_QUEUE_FAMILY_PROPERTY_COUNT = 16;

    namespace NativeAPI
    {
        // DirectX 12 have multiple structures for the same thing but with different feature sets. To prevent huge pain
        // in the butt when refactoring code due to higher struct / pointer number, we'll have one place to refactor
        // whole CDDI.
        using D3D12Fence               = ID3D12Fence1;
        using D3D12CommandAllocator    = ID3D12CommandAllocator;
        using D3D12CommandList         = ID3D12CommandList;
        using D3D12GraphicsCommandList = ID3D12GraphicsCommandList4;
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

        static const decltype( nullptr ) Null;

        template< class ObjT >
        concept Nullable = std::is_pointer_v< ObjT >;

        enum struct ResourceViewTypes : uint32_t
        {
            SRV = VKE_BIT( 1 ),
            UAV = VKE_BIT( 2 ),
            RTV = VKE_BIT( 3 ),
            DSV = VKE_BIT( 4 ),
        };

        namespace CustomTypes
        {
            struct SFence
            {
                NativeAPI::D3D12Fence* pObject = nullptr;
                HANDLE                 hEvent  = nullptr;
                UINT64                 Value   = 0;
            };

            struct SCPUFence : public SFence
            {
            };

            struct SGPUFence : public SFence
            {
            };

            struct SCommandBufferPool
            {
                D3D12_COMMAND_LIST_TYPE NativeType = D3D12_COMMAND_LIST_TYPE_DIRECT;
                uint8_t                 EngineType = 0;

                struct SCommandListWithAllocator
                {
                    NativeAPI::D3D12CommandAllocator*    pAllocator = nullptr;
                    NativeAPI::D3D12GraphicsCommandList* pCmdList   = nullptr;
                };

                Utils::TCDynamicArray< SCommandListWithAllocator, 32 > vCommandListsWithAllocators;

                NativeAPI::D3D12CommandAllocator* getAllocator( NativeAPI::D3D12GraphicsCommandList* pCommandList )
                {
                    NativeAPI::D3D12CommandAllocator* out = nullptr;
                    for( auto& Pair: vCommandListsWithAllocators )
                    {
                        if( Pair.pCmdList == pCommandList )
                        {
                            out = Pair.pAllocator;
                            break;
                        }
                    }
                    return out;
                }

                void SetName( cwstr_t pName )
                {
                    m_pName = pName;
                }

            protected:
                wstr_t m_pName;
            };

            struct SDescriptorSetLayout
            {
                Utils::TCDynamicArray< D3D12_DESCRIPTOR_RANGE1, 32 > vDescriptorRanges;
                NativeAPI::D3D12RootParameter                        RootParameter;
                uint16_t                                             aNumSlots[ D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES ];
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
                    return { pPool->pHeap->GetCPUDescriptorHandleForHeapStart().ptr +
                             ( PoolSlots.begin + slotIndexOffset ) * pPool->descriptorSize };
                }

                D3D12_GPU_DESCRIPTOR_HANDLE GetGpuDescriptorHandle( uint32_t slotIndexOffset ) const
                {
                    return { pPool->pHeap->GetGPUDescriptorHandleForHeapStart().ptr +
                             ( PoolSlots.begin + slotIndexOffset ) * pPool->descriptorSize };
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
                SClear                                                           Clear;

                SSubpassArray vSubpasses          = {};
                uint32_t      currentSubpassIndex = 0;

                SRenderPassBarriers EndBarriers;
                const char*         pName = nullptr;

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
        } // namespace CustomTypes

        struct SClearValue : D3D12_CLEAR_VALUE
        {
            SClearValue()
            {
            }

            SClearValue( DXGI_FORMAT fmt, float r, float g, float b, float a ) :
                D3D12_CLEAR_VALUE{ fmt, { r, g, b, a } }
            {
            }

            SClearValue( DXGI_FORMAT fmt, float d, uint8_t s ) : D3D12_CLEAR_VALUE{ fmt }
            {
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

            operator const D3D12_CLEAR_VALUE&() const
            {
                /// @TODO: Do we need this assert?
                VKE_ASSERT2( this->Format != DXGI_FORMAT_UNKNOWN, "Format must be set" );
                return static_cast< const D3D12_CLEAR_VALUE& >( *this );
            }
        };

        struct SDeviceLimits
        {
            // TODO(blturkot): Fill with limits
        };

        using Buffer                = D3D12Resource*;
        using Pipeline              = D3D12PipelineState*;
        using Texture               = D3D12Resource*;
        using Sampler               = void*;
        using RenderPass            = CustomTypes::SRenderPass*;
        using CommandBuffer         = D3D12GraphicsCommandList*;
        using TextureView           = CustomTypes::SResourceView*;
        using BufferView            = CustomTypes::SResourceView*;
        using CPUFence              = CustomTypes::SCPUFence*;
        using GPUFence              = CustomTypes::SGPUFence*;
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
        using Memory                = D3D12Heap*;
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

        struct SImplementation
        {
            static const uint32_t MAX_MEMORY_HEAPS = 16;

            static NativeAPI::D3D12Factory* spFactory;
            static bool                     sDebugLayerEnabled;

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
                        DeviceLimits limits;
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
                        Handle          = pDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
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

            SDescriptorHeapInfo* CreateDescriptorHeap( const NativeAPI::Device&    pDevice,
                                                       D3D12_DESCRIPTOR_HEAP_TYPE  Type,
                                                       D3D12_DESCRIPTOR_HEAP_FLAGS Flags );

            SDescriptorHeapInfo* GetDescriptorHeap( const NativeAPI::Device& pDevice, D3D12_DESCRIPTOR_HEAP_TYPE Type,
                                                    D3D12_DESCRIPTOR_HEAP_FLAGS Flags );

        private:
            Utils::TCDynamicArray< SDescriptorHeapInfo, 4 > m_vDescriptorHeapPool;
        };

    } // namespace NativeAPI

} // namespace VKE::RenderSystem

#endif // VKE_RENDER_SYSTEM_D3D12