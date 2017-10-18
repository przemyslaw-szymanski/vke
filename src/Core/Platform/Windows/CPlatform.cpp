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
