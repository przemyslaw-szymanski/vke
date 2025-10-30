#pragma once
#if VKE_D3D12_RENDER_SYSTEM

#include <d3d12.h>
#include <dxgi1_6.h>

namespace VKE
{
    namespace RenderSystem
    {
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

        
    } // RenderSystem
    namespace RenderSystem::NativeAPI
    {
        static const decltype( nullptr ) Null;

        struct FenceTypes
        {
            enum TYPE
            {
                CPU,
                GPU
            };
        };

        /*template<FenceTypes::TYPE T>
        struct TSFence
        {
            ID3D12Fence* pFence;
            TSFence()
            {
            }
            TSFence( ID3D12Fence* pPtr )
                : pFence( pPtr )
            {
            }

            operator ID3D12Fence* ()
            {
                return pFence;
            }

            operator const ID3D12Fence*() const
            {
                return pFence;
            }

            TSFence& operator=(ID3D12Fence* pPtr)
            {
                pFence = pPtr;
                return *this;
            }

            TSFence& operator=(TSFence Fence)
            {
                pFence = Fence.pFence;
                return *this;
            }
        };*/

        struct NoTrait{};
        struct CPUFenceTrait{};
        struct GPUFenceTrait{};

        template<class ObjT>
        concept Nullable = std::is_pointer_v<ObjT>;

        template<class T>
        struct TSSimpleHashT
        {
            static hash_t CalcHash(const T& Obj)
            {
                return std::hash<T>{}( Obj );
            }
        };

        template<class D3D12TypeT, class DefaultT,
            class TypeTraitT = NoTrait,
            class HashTraitT = TSSimpleHashT< D3D12TypeT > >
        struct TSObjectWrapper
        {
            D3D12TypeT Obj;

            TSObjectWrapper()
            {
            }

            TSObjectWrapper(const D3D12TypeT& Other) :
                Obj{ Other }
            {

            }

            TSObjectWrapper( decltype(Null) ) :
                Obj{ DefaultT{} }
            {
            }

            TSObjectWrapper( decltype( Null ) )
                requires std::is_pointer_v< D3D12TypeT >
                :
                Obj{ Null }
            {}

            TSObjectWrapper& operator=( const TSObjectWrapper& Other )
            {
                Obj = Other.Obj;
                return *this;
            }

            TSObjectWrapper& operator=(const D3D12TypeT& Other)
            {
                Obj = Other;
                return *this;
            }

            TSObjectWrapper& operator=( decltype( Null ) )
            {
                if constexpr( std::is_pointer_v < D3D12TypeT > )
                {
                    Obj = Null;
                }
                else
                {
                    Obj = DefaultT{};
                }
                return *this;
            }

            const bool operator==( decltype( Null ) ) const
            {
                if constexpr (std::is_pointer_v<D3D12TypeT>)
                {
                    return Obj == Null;
                }
                else
                {
                    return Obj == DefaultT{};
                }
            }

            const bool operator!=( decltype( Null ) ) const
            {
                if constexpr (std::is_pointer_v<D3D12TypeT>)
                {
                    return Obj == Null;
                }
                else
                {
                    return Obj == DefaultT{};
                }
            }

            const bool operator==(const TSObjectWrapper& Other) const
            {
                return Obj == Other.Obj;
            }


            operator D3D12TypeT()
            {
                return Obj;
            }

            operator const D3D12TypeT() const
            {
                return Obj;
            }

            static hash_t CalcHash(const D3D12TypeT& Obj)
            {
                return HashTraitT::CalcHash( Obj );
            }
        };

        struct SClearValue : D3D12_CLEAR_VALUE
        {
            SClearValue()
            {
            }

            SClearValue( DXGI_FORMAT fmt, float r, float g, float b, float a )
                : D3D12_CLEAR_VALUE{ fmt, { r, g, b, a } }
            {
            }
            SClearValue( DXGI_FORMAT fmt, float d, uint8_t s )
                : D3D12_CLEAR_VALUE{ fmt }
            {
                this->DepthStencil.Depth   = d;
                this->DepthStencil.Stencil = s;
            }

            SClearValue(float r, float g, float b, float a) :
                SClearValue( DXGI_FORMAT_UNKNOWN, r, g, b, a )
            {
                /*this->Color[ 0 ] = r;
                this->Color[ 1 ] = g;
                this->Color[ 2 ] = b;
                this->Color[ 3 ] = a;*/
            }

            SClearValue( float d, uint8_t s ) :
                SClearValue( DXGI_FORMAT_UNKNOWN, d, s )
            {
            }

            SClearValue& operator=(const SClearValue& Other)
            {
                this->Format     = Other.Format;
                this->Color[ 0 ] = Other.Color[ 0 ];
                this->Color[ 1 ] = Other.Color[ 1 ];
                this->Color[ 2 ] = Other.Color[ 2 ];
                this->Color[ 3 ] = Other.Color[ 3 ];
                return *this;
            }

            operator const D3D12_CLEAR_VALUE& () const
            {
                /// @TODO: Do we need this assert?
                VKE_ASSERT2( this->Format != DXGI_FORMAT_UNKNOWN, "Format must be set" );
                return static_cast<const D3D12_CLEAR_VALUE&>( *this );
            }
        };

        struct SPipelineLayout
        {
            bool operator==(SPipelineLayout) const
            {
                return true;
            }
        };

        struct SPipelineLayoutHash
        {
            static hash_t CalcHash(SPipelineLayout)
            {
                return 0;
            }
        };

        struct GlobalAPI
        {

        };

        struct InstanceAPI
        {

        };

        struct DeviceAPI
        {

        };
        
        using Instance            = void*;
        using Buffer              = ID3D12Resource*;
        using Pipeline            = ID3D12PipelineState*;
        using Texture             = ID3D12Resource*;
        using Sampler             = void*;
        using RenderPass          = ID3D12Object*;
        using CommandBuffer       = ID3D12CommandList*;
        using TextureView         = void*;
        using BufferView          = void*;
        using CPUFence            = TSObjectWrapper<ID3D12Fence*, std::nullopt_t, CPUFenceTrait>;
        using GPUFence            = TSObjectWrapper<ID3D12Fence*, std::nullopt_t, GPUFenceTrait>;
        using Device              = ID3D12Device10*;
        using DescriptorPool      = ID3D12DescriptorHeap*;
        using DescriptorSet       = ID3D12DescriptorHeap*;
        using DescriptorSetLayout = ID3D12DescriptorHeap*;
        using CommandBufferPool   = ID3D12CommandAllocator*;
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
        using Shader              = TSObjectWrapper<byte*, std::nullptr_t>;
        using PipelineLayout      = TSObjectWrapper<SPipelineLayout, SPipelineLayout, NoTrait, SPipelineLayoutHash>;
        using DeviceSize          = UINT64;
        using Event               = HANDLE;

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

        static vke_force_inline hash_t CalcHash(const GPUFence& Fence)
        {
            return GPUFence::CalcHash( Fence );
        }

    }
} // VKE

template<> struct std::hash<VKE::RenderSystem::NativeAPI::GPUFence>
{
    std::size_t operator()( const VKE::RenderSystem::NativeAPI::GPUFence& Fence ) const
    {
        return VKE::RenderSystem::NativeAPI::CalcHash( Fence );
    }
};

#endif // VKE_D3D12_RENDER_SYSTEM