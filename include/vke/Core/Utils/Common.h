#pragma once

#include "Core/VKECommon.h"

namespace VKE
{
    namespace Utils
    {
        template<typename InputItr>
        class TCRoundCounter
        {
            public:

            TCRoundCounter()
            {}

            explicit TCRoundCounter(InputItr Begin, InputItr End) :
                m_Begin(Begin),
                m_End(End),
                m_Curr(Begin)
            {}

            InputItr    GetNext()
            {
                if( m_Curr == m_End )
                {
                    m_Curr = m_Begin;
                }
                return m_Curr++;
            }

            protected:

                const InputItr    m_Begin;
                const InputItr    m_End;
                InputItr          m_Curr;
        };
    } // Utils
} // VKE