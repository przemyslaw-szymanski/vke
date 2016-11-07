#include "Core/Utils/CLogger.h"
#include "Core/Platform/CPlatform.h"

namespace VKE
{
    namespace Utils
    {
        CLogger::CLogger()
        {
            m_Timer.Start();
        }

        CLogger& CLogger::operator<<(const CLogger& Logger)
        {
            return *this << Logger.m_Stream;
        }

        Result CLogger::Flush()
        {
            if(m_Mode.Get())
            {
                Threads::TryLock l(m_Mutex);
                auto str = m_Stream.Get();
                if(m_Mode == LoggerModes::STDOUT)
                {
                    printf("%s\n", str.c_str());
                }

                if(m_Mode == LoggerModes::COMPILER)
                {
                    Platform::Debug::PrintOutput(str.c_str());
                }

                if(m_Mode == LoggerModes::FILE)
                {

                }

                m_Stream.Reset();
            }
            return VKE_OK;
        }

        void CLogger::SetMode(LOGGER_MODE mode)
        {
            Threads::TryLock l(m_Mutex);
            m_Mode = static_cast<uint8_t>(mode);
        }

        void CLogger::AddMode(LOGGER_MODE mode)
        {
            Threads::TryLock l(m_Mutex);
            m_Mode += static_cast<uint8_t>(mode);
        }

        void CLogger::RemoveMode(LOGGER_MODE mode)
        {
            Threads::TryLock l(m_Mutex);
            m_Mode -= static_cast<uint8_t>(mode);
        }

    } // Utils
} // VKE
