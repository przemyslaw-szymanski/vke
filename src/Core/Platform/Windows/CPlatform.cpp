#include "Core/Platform/CPlatform.h"
#if VKE_WINDOWS
#include <windows.h>
#include <crtdbg.h>

#if VKE_COMPILER_VISUAL_STUDIO || VKE_COMPILER_GCC
#   pragma push_macro(VKE_TO_STRING(LoadLibrary))
#endif
#undef LoadLibrary

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

} // VKE

#if VKE_COMPILER_VISUAL_STUDIO || VKE_COMPILER_GCC
#   pragma pop_macro(VKE_TO_STRING(LoadLibrary))
#else
#   define LoadLibrary ::LoadLibraryA
#endif

#endif // VKE_WINDOWS