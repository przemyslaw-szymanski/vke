#pragma once

#include "Core/Platform/Linux/Backend/CWayland.h"
#include "Core/Platform/Linux/Backend/CX11.h"

namespace VKE
{
    class CWindow;
}

namespace VKE::Platform::Backend
{
    struct WindowSystems
    {
        enum TYPE : uint8_t
        {
            X11,
            WAYLAND,
            _MAX_COUNT
        };
    };
    using WINDOW_SYSTEM = WindowSystems::TYPE;

    // Selected once per process. Wayland preferred when available (HDR, VRR); X11 is the fallback.
    inline WINDOW_SYSTEM GetWindowSystem()
    {
        static const WINDOW_SYSTEM sType = CWayland::IsSupported() ? WindowSystems::WAYLAND : WindowSystems::X11;
        return sType;
    }

    // Typed engine-wide access to native window data.
    // Returns nullptr if the window is not created or uses a different window system.
    const SX11*     GetX11( const CWindow* pWindow );
    const SWayland* GetWayland( const CWindow* pWindow );
} // namespace VKE::Platform::Backend
