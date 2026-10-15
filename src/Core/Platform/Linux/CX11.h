#pragma once

#include "Core/Platform/Common.h"
#include "Core/Platform/Linux/SWindowEvents.h"
#include "Core/Platform/Linux/SInputEvents.h"
#include "Core/Utils/CLogger.h"

#include <xcb/xcb.h>
#include <xcb/xinput.h>

namespace VKE
{
    class CWindow;
}

namespace VKE::Platform::Linux
{
    struct SX11Config
    {
        // Defaults - nullptr uses DISPLAY:0, -1 is default screen
        cstr_t pDisplayName = nullptr;
        int    screenIndex  = -1;
    };

    struct SX11Display
    {
        xcb_connection_t* pConnection = nullptr;
        xcb_screen_t*     pScreen     = nullptr;
        xcb_visualid_t    visualId    = 0;
        int               screenIndex = 0;

        xcb_atom_t wmProtocols          = XCB_ATOM_NONE;
        xcb_atom_t wmDeleteWindow       = XCB_ATOM_NONE;
        xcb_atom_t netWmName            = XCB_ATOM_NONE;
        xcb_atom_t utf8String           = XCB_ATOM_NONE;
        xcb_atom_t netWmState           = XCB_ATOM_NONE;
        xcb_atom_t netWmStateFullscreen = XCB_ATOM_NONE;

        uint8_t xiOpcode = 0;
        bool    hasXI2   = false;

        bool IsValid() const
        {
            return pConnection != nullptr && pScreen != nullptr;
        }
    };

    struct SX11
    {
        const SX11Display* pDisplay = nullptr;
        xcb_window_t       windowId = XCB_WINDOW_NONE;
        SWindowEvents      Events;

        bool IsValid() const
        {
            return pDisplay != nullptr && pDisplay->IsValid() && windowId != XCB_WINDOW_NONE;
        }
    };

    class CX11
    {
        // Absolute pointers (e.g. WSLg/RDP, tablets) report positions in raw events: track last value to get deltas
        struct SPointerAxes
        {
            bool  aIsAbsolute[ 2 ] = { false, false };
            bool  aHasLast[ 2 ]    = { false, false };
            float aLast[ 2 ]       = { 0.0f, 0.0f };
        };

        struct SState
        {
            SX11Config                                Config;
            SX11Display                               Display;
            std::unordered_map< xcb_window_t, SX11* > mWindows;
            std::unordered_map< uint16_t, SPointerAxes > mPointerAxes;
            InputEventArray                           vInputEvents;
            uint32_t                                  focusedCount = 0;
            std::mutex                                Mutex;
            uint32_t                                  refCount = 0;
        };

        static SState& _GetState()
        {
            static SState sState;
            return sState;
        }

    public:
        static bool IsSupported()
        {
            return true;
        }

        // Optional. Effective only before the connection is opened.
        static void SetConfig( const SX11Config& Config )
        {
            SState& State = _GetState();

            std::lock_guard< std::mutex > l( State.Mutex );
            if( State.refCount == 0 )
            {
                State.Config = Config;
            }
            else
            {
                VKE_LOG_WARN( "X11 connection already opened. Config change ignored." );
            }
        }

        // Shared display. Valid between the first Load() and the last Unload().
        static const SX11Display& GetDisplay()
        {
            return _GetState().Display;
        }

        // Moves all queued input events into pOut (pOut is cleared first). Thread safe.
        static void ConsumeInputEvents( InputEventArray* pOut )
        {
            SState&                       State = _GetState();
            std::lock_guard< std::mutex > l( State.Mutex );
            pOut->Clear();
            pOut->Append( State.vInputEvents );
            State.vInputEvents.Clear();
        }

        static bool HasXI2()
        {
            return _GetState().Display.hasXI2;
        }

        static const SX11* FindWindow( xcb_window_t windowId )
        {
            const SX11*                   pRet  = nullptr;
            SState&                       State = _GetState();
            std::lock_guard< std::mutex > l( State.Mutex );
            const auto                    Itr = State.mWindows.find( windowId );
            if( Itr != State.mWindows.end() )
            {
                pRet = Itr->second;
            }
            return pRet;
        }

        // Opens the shared connection on first call, otherwise only increments the reference count.
        static Result Load( SX11& X11Info )
        {
            Result                        ret   = VKE_OK;
            SState&                       State = _GetState();
            std::lock_guard< std::mutex > l( State.Mutex );
            if( X11Info.pDisplay == nullptr )
            {
                if( State.refCount == 0 )
                {
                    ret = _Connect( State );
                }
                if( VKE_SUCCEEDED( ret ) )
                {
                    ++State.refCount;
                    X11Info.pDisplay = &State.Display;
                }
            }
            return ret;
        }

        static void Unload( SX11& X11Info )
        {
            SState&                       State = _GetState();
            std::lock_guard< std::mutex > l( State.Mutex );
            if( X11Info.pDisplay != nullptr && State.refCount > 0 )
            {
                --State.refCount;
                if( State.refCount == 0 )
                {
                    xcb_disconnect( State.Display.pConnection );
                    State.Display = {};
                    State.mWindows.clear();
                    State.vInputEvents.Clear();
                    State.focusedCount = 0;
                }
            }
            X11Info.pDisplay = nullptr;
        }

        static Result Create( SX11& X11Info, const SWindowDesc& WindowDesc )
        {
            Result ret = VKE_FAIL;
            if( X11Info.pDisplay != nullptr && X11Info.pDisplay->IsValid() )
            {
                const SX11Display& Display = *X11Info.pDisplay;
                xcb_connection_t*  pConn   = Display.pConnection;
                xcb_screen_t*      pScreen = Display.pScreen;

                const bool     isFullscreen = WindowDesc.mode != WindowModes::WINDOW;
                const uint16_t width        = isFullscreen ? pScreen->width_in_pixels : WindowDesc.Size.width;
                const uint16_t height       = isFullscreen ? pScreen->height_in_pixels : WindowDesc.Size.height;
                int16_t        x            = 0;
                int16_t        y            = 0;
                if( !isFullscreen )
                {
                    x = WindowDesc.Position.x == UNDEFINED_U16
                            ? static_cast< int16_t >( ( pScreen->width_in_pixels - width ) / 2 )
                            : static_cast< int16_t >( WindowDesc.Position.x );
                    y = WindowDesc.Position.y == UNDEFINED_U16
                            ? static_cast< int16_t >( ( pScreen->height_in_pixels - height ) / 2 )
                            : static_cast< int16_t >( WindowDesc.Position.y );
                }

                const uint32_t valueMask    = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
                const uint32_t aValues[ 2 ] = { pScreen->black_pixel,
                                                XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE |
                                                    XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE |
                                                    XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_STRUCTURE_NOTIFY |
                                                    XCB_EVENT_MASK_FOCUS_CHANGE | XCB_EVENT_MASK_EXPOSURE };

                const xcb_window_t      windowId = xcb_generate_id( pConn );
                const xcb_void_cookie_t Cookie   = xcb_create_window_checked( pConn,
                                                                              XCB_COPY_FROM_PARENT,
                                                                              windowId,
                                                                              pScreen->root,
                                                                              x,
                                                                              y,
                                                                              width,
                                                                              height,
                                                                              0,
                                                                              XCB_WINDOW_CLASS_INPUT_OUTPUT,
                                                                              Display.visualId,
                                                                              valueMask,
                                                                              aValues );
                if( _CheckError( "xcb_create_window_checked", pConn, Cookie ) )
                {
                    X11Info.windowId      = windowId;
                    X11Info.Events        = {};
                    X11Info.Events.width  = width;
                    X11Info.Events.height = height;

                    // Handle the WM close button instead of being killed by the WM
                    if( Display.wmProtocols != XCB_ATOM_NONE && Display.wmDeleteWindow != XCB_ATOM_NONE )
                    {
                        xcb_change_property( pConn,
                                             XCB_PROP_MODE_REPLACE,
                                             windowId,
                                             Display.wmProtocols,
                                             XCB_ATOM_ATOM,
                                             32,
                                             1,
                                             &Display.wmDeleteWindow );
                    }
                    if( isFullscreen && Display.netWmState != XCB_ATOM_NONE )
                    {
                        // Set before mapping; the WM applies it on map
                        xcb_change_property( pConn,
                                             XCB_PROP_MODE_REPLACE,
                                             windowId,
                                             Display.netWmState,
                                             XCB_ATOM_ATOM,
                                             32,
                                             1,
                                             &Display.netWmStateFullscreen );
                    }
                    SetWindowTitle( X11Info, WindowDesc.pTitle );
                    {
                        // WM_CLASS is "instance\0class\0": used by taskbars/docks to group windows and pick the icon
                        static constexpr char aWmClass[] = "vke\0VKE";
                        xcb_change_property( pConn,
                                             XCB_PROP_MODE_REPLACE,
                                             windowId,
                                             XCB_ATOM_WM_CLASS,
                                             XCB_ATOM_STRING,
                                             8,
                                             sizeof( aWmClass ),
                                             aWmClass );
                    }
                    {
                        SState&                       State = _GetState();
                        std::lock_guard< std::mutex > l( State.Mutex );
                        State.mWindows[ windowId ] = &X11Info;
                    }
                    xcb_flush( pConn );
                    ret = VKE_OK;
                }
            }
            else
            {
                VKE_LOG_ERR( "X11 display is not loaded." );
            }
            return ret;
        }

        static Result Destroy( SX11& X11Info )
        {
            if( X11Info.IsValid() )
            {
                {
                    SState&                       State = _GetState();
                    std::lock_guard< std::mutex > l( State.Mutex );
                    State.mWindows.erase( X11Info.windowId );
                    if( X11Info.Events.focused && State.focusedCount > 0 )
                    {
                        --State.focusedCount;
                    }
                }
                xcb_destroy_window( X11Info.pDisplay->pConnection, X11Info.windowId );
                xcb_flush( X11Info.pDisplay->pConnection );
                X11Info.windowId = XCB_WINDOW_NONE;
            }
            return VKE_OK;
        }

        static Result SetWindowTitle( SX11& X11Info, cstr_t pTitle )
        {
            Result ret = VKE_FAIL;
            if( X11Info.IsValid() && pTitle != nullptr )
            {
                const SX11Display& Display = *X11Info.pDisplay;
                const uint32_t     len     = static_cast< uint32_t >( strlen( pTitle ) );
                xcb_change_property( Display.pConnection,
                                     XCB_PROP_MODE_REPLACE,
                                     X11Info.windowId,
                                     XCB_ATOM_WM_NAME,
                                     XCB_ATOM_STRING,
                                     8,
                                     len,
                                     pTitle );
                if( Display.netWmName != XCB_ATOM_NONE && Display.utf8String != XCB_ATOM_NONE )
                {
                    xcb_change_property( Display.pConnection,
                                         XCB_PROP_MODE_REPLACE,
                                         X11Info.windowId,
                                         Display.netWmName,
                                         Display.utf8String,
                                         8,
                                         len,
                                         pTitle );
                }
                xcb_flush( Display.pConnection );
                ret = VKE_OK;
            }
            return ret;
        }

        // Confines and hides the pointer for mouse-look. Under Xwayland raw motion keeps arriving while grabbed.
        static Result GrabPointer( SX11& X11Info, bool isGrab )
        {
            Result ret = VKE_FAIL;
            if( X11Info.IsValid() )
            {
                xcb_connection_t* pConn = X11Info.pDisplay->pConnection;
                if( isGrab )
                {
                    xcb_grab_pointer_reply_t* pReply = xcb_grab_pointer_reply(
                        pConn,
                        xcb_grab_pointer( pConn,
                                          1,
                                          X11Info.windowId,
                                          XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE |
                                              XCB_EVENT_MASK_POINTER_MOTION,
                                          XCB_GRAB_MODE_ASYNC,
                                          XCB_GRAB_MODE_ASYNC,
                                          X11Info.windowId,
                                          XCB_NONE,
                                          XCB_CURRENT_TIME ),
                        nullptr );
                    if( pReply != nullptr && pReply->status == XCB_GRAB_STATUS_SUCCESS )
                    {
                        ret = VKE_OK;
                    }
                    else
                    {
                        VKE_LOG_WARN( "xcb_grab_pointer() failed." );
                    }
                    free( pReply );
                }
                else
                {
                    xcb_ungrab_pointer( pConn, XCB_CURRENT_TIME );
                    xcb_flush( pConn );
                    ret = VKE_OK;
                }
            }
            return ret;
        }

        static void IsVisible( SX11& X11Info, bool isVisible )
        {
            if( X11Info.IsValid() )
            {
                xcb_connection_t* pConn = X11Info.pDisplay->pConnection;
                if( isVisible )
                {
                    xcb_map_window( pConn, X11Info.windowId );
                }
                else
                {
                    xcb_unmap_window( pConn, X11Info.windowId );
                }
                xcb_flush( pConn );
            }
        }

        // Drains the shared connection queue and routes events to their windows.
        // Returns false if the X server connection is lost.
        static bool PumpEvents()
        {
            bool                          ret   = false;
            SState&                       State = _GetState();
            std::lock_guard< std::mutex > l( State.Mutex );
            xcb_connection_t*             pConn = State.Display.pConnection;
            if( pConn != nullptr && xcb_connection_has_error( pConn ) == 0 )
            {
                xcb_generic_event_t* pEvent = xcb_poll_for_event( pConn );
                while( pEvent != nullptr )
                {
                    _DispatchEvent( State, pEvent );
                    free( pEvent );
                    pEvent = xcb_poll_for_event( pConn );
                }
                ret = xcb_connection_has_error( pConn ) == 0;
            }
            return ret;
        }

        static cstr_t GetXcbError( uint8_t errorCode )
        {
            cstr_t pRet = "Unknown or Extension Error";
            switch( errorCode )
            {
                case XCB_REQUEST:
                    pRet = "XCB_REQUEST";
                    break;
                case XCB_VALUE:
                    pRet = "XCB_VALUE";
                    break;
                case XCB_WINDOW:
                    pRet = "XCB_WINDOW";
                    break;
                case XCB_PIXMAP:
                    pRet = "XCB_PIXMAP";
                    break;
                case XCB_ATOM:
                    pRet = "XCB_ATOM";
                    break;
                case XCB_CURSOR:
                    pRet = "XCB_CURSOR";
                    break;
                case XCB_FONT:
                    pRet = "XCB_FONT";
                    break;
                case XCB_MATCH:
                    pRet = "XCB_MATCH";
                    break;
                case XCB_DRAWABLE:
                    pRet = "XCB_DRAWABLE";
                    break;
                case XCB_ACCESS:
                    pRet = "XCB_ACCESS";
                    break;
                case XCB_ALLOC:
                    pRet = "XCB_ALLOC";
                    break;
                case XCB_G_CONTEXT:
                    pRet = "XCB_G_CONTEXT";
                    break;
                case XCB_ID_CHOICE:
                    pRet = "XCB_ID_CHOICE";
                    break;
                case XCB_NAME:
                    pRet = "XCB_NAME";
                    break;
                case XCB_LENGTH:
                    pRet = "XCB_LENGTH";
                    break;
                case XCB_IMPLEMENTATION:
                    pRet = "XCB_IMPLEMENTATION";
                    break;
                default:
                    break;
            }
            return pRet;
        }

    protected:
        static Result _Connect( SState& State )
        {
            Result            ret         = VKE_FAIL;
            int               screenIndex = 0;
            xcb_connection_t* pConn       = xcb_connect( State.Config.pDisplayName, &screenIndex );
            const int         error       = xcb_connection_has_error( pConn );
            if( error == 0 )
            {
                if( State.Config.screenIndex >= 0 )
                {
                    screenIndex = State.Config.screenIndex;
                }
                xcb_screen_iterator_t Itr = xcb_setup_roots_iterator( xcb_get_setup( pConn ) );
                for( int i = 0; i < screenIndex && Itr.rem > 0; ++i )
                {
                    xcb_screen_next( &Itr );
                }
                if( Itr.rem > 0 && Itr.data != nullptr )
                {
                    SX11Display& Display = State.Display;
                    Display.pConnection  = pConn;
                    Display.pScreen      = Itr.data;
                    Display.visualId     = Itr.data->root_visual;
                    Display.screenIndex  = screenIndex;
                    _InternAtoms( &Display );
                    _InitXI2( &Display );
                    ret = VKE_OK;
                }
                else
                {
                    VKE_LOG_ERR( "X11 screen: " << screenIndex << " not found." );
                    xcb_disconnect( pConn );
                }
            }
            else
            {
                cstr_t pDisplayName = State.Config.pDisplayName != nullptr ? State.Config.pDisplayName : getenv( "DISPLAY" );
                VKE_LOG_ERR( "X11 display not available (DISPLAY="
                             << ( pDisplayName != nullptr ? pDisplayName : "unset" ) << ", xcb error: " << error
                             << "). X11 or Xwayland is required." );
                // xcb_connect never returns nullptr; an error connection must still be released
                xcb_disconnect( pConn );
            }
            return ret;
        }

        static void _InternAtoms( SX11Display* pDisplay )
        {
            static constexpr cstr_t  aNames[] = { "WM_PROTOCOLS", "WM_DELETE_WINDOW", "_NET_WM_NAME",
                                                  "UTF8_STRING",  "_NET_WM_STATE",    "_NET_WM_STATE_FULLSCREEN" };
            xcb_atom_t* const        aOut[]   = { &pDisplay->wmProtocols, &pDisplay->wmDeleteWindow,
                                                  &pDisplay->netWmName,   &pDisplay->utf8String,
                                                  &pDisplay->netWmState,  &pDisplay->netWmStateFullscreen };
            constexpr uint32_t       count    = sizeof( aNames ) / sizeof( aNames[ 0 ] );
            xcb_intern_atom_cookie_t aCookies[ count ];

            // Issue all requests first, then collect replies: one round-trip instead of N
            for( uint32_t i = 0; i < count; ++i )
            {
                aCookies[ i ] = xcb_intern_atom(
                    pDisplay->pConnection, 0, static_cast< uint16_t >( strlen( aNames[ i ] ) ), aNames[ i ] );
            }
            for( uint32_t i = 0; i < count; ++i )
            {
                xcb_intern_atom_reply_t* pReply =
                    xcb_intern_atom_reply( pDisplay->pConnection, aCookies[ i ], nullptr );
                if( pReply != nullptr )
                {
                    *aOut[ i ] = pReply->atom;
                    free( pReply );
                }
            }
        }

        static void _InitXI2( SX11Display* pDisplay )
        {
            xcb_connection_t*                pConn = pDisplay->pConnection;
            const xcb_query_extension_reply_t* pExt  = xcb_get_extension_data( pConn, &xcb_input_id );
            if( pExt != nullptr && pExt->present )
            {
                // 2.1+ is required to receive raw events during grabs (incl. the implicit grab of a button press)
                xcb_input_xi_query_version_reply_t* pVer =
                    xcb_input_xi_query_version_reply( pConn, xcb_input_xi_query_version( pConn, 2, 2 ), nullptr );
                if( pVer != nullptr && ( pVer->major_version > 2 || ( pVer->major_version == 2 && pVer->minor_version >= 1 ) ) )
                {
                    struct
                    {
                        xcb_input_event_mask_t Head;
                        uint32_t               mask;
                    } Mask;
                    Mask.Head.deviceid = XCB_INPUT_DEVICE_ALL_MASTER;
                    Mask.Head.mask_len = 1;
                    Mask.mask = XCB_INPUT_XI_EVENT_MASK_RAW_KEY_PRESS | XCB_INPUT_XI_EVENT_MASK_RAW_KEY_RELEASE |
                                XCB_INPUT_XI_EVENT_MASK_RAW_BUTTON_PRESS | XCB_INPUT_XI_EVENT_MASK_RAW_BUTTON_RELEASE |
                                XCB_INPUT_XI_EVENT_MASK_RAW_MOTION;
                    const xcb_void_cookie_t Cookie =
                        xcb_input_xi_select_events_checked( pConn, pDisplay->pScreen->root, 1, &Mask.Head );
                    if( _CheckError( "xcb_input_xi_select_events", pConn, Cookie ) )
                    {
                        pDisplay->xiOpcode = pExt->major_opcode;
                        pDisplay->hasXI2   = true;
                        _QueryPointerAxes( pConn );
                    }
                }
                else
                {
                    VKE_LOG_WARN( "XInput 2.1 not supported by the X server. Input disabled." );
                }
                free( pVer );
            }
            else
            {
                VKE_LOG_WARN( "XInputExtension not present. Input disabled." );
            }
        }

        static void _QueryPointerAxes( xcb_connection_t* pConn )
        {
            SState&                            State  = _GetState();
            xcb_input_xi_query_device_reply_t* pReply = xcb_input_xi_query_device_reply(
                pConn, xcb_input_xi_query_device( pConn, XCB_INPUT_DEVICE_ALL ), nullptr );
            if( pReply != nullptr )
            {
                State.mPointerAxes.clear();
                xcb_input_xi_device_info_iterator_t Itr = xcb_input_xi_query_device_infos_iterator( pReply );
                for( ; Itr.rem > 0; xcb_input_xi_device_info_next( &Itr ) )
                {
                    SPointerAxes                      Axes;
                    xcb_input_device_class_iterator_t ClassItr =
                        xcb_input_xi_device_info_classes_iterator( Itr.data );
                    for( ; ClassItr.rem > 0; xcb_input_device_class_next( &ClassItr ) )
                    {
                        if( ClassItr.data->type == XCB_INPUT_DEVICE_CLASS_TYPE_VALUATOR )
                        {
                            const auto* pVal = (const xcb_input_valuator_class_t*)ClassItr.data;
                            if( pVal->number < 2 )
                            {
                                Axes.aIsAbsolute[ pVal->number ] = pVal->mode == XCB_INPUT_VALUATOR_MODE_ABSOLUTE;
                            }
                        }
                    }
                    State.mPointerAxes[ Itr.data->deviceid ] = Axes;
                }
                free( pReply );
            }
        }

        static void _ResetPointerAxes( SState& State )
        {
            for( auto& Pair: State.mPointerAxes )
            {
                Pair.second.aHasLast[ 0 ] = false;
                Pair.second.aHasLast[ 1 ] = false;
            }
        }

        static float _FP3232ToFloat( const xcb_input_fp3232_t& Value )
        {
            return static_cast< float >( Value.integral ) + static_cast< float >( Value.frac ) / 4294967296.0f;
        }

        // Raw events are delivered from the root window regardless of focus: queue only while an engine window is focused
        static void _DispatchXI2Event( SState& State, const xcb_ge_generic_event_t* pGeneric )
        {
            if( State.focusedCount > 0 )
            {
                SInputEvent Event;
                bool        isValid = true;
                switch( pGeneric->event_type )
                {
                    case XCB_INPUT_RAW_KEY_PRESS:
                    case XCB_INPUT_RAW_KEY_RELEASE: {
                        const auto* pRaw = (const xcb_input_raw_key_press_event_t*)pGeneric;
                        Event.type       = pGeneric->event_type == XCB_INPUT_RAW_KEY_PRESS ? SInputEvent::KEY_DOWN
                                                                                         : SInputEvent::KEY_UP;
                        Event.deviceId   = pRaw->sourceid;
                        Event.detail     = pRaw->detail;
                    }
                    break;
                    case XCB_INPUT_RAW_BUTTON_PRESS:
                    case XCB_INPUT_RAW_BUTTON_RELEASE: {
                        const auto* pRaw = (const xcb_input_raw_button_press_event_t*)pGeneric;
                        Event.type       = pGeneric->event_type == XCB_INPUT_RAW_BUTTON_PRESS
                                               ? SInputEvent::BUTTON_DOWN
                                               : SInputEvent::BUTTON_UP;
                        Event.deviceId   = pRaw->sourceid;
                        Event.detail     = pRaw->detail;
                    }
                    break;
                    case XCB_INPUT_RAW_MOTION: {
                        // Raw motion shares the raw_button_press layout
                        auto*                      pRaw    = (xcb_input_raw_button_press_event_t*)pGeneric;
                        const uint32_t*            pMask   = xcb_input_raw_button_press_valuator_mask( pRaw );
                        const xcb_input_fp3232_t*  pValues = xcb_input_raw_button_press_axisvalues_raw( pRaw );
                        const int                  maskLen = xcb_input_raw_button_press_valuator_mask_length( pRaw );
                        uint32_t                   valueIdx = 0;
                        SPointerAxes&              Axes     = State.mPointerAxes[ pRaw->sourceid ];
                        float                      aDelta[ 2 ] = { 0.0f, 0.0f };
                        Event.type                          = SInputEvent::RAW_MOTION;
                        Event.deviceId                      = pRaw->sourceid;
                        // Valuators 0 and 1 are X/Y; values are packed only for set mask bits
                        for( uint32_t axis = 0; axis < 2 && maskLen > 0; ++axis )
                        {
                            if( pMask[ 0 ] & ( 1u << axis ) )
                            {
                                const float v = _FP3232ToFloat( pValues[ valueIdx++ ] );
                                if( Axes.aIsAbsolute[ axis ] )
                                {
                                    aDelta[ axis ]        = Axes.aHasLast[ axis ] ? v - Axes.aLast[ axis ] : 0.0f;
                                    Axes.aLast[ axis ]    = v;
                                    Axes.aHasLast[ axis ] = true;
                                }
                                else
                                {
                                    aDelta[ axis ] = v;
                                }
                            }
                        }
                        Event.dx = aDelta[ 0 ];
                        Event.dy = aDelta[ 1 ];
                    }
                    break;
                    default:
                        isValid = false;
                        break;
                }
                if( isValid )
                {
                    State.vInputEvents.PushBack( Event );
                }
            }
        }

        static void _DispatchEvent( SState& State, const xcb_generic_event_t* pEvent )
        {
            const uint8_t type   = pEvent->response_type & 0x7F;
            xcb_window_t  window = XCB_WINDOW_NONE;
            switch( type )
            {
                case XCB_GE_GENERIC: {
                    const auto* pGeneric = (const xcb_ge_generic_event_t*)pEvent;
                    if( State.Display.hasXI2 && pGeneric->extension == State.Display.xiOpcode )
                    {
                        _DispatchXI2Event( State, pGeneric );
                    }
                }
                break;
                case XCB_MOTION_NOTIFY: {
                    const auto* pMotion = (const xcb_motion_notify_event_t*)pEvent;
                    if( State.mWindows.find( pMotion->event ) != State.mWindows.end() )
                    {
                        SInputEvent Event;
                        Event.type = SInputEvent::POSITION;
                        Event.x    = pMotion->event_x;
                        Event.y    = pMotion->event_y;
                        State.vInputEvents.PushBack( Event );
                    }
                }
                break;
                case XCB_CLIENT_MESSAGE:
                    window = ( (const xcb_client_message_event_t*)pEvent )->window;
                    break;
                case XCB_CONFIGURE_NOTIFY:
                    window = ( (const xcb_configure_notify_event_t*)pEvent )->window;
                    break;
                case XCB_MAP_NOTIFY:
                    window = ( (const xcb_map_notify_event_t*)pEvent )->window;
                    break;
                case XCB_UNMAP_NOTIFY:
                    window = ( (const xcb_unmap_notify_event_t*)pEvent )->window;
                    break;
                case XCB_FOCUS_IN:
                case XCB_FOCUS_OUT:
                    window = ( (const xcb_focus_in_event_t*)pEvent )->event;
                    break;
                case XCB_DESTROY_NOTIFY:
                    window = ( (const xcb_destroy_notify_event_t*)pEvent )->window;
                    break;
                default:
                    break;
            }

            const auto Itr = State.mWindows.find( window );
            if( Itr != State.mWindows.end() )
            {
                SX11& X11 = *Itr->second;
                switch( type )
                {
                    case XCB_CLIENT_MESSAGE: {
                        const auto* pMsg = (const xcb_client_message_event_t*)pEvent;
                        if( pMsg->type == State.Display.wmProtocols &&
                            pMsg->data.data32[ 0 ] == State.Display.wmDeleteWindow )
                        {
                            X11.Events.closeRequested = true;
                        }
                    }
                    break;
                    case XCB_CONFIGURE_NOTIFY: {
                        const auto* pCfg = (const xcb_configure_notify_event_t*)pEvent;
                        if( pCfg->width != X11.Events.width || pCfg->height != X11.Events.height )
                        {
                            X11.Events.width   = pCfg->width;
                            X11.Events.height  = pCfg->height;
                            X11.Events.resized = true;
                        }
                    }
                    break;
                    case XCB_MAP_NOTIFY:
                        X11.Events.mapped = true;
                        break;
                    case XCB_UNMAP_NOTIFY:
                        X11.Events.mapped = false;
                        break;
                    case XCB_FOCUS_IN:
                        if( !X11.Events.focused )
                        {
                            ++State.focusedCount;
                            _ResetPointerAxes( State );
                        }
                        X11.Events.focused = true;
                        break;
                    case XCB_FOCUS_OUT:
                        if( X11.Events.focused && State.focusedCount > 0 )
                        {
                            --State.focusedCount;
                            if( State.focusedCount == 0 )
                            {
                                // Releases outside a focused window are filtered: reset input state to avoid stuck keys
                                SInputEvent Event;
                                Event.type = SInputEvent::RESET;
                                State.vInputEvents.PushBack( Event );
                            }
                        }
                        X11.Events.focused = false;
                        break;
                    case XCB_DESTROY_NOTIFY:
                        X11.Events.closeRequested = true;
                        break;
                    default:
                        break;
                }
            }
        }

        static bool _CheckError( cstr_t pFunc, xcb_connection_t* pConn, xcb_void_cookie_t Cookie )
        {
            bool                 ret    = true;
            xcb_generic_error_t* pError = xcb_request_check( pConn, Cookie );
            if( pError != nullptr )
            {
                VKE_LOG_ERR( pFunc << "() failed. Error: " << GetXcbError( pError->error_code ) << " code: "
                                   << (uint32_t)pError->error_code << " major: " << (uint32_t)pError->major_code
                                   << " minor: " << (uint32_t)pError->minor_code );
                free( pError );
                ret = false;
            }
            return ret;
        }
    };

    // Typed engine-wide access to native window data. Returns nullptr if the window is not created.
    const SX11* GetX11( const CWindow* pWindow );
} // namespace VKE::Platform::Linux
