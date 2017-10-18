#pragma once

#include "Common.h"

namespace VKE
{
    class VKE_API Platform
    {
        public:

            struct Architectures
            {
                enum _ARCHITECTURE
                {
                    UNKNOWN,
                    X86,
                    X64,
                    ARM32,
                    ARM64
                };
            };
            using ARCHITECTURE = Architectures::_ARCHITECTURE;

            struct SProcessorInfo
            {
                uint16_t count = 0;
                uint16_t coreCount = 0;
                uint16_t logicalCoreCount = 0;
                uint32_t l1CacheSize = 0;
                uint32_t l2CacheSize = 0;
                uint32_t l3CacheSize = 0;
                ARCHITECTURE architecture = Architectures::UNKNOWN;
            };

        protected:

            static SProcessorInfo m_ProcessorInfo;

        public:

            static const SProcessorInfo& GetProcessorInfo();
            
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

                        CSpinlock() {}

                        vke_force_inline
                        CSpinlock(const CSpinlock& Other) :
                            m_threadId(Other.m_threadId),
                            m_lockCount(Other.m_lockCount)
                        {}
                        

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
                static void Pause();
                static void MemoryBarrier();
                static uint32_t GetMaxConcurrentThreadCount();
            };
            using ThisThread = Thread;
    };
} // vke