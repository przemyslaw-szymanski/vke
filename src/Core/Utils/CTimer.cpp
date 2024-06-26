#include "Core/Utils/CTimer.h"

namespace VKE
{
    namespace Utils
    {
        CTimer::CTimer()
        {
            m_frequency = Platform::Time::GetHighResClockFrequency();
            VKE_ASSERT2( m_frequency > 1, "" );
        }

        void CTimer::Now(TimePoint* pPoint, TimePoint* pFreq) const
        {
            *pPoint = Platform::Time::GetHighResClockTimePoint();
            VKE_ASSERT2( *pPoint > 0, "" );
            *pFreq = m_frequency;
            
        }

        void CTimer::Now(TimePoint* pPoint) const
        {
            *pPoint = Platform::Time::GetHighResClockTimePoint();
            VKE_ASSERT2( *pPoint > 0, "" );
        }

        void CTimer::Start()
        {
            Now(&m_starTime, &m_frequency);
        }

        void CTimer::Stop()
        {
            Now(&m_stopTime);
        }

    } // Utils
} // VKE