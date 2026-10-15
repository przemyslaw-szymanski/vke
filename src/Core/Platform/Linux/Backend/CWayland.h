#pragma once

#include "Core/Platform/Common.h"
#include "Core/Platform/Linux/Backend/SWindowEvents.h"

#include <wayland-client.h>

namespace VKE::Platform::Backend
{
    struct SWaylandDisplay
    {
        wl_display* pDisplay = nullptr;

        bool IsValid() const
        {
            return pDisplay != nullptr;
        }
    };

    struct SWayland
    {
        const SWaylandDisplay* pDisplay = nullptr;
        wl_surface*            pSurface = nullptr;
        SWindowEvents          Events;

        bool IsValid() const
        {
            return pDisplay != nullptr && pDisplay->IsValid() && pSurface != nullptr;
        }
    };

    // Stub. Same static interface as CX11.
    class CWayland
    {
    public:
        static bool IsSupported()
        {
            // const char* pSessionType = getenv( "XDG_SESSION_TYPE" );
            // return ( pSessionType && strcmp( pSessionType, "wayland" ) == 0 );
            return false;
        }

        static const SWaylandDisplay& GetDisplay()
        {
            static SWaylandDisplay sDisplay;
            return sDisplay;
        }

        static Result Load( SWayland& )
        {
            return VKE_FAIL;
        }

        static void Unload( SWayland& WaylandInfo )
        {
            WaylandInfo.pDisplay = nullptr;
        }

        static Result Create( SWayland&, const SWindowDesc& )
        {
            return VKE_FAIL;
        }

        static Result Destroy( SWayland& )
        {
            return VKE_OK;
        }

        static Result SetWindowTitle( SWayland&, cstr_t )
        {
            return VKE_FAIL;
        }

        static void IsVisible( SWayland&, bool )
        {
        }

        static bool PumpEvents()
        {
            return false;
        }

    protected:
        void steps()
        {
            /*
            // Step A: Initialize wayland
            struct wl_display*  display  = wl_display_connect( NULL );
            struct wl_registry* registry = wl_display_get_registry( display );
            // Implement registry listeners to bind wl_compositor and xdg_wm_base

            // Step B: Create a wayland surface
            struct wl_surface*   wl_surface   = wl_compositor_create_surface( compositor );
            struct xdg_surface*  xdg_surface  = xdg_wm_base_get_xdg_surface( wm_base, wl_surface );
            struct xdg_toplevel* xdg_toplevel = xdg_surface_get_toplevel( xdg_surface );
            // Signal the compositor that you are ready to configure
            wl_surface_commit( wl_surface );

            // • Performance Note: To achieve raw, zero-overhead performance, configure your xdg_toplevel to fullscreen.
            // Wayland compositors detect this and activate Direct Scanout, meaning the compositor steps out of the way
            // and your Vulkan swapchain flips directly to the display scanout hardware.


            // Step C: Hand over to Vulkan
            VkWaylandSurfaceCreateInfoKHR createInfo = {};
            createInfo.sType                         = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
            createInfo.display                       = display;
            createInfo.surface                       = wl_surface;

            VkSurfaceKHR surface;
            vkCreateWaylandSurfaceKHR( instance, &createInfo, NULL, &surface );
            */
        }
    };
}; // namespace VKE::Platform::Backend