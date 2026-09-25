#include "Core/Platform/CPlatform.h"
#include "Core/Utils/CLogger.h"
#if VKE_WINDOWS

#include <windows.h>
#include <shlwapi.h>
#include <crtdbg.h>
#include <DbgHelp.h>

#include "Core/Utils/TCList.h"

#include <filesystem>

#ifdef GetCommandLine
#undef GetCommandLine
#endif

// #if VKE_COMPILER_VISUAL_STUDIO || VKE_COMPILER_GCC
// #   pragma push_macro(VKE_TO_STRING(LoadLibrary))
// #endif
// #undef LoadLibrary
// #if defined LoadLibrary
// #   define Win32LoadLibrary ::LoadLibraryA
// #   undef LoadLibrary
// #endif // LoadLibrary
// #if defined MemoryBarrier
// #   define Win32MemoryBarrier MemoryBarrier
// #   undef MemoryBarrier
// #endif // MemoryBarrier

#if VKE_COMPILER_VISUAL_STUDIO && _MSC_VER < 1920
#define std_filesystem std::experimental::filesystem::v1
#else
#define std_filesystem std::filesystem
#endif

#undef MemoryBarrier
#undef Yield

namespace VKE
{
    void GetErrorMessage( ::DWORD errorCode, char* pBuffer, uint32_t bufferSize )
    {
        ::FormatMessageA( FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                          nullptr,
                          errorCode,
                          MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),
                          pBuffer,
                          bufferSize,
                          nullptr );
    }

    void LogError( cstr_t pText = "" )
    {
        char pBuffer[ 2048 ];
        GetErrorMessage( GetLastError(), pBuffer, sizeof( pBuffer ) );
        VKE_LOG_ERR( "System Error: " << pBuffer << "\t" << pText );
    }

    Platform::SProcessorInfo Platform::m_ProcessorInfo;

    const Input::KEY Platform::KeyMap::s_aKeyMap[ Platform::KeyMap::MAP_SIZE ] = {
        Input::KEY::UNKNOWN,         // 0x00
        Input::KEY::LBUTTON,         // 0x01: VK_LBUTTON
        Input::KEY::RBUTTON,         // 0x02: VK_RBUTTON
        Input::KEY::UNKNOWN,         // 0x03: VK_CANCEL
        Input::KEY::MBUTTON,         // 0x04: VK_MBUTTON
        Input::KEY::X_BUTTON1,       // 0x05: VK_XBUTTON1
        Input::KEY::X_BUTTON2,       // 0x06: VK_XBUTTON2
        Input::KEY::UNKNOWN,         // 0x07: Reserved
        Input::KEY::BACK,            // 0x08: VK_BACK
        Input::KEY::TAB,             // 0x09: VK_TAB
        Input::KEY::UNKNOWN,         // 0x0A: Reserved
        Input::KEY::UNKNOWN,         // 0x0B: Reserved
        Input::KEY::CLEAR,           // 0x0C: VK_CLEAR
        Input::KEY::ENTER,           // 0x0D: VK_RETURN
        Input::KEY::UNKNOWN,         // 0x0E: Unassigned
        Input::KEY::UNKNOWN,         // 0x0F: Unassigned
        Input::KEY::SHIFT,           // 0x10: VK_SHIFT
        Input::KEY::CONTROL,         // 0x11: VK_CONTROL
        Input::KEY::ALT,             // 0x12: VK_MENU
        Input::KEY::PAUSE,           // 0x13: VK_PAUSE
        Input::KEY::CAPSLOCK,        // 0x14: VK_CAPITAL
        Input::KEY::UNKNOWN,         // 0x15: VK_KANA, VK_HANGUL
        Input::KEY::UNKNOWN,         // 0x16: VK_IME_ON
        Input::KEY::UNKNOWN,         // 0x17: VK_JUNJA
        Input::KEY::UNKNOWN,         // 0x18: VK_FINAL
        Input::KEY::UNKNOWN,         // 0x19: VK_HANJA, VK_KANJI
        Input::KEY::UNKNOWN,         // 0x1A: VK_IME_OFF
        Input::KEY::ESC,             // 0x1B: VK_ESCAPE
        Input::KEY::UNKNOWN,         // 0x1C: VK_CONVERT
        Input::KEY::UNKNOWN,         // 0x1D: VK_NONCONVERT
        Input::KEY::UNKNOWN,         // 0x1E: VK_ACCEPT
        Input::KEY::UNKNOWN,         // 0x1F: VK_MODECHANGE
        Input::KEY::SPACE,           // 0x20: VK_SPACE
        Input::KEY::PAGE_UP,         // 0x21: VK_PRIOR
        Input::KEY::PAGE_DOWN,       // 0x22: VK_NEXT
        Input::KEY::END,             // 0x23: VK_END
        Input::KEY::HOME,            // 0x24: VK_HOME
        Input::KEY::LEFT,            // 0x25: VK_LEFT
        Input::KEY::UP,              // 0x26: VK_UP
        Input::KEY::RIGHT,           // 0x27: VK_RIGHT
        Input::KEY::DOWN,            // 0x28: VK_DOWN
        Input::KEY::SELECT,          // 0x29: VK_SELECT
        Input::KEY::PRINT,           // 0x2A: VK_PRINT
        Input::KEY::EXECUTE,         // 0x2B: VK_EXECUTE
        Input::KEY::PRINT_SCREEN,    // 0x2C: VK_SNAPSHOT
        Input::KEY::INSERT,          // 0x2D: VK_INSERT
        Input::KEY::DEL,             // 0x2E: VK_DELETE
        Input::KEY::HELP,            // 0x2F: VK_HELP
        Input::KEY::NUM_0,           // 0x30: 0
        Input::KEY::NUM_1,           // 0x31: 1
        Input::KEY::NUM_2,           // 0x32: 2
        Input::KEY::NUM_3,           // 0x33: 3
        Input::KEY::NUM_4,           // 0x34: 4
        Input::KEY::NUM_5,           // 0x35: 5
        Input::KEY::NUM_6,           // 0x36: 6
        Input::KEY::NUM_7,           // 0x37: 7
        Input::KEY::NUM_8,           // 0x38: 8
        Input::KEY::NUM_9,           // 0x39: 9
        Input::KEY::UNKNOWN,         // 0x3A: Undefined
        Input::KEY::UNKNOWN,         // 0x3B: Undefined
        Input::KEY::UNKNOWN,         // 0x3C: Undefined
        Input::KEY::UNKNOWN,         // 0x3D: Undefined
        Input::KEY::UNKNOWN,         // 0x3E: Undefined
        Input::KEY::UNKNOWN,         // 0x3F: Undefined
        Input::KEY::UNKNOWN,         // 0x40: Undefined
        Input::KEY::A,               // 0x41: A
        Input::KEY::B,               // 0x42: B
        Input::KEY::C,               // 0x43: C
        Input::KEY::D,               // 0x44: D
        Input::KEY::E,               // 0x45: E
        Input::KEY::F,               // 0x46: F
        Input::KEY::G,               // 0x47: G
        Input::KEY::H,               // 0x48: H
        Input::KEY::I,               // 0x49: I
        Input::KEY::J,               // 0x4A: J
        Input::KEY::K,               // 0x4B: K
        Input::KEY::L,               // 0x4C: L
        Input::KEY::M,               // 0x4D: M
        Input::KEY::N,               // 0x4E: N
        Input::KEY::O,               // 0x4F: O
        Input::KEY::P,               // 0x50: P
        Input::KEY::Q,               // 0x51: Q
        Input::KEY::R,               // 0x52: R
        Input::KEY::S,               // 0x53: S
        Input::KEY::T,               // 0x54: T
        Input::KEY::U,               // 0x55: U
        Input::KEY::V,               // 0x56: V
        Input::KEY::W,               // 0x57: W
        Input::KEY::X,               // 0x58: X
        Input::KEY::Y,               // 0x59: Y
        Input::KEY::Z,               // 0x5A: Z
        Input::KEY::LEFT_WINDOWS,    // 0x5B: VK_LWIN
        Input::KEY::RIGHT_WINDOW,    // 0x5C: VK_RWIN
        Input::KEY::UNKNOWN,         // 0x5D: VK_APPS
        Input::KEY::UNKNOWN,         // 0x5E: Reserved
        Input::KEY::SLEEP,           // 0x5F: VK_SLEEP
        Input::KEY::NUMPAD_0,        // 0x60: VK_NUMPAD0
        Input::KEY::NUMPAD_1,        // 0x61: VK_NUMPAD1
        Input::KEY::NUMPAD_2,        // 0x62: VK_NUMPAD2
        Input::KEY::NUMPAD_3,        // 0x63: VK_NUMPAD3
        Input::KEY::NUMPAD_4,        // 0x64: VK_NUMPAD4
        Input::KEY::NUMPAD_5,        // 0x65: VK_NUMPAD5
        Input::KEY::NUMPAD_6,        // 0x66: VK_NUMPAD6
        Input::KEY::NUMPAD_7,        // 0x67: VK_NUMPAD7
        Input::KEY::NUMPAD_8,        // 0x68: VK_NUMPAD8
        Input::KEY::NUMPAD_9,        // 0x69: VK_NUMPAD9
        Input::KEY::NUMPAD_MULTIPLY, // 0x6A: VK_MULTIPLY
        Input::KEY::NUMPAD_PLUS,     // 0x6B: VK_ADD
        Input::KEY::SEPARATOR,       // 0x6C: VK_SEPARATOR
        Input::KEY::NUMPAD_MINUS,    // 0x6D: VK_SUBTRACT
        Input::KEY::DECIMAL,         // 0x6E: VK_DECIMAL
        Input::KEY::NUMPAD_SLASH,    // 0x6F: VK_DIVIDE
        Input::KEY::F1,              // 0x70: VK_F1
        Input::KEY::F2,              // 0x71: VK_F2
        Input::KEY::F3,              // 0x72: VK_F3
        Input::KEY::F4,              // 0x73: VK_F4
        Input::KEY::F5,              // 0x74: VK_F5
        Input::KEY::F6,              // 0x75: VK_F6
        Input::KEY::F7,              // 0x76: VK_F7
        Input::KEY::F8,              // 0x77: VK_F8
        Input::KEY::F9,              // 0x78: VK_F9
        Input::KEY::F10,             // 0x79: VK_F10
        Input::KEY::F11,             // 0x7A: VK_F11
        Input::KEY::F12,             // 0x7B: VK_F12
        Input::KEY::UNKNOWN,         // 0x7C: VK_F13
        Input::KEY::UNKNOWN,         // 0x7D: VK_F14
        Input::KEY::UNKNOWN,         // 0x7E: VK_F15
        Input::KEY::UNKNOWN,         // 0x7F: VK_F16
        Input::KEY::UNKNOWN,         // 0x80: VK_F17
        Input::KEY::UNKNOWN,         // 0x81: VK_F18
        Input::KEY::UNKNOWN,         // 0x82: VK_F19
        Input::KEY::UNKNOWN,         // 0x83: VK_F20
        Input::KEY::UNKNOWN,         // 0x84: VK_F21
        Input::KEY::UNKNOWN,         // 0x85: VK_F22
        Input::KEY::UNKNOWN,         // 0x86: VK_F23
        Input::KEY::UNKNOWN,         // 0x87: VK_F24
        Input::KEY::UNKNOWN,         // 0x88: Reserved
        Input::KEY::UNKNOWN,         // 0x89: Reserved
        Input::KEY::UNKNOWN,         // 0x8A: Reserved
        Input::KEY::UNKNOWN,         // 0x8B: Reserved
        Input::KEY::UNKNOWN,         // 0x8C: Reserved
        Input::KEY::UNKNOWN,         // 0x8D: Reserved
        Input::KEY::UNKNOWN,         // 0x8E: Reserved
        Input::KEY::UNKNOWN,         // 0x8F: Reserved
        Input::KEY::NUMLOCK,         // 0x90: VK_NUMLOCK
        Input::KEY::SCROLL,          // 0x91: VK_SCROLL
        Input::KEY::UNKNOWN,         // 0x92: OEM specific
        Input::KEY::UNKNOWN,         // 0x93: OEM specific
        Input::KEY::UNKNOWN,         // 0x94: OEM specific
        Input::KEY::UNKNOWN,         // 0x95: OEM specific
        Input::KEY::UNKNOWN,         // 0x96: OEM specific
        Input::KEY::UNKNOWN,         // 0x97: Unassigned
        Input::KEY::UNKNOWN,         // 0x98: Unassigned
        Input::KEY::UNKNOWN,         // 0x99: Unassigned
        Input::KEY::UNKNOWN,         // 0x9A: Unassigned
        Input::KEY::UNKNOWN,         // 0x9B: Unassigned
        Input::KEY::UNKNOWN,         // 0x9C: Unassigned
        Input::KEY::UNKNOWN,         // 0x9D: Unassigned
        Input::KEY::UNKNOWN,         // 0x9E: Unassigned
        Input::KEY::UNKNOWN,         // 0x9F: Unassigned
        Input::KEY::LEFT_SHIFT,      // 0xA0: VK_LSHIFT
        Input::KEY::RIGHT_SHIFT,     // 0xA1: VK_RSHIFT
        Input::KEY::LEFT_CONTROL,    // 0xA2: VK_LCONTROL
        Input::KEY::RIGHT_CONTROL,   // 0xA3: VK_RCONTROL
        Input::KEY::LEFT_ALT,        // 0xA4: VK_LMENU
        Input::KEY::RIGHT_ALT,       // 0xA5: VK_RMENU
        Input::KEY::UNKNOWN,         // 0xA6: VK_BROWSER_BACK
        Input::KEY::UNKNOWN,         // 0xA7: VK_BROWSER_FORWARD
        Input::KEY::UNKNOWN,         // 0xA8: VK_BROWSER_REFRESH
        Input::KEY::UNKNOWN,         // 0xA9: VK_BROWSER_STOP
        Input::KEY::UNKNOWN,         // 0xAA: VK_BROWSER_SEARCH
        Input::KEY::UNKNOWN,         // 0xAB: VK_BROWSER_FAVORITES
        Input::KEY::UNKNOWN,         // 0xAC: VK_BROWSER_HOME
        Input::KEY::VOLUME_MUTE,     // 0xAD: VK_VOLUME_MUTE
        Input::KEY::VOLUME_DOWN,     // 0xAE: VK_VOLUME_DOWN
        Input::KEY::VOLUME_UP,       // 0xAF: VK_VOLUME_UP
        Input::KEY::UNKNOWN,         // 0xB0: VK_MEDIA_NEXT_TRACK
        Input::KEY::UNKNOWN,         // 0xB1: VK_MEDIA_PREV_TRACK
        Input::KEY::UNKNOWN,         // 0xB2: VK_MEDIA_STOP
        Input::KEY::UNKNOWN,         // 0xB3: VK_MEDIA_PLAY_PAUSE
        Input::KEY::UNKNOWN,         // 0xB4: VK_LAUNCH_MAIL
        Input::KEY::UNKNOWN,         // 0xB5: VK_LAUNCH_MEDIA_SELECT
        Input::KEY::UNKNOWN,         // 0xB6: VK_LAUNCH_APP1
        Input::KEY::UNKNOWN,         // 0xB7: VK_LAUNCH_APP2
        Input::KEY::UNKNOWN,         // 0xB8: Reserved
        Input::KEY::UNKNOWN,         // 0xB9: Reserved
        Input::KEY::UNKNOWN,         // 0xBA: VK_OEM_1
        Input::KEY::UNKNOWN,         // 0xBB: VK_OEM_PLUS
        Input::KEY::UNKNOWN,         // 0xBC: VK_OEM_COMMA
        Input::KEY::UNKNOWN,         // 0xBD: VK_OEM_MINUS
        Input::KEY::UNKNOWN,         // 0xBE: VK_OEM_PERIOD
        Input::KEY::UNKNOWN,         // 0xBF: VK_OEM_2
        Input::KEY::UNKNOWN,         // 0xC0: VK_OEM_3
        Input::KEY::UNKNOWN,         // 0xC1: Reserved
        Input::KEY::UNKNOWN,         // 0xC2: Reserved
        Input::KEY::UNKNOWN,         // 0xC3: VK_GAMEPAD_A
        Input::KEY::UNKNOWN,         // 0xC4: VK_GAMEPAD_B
        Input::KEY::UNKNOWN,         // 0xC5: VK_GAMEPAD_X
        Input::KEY::UNKNOWN,         // 0xC6: VK_GAMEPAD_Y
        Input::KEY::UNKNOWN,         // 0xC7: VK_GAMEPAD_RIGHT_SHOULDER
        Input::KEY::UNKNOWN,         // 0xC8: VK_GAMEPAD_LEFT_SHOULDER
        Input::KEY::UNKNOWN,         // 0xC9: VK_GAMEPAD_LEFT_TRIGGER
        Input::KEY::UNKNOWN,         // 0xCA: VK_GAMEPAD_RIGHT_TRIGGER
        Input::KEY::UNKNOWN,         // 0xCB: VK_GAMEPAD_DPAD_UP
        Input::KEY::UNKNOWN,         // 0xCC: VK_GAMEPAD_DPAD_DOWN
        Input::KEY::UNKNOWN,         // 0xCD: VK_GAMEPAD_DPAD_LEFT
        Input::KEY::UNKNOWN,         // 0xCE: VK_GAMEPAD_DPAD_RIGHT
        Input::KEY::UNKNOWN,         // 0xCF: VK_GAMEPAD_MENU
        Input::KEY::UNKNOWN,         // 0xD0: VK_GAMEPAD_VIEW
        Input::KEY::UNKNOWN,         // 0xD1: VK_GAMEPAD_LEFT_THUMBSTICK_BUTTON
        Input::KEY::UNKNOWN,         // 0xD2: VK_GAMEPAD_RIGHT_THUMBSTICK_BUTTON
        Input::KEY::UNKNOWN,         // 0xD3: VK_GAMEPAD_LEFT_THUMBSTICK_UP
        Input::KEY::UNKNOWN,         // 0xD4: VK_GAMEPAD_LEFT_THUMBSTICK_DOWN
        Input::KEY::UNKNOWN,         // 0xD5: VK_GAMEPAD_LEFT_THUMBSTICK_RIGHT
        Input::KEY::UNKNOWN,         // 0xD6: VK_GAMEPAD_LEFT_THUMBSTICK_LEFT
        Input::KEY::UNKNOWN,         // 0xD7: VK_GAMEPAD_RIGHT_THUMBSTICK_UP
        Input::KEY::UNKNOWN,         // 0xD8: VK_GAMEPAD_RIGHT_THUMBSTICK_DOWN
        Input::KEY::UNKNOWN,         // 0xD9: VK_GAMEPAD_RIGHT_THUMBSTICK_RIGHT
        Input::KEY::UNKNOWN,         // 0xDA: VK_GAMEPAD_RIGHT_THUMBSTICK_LEFT
        Input::KEY::UNKNOWN,         // 0xDB: VK_OEM_4
        Input::KEY::UNKNOWN,         // 0xDC: VK_OEM_5
        Input::KEY::UNKNOWN,         // 0xDD: VK_OEM_6
        Input::KEY::UNKNOWN,         // 0xDE: VK_OEM_7
        Input::KEY::UNKNOWN,         // 0xDF: VK_OEM_8
        Input::KEY::UNKNOWN,         // 0xE0: Reserved
        Input::KEY::UNKNOWN,         // 0xE1: OEM specific
        Input::KEY::UNKNOWN,         // 0xE2: VK_OEM_102
        Input::KEY::UNKNOWN,         // 0xE3: OEM specific
        Input::KEY::UNKNOWN,         // 0xE4: OEM specific
        Input::KEY::UNKNOWN,         // 0xE5: VK_PROCESSKEY
        Input::KEY::UNKNOWN,         // 0xE6: OEM specific
        Input::KEY::UNKNOWN,         // 0xE7: VK_PACKET
        Input::KEY::UNKNOWN,         // 0xE8: Unassigned
        Input::KEY::UNKNOWN,         // 0xE9: OEM specific
        Input::KEY::UNKNOWN,         // 0xEA: OEM specific
        Input::KEY::UNKNOWN,         // 0xEB: OEM specific
        Input::KEY::UNKNOWN,         // 0xEC: OEM specific
        Input::KEY::UNKNOWN,         // 0xED: OEM specific
        Input::KEY::UNKNOWN,         // 0xEE: OEM specific
        Input::KEY::UNKNOWN,         // 0xEF: OEM specific
        Input::KEY::UNKNOWN,         // 0xF0: OEM specific
        Input::KEY::UNKNOWN,         // 0xF1: OEM specific
        Input::KEY::UNKNOWN,         // 0xF2: OEM specific
        Input::KEY::UNKNOWN,         // 0xF3: OEM specific
        Input::KEY::UNKNOWN,         // 0xF4: OEM specific
        Input::KEY::UNKNOWN,         // 0xF5: OEM specific
        Input::KEY::UNKNOWN,         // 0xF6: VK_ATTN
        Input::KEY::UNKNOWN,         // 0xF7: VK_CRSEL
        Input::KEY::UNKNOWN,         // 0xF8: VK_EXSEL
        Input::KEY::UNKNOWN,         // 0xF9: VK_EREOF
        Input::KEY::UNKNOWN,         // 0xFA: VK_PLAY
        Input::KEY::UNKNOWN,         // 0xFB: VK_ZOOM
        Input::KEY::UNKNOWN,         // 0xFC: VK_NONAME
        Input::KEY::UNKNOWN,         // 0xFD: VK_PA1
        Input::KEY::UNKNOWN,         // 0xFE: VK_OEM_CLEAR
    };

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
            ::DWORD res       = ::GetLogicalProcessorInformationEx( ::RelationProcessorCore, pBuffer, &bufferLen );
            if( res == FALSE )
            {
                auto err = ::GetLastError();
                if( err == ERROR_INSUFFICIENT_BUFFER )
                {
                    pBuffer = (::PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)VKE_MALLOC( bufferLen );
                }
            }
            if( pBuffer )
            {
                VKE_FREE( pBuffer );
                bufferLen = 0;
            }
            res = ::GetLogicalProcessorInformationEx( ::RelationCache, pBuffer, &bufferLen );
            if( res == FALSE )
            {
                auto err = ::GetLastError();
                if( err == ERROR_INSUFFICIENT_BUFFER )
                {
                    pBuffer = (::PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)VKE_MALLOC( bufferLen );
                }
                if( pBuffer )
                {
                    // const ::CACHE_RELATIONSHIP& Cache = pBuffer->Cache;
                    VKE_FREE( pBuffer );
                    bufferLen = 0;
                }
            }
        }
        return m_ProcessorInfo;
    }

    cstr_t Platform::GetCmdLine()
    {
        return ::GetCommandLineA();
    }

    static _CrtMemState                         g_sMemState1, g_sMemState2;
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
        // TODO(blturkot): If we go for the option -fsanitize:
        // if(VKE_COMPILER_GCC AND VKE_DEBUG)
        //     target_compile_options(vke PRIVATE -fsanitize=address -fno-omit-frame-pointer)
        //     target_link_options( vke PRIVATE - fsanitize = address )
        // endif()
        //
        // Then we can achieve something similar with:
        // #if VKE_COMPILER_GCC && defined(__SANITIZE_ADDRESS__)
        //     #include <sanitizer/lsan_interface.h>
        //     __lsan_do_recoverable_leak_check(); // reports leaks so far, keeps running
        // #endif

        return ret;
    }

    void Platform::Time::Sleep( uint32_t us )
    {
        std::this_thread::sleep_for( std::chrono::microseconds( us ) );
    }

    void Platform::Debug::PrintOutput( const cstr_t msg )
    {
        ::OutputDebugStringA( (LPCSTR)msg );
    }

    void Platform::Debug::PrintStallstack()
    {
        const int MAX_FRAMES = 64;
        void*     stack[ MAX_FRAMES ];

        // 1. Pobranie surowych adresów ramek stosu
        USHORT framesCount = CaptureStackBackTrace( 0, MAX_FRAMES, stack, NULL );

        // 2. Inicjalizacja menedżera symboli Windows
        HANDLE process = GetCurrentProcess();
        SymSetOptions( SYMOPT_LOAD_LINES | SYMOPT_DEFERRED_LOADS );
        if( !SymInitialize( process, NULL, TRUE ) )
        {
            return;
        }

        VKE_LOG( "CALLSTACK" );

        // Alokacja bufora na strukturę informacji o symbolu (nazwa funkcji)
        char         symbolBuffer[ sizeof( SYMBOL_INFO ) + MAX_SYM_NAME * sizeof( TCHAR ) ];
        PSYMBOL_INFO pSymbol  = (PSYMBOL_INFO)symbolBuffer;
        pSymbol->SizeOfStruct = sizeof( SYMBOL_INFO );
        pSymbol->MaxNameLen   = MAX_SYM_NAME;

        IMAGEHLP_LINE64 line;
        line.SizeOfStruct = sizeof( IMAGEHLP_LINE64 );
        DWORD displacement;

        // 3. Iteracja po zebranych ramkach i zamiana adresów na tekst
        for( USHORT i = 0; i < framesCount; ++i )
        {
            DWORD64 address = (DWORD64)( stack[ i ] );

            // Pomiń ramki samego mechanizmu drukowania, jeśli chcesz zachować czystszy log
            if( address == 0 )
            {
                continue;
            }

            // std::cout << "[" << i << "] Adres: 0x" << std::hex << address << std::dec << " -> ";

            // Pobieranie nazwy funkcji
            if( SymFromAddr( process, address, 0, pSymbol ) )
            {
                // std::cout << pSymbol->Name;

                // Pobieranie pliku i numeru linii (opcjonalnie)
                if( SymGetLineFromAddr64( process, address, &displacement, &line ) )
                {
                    // std::cout << " (" << line.FileName << ":" << line.LineNumber << ")";
                    VKE_LOGF( "{}", pSymbol->Name );
                }
            }
            else
            {
                VKE_LOGF( "Unknown function" );
            }
        }

        // Cyszczenie pamięci po dbghelp
        SymCleanup( process );
    }

    void Platform::Debug::ConvertErrorCodeToText( uint32_t err, char* pBuffOut, uint32_t buffSize )
    {
        ::LPVOID pMsgBuff;
        ::FormatMessageA( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                          nullptr,
                          err,
                          MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),
                          (LPSTR)&pMsgBuff,
                          0,
                          nullptr );

        // const auto len = lstrlen( ( LPCTSTR )pMsgBuff );
        strcpy_s( pBuffOut, buffSize, (LPCTSTR)pMsgBuff );

        LocalFree( pMsgBuff );
    }

    handle_t Platform::DynamicLibrary::Load( const cstr_t name )
    {
        return reinterpret_cast< handle_t >( ::LoadLibraryA( name ) );
    }

    void Platform::DynamicLibrary::Close( const handle_t& handle )
    {
        ::FreeLibrary( reinterpret_cast< HMODULE >( handle ) );
    }

    void* Platform::DynamicLibrary::GetProcAddress( const handle_t& handle, const void* pSymbol )
    {
        return reinterpret_cast< void* >(
            ::GetProcAddress( reinterpret_cast< HMODULE >( handle ), reinterpret_cast< LPCSTR >( pSymbol ) ) );
    }

    Platform::Time::TimePoint Platform::Time::GetHighResClockFrequency()
    {
        ::LARGE_INTEGER Freq;
        if( ::QueryPerformanceFrequency( &Freq ) == TRUE )
        {
            return Freq.QuadPart;
        }
        return 1;
    }

    Platform::Time::TimePoint Platform::Time::GetHighResClockTimePoint()
    {
        ::LARGE_INTEGER Counter;
        if( ::QueryPerformanceCounter( &Counter ) == TRUE )
        {
            return Counter.QuadPart;
        }
        return 0;
    }

    double Platform::Time::TimePointToMicroseconds( TimePoint ticks, TimePoint freq )
    {
        double t = (double)ticks * 1000000;
        return t / freq;
    }

    bool Platform::File::Exists( cstr_t pFileName )
    {
        ::WIN32_FIND_DATA FindData;
        ::HANDLE          handle = ::FindFirstFileA( pFileName, &FindData );
        bool              exists = false;
        if( handle != INVALID_HANDLE_VALUE )
        {
            exists = true;
        }
        FindClose( handle );
        return exists;
    }

    bool Platform::File::IsDirectory( cstr_t pFileName )
    {
        bool ret;
        /*const std_filesystem::path path(pFileName);
        std::error_code err;
        ret = std_filesystem::is_directory(path, err);
        return ret;*/
        ::DWORD dwAttrs = ::GetFileAttributesA( pFileName );
        ret             = dwAttrs & FILE_ATTRIBUTE_DIRECTORY;
        return ret;
    }

    uint32_t Platform::File::GetSize( cstr_t pFileName )
    {
        handle_t hFile = Open( pFileName, Modes::READ );
        uint32_t size  = GetSize( hFile );
        Close( &hFile );
        return size;
    }

    uint32_t Platform::File::GetSize( handle_t hFile )
    {
        ::HANDLE hNative = reinterpret_cast< ::HANDLE >( hFile );
        return ::GetFileSize( hNative, nullptr );
    }

    uint32_t Platform::File::GetDirectory( cstr_t pFileName, uint32_t fileNameSize, char** ppOut )
    {
        assert( ppOut && *ppOut );
        uint32_t dirNameSize = fileNameSize;
        if( pFileName )
        {
            if( !IsDirectory( pFileName ) )
            {
                char*    pBuff        = *ppOut;
                uint32_t charPosition = 0;
                // Find last '/' or '\\' character
                for( uint32_t i = fileNameSize; i-- > 0; )
                {
                    if( pFileName[ i ] == '\\' || pFileName[ i ] == '/' )
                    {
                        charPosition = fileNameSize - i;
                        break;
                    }
                }

                dirNameSize = fileNameSize - charPosition;
                Memory::Copy( pBuff, fileNameSize, pFileName, dirNameSize );
                pBuff[ dirNameSize ] = '\0';
            }
            else
            {
                Memory::Copy( *ppOut, fileNameSize, pFileName, fileNameSize );
                ( *ppOut )[ fileNameSize ] = '\0';
            }
        }
        return dirNameSize;
    }

    bool Platform::File::GetWorkingDirectory( const uint32_t bufferSize, char** ppOut )
    {
        bool  ret = false;
        DWORD dw  = ::GetCurrentDirectory( (DWORD)bufferSize, *ppOut );
        ret       = dw != 0;
        return ret;
    }

    handle_t Platform::File::Create( cstr_t pFileName, MODE mode )
    {
        ::DWORD dwAccess = 0;
        ::DWORD dwShare  = 0;
        if( mode & Modes::READ )
        {
            dwAccess |= GENERIC_READ;
            dwShare  |= FILE_SHARE_READ;
        }
        if( mode & Modes::WRITE )
        {
            dwAccess |= GENERIC_WRITE;
            dwShare  |= FILE_SHARE_WRITE;
        }

        handle_t ret = 0;
        ::HANDLE hFile =
            ::CreateFileA( pFileName, dwAccess, dwShare, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr );
        if( hFile != INVALID_HANDLE_VALUE )
        {
            ret = reinterpret_cast< handle_t >( hFile );
        }
        else
        {
            char pBuffer[ 2048 ];
            GetErrorMessage( GetLastError(), pBuffer, sizeof( pBuffer ) );
            VKE_LOG_ERR( "Unable to create file: " << pFileName << " error:\n" << pBuffer );
        }
        return ret;
    }

    bool Platform::File::CreateDir( cstr_t pDirPath )
    {
        bool ret = true;
        if( !Exists( pDirPath ) )
        {
            char pFolder[ MAX_PATH ], pPath[ MAX_PATH ];
            ZeroMemory( pFolder, sizeof( pFolder ) );
            const uint32_t len = (uint32_t)strlen( pDirPath );
            for( uint32_t i = 0; i < len; ++i )
            {
                if( pDirPath[ i ] == '/' )
                {
                    pPath[ i ] = '\\';
                }
                else
                {
                    pPath[ i ] = pDirPath[ i ];
                }
            }
            pPath[ len + 0 ] = '\\';
            pPath[ len + 1 ] = 0;

            cstr_t pEnd = strchr( pPath, '\\' );
            while( pEnd != nullptr )
            {
                strncpy_s( pFolder, pPath, pEnd - pPath + 1 );
                if( !::CreateDirectoryA( pFolder, NULL ) )
                {
                    LogError();
                    // break;
                }
                pEnd = strchr( ++pEnd, '\\' );
            }
        }
        return ret;
    }

    bool Platform::File::IsRelativePath( cstr_t pPath )
    {
        std_filesystem::path Path( pPath );
        return Path.is_relative();
    }

    bool Platform::File::IsAbsolutePath( cstr_t pPath )
    {
        std_filesystem::path Path( pPath );
        return Path.is_absolute();
    }

    handle_t Platform::File::Open( cstr_t pFileName, MODE mode )
    {
        ::DWORD dwAccess     = 0;
        ::DWORD dwShare      = 0;
        ::DWORD dwCreateDesc = 0;
        if( mode & Modes::READ )
        {
            dwAccess     |= GENERIC_READ;
            dwShare      |= FILE_SHARE_READ;
            dwCreateDesc  = OPEN_EXISTING;
        }
        if( mode & Modes::WRITE )
        {
            dwAccess     |= GENERIC_WRITE;
            dwShare      |= FILE_SHARE_WRITE;
            dwCreateDesc  = OPEN_ALWAYS;
        }

        handle_t ret = 0;
        ::HANDLE hFile =
            ::CreateFileA( pFileName, dwAccess, dwShare, nullptr, dwCreateDesc, FILE_ATTRIBUTE_NORMAL, nullptr );
        if( hFile != INVALID_HANDLE_VALUE )
        {
            ret = reinterpret_cast< handle_t >( hFile );
        }
        else
        {
            LogError( pFileName );
        }
        return ret;
    }

    void Platform::File::Close( handle_t* phFile )
    {
        handle_t& hFile   = *phFile;
        ::HANDLE  hNative = reinterpret_cast< ::HANDLE >( hFile );
        ::CloseHandle( hNative );
        hFile = 0;
    }

    bool Platform::File::Seek( handle_t hFile, uint32_t offset, SEEK_MODE mode )
    {
        static const uint32_t aModes[] = { FILE_BEGIN, FILE_CURRENT, FILE_END };
        ::HANDLE              hNative  = reinterpret_cast< ::HANDLE >( hFile );
        ::LARGE_INTEGER       Offset;
        Offset.QuadPart = offset;
        return ::SetFilePointerEx( hNative, Offset, nullptr, aModes[ mode ] );
    }

    uint32_t Platform::File::Read( handle_t hFile, SReadData* pData )
    {
        ::HANDLE hNative = reinterpret_cast< ::HANDLE >( hFile );
        ::DWORD  dwCount;
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

    uint32_t Platform::File::Write( handle_t hFile, const SWriteInfo& Info )
    {
        ::HANDLE hNative = reinterpret_cast< ::HANDLE >( hFile );
        ::DWORD  dwCount;
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

    cstr_t Platform::File::GetExtension( cstr_t pFileName )
    {
        cstr_t pExt = strrchr( pFileName, '.' );
        if( pExt )
        {
            return pExt + 1;
        }
        return pExt;
    }

    cstr_t Platform::File::GetExtension( handle_t /*hFile*/ )
    {
        assert( 0 && "not implemented" );
        return nullptr;
    }

    bool Platform::File::GetFileName( cstr_t pFilePath, bool includeExtension, char** ppOut )
    {
        bool                 ret = false;
        std_filesystem::path Path( pFilePath );
        if( Path.has_filename() )
        {
            if( includeExtension )
            {
                const auto& strFileName = Path.filename().string();
                Memory::Copy( *ppOut, strFileName.size(), strFileName.c_str(), strFileName.size() );
                ( *ppOut )[ strFileName.length() ] = 0;
            }
            else
            {
                const auto& strFileName = Path.stem().string();
                Memory::Copy( *ppOut, strFileName.size(), strFileName.c_str(), strFileName.size() );
                ( *ppOut )[ strFileName.length() ] = 0;
            }
            ret = true;
        }
        return ret;
    }

    Platform::Thread::ID Platform::Thread::GetID()
    {
        return ::GetCurrentThreadId();
    }

    Platform::Thread::ID Platform::Thread::GetID( const handle_t& hThread )
    {
        return ::GetThreadId( reinterpret_cast< HANDLE >( hThread ) );
    }

    Platform::Thread::ID Platform::Thread::GetID( void* pHandle )
    {
        return ::GetThreadId( pHandle );
    }

    void Platform::Thread::Sleep( uint32_t us )
    {
        Platform::Time::Sleep( us );
    }

    /* void Platform::Thread::MemoryBarrier()
     {
         __faststorefence();
     }*/

    void Platform::Thread::Pause()
    {
        ::YieldProcessor();
        // std::this_thread::sleep_for( std::chrono::nanoseconds( 1 ) );
        // Sleep( 1000 );
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
        // while( m_interlock == 1 || __sync_lock_test_and_set(&m_interlock, 1) == 1 );
    }

    void Platform::Thread::CSpinlock::Unlock()
    {
        if( --m_lockCount == 0 )
        {
            // m_isLocked = 0;
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

    void Platform::Thread::SetDesc( cstr_t pText )
    {
        Utils::TCString< wchar_t > Text = ResourceName( pText );
        ::SetThreadDescription( ::GetCurrentThread(), Text.GetData() );
    }

    bool Platform::Thread::Wait( const ThreadFence& hFence, uint32_t value, Time::TimePoint timeout )
    {
        bool timeoutReached = false;
        if( timeout == 0 )
        {
            timeoutReached = hFence.Load() <= value;
        }
        else
        {
            const auto      Freq      = Time::GetHighResClockFrequency();
            Time::TimePoint StartTime = Time::GetHighResClockTimePoint();
            auto volatile fenceValue  = hFence.Load();
            while( fenceValue != value )
            {
                fenceValue              = hFence.Load();
                Time::TimePoint EndTime = Time::GetHighResClockTimePoint();
                auto            deltaT  = Time::TimePointToMicroseconds( EndTime - StartTime, Freq );
                if( deltaT > timeout )
                {
                    timeoutReached = true;
                    break;
                }
                Pause();
            }
        }
        return timeoutReached;
    }

} // namespace VKE

// #if VKE_COMPILER_VISUAL_STUDIO || VKE_COMPILER_GCC
// #   pragma pop_macro(VKE_TO_STRING(LoadLibrary))
// #else
// #   define LoadLibrary ::LoadLibraryA
// #endif

#endif // VKE_WINDOWS
