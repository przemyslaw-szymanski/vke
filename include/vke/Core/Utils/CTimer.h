#pragma once

#include "Core/Platform/CPlatform.h"

namespace VKE
{
    namespace Utils
    {
        class VKE_API CTimer
        {
            protected:

                struct TimeUnits
                {
                    enum UNIT
                    {
                        NANOSECONDS,
                        MICROSECONDS,
                        MILLISECONDS,
                        SECONDS
                    };
                };
                using TIME_UNIT = TimeUnits::UNIT;

            public:

                using TimePoint = Platform::Time::TimePoint;

                template<typename T, TIME_UNIT Unit>
                struct TSTimeUnit{};

                template<>
                struct TSTimeUnit<uint32_t, TimeUnits::NANOSECONDS>
                {
                    uint32_t time;
                    static auto Calc(const TimePoint& t, const TimePoint& freq) -> decltype(time)
                    {
                        return static_cast<uint32_t>(t * 1000 * 1000 * 1000 / freq);
                    }
                };

                template<>
                struct TSTimeUnit<float, TimeUnits::MICROSECONDS>
                {
                    float time;
                    static auto Calc(const TimePoint& t, const TimePoint& freq) -> decltype(time)
                    {
                        return static_cast<decltype(time)>(t * 1000 * 1000) / freq;
                    }
                };

                template<>
                struct TSTimeUnit<float, TimeUnits::MILLISECONDS>
                {
                    float time;
                    static auto Calc(const TimePoint& t, const TimePoint& freq) -> decltype(time)
                    {
                        return static_cast<float>(t * 1000.0f) / freq;
                    }
                };

                template<>
                struct TSTimeUnit<float, TimeUnits::SECONDS>
                {
                    float time;
                    static auto Calc(const TimePoint& t, const TimePoint& freq) -> decltype(time)
                    {
                        return static_cast<float>(t) / freq;
                    }
                };

                using Nanoseconds = TSTimeUnit<uint32_t, TimeUnits::NANOSECONDS>;
                using Microseconds = TSTimeUnit<float, TimeUnits::MICROSECONDS>;
                using Milliseconds = TSTimeUnit<float, TimeUnits::MILLISECONDS>;
                using Seconds = TSTimeUnit<float, TimeUnits::SECONDS>;

            public:

                CTimer();

                void Start();
                void Stop();

                void Now(TimePoint* pPoint, TimePoint* pFreq) const;
                void Now(TimePoint* pPoint) const;

                template<class TimeUnit = Microseconds>
                auto GetElapsedTime() const -> decltype(TimeUnit::time)
                {
                    TimePoint now;
                    Now(&now);
                    return TimeUnit::Calc( static_cast<TimePoint>( now - m_starTime ), m_frequency);
                }

            protected:

                template<typename TimeUnit>
                void _CheckType() const
                {
                    static_assert(
                        std::is_same<TimeUnit, Microseconds>::value ||
                        std::is_same<TimeUnit, Milliseconds>::value ||
                        std::is_same<TimeUnit, Seconds>::value,
                        "Invalid TimeUnit type");
                }

            protected:

                TimePoint   m_starTime;
                TimePoint   m_stopTime;
                TimePoint   m_frequency;
        };

    } // Utils

    struct TimeUnits
    {
        using Nanoseconds = Utils::CTimer::Nanoseconds;
        using Microseconds = Utils::CTimer::Microseconds;
        using Milliseconds = Utils::CTimer::Milliseconds;
        using Seconds = Utils::CTimer::Seconds;
    };

} // VKE