#include "Core/Utils/CTimer.h"

namespace VKE
{
    namespace Utils
    {


        CTimer::CTimer()
        {
            
        }

        void CTimer::Now(TimePoint* pPoint, float* pFreq) const
        {
            *pPoint = Platform::Time::GetHighResClockTimePoint();
            assert(*pPoint > 0);
            *pFreq = Platform::Time::GetHighResClockFrequency();
            assert(*pFreq > 1);
        }

        void CTimer::Now(TimePoint* pPoint) const
        {
            *pPoint = Platform::Time::GetHighResClockTimePoint();
            assert(*pPoint > 0);
        }

        void CTimer::Start()
        {
            Now(&m_starTime, &m_frequency);
        }

        void CTimer::Stop()
        {
            Now(&m_stopTime);
            assert(m_stopTime > 0);
            assert(m_stopTime > m_starTime);
        }

    } // Utils
} // VKE