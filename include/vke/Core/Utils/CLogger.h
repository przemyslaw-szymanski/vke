#ifndef __VKE_CLOGGER_H__
#define __VKE_CLOGGER_H__

#include "CStringStream.h"
#include "Core/Utils/TCBitset.h"
#include "Core/Utils/TCSingleton.h"
#include "Core/Utils/CTimer.h"
#include "Core/Threads/Common.h"
#include "Core/VKEConfig.h"

#include <fstream>

namespace VKE
{
    namespace Utils
    {
        using LOGGER_MODE_FLAGS = uint8_t;

        struct LoggerModeFlagBits
        {
            enum MODE : LOGGER_MODE_FLAGS
            {
                DISABLED = VKE_BIT(0),
                FILE = VKE_BIT(1),
                STDOUT = VKE_BIT(2),
                COMPILER = VKE_BIT(3),
                _MAX_COUNT = 4
            };
        };
        using LoggerModeFlags = Utils::TCBitset<LOGGER_MODE_FLAGS>;

        class VKE_API CLogger
        {
            public:

                CLogger();
                ~CLogger()
                {
                    m_File.close();
                }

                static CLogger& GetInstance()
                {
                    static CLogger Logger;
                    return Logger;
                }

                CLogger& Log( const double& msg )
                {
                    Threads::ScopedLock l( m_SyncObj );
                    m_Stream << std::fixed << msg;
                    return *this;
                }

                CLogger& Log( const float& msg )
                {
                    Threads::ScopedLock l( m_SyncObj );
                    m_Stream << std::fixed << msg;
                    return *this;
                }

                template<typename _T_>
                CLogger& Log(const _T_& msg)
                {
                    Threads::ScopedLock l(m_SyncObj);
                    m_Stream << msg;
                    return *this;
                }

                void SetMode(LOGGER_MODE_FLAGS mode);
                void AddMode(LOGGER_MODE_FLAGS mode);
                void RemoveMode(LOGGER_MODE_FLAGS mode);

                CLogger& Begin()
                {
                    m_SyncObj.Lock(); return *this;
                }
                CLogger& End()
                {
                    m_SyncObj.Unlock(); return *this;
                }

                CLogger& operator<<( const double& msg ) { return Log( msg ); }
                CLogger& operator<<( const float& msg ) { return Log( msg ); }

                template<typename _T_>
                CLogger& operator<<(const _T_& msg) { return Log(msg); }

                CLogger& operator<<(const CLogger& Logger);

                Result Flush();

                const CTimer& GetTimer() const { return m_Timer; }

                void SetSeparator(const cstr_t pSeparator) { m_pSeparator = pSeparator; }
                const cstr_t GetSeparator() const { return m_pSeparator; }

                uint32_t GetTid() const { return Platform::ThisThread::GetID(); }

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
                std::ofstream   m_File;
                BitsetU8        m_Mode = BitsetU8(LoggerModeFlagBits::STDOUT);
                cstr_t          m_pSeparator = "::";
        };
    } // Utils
} // VKE

//extern VKE_API class CVkEngine* VKEGetEngine();

#ifndef VKE_LOG_ENABLE
#   define VKE_LOG_ENABLE 1
#endif

#ifndef VKE_LOG_ERR_ENABLE
#   define VKE_LOG_ERR_ENABLE 1
#endif

#ifndef VKE_LOG_WARN_ENABLE
#   define VKE_LOG_WARN_ENABLE 1
#endif

#ifndef VKE_DBG_LOG_ENABLE
#   define VKE_DBG_LOG_ENABLE 1
#endif

//#define VKE_LOGGER VKE::Utils::CLogger::GetSingleton()
#define VKE_LOGGER VKE::Utils::CLogger::GetInstance()
#define VKE_LOG_FILE VKE_FILE
#define VKE_LOG_LINE VKE_LINE
#define VKE_LOG_FUNC VKE_FUNCTION
#define VKE_LOG_TID VKE_LOGGER.GetTid()
#define VKE_LOG_TIME VKE_LOGGER.GetTimer().GetElapsedTime()
#define VKE_LOGGER_SEPARATOR VKE_LOGGER.GetSeparator()
#define VKE_LOG_PRECISION( _num ) std::fixed << std::setprecision( (_num) )
#define VKE_LOGGER_BEGIN(_type) do{ VKE_LOGGER.Begin(); VKE_LOGGER << "\n" << _type << "[" << VKE_LOG_TID << "]" << VKE_LOGGER_SEPARATOR << VKE_LOG_FUNC << VKE_LOGGER_SEPARATOR << VKE_LOG_LINE << VKE_LOGGER_SEPARATOR; }while(0,0)
#define VKE_LOGGER_END do{VKE_LOGGER.Flush(); VKE_LOGGER.End();}while(0,0)
#define VKE_LOGGER_LOG2( _type, _msg )                                                                                  \
    VKE_CODE( VKE_LOGGER.Begin(); VKE_LOGGER << _type << "[" << VKE_LOG_TID << "]" << VKE_LOGGER_SEPARATOR << \
 VKE_LOG_FUNC << VKE_LOGGER_SEPARATOR << VKE_LOG_LINE << VKE_LOGGER_SEPARATOR << _msg << "\n"; VKE_LOGGER.Flush(); \
 VKE_LOGGER.End(); )
#define VKE_LOGGER_LOG( _type, _msg )                                                                                  \
    do                                                                                                                 \
    {                                                                                                                  \
        VKE_LOGGER_BEGIN( _type );                                                                                     \
        VKE_LOGGER << _msg;                                                                                            \
        VKE_LOGGER_END;                                                                                                \
    }                                                                                                                  \
    while(0,0)
#define VKE_LOGGER_LOG_BEGIN VKE_LOGGER_BEGIN( "[INFO]" )
#define VKE_LOGGER_LOG_ERROR(_err, _msg) VKE_LOGGER_LOG( "[ERROR]", _msg )
#define VKE_LOGGER_LOG_WARNING(_msg) VKE_LOGGER_LOG( "[WARNING]", _msg )
#define VKE_LOGGER_SIZE_MB( _bytes ) ( (float)( _bytes ) / 1024 / 1024 ) << " MB"
#define VKE_LOG_MEM_SIZE(_bytes) (_bytes) << " bytes (" << (float)((_bytes)/1024/1024) << " MB)"

#if VKE_LOG_ENABLE
#define VKE_LOG( _msg ) VKE_LOGGER_LOG( "[INFO]", _msg )
#else
#   define VKE_LOG(_msg)
#endif
#if VKE_DBG_LOG_ENABLE
#   define VKE_DBG_LOG(_msg) VKE_CODE( VKE_LOGGER.Begin(); VKE_LOGGER << _msg; VKE_LOGGER.Flush(); VKE_LOGGER.End(); )
#else
#   define VKE_DBG_LOG(_msg)
#endif
#if VKE_LOG_ERR_ENABLE
#   if VKE_ASSERT_ON_ERROR_ENABLE
#       define VKE_LOG_ERR(_msg) VKE_LOGGER_LOG_ERROR(VKE_FAIL, _msg); VKE_ASSERT(0)
#else
#       define VKE_LOG_ERR( _msg ) VKE_LOGGER_LOG_ERROR( VKE_FAIL, _msg )
#endif // VKE_LOG_ERR_ENABLE
#else
#   define VKE_LOG_ERR(_msg)
#endif
#if VKE_LOG_WARN_ENABLE
#   define VKE_LOG_WARN(_msg) VKE_LOGGER_LOG_WARNING(_msg)
#else
#   define VKE_LOG_WARN(_msg)
#endif

#define VKE_LOG_RET(_ret, _msg)  VKE_CODE( VKE_LOGGER_LOG_MSG( (_ret), _msg ); return (_ret); )
#define VKE_LOG_ERR_RET(_ret, _msg) VKE_CODE( VKE_LOGGER_LOG_ERROR( (_ret), _msg ); return (_ret); )
#define VKE_LOG_WRN_RET(_ret, _msg) VKE_CODE( VKE_LOGGER_LOG_WARNING( (_ret), _msg ); return (_ret); )

#if VKE_CFG_LOG_PROGRESS
#   define VKE_LOG_PROG(_msg) VKE_LOG(_msg)
#else
#   define VKE_LOG_PROG(_msg)
#endif // VKE_CFG_LOG_PROGRESS

#endif // __VKE_CLOGGER_H__