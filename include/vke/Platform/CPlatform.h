#pragma once

#include "Common.h"

namespace VKE
{
    class VKE_API Platform
    {
        public:
            
            static void     BeginDumpMemoryLeaks();
            static void     EndDumpMemoryLeaks();
            static void     Sleep(uint32_t uMilliseconds);
            static void     PrintOutput(const cstr_t msg);
            static handle_t LoadDynamicLibrary(const cstr_t name);
            static void     CloseDynamicLibrary(const handle_t& handle);
            static void*    GetProcAddress(const handle_t& handle, const void* pSymbol);
    };
} // vke