#include "Core/Platform/CPlatform.h"
#if VKE_WINDOWS
#include <windows.h>
#include <crtdbg.h>

#include "Core/Utils/TCList.h"

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

#undef MemoryBarrier
#undef Yield

namespace VKE
{
    Platform::SProcessorInfo Platform::m_ProcessorInfo;
    const Platform::SProcessorInfo& Platform::GetProcessorInfo()
    {
        if( m_ProcessorInfo.count == 0 )
        {
            ::SYSTEM_INFO SysInfo;
            GetSystemInfo( &SysInfo );
            m_ProcessorInfo.count = SysInfo.dwNumberOfProcessors;
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
            ::PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX pCurr = nullptr;
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
                    const ::CACHE_RELATIONSHIP& Cache = pBuffer->Cache;
                    VKE_FREE( pBuffer );
                    bufferLen = 0;
                }
            }
        }
        return m_ProcessorInfo;
    }

    void Platform::Debug::EndDumpMemoryLeaks()
    {
        //_CrtDumpMemoryLeaks();
    }

    void Platform::Debug::BeginDumpMemoryLeaks()
    {
        _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    }

    void Platform::Time::Sleep(uint32_t uMilliseconds)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(uMilliseconds));
    }

    void Platform::Debug::PrintOutput(const cstr_t msg)
    {
        ::OutputDebugStringA((LPCSTR)msg);
    }

    handle_t Platform::DynamicLibrary::Load(const cstr_t name)
    {
        return reinterpret_cast<handle_t>(::LoadLibraryA(name));
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
        return ret;
    }

    handle_t Platform::File::Open(cstr_t pFileName, MODE mode)
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
        ::HANDLE hFile = ::CreateFileA( pFileName, dwAccess, dwShare, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr );
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

    bool Platform::File::GetExtension(cstr_t pFileName, char* pBuff, uint32_t buffSize)
    {
        return false;
    }

    bool Platform::File::GetExtension(handle_t hFile, char* pBuff, uint32_t buffSize )
    {
        return false;
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

    void Platform::Thread::MemoryBarrier()
    {
        __faststorefence();
    }

    void Platform::Thread::Pause()
    {
        _mm_pause();
        //std::this_thread::yield();
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
