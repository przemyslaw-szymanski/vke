#pragma once

#include "Common.h"

namespace VKE
{
    class VKE_API Platform
    {
        public:
            
            struct Debug
            {
                static VKE_API void BeginDumpMemoryLeaks();
                static VKE_API void EndDumpMemoryLeaks();
                static VKE_API void PrintOutput(const cstr_t msg);
            };
            
            struct DynamicLibrary
            {
                static VKE_API handle_t Load(const cstr_t name);
                static VKE_API void     Close(const handle_t& handle);
                static VKE_API void*    GetProcAddress(const handle_t& handle, const void* pSymbol);
            };

            struct Time
            {
                using TimePoint = uint64_t;

                static VKE_API void         Sleep(uint32_t uMilliseconds);
                static VKE_API TimePoint    GetHighResClockFrequency();
                static VKE_API TimePoint    GetHighResClockTimePoint();
            };
    };
} // vke