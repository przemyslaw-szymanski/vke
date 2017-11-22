#include "Core/VKECommon.h"
#if VKE_WINDOWS
#include <windows.h>
#endif // VKE_WINDOWS

namespace VKE
{
    void Assert( bool condition, cstr_t pConditionMsg, uint32_t flags, cstr_t pFile, cstr_t pFunction,
                        uint32_t line, cstr_t pMsg )
    {
        if( !condition )
        {
#if VKE_WINDOWS
            char buff[ 4096 ];
            sprintf_s( buff, sizeof( buff ), "VKE ASSERT: [%d][%s][%s][%d]:\n\"%s\"\n\"%s\"",
                        flags, pFile, pFunction, line,
                        pConditionMsg,
                        pMsg );
            ::OutputDebugStringA( buff );
            __debugbreak();
#else

#endif
        }
    }
}