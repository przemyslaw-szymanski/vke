#pragma once

// #include <X11/Xlib-xcb.h>
#include <xcb/xcb.h>
// #include <vulkan/vulkan_xcb.h>

namespace VKE::Platform::Backend
{
    class CX11
    {
    public:
        static bool IsSupported()
        {
            return true;
        }

        static bool CheckRequiredVulkanExtension()
        {
            return false;
        }

        static bool Load()
        {
            static cstr_t scLibrary[] = { "libX11.so", "libX11-xcb.so" };
            return false;
        }

        static Result Create( const SWindowDesc& Info )
        {
            // auto& XCB                  = m_pPrivate->XCB;
            // int   scr                  = -1;
            // m_pPrivate->pXcbConnection = xcb_connect( nullptr, &scr );
            // if( !XCB.pConnection )
            //{
            //     VKE_LOG_ERR( "Unable to create xcb connection." );
            //     return VKE_FAIL;
            // }

            // XCB.pSetup                = xcb_get_setup( XCB.pConnection );
            // xcb_screen_iterator_t itr = xcb_setup_roots_iterator( XCB.pSetup );

            // while( scr-- > 0 )
            //{
            //     xcb_screen_next( &itr );
            // }
            // xcb_screen_t* pXcbScreen = itr.data;
            // XCB.windowId             = xcb_generate_id( XCB.pConnection );

            // uint32_t aValues[ 2 ];
            // aValues[ 0 ] = pXcbScreen->black_pixel;
            // aValues[ 1 ] = XCB_EVENT_MASK_KEY_RELEASE | XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_STRUCTURE_NOTIFY;

            // XCB.visualId = pXcbScreen->root_visual;

            // xcb_create_window( XCB.pConnection,
            //                    XCB_COPY_FROM_PARENT,
            //                    XCB.windowId,
            //                    pXcbScreen->root,
            //                    0,
            //                    0,
            //                    m_Desc.Size.width,
            //                    m_Desc.Size.height,
            //                    0,
            //                    XCB_WINDOW_CLASS_INPUT_OUTPUT,
            //                    XCB.visualId,
            //                    XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK,
            //                    aValues );

            // xcb_intern_atom_cookie_t cookie       = xcb_intern_atom( XCB.pConnection, 1, 12, "WM_PROTOCOLS" );
            // xcb_intern_atom_reply_t* pReply       = xcb_intern_atom_reply( XCB.pConnection, cookie, 0 );
            // xcb_intern_atom_cookie_t cookie2      = xcb_intern_atom( XCB.pConnection, 0, 16, "WM_DELETE_WINDOW" );
            // auto                     deleteWindow = xcb_intern_atom_reply( XCB.pConnection, cookie2, 0 );

            return VKE_OK;
        }

    protected:
        void steps()
        {
            /*
            // Step A: Connect via X11-XCB
            Display*          dpy        = XOpenDisplay( NULL );
            xcb_connection_t* connection = XGetXCBConnection( dpy );
            XSetEventQueueOwner( dpy, XCBOwn_ViaXCB ); // Hand event queue control to XCB

            // Step B: Create the XCB window
            xcb_screen_t* screen = xcb_setup_roots_iterator( xcb_get_setup( connection ) ).data;
            xcb_window_t  window = xcb_generate_id( connection );

            uint32_t value_mask      = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
            uint32_t value_list[ 2 ] = { screen->white_pixel,
                                         XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_KEY_PRESS |
                                             XCB_EVENT_MASK_STRUCTURE_NOTIFY };

            // Performance Note: To avoid tearing on X11 without relying on the desktop window manager's compositor
            // (which adds lag), you must use Vulkan's VK_PRESENT_MODE_FIFO_KHR or query for VK_PRESENT_MODE_MAILBOX_KHR
            // (triple buffering) for tear-free, low-latency rendering.

            xcb_create_window( connection,
                               XCB_COPY_FROM_PARENT,
                               window,
                               screen->root,
                               0,
                               0,
                               width,
                               height,
                               0,
                               XCB_WINDOW_CLASS_INPUT_OUTPUT,
                               screen->root_visual,
                               value_mask,
                               value_list );
            xcb_map_window( connection, window );
            xcb_flush( connection );

            // Step C: Hand over to Vulkan
            VkXcbSurfaceCreateInfoKHR createInfo = {};
            createInfo.sType                     = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
            createInfo.connection                = connection;
            createInfo.window                    = window;

            VkSurfaceKHR surface;
            vkCreateXcbSurfaceKHR( instance, &createInfo, NULL, &surface );
            */
        }
    };
}; // namespace VKE::Platform::Backend