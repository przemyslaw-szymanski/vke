#pragma once

#if ( VKE_COMPILE_D3D12_RHI + VKE_COMPILE_VULKAN_RHI ) > 1
#define VKE_RHI_USE_VIRTUAL 1
#define VKE_RHI_VIRTUAL virtual
#define VKE_RHI_PURE = 0
#define VKE_RHI_FRIEND_CLASS                                                                                           \
    friend class RHI::CD3D12RHI;                                                                                       \
    friend class RHI::CVulkanRHI;
#else
#define VKE_RHI_USE_VIRTUAL 0
#define VKE_RHI_VIRTUAL
#define VKE_RHI_PURE
#define VKE_RHI_FRIEND_CLASS friend class RHI::CRHI;
#endif

// Backend-specific implementation types live in separate namespaces to avoid ODR clashes
// when multiple RHIs are compiled into one binary.
namespace VKE::RenderSystem::RHI
{
    namespace D3D12API
    {
        struct SImplementation;
    }
    namespace VulkanAPI
    {
        struct SImplementation;
    }
#if VKE_RENDER_SYSTEM == VKE_D3D12
    using SRHIImplementation = D3D12API::SImplementation;
#else
    using SRHIImplementation = VulkanAPI::SImplementation;
#endif
} // namespace VKE::RenderSystem::RHI

#include "CCommandLineArgs.h"

#if VKE_RHI_USE_VIRTUAL
#define VKE_RHI_VULKAN_STATIC_CLASS ::VKE::RenderSystem::RHI::CVulkanRHI
#define VKE_RHI_D3D12_STATIC_CLASS ::VKE::RenderSystem::RHI::CD3D12RHI
#else
#define VKE_RHI_VULKAN_STATIC_CLASS ::VKE::RenderSystem::RHI::CRHI
#define VKE_RHI_D3D12_STATIC_CLASS ::VKE::RenderSystem::RHI::CRHI
#endif

#if VKE_COMPILE_VULKAN_RHI
#define VKE_RHI_CHECK_VULKAN( _api ) _api == RHI_API::Vulkan
#define VKE_RHI_VULKAN_ENUM Vulkan = VKE_VULKAN,
#define VKE_RHI_VULKAN_CALL_STATIC( _func, ... )                                                                       \
    case ::VKE::RenderSystem::RHI::SRHIAPI::Vulkan:                                                                    \
        return VKE_RHI_VULKAN_STATIC_CLASS::_func( __VA_ARGS__ );
#else
#define VKE_RHI_CHECK_VULKAN( _api ) false
#define VKE_RHI_VULKAN_ENUM
#define VKE_RHI_VULKAN_CALL_STATIC( _func, ... )
#endif

#if VKE_COMPILE_D3D12_RHI
#define VKE_RHI_CHECK_D3D12( _api ) _api == RHI_API::D3D12
#define VKE_RHI_D3D12_ENUM D3D12 = VKE_D3D12,
#define VKE_RHI_D3D12_CALL_STATIC( _func, ... )                                                                        \
    case ::VKE::RenderSystem::RHI::SRHIAPI::D3D12:                                                                     \
        return VKE_RHI_D3D12_STATIC_CLASS::_func( __VA_ARGS__ );
#else
#define VKE_RHI_CHECK_D3D12( _api ) false
#define VKE_RHI_D3D12_ENUM
#define VKE_RHI_D3D12_CALL_STATIC( _func, ... )
#endif

// Usage: VKE_RHI_CALL_STATIC( api, Load, LoadInfo, &m_DriverData )
#if VKE_RHI_USE_VIRTUAL
#define VKE_RHI_CALL_STATIC( _func, ... )                                                                              \
    [ & ]() -> decltype( auto ) {                                                                                      \
        switch( GetRuntimeRHI() )                                                                                      \
        {                                                                                                              \
            VKE_RHI_VULKAN_CALL_STATIC( _func __VA_OPT__(, ) __VA_ARGS__ )                                             \
            VKE_RHI_D3D12_CALL_STATIC( _func __VA_OPT__(, ) __VA_ARGS__ )                                              \
        }                                                                                                              \
        VKE_ASSERT( 0 );                                                                                               \
        std::unreachable();                                                                                            \
    }()
#else
#define VKE_RHI_CALL_STATIC( _func, ... ) ::VKE::RenderSystem::RHI::CRHI::_func( __VA_ARGS__ )
#endif

#if VKE_RHI_USE_VIRTUAL
#define VKE_RHI_CREATE_OBJECT( _ptr )                                                                                  \
    do                                                                                                                 \
    {                                                                                                                  \
        ::VKE::RenderSystem::RHI::RHI_API api = ::VKE::RenderSystem::RHI::GetRuntimeRHI();                             \
        if( VKE_RHI_CHECK_VULKAN( api ) )                                                                              \
        {                                                                                                              \
            VKE_RHI_VULKAN_STATIC_CLASS* pVulkan;                                                                      \
            Memory::CreateObject( &HeapAllocator, &pVulkan );                                                          \
        }                                                                                                              \
    } while( false );
    ;
#else
#define VKE_RHI_CREATE_OBJECT( _ptr )                                                                                  \
    if( VKE_FAILED( Memory::CreateObject( &HeapAllocator, &_ptr ) ) )                                                  \
    {                                                                                                                  \
        VKE_LOG_ERR( "Out of memory" );                                                                                \
    }
#endif

namespace VKE::RenderSystem::RHI
{
    struct SRHIAPI
    {
        enum API : uint8_t
        {
            VKE_RHI_VULKAN_ENUM VKE_RHI_D3D12_ENUM
        };
    };

    using RHI_API = SRHIAPI::API;

    static inline RHI_API GetRuntimeRHI()
    {
        uint8_t api = VKE_RENDER_SYSTEM;
#if VKE_RHI_USE_VIRTUAL
        auto rhi = GetCommandLineParam< uint32_t >( "rhi" );
        if( rhi.has_value() )
        {
            uint32_t requestedRhi = rhi.value().uintValue;
            if( VKE_RHI_CHECK_VULKAN( requestedRhi ) )
            {
                api = VKE_VULKAN;
            }
            else if( VKE_RHI_CHECK_D3D12( requestedRhi ) )
            {
                api = VKE_D3D12;
            }
        }
#endif
        return static_cast< RHI_API >( api );
    }

}; // namespace VKE::RenderSystem::RHI