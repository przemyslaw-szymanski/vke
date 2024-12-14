#include "Core/Utils/CLogger.h"
#include "Core/Platform/CPlatform.h"
#include "CVkEngine.h"

namespace VKE
{
    namespace Utils
    {
        CLogger::CLogger()
        {
#if VKE_DEBUG
            AddMode(LoggerModeFlagBits::COMPILER);
#endif
            m_Timer.Start();
        }

        CLogger& CLogger::operator<<(const CLogger& Logger)
        {
            return *this << Logger.m_Stream;
        }

        void CLogger::SetMode(LOGGER_MODE_FLAGS mode)
        {
            m_Mode = static_cast<uint8_t>(mode);
        }

        void CLogger::AddMode(LOGGER_MODE_FLAGS mode)
        {
            m_Mode += static_cast<uint8_t>(mode);
        }

        void CLogger::RemoveMode(LOGGER_MODE_FLAGS mode)
        {
            m_Mode -= static_cast<uint8_t>(mode);
        }

    } // Utils
} // VKE
