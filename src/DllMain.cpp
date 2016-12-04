#include <windows.h>
#include <crtdbg.h>

BOOL WINAPI DllMain(_In_ void* _DllHandle, _In_ unsigned long _Reason, _In_opt_ void* _Reserved)
{
    switch( _Reason )
    {
        case DLL_PROCESS_ATTACH:
        {
            _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
        }
        break;
        case DLL_PROCESS_DETACH:
        {
            //_CrtDumpMemoryLeaks();
        }
        break;
    }
    return TRUE;
}