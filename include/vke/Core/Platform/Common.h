#ifndef __PLATFORM_COMMON_H__
#define __PLATFORM_COMMON_H__

#include "Core/VKECommon.h"

namespace VKE
{
    
    struct WindowModes
    {
        enum MODE
        {
            WINDOW,
            FULLSCREEN_WINDOW,
            FULLSCREEN,
            _MAX_COUNT
        };
    };
    using WINDOW_MODE = WindowModes::MODE;

    struct SWindowInfo
    {
        ExtentU32   Size;
        ExtentU32   Position;
        handle_t    wndHandle;
        handle_t    platformHandle;
        cstr_t      pTitle;
        void*       pUserData;
        uint32_t    threadId;
        WINDOW_MODE mode;
        bool        vSync;
    };
} // vke

#endif // __PLATFORM_COMMON_H__