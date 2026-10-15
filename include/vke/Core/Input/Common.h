#pragma once

#include "Core/VKECommon.h"
#include "Core/Utils/TCBitSetArray.h"

#if VKE_WINDOWS
#include <windows.h>
#endif // VKE_WINDOWS

namespace VKE::Input
{
    struct DeviceTypes
    {
        enum TYPE
        {
            MOUSE,
            KEYBOARD,
            GAMEPAD,
            JOYSTICK,
            HUMAN_INTERFACE_DEVICE,
            _MAX_COUNT
        };
    };

    using DEVICE_TYPE = DeviceTypes::TYPE;

    struct Keys
    {
        enum KEY : uint8_t
        {
            UNKNOWN = 0x0u,
            LBUTTON,
            RBUTTON,
            CANCEL,
            MBUTTON,
            X_BUTTON1,
            X_BUTTON2,
            BACK,
            TAB,
            CLEAR,
            ENTER,
            SHIFT,
            CONTROL,
            ALT,
            PAUSE,
            CAPSLOCK,
            ESC,
            SPACE,
            PAGE_UP,
            PAGE_DOWN,
            END,
            HOME,
            LEFT,
            UP,
            RIGHT,
            DOWN,
            SELECT,
            PRINT,
            EXECUTE,
            PRINT_SCREEN,
            INSERT,
            DEL,
            HELP,
            NUM_0,
            NUM_1,
            NUM_2,
            NUM_3,
            NUM_4,
            NUM_5,
            NUM_6,
            NUM_7,
            NUM_8,
            NUM_9,
            A,
            B,
            C,
            D,
            E,
            F,
            G,
            H,
            I,
            J,
            K,
            L,
            M,
            N,
            O,
            P,
            Q,
            R,
            S,
            T,
            U,
            V,
            W,
            X,
            Y,
            Z,
            LEFT_WINDOWS,
            RIGHT_WINDOW,
            SLEEP,
            NUMPAD_0,
            NUMPAD_1,
            NUMPAD_2,
            NUMPAD_3,
            NUMPAD_4,
            NUMPAD_5,
            NUMPAD_6,
            NUMPAD_7,
            NUMPAD_8,
            NUMPAD_9,
            NUMPAD_PLUS,
            NUMPAD_MINUS,
            NUMPAD_MULTIPLY,
            NUMPAD_SLASH,
            MULTIPLY,
            ADD,
            SEPARATOR,
            SUBTRACT,
            DECIMAL,
            DIVIDE,
            F1,
            F2,
            F3,
            F4,
            F5,
            F6,
            F7,
            F8,
            F9,
            F10,
            F11,
            F12,
            NUMLOCK,
            SCROLL,
            LEFT_SHIFT,
            RIGHT_SHIFT,
            LEFT_CONTROL,
            RIGHT_CONTROL,
            LEFT_ALT,
            RIGHT_ALT,
            VOLUME_UP,
            VOLUME_DOWN,
            VOLUME_MUTE,

            _MAX_COUNT
        };
    };

    using KEY = Keys::KEY;

    struct MouseButtons
    {
        enum BUTTON
        {
            UNKNOWN,
            BUTTON_1,
            BUTTON_2,
            BUTTON_3,
            BUTTON_4,
            BUTTON_5,
            WHEEL,
            _MAX_COUNT,
            LEFT   = BUTTON_1,
            RIGHT  = BUTTON_2,
            MIDDLE = BUTTON_3
        };
    };

    using MOUSE_BUTTON = MouseButtons::BUTTON;

    struct MouseButtonStates
    {
        enum STATE
        {
            BUTTON_1_DOWN      = VKE_BIT( 0 ),
            BUTTON_1_UP        = VKE_BIT( 1 ),
            BUTTON_2_DOWN      = VKE_BIT( 2 ),
            BUTTON_2_UP        = VKE_BIT( 3 ),
            BUTTON_3_DOWN      = VKE_BIT( 4 ),
            BUTTON_3_UP        = VKE_BIT( 5 ),
            BUTTON_4_DOWN      = VKE_BIT( 6 ),
            BUTTON_4_UP        = VKE_BIT( 7 ),
            BUTTON_5_DOWN      = VKE_BIT( 8 ),
            BUTTON_5_UP        = VKE_BIT( 9 ),
            WHEEL_MOVE         = VKE_BIT( 10 ),
            HWHEEL_MOVE        = VKE_BIT( 11 ),
            LEFT_BUTTON_DOWN   = BUTTON_1_DOWN,
            LEFT_BUTTON_UP     = BUTTON_1_UP,
            RIGHT_BUTTON_DOWN  = BUTTON_2_DOWN,
            RIGHT_BUTTON_UP    = BUTTON_2_UP,
            MIDDLE_BUTTON_DOWN = BUTTON_3_DOWN,
            MIDDLE_BUTTON_UP   = BUTTON_3_UP,
            _MAX_COUNT         = 12
        };
    };

    using MOUSE_BUTTON_STATE = uint16_t;

    using MousePosition = ExtentI16;

    static MOUSE_BUTTON_STATE vke_force_inline ConvertMouseButtonToMouseButtonState( const MOUSE_BUTTON& button )
    {
        static const MOUSE_BUTTON_STATE aStates[] = {
            0, // unkonwn
            MouseButtonStates::BUTTON_1_DOWN,
            MouseButtonStates::BUTTON_2_DOWN,
            MouseButtonStates::BUTTON_3_DOWN,
            MouseButtonStates::BUTTON_4_DOWN,
            MouseButtonStates::BUTTON_5_DOWN,
            MouseButtonStates::WHEEL_MOVE,
        };
        return aStates[ button ];
    }

    // DOWN bits are even (0,2,4,6,8); matching UP bit is DOWN << 1
    static constexpr MOUSE_BUTTON_STATE MOUSE_BUTTONS_DOWN_MASK =
        MouseButtonStates::BUTTON_1_DOWN | MouseButtonStates::BUTTON_2_DOWN | MouseButtonStates::BUTTON_3_DOWN |
        MouseButtonStates::BUTTON_4_DOWN | MouseButtonStates::BUTTON_5_DOWN;
    static constexpr MOUSE_BUTTON_STATE MOUSE_BUTTONS_UP_MASK =
        static_cast< MOUSE_BUTTON_STATE >( MOUSE_BUTTONS_DOWN_MASK << 1 );
    static constexpr MOUSE_BUTTON_STATE MOUSE_WHEELS_MASK =
        MouseButtonStates::WHEEL_MOVE | MouseButtonStates::HWHEEL_MOVE;

    // Accumulates button transitions: DOWN bits stay set while a button is held, UP bit marks the last release
    static vke_force_inline MOUSE_BUTTON_STATE ApplyMouseButtonTransitions( const MOUSE_BUTTON_STATE current,
                                                                            const MOUSE_BUTTON_STATE downs,
                                                                            const MOUSE_BUTTON_STATE ups )
    {
        MOUSE_BUTTON_STATE ret = static_cast< MOUSE_BUTTON_STATE >( ( current | downs ) & ~( downs << 1 ) );
        ret                    = static_cast< MOUSE_BUTTON_STATE >( ( ret & ~ups ) | ( ups << 1 ) );
        return ret;
    }

#if VKE_WINDOWS
    using Keyboard     = ::RAWINPUTDEVICE;
    using Mouse        = ::RAWINPUTDEVICE;
    using GamePad      = ::RAWINPUTDEVICE;
    using Joystick     = ::RAWINPUTDEVICE;
    using DeviceHandle = ::HANDLE;
#elif VKE_LINUX
    using Keyboard     = void*;
    using Mouse        = void*;
    using GamePad      = void*;
    using Joystick     = void*;
    using DeviceHandle = void*;
#endif // VKE_WINDOWS

    struct SMouseState
    {
        ExtentI16          Position     = { 0, 0 };
        ExtentI16          LastPosition = { 0, 0 };
        ExtentI16          Move         = { 0, 0 };
        ExtentI16          LastMove     = { 0, 0 };
        MOUSE_BUTTON_STATE buttonState  = 0;
        int16_t            wheelMove;

        VKE::Utils::TCBitSetArray< uint8_t, 1 > aButtons;

        bool IsButtonDown( const MOUSE_BUTTON& button ) const
        {
            // return buttonState[ ConvertMouseButtonToMouseButtonState( button ) ];
            return buttonState & ConvertMouseButtonToMouseButtonState( button );
        }
    };

    struct SKeyboardState
    {
        VKE::Utils::TCBitSetArray< uint64_t, 4 > aKeys;

        // bool aKeys[ 256 ];

        bool IsKeyDown( const KEY& key ) const
        {
            return aKeys[ key ];
        }
    };

    struct SInputState
    {
        SMouseState    Mouse;
        SKeyboardState Keyboard;
    };
} // namespace VKE::Input