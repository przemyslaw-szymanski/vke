#pragma once

#include "Core/VKECommon.h"


namespace VKE::RenderSystem
{
//#if VKE_VULKAN_RENDER_SYSTEM || VKE_COMPILE_VULKAN_RHI
//    using NativeTypes = VKE::RenderSystem::Vulkan::NativeTypes;
//    //using namespace VKE::RenderSystem::Vulkan;
//#elif VKE_D3D12_RENDER_SYSTEM || VKE_COMPILE_D3D12_RHI
//    using NativeTypes = VKE::RenderSystem::D3D12::NativeTypes;
//    //using namespace VKE::RenderSystem::D3D12;
//#else
//#error "Unsupported 3D API"
//#endif

    namespace NativeTagTypes
    {
        struct Null{};
        struct Buffer {};
        struct Pipeline {};
        struct Texture {};
        struct Sampler {};
        struct RenderPass {};
        struct CommandBuffer {};
        struct TextureView {};
        struct BufferView {};
        struct CPUFence {};
        struct GPUFence {};
        struct Fence {};
        struct Device {};
        struct DescriptorPool {};
        struct DescriptorSet {};
        struct DescriptorSetLayout {};
        struct CommandBufferPool {};
        struct Framebuffer {};
        struct ClearValue {};
        struct Queue {};
        struct Format {};
        struct ImageType {};
        struct ImageLayout {};
        struct ImageUsageFlags {};
        struct Memory {};
        struct PresentSurface {};
        struct SwapChain {};
        struct Adapter {};
        struct Shader{};
        struct PipelineLayout{};
        struct Event{};
        struct QueueFamilyProperties{};
    }

	namespace NativeTypes
	{
        const NativeTagTypes::Null Null;

        template<class TagT, typename HandleT>
        struct TSHandle
        {
            using TagType = TagT;
            using HandleType = HandleT;

            private:
            HandleT handle;

            public:
            TSHandle()
            {
            }

            TSHandle( const TSHandle& o ) : handle{ o.handle }
            {
            }

            explicit TSHandle( HandleType h ) : handle( h )
            {
            }
            TSHandle( NativeTagTypes::Null ) : handle( 0 )
            {
            }

            TSHandle& operator=( const TSHandle& other ) = default;
            bool      operator==( const TSHandle& other ) const = default;
            bool      operator!=( const TSHandle& other ) const = default;
                        
            TSHandle& operator=( NativeTagTypes::Null )
            {
                handle = 0;
                return *this;
            }

            bool operator==( NativeTagTypes::Null ) const
            {
                return handle == 0;
            }
            
            bool operator!=( NativeTagTypes::Null ) const
            {
                return handle != 0;
            }

            void* ToVoidPtr() const
            {
                return reinterpret_cast<void*>( handle );
            }

            uint64_t ToUint64() const
            {
                return static_cast<uint64_t>( handle );
            }
        };

        using Buffer                = TSHandle<NativeTagTypes::Buffer, handle_t>;
        using Pipeline              = TSHandle<NativeTagTypes::Pipeline, handle_t>;
        using Texture               = TSHandle<NativeTagTypes::Texture, handle_t>;
        using Sampler               = TSHandle<NativeTagTypes::Sampler, handle_t>;
        using RenderPass            = TSHandle<NativeTagTypes::RenderPass, handle_t>;
        using CommandBuffer         = TSHandle<NativeTagTypes::CommandBuffer, handle_t>;
        using TextureView           = TSHandle<NativeTagTypes::TextureView, handle_t>;
        using BufferView            = TSHandle<NativeTagTypes::BufferView, handle_t>;
        using CPUFence              = TSHandle<NativeTagTypes::CPUFence, handle_t>;
        using GPUFence              = TSHandle<NativeTagTypes::GPUFence, handle_t>;
        using Fence                 = TSHandle<NativeTagTypes::Fence, handle_t>;
        using FenceValue            = uint64_t;
        using Device                = TSHandle<NativeTagTypes::Device, handle_t>;
        using DescriptorPool        = TSHandle<NativeTagTypes::DescriptorPool, handle_t>;
        using DescriptorSet         = TSHandle<NativeTagTypes::DescriptorSet, handle_t>;
        using DescriptorSetLayout   = TSHandle<NativeTagTypes::DescriptorSetLayout, handle_t>;
        using CommandBufferPool     = TSHandle<NativeTagTypes::CommandBufferPool, handle_t>;
        using Framebuffer           = TSHandle<NativeTagTypes::Framebuffer, handle_t>;
        //using ClearValue            = SClearValue;
        using Queue                 = TSHandle<NativeTagTypes::Queue, handle_t>;
        using Format                = TSHandle<NativeTagTypes::Format, handle_t>;
        using ImageType             = TSHandle<NativeTagTypes::ImageType, handle_t>;
        using ImageLayout           = TSHandle<NativeTagTypes::ImageLayout, handle_t>;
        using ImageUsageFlags       = TSHandle<NativeTagTypes::ImageUsageFlags, handle_t>;
        using MemoryHeap            = TSHandle<NativeTagTypes::Memory, handle_t>;
        using PresentSurface        = TSHandle<NativeTagTypes::PresentSurface, handle_t>;
        using SwapChain             = TSHandle<NativeTagTypes::SwapChain, handle_t>;
        using Adapter               = TSHandle<NativeTagTypes::Adapter, handle_t>;
        using Shader                = TSHandle<NativeTagTypes::Shader, handle_t>;
        using PipelineLayout        = TSHandle<NativeTagTypes::PipelineLayout, handle_t>;
        using DeviceSize            = uint64_t;
        using Event                 = TSHandle<NativeTagTypes::Event, handle_t>;
        using QueueFamilyProperties = TSHandle<NativeTagTypes::QueueFamilyProperties, handle_t>;
	} // NativeTypes

#define VKE_RENDER_SYSTEM_DEFINE_TO_NATIVE( _type )                                                                                      \
    template<typename NativeAPIT>                                                                                                         \
    inline NativeAPIT::_type ToNative( NativeTypes::_type hObj )                                                   \
    {                                                                                                                  \
        return reinterpret_cast< NativeAPIT::_type >( hObj.ToUint64() );                                                            \
    }                                                                 

VKE_RENDER_SYSTEM_DEFINE_TO_NATIVE( Buffer )
VKE_RENDER_SYSTEM_DEFINE_TO_NATIVE( Pipeline )
VKE_RENDER_SYSTEM_DEFINE_TO_NATIVE( Texture )
VKE_RENDER_SYSTEM_DEFINE_TO_NATIVE( Sampler )
VKE_RENDER_SYSTEM_DEFINE_TO_NATIVE( RenderPass )
VKE_RENDER_SYSTEM_DEFINE_TO_NATIVE( CommandBuffer )
VKE_RENDER_SYSTEM_DEFINE_TO_NATIVE( TextureView )
VKE_RENDER_SYSTEM_DEFINE_TO_NATIVE( BufferView )
VKE_RENDER_SYSTEM_DEFINE_TO_NATIVE( CPUFence )
VKE_RENDER_SYSTEM_DEFINE_TO_NATIVE( GPUFence )
VKE_RENDER_SYSTEM_DEFINE_TO_NATIVE( Fence )
VKE_RENDER_SYSTEM_DEFINE_TO_NATIVE( Device )
VKE_RENDER_SYSTEM_DEFINE_TO_NATIVE( DescriptorPool )
VKE_RENDER_SYSTEM_DEFINE_TO_NATIVE( DescriptorSet )
VKE_RENDER_SYSTEM_DEFINE_TO_NATIVE( DescriptorSetLayout )
VKE_RENDER_SYSTEM_DEFINE_TO_NATIVE( CommandBufferPool )
VKE_RENDER_SYSTEM_DEFINE_TO_NATIVE( Framebuffer )
VKE_RENDER_SYSTEM_DEFINE_TO_NATIVE( Queue )
VKE_RENDER_SYSTEM_DEFINE_TO_NATIVE( Format )
VKE_RENDER_SYSTEM_DEFINE_TO_NATIVE( ImageType )
VKE_RENDER_SYSTEM_DEFINE_TO_NATIVE( ImageLayout )
VKE_RENDER_SYSTEM_DEFINE_TO_NATIVE( ImageUsageFlags )
VKE_RENDER_SYSTEM_DEFINE_TO_NATIVE( MemoryHeap )
VKE_RENDER_SYSTEM_DEFINE_TO_NATIVE( PresentSurface )
VKE_RENDER_SYSTEM_DEFINE_TO_NATIVE( SwapChain )
VKE_RENDER_SYSTEM_DEFINE_TO_NATIVE( Adapter )
VKE_RENDER_SYSTEM_DEFINE_TO_NATIVE( Shader )
VKE_RENDER_SYSTEM_DEFINE_TO_NATIVE( PipelineLayout )
VKE_RENDER_SYSTEM_DEFINE_TO_NATIVE( Event )
VKE_RENDER_SYSTEM_DEFINE_TO_NATIVE( QueueFamilyProperties )

    template< typename RenderSystemNativeT >
    concept IsNativeHandle =
    std::is_base_of_v< NativeTypes::TSHandle< typename RenderSystemNativeT::TagType, typename RenderSystemNativeT::HandleType >,
                       RenderSystemNativeT >;

} // namespace VKE::RenderSystem

namespace std
{
    template< VKE::RenderSystem::IsNativeHandle RenderSystemNativeT >
    struct hash< RenderSystemNativeT >
    {
        size_t operator()( const RenderSystemNativeT& hObj ) const
        {
            return std::hash< uint64_t >{}( hObj.ToUint64() );
        }
    };

    template< VKE::RenderSystem::IsNativeHandle RenderSystemNativeT >
    ostream& operator<<( ostream& os, const RenderSystemNativeT& hObj )
    {
        os << "0x" << std::hex << hObj.ToVoidPtr() << std::dec;
        return os;
    }
} // namespace std

namespace VKE::Utils::Hash
{
    /*template< VKE::RenderSystem::IsNativeHandle RenderSystemNativeT >
    static vke_force_inline void Combine( hash_t* pInOut, RenderSystemNativeT hObj )
    {
        *pInOut ^= std::hash< uint64_t >{}( hObj.ToUint64() ) + CalcMagic( *pInOut );
    }*/
}

