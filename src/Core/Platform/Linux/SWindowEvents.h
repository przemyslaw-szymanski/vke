#pragma once

#include "Core/Platform/Common.h"

namespace VKE::Platform::Linux
{
    // Window events collected by a backend event pump, consumed by the owning CWindow.
    struct SWindowEvents
    {
        uint16_t width          = 0;
        uint16_t height         = 0;
        bool     closeRequested = false;
        bool     resized        = false;
        bool     mapped         = false;
        bool     focused        = false;
    };
} // namespace VKE::Platform::Linux
