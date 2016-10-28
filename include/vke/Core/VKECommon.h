#pragma once

#include "VKEPreprocessor.h"
#include "VKEErrorHandling.h"
#include "Core/VKETypes.h"

namespace VKE
{
    namespace Constants
    {
        static const uint32_t   ENGINE_VERSION = 1000;
        static const cstr_t     ENGINE_NAME = "Vulkan Engine";
        static const uint32_t   MAX_NAME_LENGTH = 256;

        namespace Thread
        {
            static const int32_t ID_BALANCED = -1;
            static const int32_t COUNT_OPTIMAL = 0xFFFFFFFF;
        } // Thread

        namespace RenderSystem
        {
            static const uint32_t MAX_RENDER_QUEUES = 1;
            static const uint32_t MAX_PHYSICAL_DEVICES = 10;
            static const uint32_t MAX_PHYSICAL_DEVICES_QUEUE_FAMILY_PROPERTIES = 10;
            static const uint32_t MAX_EXTENSION_COUNT = 10;
            static const uint32_t MAX_SWAP_CHAIN_ELEMENTS = 10;
            static const uint32_t DEFAULT_DRAWCALL_COUNT = 1000;
        } // RenderSystem

        struct _SOptimal
        {
            template<typename _T_>
            static const _T_ Value() { return std::numeric_limits< _T_ >::max(); }
            operator uint16_t() const { return Value<uint16_t>(); }
            operator uint32_t() const { return Value<uint32_t>(); }
            template<typename _T_>
            static bool IsOptimal(const _T_& v) { return v == Value<_T_>(); }
        };

        static _SOptimal OPTIMAL;
#define VKE_OPTIMAL (~0UL)

    } // Constants

#define VKE_SET_BIT(_bit) (1ULL << (_bit))


    namespace Thread
    {
        using LockGuard = std::lock_guard< std::mutex >;
        using UniqueLock = std::unique_lock< std::mutex >;

        class CTryLock final
        {
            public:
                CTryLock(std::mutex& m) : m_Mutex(m)
                {
                    locked = m_Mutex.try_lock();
                }
                ~CTryLock()
                {
                    if(locked)
                        m_Mutex.unlock();
                }
                void operator=(const CTryLock&) = delete;
            private:
                std::mutex& m_Mutex;
                bool locked;
        };

        using TryLock = CTryLock;
    } // Thread
} // VKE