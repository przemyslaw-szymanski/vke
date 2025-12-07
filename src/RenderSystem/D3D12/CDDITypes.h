#pragma once

#if VKE_D3D12_RENDER_SYSTEM

#include <directx/d3d12.h>
#include <dxgi1_6.h>

namespace VKE::RenderSystem
{
    static const uint32_t DEFAULT_QUEUE_FAMILY_PROPERTY_COUNT = 16;

    // TODO(blturkot): D3D12 use single desc for all shader bindings
    enum DDIImageViewType
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

    namespace NativeAPI
    {
        static const decltype( nullptr ) Null;

        template< class ObjT >
        concept Nullable = std::is_pointer_v< ObjT >;

        template< class T >
        struct TSSimpleHashT
        {
            static hash_t CalcHash( const T& Obj )
            {
                return std::hash< T >{}( Obj );
            }
        };

        template< class D3D12TypeT, class DefaultT, class ChildObjectT, class HashTraitT = TSSimpleHashT< D3D12TypeT > >
        struct TSObjectWrapper
        {
            D3D12TypeT Obj;

            TSObjectWrapper() = default;

            TSObjectWrapper( const D3D12TypeT& Other ) : Obj{ Other }
            {
            }

            TSObjectWrapper( decltype( Null ) )
                //    requires std::is_pointer_v< D3D12TypeT >
                : Obj{ Null }
            {
            }

            ChildObjectT& Reinterpret()
            {
                return reinterpret_cast< ChildObjectT& >( *this );
            }

            ChildObjectT& Reinterpret( const TSObjectWrapper& pOther )
            {
                return reinterpret_cast< ChildObjectT& >( const_cast< TSObjectWrapper& >( pOther ) );
            }

            const ChildObjectT& Reinterpret() const
            {
                return reinterpret_cast< const ChildObjectT& >( *this );
            }

            TSObjectWrapper& operator=( const TSObjectWrapper& Other )
            {
                auto& ref = Reinterpret();
                ref.Assign( Reinterpret( Other ) );
                return *this;
            }

            TSObjectWrapper& operator=( const D3D12TypeT& Other )
            {
                auto& ref = Reinterpret();
                ref.Assign( Other );
                return *this;
            }

            TSObjectWrapper& operator=( decltype( Null ) )
            {
                auto& ref = Reinterpret();
                ref.SetNull( Null );
                return *this;
            }

            const bool operator==( decltype( Null ) ) const
            {
                auto& ref = Reinterpret();
                return ref.IsNull();
            }

            const bool operator!=( decltype( Null ) ) const
            {
                auto& ref = Reinterpret();
                return !ref.IsNull();
            }

            const bool operator==( const TSObjectWrapper& Other ) const
            {
                auto& ref = Reinterpret();
                return ref.Equals( Other.Obj );
            }

            operator D3D12TypeT()
            {
                return Obj;
            }

            operator const D3D12TypeT() const
            {
                return Obj;
            }

            static hash_t CalcHash( const D3D12TypeT& Obj )
            {
                return HashTraitT::CalcHash( Obj );
            }

            void Assign( const TSObjectWrapper& Other )
            {
                Obj = Other.Obj;
            }

            void Assign( const D3D12TypeT& Other )
            {
                Obj = Other;
            }

            void SetNull()
            {
                Obj = Null;
            }

            bool IsNull() const
            {
                return Obj == Null;
            }

            const bool Equals( const D3D12TypeT& Other ) const
            {
                return Obj == Other;
            }
        };

        namespace CustomTypes
        {
            using DDIFence               = ID3D12Fence1*;
            using DDICommandBufferPool   = ID3D12CommandAllocator*;
            using DDIShader              = D3D12_SHADER_BYTECODE;
            using DDIDescriptorSetLayout = D3D12_ROOT_PARAMETER1;
            using DDIDescriptorSetRange  = D3D12_DESCRIPTOR_RANGE1;

            struct CPUFence : TSObjectWrapper< DDIFence, std::nullopt_t, CPUFence >
            {
                using Wrapper = TSObjectWrapper< DDIFence, std::nullopt_t, CPUFence >;

                HANDLE hEvent = nullptr;
                UINT64 Value  = 0;

                CPUFence() = default;

                CPUFence( decltype( Null ) ) : Wrapper( Null )
                {
                }

                CPUFence( const DDIFence& Other ) : Wrapper( Other )
                {
                }
            };

            struct GPUFence : TSObjectWrapper< DDIFence, std::nullopt_t, GPUFence >
            {
                using Wrapper = TSObjectWrapper< DDIFence, std::nullopt_t, GPUFence >;

                HANDLE hEvent = nullptr;
                UINT64 Value  = 0;

                GPUFence() = default;

                GPUFence( decltype( Null ) ) : Wrapper( Null )
                {
                }

                GPUFence( const DDIFence& Other ) : Wrapper( Other )
                {
                }
            };

            struct CommandBufferPool : TSObjectWrapper< DDICommandBufferPool, std::nullopt_t, CommandBufferPool >
            {
                using Wrapper = TSObjectWrapper< DDICommandBufferPool, std::nullopt_t, CommandBufferPool >;

                D3D12_COMMAND_LIST_TYPE NativeType = D3D12_COMMAND_LIST_TYPE_DIRECT;
                uint8_t                 EngineType = 0;

                CommandBufferPool() = default;

                CommandBufferPool( decltype( Null ) ) : Wrapper( Null )
                {
                }

                CommandBufferPool( const DDICommandBufferPool& Other ) : Wrapper( Other )
                {
                }
            };

            struct Shader : TSObjectWrapper< DDIShader, std::nullptr_t, Shader >
            {
                using Wrapper = TSObjectWrapper< DDIShader, std::nullptr_t, Shader >;

                Shader() = default;

                Shader( decltype( Null ) )
                {
                    SetNull();
                }

                Shader( const DDIShader& Other )
                {
                    Assign( Other );
                }

                void SetNull()
                {
                    Obj.pShaderBytecode = nullptr;
                    Obj.BytecodeLength  = 0;
                }

                bool IsNull() const
                {
                    return Obj.pShaderBytecode == nullptr;
                }

                void Assign( const DDIShader& Other )
                {
                    Obj.pShaderBytecode = Other.pShaderBytecode;
                    Obj.BytecodeLength  = Other.BytecodeLength;
                }

                const bool Equals( const DDIShader& Other ) const
                {
                    return Obj.pShaderBytecode == Other.pShaderBytecode;
                }
            };

            struct SPipelineLayout : TSObjectWrapper< uint32_t, std::nullopt_t, SPipelineLayout, SPipelineLayout >
            {
                SPipelineLayout() = default;

                SPipelineLayout( decltype( Null ) )
                {
                    SetNull();
                }

                SPipelineLayout( const SPipelineLayout& Other )
                {
                    Assign( Other );
                }

                static hash_t CalcHash( SPipelineLayout* pLayout )
                {
                    return 0;
                }

                void SetNull()
                {
                    Obj = 0;
                }

                bool IsNull() const
                {
                    return Obj == 0;
                }

                const bool Equals( const uint32_t& Other ) const
                {
                    return Obj == Other;
                }
            };

            struct SDescriptorPool : TSObjectWrapper< uint32_t, std::nullopt_t, SDescriptorPool >
            {
                ID3D12DescriptorHeap* Heaps[ D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES ];

                SDescriptorPool() = default;

                SDescriptorPool( decltype( Null ) )
                {
                    SetNull();
                }

                SDescriptorPool( const SDescriptorPool& Other ) : Heaps{ Null }
                {
                    Assign( Other.Obj );
                }

                // static hash_t CalcHash( SDescriptorPool* pPool )
                //{
                //     return 0;
                // }

                void SetNull()
                {
                    Obj = 0;
                }

                bool IsNull() const
                {
                    for( auto& heap: Heaps )
                    {
                        if( heap != Null )
                        {
                            return false;
                        }
                    }
                    return true;
                }

                const bool Equals( const uint32_t& Other ) const
                {
                    return Obj == Other;
                }
            };

            struct SDescriptorSetLayout
                : TSObjectWrapper< DDIDescriptorSetLayout, std::nullptr_t, SDescriptorSetLayout >
            {
                using Wrapper = TSObjectWrapper< DDIDescriptorSetLayout, std::nullptr_t, SDescriptorSetLayout >;

                Utils::TCDynamicArray< DDIDescriptorSetRange, 32 > vDescriptorRanges;

                SDescriptorSetLayout() = default;

                SDescriptorSetLayout( decltype( Null ) )
                {
                    SetNull();
                }

                SDescriptorSetLayout( const DDIDescriptorSetLayout& Other )
                {
                    Assign( Other );
                }

                void SetNull()
                {
                    Obj.DescriptorTable.NumDescriptorRanges = 0;
                    Obj.DescriptorTable.pDescriptorRanges   = nullptr;
                }

                bool IsNull() const
                {
                    return Obj.DescriptorTable.pDescriptorRanges == nullptr;
                }

                void Assign( const SDescriptorSetLayout& Other )
                {
                    Obj               = Other.Obj;
                    vDescriptorRanges = Other.vDescriptorRanges;

                    // Update pointer to copied data
                    Obj.DescriptorTable.pDescriptorRanges = vDescriptorRanges.GetData();
                }

                const bool Equals( const DDIDescriptorSetLayout& Other ) const
                {
                    return memcmp( &Obj, &Other, sizeof( DDIDescriptorSetLayout ) ) == 0;
                }
            };

            struct SFenceTypes
            {
                struct GPU
                {
                };

                struct CPU
                {
                };
            };

            template< class TypeT >
            struct SMonitoredFence
            {
                TypeT Type;

                ID3D12Fence1* pFence;
                HANDLE        hHandle;
                UINT64        counter = 0;
            };

            struct SCommandQueue
            {
                static Utils::TCDynamicArray< SCommandQueue > s_QueuePool;

                ID3D12CommandQueue*                 pQueue = nullptr;
                SMonitoredFence< SFenceTypes::GPU > GpuFence;
            };

        } // namespace CustomTypes

        struct FenceTypes
        {
            enum TYPE
            {
                CPU,
                GPU
            };
        };

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

        using Buffer              = ID3D12Resource*;
        using Pipeline            = ID3D12PipelineState*;
        using Texture             = ID3D12Resource*;
        using Sampler             = void*;
        using RenderPass          = ID3D12Object*;
        using CommandBuffer       = ID3D12GraphicsCommandList*;
        using TextureView         = void*;
        using BufferView          = void*;
        using CPUFence            = CustomTypes::CPUFence;
        using GPUFence            = CustomTypes::GPUFence;
        using Device              = ID3D12Device10*;
        using DescriptorPool      = CustomTypes::SDescriptorPool;
        using DescriptorSet       = ID3D12DescriptorHeap*;
        using DescriptorSetLayout = CustomTypes::SDescriptorSetLayout;
        using CommandBufferPool   = CustomTypes::CommandBufferPool;
        using Framebuffer         = ID3D12Resource*;
        using ClearValue          = SClearValue;
        using Queue               = ID3D12CommandQueue*;
        using Format              = DXGI_FORMAT;
        using ImageType           = D3D12_RESOURCE_DIMENSION;
        using ImageLayout         = D3D12_RESOURCE_FLAGS;
        using ImageUsageFlags     = D3D12_RESOURCE_FLAGS;
        using Memory              = ID3D12Object*;
        using PresentSurface      = ID3D12Resource*;
        using SwapChain           = IDXGISwapChain4*;
        using Adapter             = IDXGIAdapter4*;
        using Shader              = CustomTypes::Shader;
        // using PipelineLayout        = CustomTypes::SPipelineLayout;
        using PipelineLayout        = ID3D12RootSignature*;
        using DeviceSize            = UINT64;
        using Event                 = HANDLE;
        using QueueFamilyProperties = void*;
        using DeviceLimits          = SDeviceLimits;

        using Result  = HRESULT;
        using Factory = IDXGIFactory7*;

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

        static vke_force_inline hash_t CalcHash( const GPUFence& Fence )
        {
            return GPUFence::CalcHash( Fence );
        }

        struct SImplementation
        {
            static const uint32_t MAX_MEMORY_HEAPS = 16;

            static Factory spFactory;
            static bool    sTearingSupported;
            static bool    sDebugLayerEnabled;

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
                    bool                  UploadHeapSupported;
                } Memory;

                void* aFormatProperties[ Formats::_MAX_COUNT ];
            } Properties; // struct SDeviceProperties

            struct SDeviceFeatures
            {
            } Features; // struct SDeviceFeatures
        };

    } // namespace NativeAPI

} // namespace VKE::RenderSystem

template<>
struct std::hash< VKE::RenderSystem::NativeAPI::GPUFence >
{
    std::size_t operator()( const VKE::RenderSystem::NativeAPI::GPUFence& Fence ) const
    {
        return VKE::RenderSystem::NativeAPI::CalcHash( Fence );
    }
};

#endif // VKE_D3D12_RENDER_SYSTEM