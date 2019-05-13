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

    struct SWindowDesc
    {
        ExtentU32   Size = { 800, 600 };
        ExtentU32   Position = { UNDEFINED_U32, UNDEFINED_U32 };
        handle_t    hWnd;
        handle_t    hProcess;
        cstr_t      pTitle;
        void*       pUserData;
        uint32_t    threadId;
        WINDOW_MODE mode;
        bool        vSync;
    };
} // vke

#endif // __PLATFORM_COMMON_H__