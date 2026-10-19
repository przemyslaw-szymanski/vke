#pragma once

#include <windows.h>

namespace VKE::Input
{
    using Keyboard     = ::RAWINPUTDEVICE;
    using Mouse        = ::RAWINPUTDEVICE;
    using GamePad      = ::RAWINPUTDEVICE;
    using Joystick     = ::RAWINPUTDEVICE;
    using DeviceHandle = ::HANDLE;
}; // namespace VKE::Input