#ifndef __VKE_CLOGGER_H__
#define __VKE_CLOGGER_H__

#include "CStringStream.h"
#include "Core/Utils/TCBitset.h"
#include "Core/Utils/TCSingleton.h"
#include "Core/Utils/CTimer.h"
#include "Core/Threads/Common.h"

namespace VKE
{
    namespace Utils
    {
        struct LoggerModes
        {
            enum MODE
            {
                DISABLED = VKE_BIT(0),
                FILE = VKE_BIT(1),
                STDOUT = VKE_BIT(2),
                COMPILER = VKE_BIT(3),
                _MAX_COUNT = 4
            };
        };
        using LOGGER_MODE = LoggerModes::MODE;

        class VKE_API CLogger : public TCSingleton< CLogger >
        {
            public:

                CLogger();

                template<typename _T_>
                CLogger& Log(const _T_& msg)
                {
                    Threads::ScopedLock l(m_SyncObj);
                    m_Stream << msg;
                    return *this;
                }

                void SetMode(LOGGER_MODE mode);
                void AddMode(LOGGER_MODE mode);
                void RemoveMode(LOGGER_MODE mode);

                CLogger& Begin()
                {
                    m_SyncObj.Lock(); return *this;
                }
                CLogger& End()
                {
                    m_SyncObj.Unlock(); return *this;
                }

                template<typename _T_>
                CLogger& operator<<(const _T_& msg) { return Log(msg); }

                CLogger& operator<<(const CLogger& Logger);

                Result Flush();

                const CTimer& GetTimer() const { return m_Timer; }

                void SetSeparator(const cstr_t pSeparator) { m_pSeparator = pSeparator; }
                const cstr_t GetSeparator() const { return m_pSeparator; }

            protected:

                template<typename _T_>
                CLogger& _Log(const _T_& str) { m_Stream << str; }

                Result _FlushToConsole();
                Result _FlushToCompilerOutput();
                Result _FlushToFile();

            protected:

                Threads::SyncObject m_SyncObj;
                CStringStream   m_Stream;
                Utils::CTimer   m_Timer;
                BitsetU8        m_Mode = BitsetU8(LoggerModes::STDOUT);
                cstr_t          m_pSeparator = "::";
        };
    } // Utils
} // VKE

#define VKE_LOGGER VKE::Utils::CLogger::GetSingleton()
#define VKE_LOG_FILE VKE_FILE
#define VKE_LOG_LINE VKE_LINE
#define VKE_LOG_FUNC VKE_FUNCTION
#define VKE_LOG_TIME VKE_LOGGER.GetTimer().GetElapsedTime()
#define VKE_LOGGER_SEPARATOR VKE_LOGGER.GetSeparator()
#define VKE_LOGGER_LOG(_msg) VKE_CODE( VKE_LOGGER.Begin(); VKE_LOGGER << VKE_LOG_FUNC << VKE_LOGGER_SEPARATOR \
    << VKE_LOG_LINE << VKE_LOGGER_SEPARATOR << _msg << "\n"; VKE_LOGGER.Flush(); VKE_LOGGER.End(); )
#define VKE_LOGGER_LOG_ERROR(_err, _msg) VKE_LOGGER_LOG( "[ERROR] " << _msg )
#define VKE_LOGGER_LOG_WARNING(_msg) VKE_LOGGER_LOG( "[WARNING] " << _msg )

#endif // __VKE_CLOGGER_H__