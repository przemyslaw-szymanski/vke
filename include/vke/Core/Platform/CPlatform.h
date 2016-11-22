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

            struct VKE_API Thread
            {
                using ID = uint32_t;
                using AtomicType = volatile unsigned long long;
                static const ID UNKNOWN_THREAD_ID = static_cast<ID>(-1);

                class VKE_API CSpinlock
                {
                     
                    public:

                        void Lock();
                        void Unlock();
                        bool TryLock();
                        bool IsLocked() const { return m_threadId != UNKNOWN_THREAD_ID; }

                    private:

                        AtomicType  m_threadId = UNKNOWN_THREAD_ID;
                        uint32_t    m_lockCount = 0;
                };

                static ID GetID();
                static ID GetID(const handle_t& hThread);
                static ID GetID(void* pHandle);
                static void Sleep(uint32_t milliseconds);
                static void Yield();
                static void MemoryBarrier();
            };
            using ThisThread = Thread;
    };
} // vke