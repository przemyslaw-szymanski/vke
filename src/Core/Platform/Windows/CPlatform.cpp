#include "Core/Platform/CPlatform.h"
#include "Utils/CLogger.h"
#if VKE_WINDOWS
#define NOMINMAX
#include <windows.h>
#include <crtdbg.h>

#include "Core/Utils/TCList.h"

#include <filesystem>


//#if VKE_COMPILER_VISUAL_STUDIO || VKE_COMPILER_GCC
//#   pragma push_macro(VKE_TO_STRING(LoadLibrary))
//#endif
//#undef LoadLibrary
//#if defined LoadLibrary
//#   define Win32LoadLibrary ::LoadLibraryA
//#   undef LoadLibrary
//#endif // LoadLibrary
//#if defined MemoryBarrier
//#   define Win32MemoryBarrier MemoryBarrier
//#   undef MemoryBarrier
//#endif // MemoryBarrier

#if _MSC_VER < 1920
#   define std_filesystem std::experimental::filesystem::v1
#else
#   define std_filesystem std::filesystem
#endif

#undef MemoryBarrier
#undef Yield

namespace VKE
{
    void GetErrorMessage(::DWORD errorCode, char* pBuffer, uint32_t bufferSize)
    {
        ::FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr,
            errorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), pBuffer, bufferSize, nullptr);
    }

    void LogError()
    {
        char pBuffer[2048];
        GetErrorMessage(GetLastError(), pBuffer, sizeof(pBuffer));
        VKE_LOG_ERR("System Error: " << pBuffer);
    }

    Platform::SProcessorInfo Platform::m_ProcessorInfo;
    const Platform::SProcessorInfo& Platform::GetProcessorInfo()
    {
        if( m_ProcessorInfo.count == 0 )
        {
            ::SYSTEM_INFO SysInfo;
            GetSystemInfo( &SysInfo );
            m_ProcessorInfo.count = static_cast< uint16_t >( SysInfo.dwNumberOfProcessors );
            switch( SysInfo.wProcessorArchitecture )
            {
                case PROCESSOR_ARCHITECTURE_AMD64:
                    m_ProcessorInfo.architecture = Architectures::X64;
                break;
                case PROCESSOR_ARCHITECTURE_INTEL:
                    m_ProcessorInfo.architecture = Architectures::X86;
                break;
                case PROCESSOR_ARCHITECTURE_ARM:
                case PROCESSOR_ARCHITECTURE_ARM32_ON_WIN64:
                    m_ProcessorInfo.architecture = Architectures::ARM32;
                break;
                case PROCESSOR_ARCHITECTURE_ARM64:
                    m_ProcessorInfo.architecture = Architectures::ARM64;
                break;
            }
            ::PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX pBuffer = nullptr;
            //::PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX pCurr = nullptr;
            ::DWORD bufferLen = 0;
            ::DWORD res = ::GetLogicalProcessorInformationEx(::RelationProcessorCore, pBuffer, &bufferLen);
            if( res == FALSE )
            {
                auto err = ::GetLastError();
                if( err == ERROR_INSUFFICIENT_BUFFER )
                {
                    pBuffer = ( ::PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX )VKE_MALLOC(bufferLen);
                }
            }
            if( pBuffer )
            {
                VKE_FREE( pBuffer );
                bufferLen = 0;
            }
            res = ::GetLogicalProcessorInformationEx(::RelationCache, pBuffer, &bufferLen);
            if( res == FALSE )
            {
                auto err = ::GetLastError();
                if( err == ERROR_INSUFFICIENT_BUFFER )
                {
                    pBuffer = ( ::PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX )VKE_MALLOC(bufferLen);
                }
                if( pBuffer )
                {
                    //const ::CACHE_RELATIONSHIP& Cache = pBuffer->Cache;
                    VKE_FREE( pBuffer );
                    bufferLen = 0;
                }
            }
        }
        return m_ProcessorInfo;
    }

    static _CrtMemState g_sMemState1, g_sMemState2;
    static Platform::Debug::CMemoryLeakDetector g_sMemLeakDetector;

    void Platform::Debug::BeginDumpMemoryLeaks()
    {
        _CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_DEBUG );
        //_CrtSetDbgFlag( _CRTDBG_DELAY_FREE_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
        //_CrtMemCheckpoint( &g_sMemState1 );
        g_sMemLeakDetector.Start( "VKE GLOBAL SCOPE" );
    }

    void Platform::Debug::EndDumpMemoryLeaks()
    {
        g_sMemLeakDetector.End();
        /*_CrtMemCheckpoint( &g_sMemState2 );
        _CrtMemState MemState3;
        if( _CrtMemDifference( &MemState3, &g_sMemState1, &g_sMemState2 ) )
        {
            _CrtMemDumpStatistics( &MemState3 );
        }*/
        //_CrtDumpMemoryLeaks();
    }

    void Platform::Debug::BreakAtAllocation( uint32_t idx )
    {
        _CrtSetBreakAlloc( idx );
    }

    void Platform::Debug::CMemoryLeakDetector::Start( cstr_t pName )
    {
#if VKE_DEBUG
        m_pName = pName;
        _CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_DEBUG );
        _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_DELAY_FREE_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
        _CrtMemCheckpoint( &m_BeginState );
#endif
    }

    bool Platform::Debug::CMemoryLeakDetector::End()
    {
        bool ret = true;
#if VKE_DEBUG
        if( m_pName )
        {
            _CrtMemCheckpoint( &m_EndState );
            _CrtMemState DiffState;
            ret = _CrtMemDifference( &DiffState, &m_BeginState, &m_EndState );
            if( ret )
            {
                Platform::Debug::PrintOutput( "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n" );
                Platform::Debug::PrintOutput( "VKE MEMORY LEAKS DETECTION IN REGION:\n" );
                Platform::Debug::PrintOutput( m_pName );
                Platform::Debug::PrintOutput( "\n" );
                _CrtMemDumpStatistics( &DiffState );
                Platform::Debug::PrintOutput( "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n" );
            }
        }
#endif
        return ret;
    }

    void Platform::Time::Sleep(uint32_t uMilliseconds)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(uMilliseconds));
    }

    void Platform::Debug::PrintOutput(const cstr_t msg)
    {
        ::OutputDebugStringA((LPCSTR)msg);
    }

    void Platform::Debug::ConvertErrorCodeToText( uint32_t err, char* pBuffOut, uint32_t buffSize )
    {
        ::LPVOID pMsgBuff;
        ::FormatMessageA(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            nullptr,
            err,
            MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),
            ( LPSTR )&pMsgBuff,
            0,
            nullptr );

        const auto len = lstrlen( ( LPCTSTR )pMsgBuff );
        strcpy_s( pBuffOut, buffSize, ( LPCTSTR )pMsgBuff );

        LocalFree( pMsgBuff );
    }

    handle_t Platform::DynamicLibrary::Load(const cstr_t name)
    {
        return reinterpret_cast<handle_t>(::LoadLibraryA( name ));
    }

    void Platform::DynamicLibrary::Close(const handle_t& handle)
    {
        ::FreeLibrary(reinterpret_cast<HMODULE>(handle));
    }

    void* Platform::DynamicLibrary::GetProcAddress(const handle_t& handle, const void* pSymbol)
    {
        return reinterpret_cast<void*>(::GetProcAddress(reinterpret_cast<HMODULE>(handle), reinterpret_cast<LPCSTR>(pSymbol)));
    }

    Platform::Time::TimePoint Platform::Time::GetHighResClockFrequency()
    {
        ::LARGE_INTEGER Freq;
        if(::QueryPerformanceFrequency(&Freq) == TRUE)
            return  Freq.QuadPart;
        return 1;
    }

    Platform::Time::TimePoint Platform::Time::GetHighResClockTimePoint()
    {
        ::LARGE_INTEGER Counter;
        if(::QueryPerformanceCounter(&Counter) == TRUE)
            return Counter.QuadPart;
        return 0;
    }

    bool Platform::File::Exists(cstr_t pFileName)
    {
        ::WIN32_FIND_DATA FindData;
        ::HANDLE handle = ::FindFirstFileA( pFileName, &FindData );
        bool exists = false;
        if( handle != INVALID_HANDLE_VALUE )
        {
            exists = true;
        }
        FindClose( handle );
        return exists;
    }

    bool Platform::File::IsDirectory(cstr_t pFileName)
    {
        bool ret;
        const std_filesystem::path path(pFileName);
        std::error_code err;
        ret = std_filesystem::is_directory(path, err);
        return ret;
    }

    uint32_t Platform::File::GetSize(cstr_t pFileName)
    {
        handle_t hFile = Open( pFileName, Modes::READ );
        uint32_t size = GetSize( hFile );
        Close( &hFile );
        return size;
    }

    uint32_t Platform::File::GetSize(handle_t hFile)
    {
        ::HANDLE hNative = reinterpret_cast< ::HANDLE >( hFile );
        return ::GetFileSize( hNative, nullptr );
    }

    uint32_t Platform::File::GetDirectory(cstr_t pFileName, uint32_t fileNameSize, char** ppOut)
    {
        assert( ppOut && *ppOut );
        char* pBuff = *ppOut;
        uint32_t charPosition = 0;
        // Find last '/' or '\\' character
        for( uint32_t i = fileNameSize; i-- > 0; )
        {
            if (pFileName[i] == '\\' || pFileName[i] == '/')
            {
                charPosition = fileNameSize - i;
                break;
            }
        }
        
        uint32_t dirNameSize = fileNameSize - charPosition;
        Memory::Copy(pBuff, fileNameSize, pFileName, dirNameSize);
        pBuff[dirNameSize] = '\0';
        return dirNameSize;
    }

    handle_t Platform::File::Create(cstr_t pFileName, MODE mode)
    {
        ::DWORD dwAccess = 0;
        ::DWORD dwShare = 0;
        if( mode & Modes::READ )
        {
            dwAccess |= GENERIC_READ;
            dwShare |= FILE_SHARE_READ;
        }
        if( mode & Modes::WRITE )
        {
            dwAccess |= GENERIC_WRITE;
            dwShare |= FILE_SHARE_WRITE;
        }

        handle_t ret = 0;
        ::HANDLE hFile = ::CreateFileA( pFileName, dwAccess, dwShare, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr );
        if( hFile != INVALID_HANDLE_VALUE )
        {
            ret = reinterpret_cast< handle_t >( hFile );
        }
        else
        {
            char pBuffer[2048];
            GetErrorMessage(GetLastError(), pBuffer, sizeof(pBuffer));
            VKE_LOG_ERR("Unable to create file: " << pFileName << " error:\n" << pBuffer);
        }
        return ret;
    }

    bool Platform::File::CreateDir(cstr_t pDirName)
    {
        bool ret = true;
        size_t pos = 0;
        std::string path(pDirName);
        do
        {
            pos = path.find_first_of("\\/", pos + 1);
            if (!::CreateDirectoryA(path.substr(0, pos).c_str(), NULL))
            {
                ret = false;
                LogError();
                break;
            }
        } while (pos != std::string::npos);
        return ret;
    }

    handle_t Platform::File::Open(cstr_t pFileName, MODE mode)
    {
        ::DWORD dwAccess = 0;
        ::DWORD dwShare = 0;
        ::DWORD dwCreateDesc = 0;
        if( mode & Modes::READ )
        {
            dwAccess |= GENERIC_READ;
            dwShare |= FILE_SHARE_READ;
            dwCreateDesc = OPEN_EXISTING;
        }
        if( mode & Modes::WRITE )
        {
            dwAccess |= GENERIC_WRITE;
            dwShare |= FILE_SHARE_WRITE;
            dwCreateDesc = OPEN_ALWAYS;
        }

        handle_t ret = 0;
        ::HANDLE hFile = ::CreateFileA( pFileName, dwAccess, dwShare, nullptr, dwCreateDesc, FILE_ATTRIBUTE_NORMAL, nullptr );
        if( hFile != INVALID_HANDLE_VALUE )
        {
            ret = reinterpret_cast< handle_t >( hFile );
        }
        return ret;
    }

    void Platform::File::Close(handle_t* phFile)
    {
        handle_t& hFile = *phFile;
        ::HANDLE hNative = reinterpret_cast< ::HANDLE >( hFile );
        ::CloseHandle( hNative );
        hFile = 0;
    }

    bool Platform::File::Seek( handle_t hFile, uint32_t offset, SEEK_MODE mode )
    {
        static const uint32_t aModes[] =
        {
            FILE_BEGIN,
            FILE_CURRENT,
            FILE_END
        };
        ::HANDLE hNative = reinterpret_cast< ::HANDLE >( hFile );
        ::LARGE_INTEGER Offset;
        Offset.QuadPart = offset;
        return ::SetFilePointerEx( hNative, Offset, nullptr, aModes[ mode ] );
    }

    uint32_t Platform::File::Read(handle_t hFile, SReadData* pData)
    {
        ::HANDLE hNative = reinterpret_cast< ::HANDLE >( hFile );
        ::DWORD dwCount;
        if( pData->offset )
        {
            Seek( hFile, pData->offset, SeekModes::BEGIN );
        }
        if( ::ReadFile( hNative, pData->pData, pData->readByteCount, &dwCount, nullptr ) != TRUE )
        {
            dwCount = 0;
        }
        return dwCount;
    }

    uint32_t Platform::File::Write(handle_t hFile, const SWriteInfo& Info)
    {
        ::HANDLE hNative = reinterpret_cast< ::HANDLE >( hFile );
        ::DWORD dwCount;
        if( Info.offset )
        {
            Seek( hFile, Info.offset, SeekModes::BEGIN );
        }
        if( ::WriteFile( hNative, Info.pData, Info.dataSize, &dwCount, nullptr ) != TRUE )
        {
            dwCount = 0;
        }
        return dwCount;
    }

    cstr_t Platform::File::GetExtension(cstr_t pFileName)
    {
        cstr_t pExt = strrchr( pFileName, '.' );
        if( pExt )
        {
            return pExt + 1;
        }
        return pExt;
    }

    cstr_t Platform::File::GetExtension(handle_t /*hFile*/)
    {
        assert( 0 && "not implemented" );
        return nullptr;
    }

    Platform::Thread::ID Platform::Thread::GetID()
    {
        return ::GetCurrentThreadId();
    }

    Platform::Thread::ID Platform::Thread::GetID(const handle_t& hThread)
    {
        return ::GetThreadId(reinterpret_cast< HANDLE >( hThread ));
    }

    Platform::Thread::ID Platform::Thread::GetID(void* pHandle)
    {
        return ::GetThreadId(pHandle);
    }

    void Platform::Thread::Sleep(uint32_t ms)
    {
        ::Sleep( ms );
    }

   /* void Platform::Thread::MemoryBarrier()
    {
        __faststorefence();
    }*/

    void Platform::Thread::Pause()
    {
        ::YieldProcessor();
        std::this_thread::sleep_for( std::chrono::nanoseconds( 1 ) );
    }

    uint32_t Platform::Thread::GetMaxConcurrentThreadCount()
    {
        return std::thread::hardware_concurrency();
    }

    void Platform::Thread::CSpinlock::Lock()
    {
        const auto id = Platform::Thread::GetID();
        if( m_threadId == id )
        {
            ++m_lockCount;
            return;
        }
        while( ::InterlockedCompareExchange( &m_threadId, id, UNKNOWN_THREAD_ID ) != UNKNOWN_THREAD_ID )
        {
            Platform::Thread::Pause();
        }
        m_lockCount = 1;
        // linux
        //while( m_interlock == 1 || __sync_lock_test_and_set(&m_interlock, 1) == 1 );
    }

    void Platform::Thread::CSpinlock::Unlock()
    {
        if( --m_lockCount == 0 )
        {
            //m_isLocked = 0;
            ::InterlockedExchange( &m_threadId, UNKNOWN_THREAD_ID );
        }
    }

    bool Platform::Thread::CSpinlock::TryLock()
    {
        const auto id = Platform::Thread::GetID();
        if( m_threadId == id )
        {
            ++m_lockCount;
            return true;
        }
        if( ::InterlockedCompareExchange( &m_threadId, id, UNKNOWN_THREAD_ID ) != UNKNOWN_THREAD_ID )
        {
            return false;
        }
        m_lockCount = 1;
        return true;
    }

} // VKE

//#if VKE_COMPILER_VISUAL_STUDIO || VKE_COMPILER_GCC
//#   pragma pop_macro(VKE_TO_STRING(LoadLibrary))
//#else
//#   define LoadLibrary ::LoadLibraryA
//#endif

#endif // VKE_WINDOWS
