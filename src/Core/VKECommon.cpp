#include "Core/VKECommon.h"
#if VKE_WINDOWS
#define NOMINMAX
#include <windows.h>
#endif // VKE_WINDOWS

namespace VKE
{
    void DebugBreak(cstr_t pBuff)
    {
#if VKE_WINDOWS
        ::OutputDebugStringA( pBuff );
        __debugbreak();
#else

#endif
    }
}