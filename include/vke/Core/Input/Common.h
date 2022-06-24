#pragma once

#include "Core/VKECommon.h"

#if VKE_WINDOWS
#include <windows.h>
#endif // VKE_WINDOWS

namespace VKE
{
    namespace Input
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
            enum KEY
            {
                UNKNOWN,
                LBUTTON         = VK_LBUTTON,
                RBUTTON         = VK_RBUTTON,
                CANCEL          = VK_CANCEL,
                MBUTTON         = VK_MBUTTON,
                X_BUTTON1       = VK_XBUTTON1,
                X_BUTTON2       = VK_XBUTTON2,
                BACK            = VK_BACK,
                TAB             = VK_TAB,
                CLEAR           = VK_CLEAR,
                ENTER           = VK_RETURN,
                SHIFT           = VK_SHIFT,
                CONTROL         = VK_CONTROL,
                ALT             = VK_MENU,
                PAUSE           = VK_PAUSE,
                CAPSLOCK        = VK_CAPITAL,
                ESC             = VK_ESCAPE,
                SPACE           = VK_SPACE,
                PAGE_UP         = VK_PRIOR,
                PAGE_DOWN       = VK_NEXT,
                END             = VK_END,
                HOME            = VK_HOME,
                LEFT            = VK_LEFT,
                UP              = VK_UP,
                RIGHT           = VK_RIGHT,
                DOWN            = VK_DOWN,
                SELECT          = VK_SELECT,
                PRINT           = VK_PRINT,
                EXECUTE         = VK_EXECUTE,
                PRINT_SCREEN    = VK_SNAPSHOT,
                INSERT          = VK_INSERT,
                DEL             = VK_DELETE,
                HELP            = VK_HELP,
                NUM_0           = 0x30,
                NUM_1,
                NUM_2,
                NUM_3,
                NUM_4,
                NUM_5,
                NUM_6,
                NUM_7,
                NUM_8,
                NUM_9,
                A       = 0x41,
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
                LEFT_WINDOWS    = VK_LWIN,
                RIGHT_WINDOW    = VK_RWIN,
                SLEEP           = VK_SLEEP,
                NUMPAD_0        = VK_NUMPAD0,
                NUMPAD_1,
                NUMPAD_2,
                NUMPAD_3,
                NUMPAD_4,
                NUMPAD_5,
                NUMPAD_6,
                NUMPAD_7,
                NUMPAD_8,
                NUMPAD_9,
                NUMPAD_PLUS     = VK_ADD,
                NUMPAD_MINUS    = VK_SUBTRACT,
                NUMPAD_MULTIPLY = VK_MULTIPLY,
                NUMPAD_SLASH    = VK_DIVIDE,
                MULTIPLY        = VK_MULTIPLY,
                ADD             = VK_ADD,
                SEPARATOR       = VK_SEPARATOR,
                SUBTRACT        = VK_SUBTRACT,
                DECIMAL         = VK_DECIMAL,
                DIVIDE          = VK_DIVIDE,
                F1              = VK_F1,
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
                NUMLOCK         = VK_NUMLOCK,
                SCROLL          = VK_SCROLL,
                LEFT_SHIFT      = VK_LSHIFT,
                RIGHT_SHIFT     = VK_RSHIFT,
                LEFT_CONTROL    = VK_LCONTROL,
                RIGHT_CONTROL   = VK_RCONTROL,
                LEFT_ALT        = VK_LMENU,
                RIGHT_ALT       = VK_RMENU,
                VOLUME_UP       = VK_VOLUME_UP,
                VOLUME_DOWN     = VK_VOLUME_DOWN,
                VOLUME_MUTE     = VK_VOLUME_MUTE,

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
                LEFT = BUTTON_1,
                RIGHT = BUTTON_2,
                MIDDLE = BUTTON_3
            };
        };
        using MOUSE_BUTTON = MouseButtons::BUTTON;

        struct MouseButtonStates
        {
            enum STATE
            {
                BUTTON_1_DOWN = VKE_BIT( 0 ),
                BUTTON_1_UP = VKE_BIT( 1 ),
                BUTTON_2_DOWN = VKE_BIT( 2 ),
                BUTTON_2_UP = VKE_BIT( 3 ),
                BUTTON_3_DOWN = VKE_BIT( 4 ),
                BUTTON_3_UP = VKE_BIT( 5 ),
                BUTTON_4_DOWN = VKE_BIT( 6 ),
                BUTTON_4_UP = VKE_BIT( 7 ),
                BUTTON_5_DOWN = VKE_BIT( 8 ),
                BUTTON_5_UP = VKE_BIT( 9 ),
                WHEEL_MOVE = VKE_BIT( 10 ),
                HWHEEL_MOVE = VKE_BIT( 11 ),
                LEFT_BUTTON_DOWN = BUTTON_1_DOWN,
                LEFT_BUTTON_UP = BUTTON_1_UP,
                RIGHT_BUTTON_DOWN = BUTTON_2_DOWN,
                RIGHT_BUTTON_UP = BUTTON_2_UP,
                MIDDLE_BUTTON_DOWN = BUTTON_3_DOWN,
                MIDDLE_BUTTON_UP = BUTTON_3_UP,
                _MAX_COUNT = 12
            };
        };
        using MOUSE_BUTTON_STATE = uint16_t;

        using MousePosition = ExtentI16;

        static MOUSE_BUTTON_STATE vke_force_inline ConvertMouseButtonToMouseButtonState( const MOUSE_BUTTON& button )
        {
            static const MOUSE_BUTTON_STATE aStates[] =
            {
                0, // unkonwn
                MouseButtonStates::BUTTON_1_DOWN,
                MouseButtonStates::BUTTON_2_DOWN,
                MouseButtonStates::BUTTON_3_DOWN,
                MouseButtonStates::BUTTON_4_DOWN,
                MouseButtonStates::BUTTON_5_DOWN,
                MouseButtonStates::WHEEL_MOVE
            };
            return aStates[button];
        }

    } // Input
} // VKE

#if VKE_WINDOWS
namespace VKE
{
    namespace Input
    {
        using Keyboard  = ::RAWINPUTDEVICE;
        using Mouse     = ::RAWINPUTDEVICE;
        using GamePad   = ::RAWINPUTDEVICE;
        using Joystick  = ::RAWINPUTDEVICE;
        using DeviceHandle = ::HANDLE;
    } // Input
} // VKE
#endif // VKE_WINDOWS

#if VKE_USE_XINPUT
namespace VKE
{
    namespace Input
    {

    } // Input
} // VKE
#endif // VKE_USE_XINPUT

namespace VKE
{
    namespace Input
    {
        struct SMouseState
        {
            ExtentI16           Position = { 0,0 };
            ExtentI16           LastPosition = { 0,0 };
            ExtentI16           Move = { 0,0 };
            ExtentI16           LastMove = { 0,0 };
            MOUSE_BUTTON_STATE  buttonState = 0;
            int16_t             wheelMove;

            bool IsButtonDown( const MOUSE_BUTTON& button ) const { return buttonState & ConvertMouseButtonToMouseButtonState( button ); }
        };

        struct SKeyboardState
        {
            bool    aKeys[256];

            bool    IsKeyDown( const KEY& key ) const { return aKeys[key]; }
        };

        struct SInputState
        {
            SMouseState      Mouse;
            SKeyboardState   Keyboard;
        };
    } // Input
} // VKE