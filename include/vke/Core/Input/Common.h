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
                HID,
                _MAX_COUNT
            };
        };
        using DEVICE_TYPE = DeviceTypes::TYPE;

        struct Keys
        {
            enum KEY
            {
                UNKNOWN,
                CAPITAL_Q,
                CAPITAL_W,
                CAPITAL_E,
                CAPITAL_R,
                CAPITAL_T,
                CAPITAL_Y,
                CAPITAL_U,
                CAPITAL_I,
                CAPITAL_O,
                CAPITAL_P,
                CAPITAL_A,
                CAPITAL_S,
                CAPITAL_D,
                CAPITAL_F,
                CAPITAL_G,
                CAPITAL_H,
                CAPITAL_J,
                CAPITAL_K,
                CAPITAL_L,
                CAPITAL_Z,
                CAPITAL_X,
                CAPITAL_C,
                CAPITAL_V,
                CAPITAL_B,
                CAPITAL_N,
                CAPITAL_M,
                Q,
                W,
                E,
                R,
                T,
                Y,
                U,
                I,
                O,
                P,
                A,
                S,
                D,
                F,
                G,
                H,
                J,
                K,
                L,
                Z,
                X,
                C,
                V,
                B,
                N,
                M,
                _MAX_COUNT
            };
        };
        using KEY = Keys::KEY;

        struct MouseButtons
        {
            enum BUTTON
            {
                UNKNOWN,
                LEFT,
                RIGHT,
                WHEEL,
                _MAX_COUNT
            };
        };
        using MOUSE_BUTTON = MouseButtons::BUTTON;

        using MousePosition = ExtentI16;

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