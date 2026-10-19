#pragma once

#include <xcb/xinput.h>

#include "Core/Utils/TCDynamicArray.h"

namespace VKE::Input
{
    using Keyboard     = void*;
    using Mouse        = void*;
    using GamePad      = void*;
    using Joystick     = void*;
    using DeviceHandle = void*;

}; // namespace VKE::Input