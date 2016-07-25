#ifndef __VKE_CSTRING_STREAM_H__
#define __VKE_CSTRING_STREAM_H__

#include "VKECommon.h"

namespace VKE
{
    namespace Utils
    {
        class CStringStream
        {
            public:

                            CStringStream();
                virtual     ~CStringStream();

                template<typename _T_>
                CStringStream&  operator<<(const _T_& tValue)
                {
                    m_ss << tValue;
                    return *this;
                }

                CStringStream& operator<<(const CStringStream& Other)
                {
                    m_ss << Other.m_ss.str();
                    return *this;
                }

                void Reset();

                const str_t Get() const { return m_ss.str(); }
                str_t Get() { return m_ss.str(); }

            protected:

                std::stringstream   m_ss;
                static std::string  CLEAR_STRING;
        };
    } // Utils
} // VKE

#endif // __VKE_CSTRING_STREAM_H__