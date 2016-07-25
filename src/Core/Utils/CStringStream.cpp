#include "Core/Utils/CStringStream.h"

namespace VKE
{
    namespace Utils
    {
        std::string CStringStream::CLEAR_STRING;

        CStringStream::CStringStream()
        {
            CLEAR_STRING.reserve(1024);
            m_ss.str(CLEAR_STRING);
        }

        CStringStream::~CStringStream()
        {

        }

        void CStringStream::Reset()
        {
            m_ss.str(CLEAR_STRING);
        }

    } // Utils
} // VKE
