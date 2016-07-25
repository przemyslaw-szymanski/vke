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
    void Platform::EndDumpMemoryLeaks()
    {
    }

    void Platform::BeginDumpMemoryLeaks()
    {
        _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    }

    void Platform::Sleep(uint32_t uMilliseconds)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(uMilliseconds));
    }

    void Platform::PrintOutput(const cstr_t msg)
    {
        ::OutputDebugStringA((LPCSTR)msg);
    }

    handle_t Platform::LoadDynamicLibrary(const cstr_t name)
    {
        return reinterpret_cast<handle_t>(::LoadLibraryA(name));
    }

    void Platform::CloseDynamicLibrary(const handle_t& handle)
    {
        ::FreeLibrary(reinterpret_cast<HMODULE>(handle));
    }

    void* Platform::GetProcAddress(const handle_t& handle, const void* pSymbol)
    {
        return reinterpret_cast<void*>(::GetProcAddress(reinterpret_cast<HMODULE>(handle), reinterpret_cast<LPCSTR>(pSymbol)));
    }

} // VKE

#if VKE_COMPILER_VISUAL_STUDIO || VKE_COMPILER_GCC
#   pragma pop_macro(VKE_TO_STRING(LoadLibrary))
#else
#   define LoadLibrary ::LoadLibraryA
#endif

#endif // VKE_WINDOWS
