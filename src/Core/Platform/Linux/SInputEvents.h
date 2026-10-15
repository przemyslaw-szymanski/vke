#pragma once

#include "Core/Platform/Common.h"
#include "Core/Utils/TCDynamicArray.h"

namespace VKE::Platform::Linux
{
    // Input record produced by the X11 event pump (XInput2 raw events + core pointer position),
    // consumed by CInputSystem::Update().
    struct SInputEvent
    {
        enum TYPE : uint8_t
        {
            KEY_DOWN,
            KEY_UP,
            BUTTON_DOWN,
            BUTTON_UP,
            RAW_MOTION,
            POSITION,
            RESET // focus lost: release everything held
        };

        TYPE     type     = KEY_DOWN;
        uint16_t deviceId = 0;
        uint32_t detail   = 0; // X keycode (evdev + 8) or X button number
        float    dx       = 0.0f;
        float    dy       = 0.0f;
        int16_t  x        = 0;
        int16_t  y        = 0;
    };

    using InputEventArray = Utils::TCDynamicArray< SInputEvent, 256 >;
} // namespace VKE::Platform::Linux
