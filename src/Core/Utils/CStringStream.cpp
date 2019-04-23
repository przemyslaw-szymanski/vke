#include "Core/Utils/CStringStream.h"

namespace VKE
{
    namespace Utils
    {
        CStringStream::CStringStream()
        {
            m_ss.str( "" );
        }

        CStringStream::~CStringStream()
        {

        }

        void CStringStream::Reset()
        {
            m_ss.str( "" );
        }

    } // Utils
} // VKE
