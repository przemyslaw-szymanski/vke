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

        Result CLogger::Flush()
        {
            if(m_Mode.Get())
            {
                m_SyncObj.Lock();
                const auto& str = m_Stream.Get();
                if(m_Mode == LoggerModeFlagBits::STDOUT)
                {
                    printf("%s\n", str.c_str());
                }

                if(m_Mode == LoggerModeFlagBits::COMPILER)
                {
                    Platform::Debug::PrintOutput(str.c_str());
                }

                if(m_Mode == LoggerModeFlagBits::FILE)
                {
                    if(!m_File.is_open())
                    {
                        m_File.open( "f:\\projects\\vke\\bin\\log.txt", std::ios::out );
                    }
                    if(m_File.is_open())
                    {
                        m_File << str;
                        m_File.flush();
                    }
                }

                m_Stream.Reset();
                m_SyncObj.Unlock();
            }
            return VKE_OK;
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
