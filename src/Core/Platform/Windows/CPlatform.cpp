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

    void Platform::Debug::EndDumpMemoryLeaks()
    {
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

    void Platform::Thread::Yield()
    {
        _mm_pause();
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
            Platform::Thread::Yield();
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
