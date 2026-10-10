#pragma once

#include <wayland-client.h>

// #include <xdg-shell-client-protocol.h>
// #include <vulkan/vulkan_wayland.h>

namespace VKE::Platform::Backend
{
    class CWayland
    {
    public:
        static bool IsSupported()
        {
            const char* pSessionType = getenv( "XDG_SESSION_TYPE" );
            return ( pSessionType && strcmp( pSessionType, "wayland" ) == 0 );
        }

        static bool CheckRequiredVulkanExtension()
        {
            return false;
        }

        static bool Load()
        {
            static cstr_t scLibrary[] = { "libwayland-client.so" };
            return false;
        }

        static Result Create( const SWindowDesc& Info )
        {
            return VKE_OK;
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