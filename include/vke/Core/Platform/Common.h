#ifndef __PLATFORM_COMMON_H__
#define __PLATFORM_COMMON_H__

#include "Core/VKECommon.h"

namespace VKE
{
    struct SWindowInfo
    {
        ExtentU32   Size;
        handle_t    wndHandle;
        handle_t    platformHandle;
        cstr_t      pTitle;
        void*       pUserData;
        bool        fullScreen;
        bool        vSync;
    };
} // vke

#endif // __PLATFORM_COMMON_H__