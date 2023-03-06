#include "Core/Platform/CWindow.h"
#include "Core/Input/EventListeners.h"
#if VKE_WINDOWS
#define NOMINMAX
#include <windows.h>
#include <windowsx.h>
#include <iostream>

#include "Core/Utils/CLogger.h"
#include "CVkEngine.h"
#include "Core/Threads/CThreadPool.h"

#include "RenderSystem/CGraphicsContext.h"
#include "RenderSystem/CSwapChain.h"
#include "Core/Platform/CPlatform.h"

#include "Core/Input/CInputSystem.h"

namespace VKE
{
    using ResizeCallbackVec = vke_vector< CWindow::ResizeCallback >;
    using PaintCallbackVec = vke_vector< CWindow::PaintCallback >;
    using DestroyCallbackVec = vke_vector< CWindow::DestroyCallback >;
    using KeyboardCallbackVec = vke_vector< CWindow::KeyboardCallback >;
    using MouseCallbackVec = vke_vector< CWindow::MouseCallback >;
    using UpdateCallbackVec = vke_vector< CWindow::UpdateCallback >;
    using ShowCallbackVec = vke_vector< CWindow::ShowCallback >;

    static const DWORD SWP_FLAGS = SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED;

    struct WindowMessages
    {
        enum MSG
        {
            UNKNOWN,
            SHOW,
            CLOSE,
            RESIZE,
            SET_MODE,
            SET_TEXT,
            _MAX_COUNT
        };
    };
    using WINDOW_MSG = WindowMessages::MSG;

    struct SDefaultInputListener : public Input::EventListeners::IInput
    {

    };
    static SDefaultInputListener    g_DefaultInputListener;

    struct SKeyMapping
    {
        static SKeyMapping& GetInstance()
        {
            static SKeyMapping m;
            return m;
        }

        SKeyMapping()
        {

        }

        Input::KEY operator[]( const uint64_t& idx ) { return m_mKeys[static_cast<uint32_t>(idx)]; }

        using KeyMap = vke_hash_map< uint32_t, Input::KEY >;
        KeyMap m_mKeys;
    };

    struct SWindowInternal
    {
        HWND    hWnd;
        HDC     hDC;
        HDC     hCompatibleDC;
        RenderSystem::CGraphicsContext* pCtx = nullptr;
        RenderSystem::CRenderSystem*    pRenderSystem = nullptr;
        Platform::Thread::ID            osThreadId;
        using MessageQueue = std::deque< WINDOW_MSG >;
        MessageQueue qMessages;

        struct
        {
            ResizeCallbackVec       vResizeCallbacks;
            PaintCallbackVec        vPaintCallbacks;
            DestroyCallbackVec      vDestroyCallbacks;
            KeyboardCallbackVec     vKeyboardCallbacks;
            MouseCallbackVec        vMouseCallbacks;
            UpdateCallbackVec	    vUpdateCallbacks;
            ShowCallbackVec         vShowCallbacks;
        } Callbacks;

        struct SWindowMode
        {
            DWORD       style;
            DWORD       exStyle;
            ExtentU16   Size;
        };

        SWindowMode aWindowModes[ WindowModes::_MAX_COUNT ];

        SWindowInternal()
        {
        }
    };

    LRESULT CALLBACK WndProc(HWND window, UINT msg, WPARAM wparam, LPARAM lparam);

    CWindow::CWindow(CVkEngine* pEngine) :
        m_pEngine(pEngine)
        , m_pInputListener{ &g_DefaultInputListener }
    {}

    CWindow::~CWindow()
    {
        Destroy();
    }

    void CWindow::Destroy()
    {
        if( m_isDestroyed )
            return;

        Close();
        printf("WND %p send close\n", this);

        if( m_pPrivate )
        {
            m_needDestroy = true;

            printf("WND %p waiting for messages\n", this);
            {
                Threads::ScopedLock l( m_MsgQueueSyncObj );
                m_pPrivate->qMessages.clear();
            }
            //WaitForMessages();
            {
                printf( "WND messages processed\n" );
                Threads::ScopedLock l( m_SyncObj );
                VKE_DELETE( m_pPrivate );
                m_pPrivate = nullptr;
            }
            printf("WND private deleted\n");
        }
        printf("WND destroyed\n");
        m_isDestroyed = true;
    }

    void CWindow::WaitForMessages()
    {
        while( !m_pPrivate->qMessages.empty() )
        {
            Platform::ThisThread::Pause();
        }
    }

    Result CWindow::Create(const SWindowDesc& Info)
    {
        Result ret = VKE_FAIL;
        m_Desc = Info;
        m_pPrivate = VKE_NEW SWindowInternal;
        m_pPrivate->osThreadId = Platform::ThisThread::GetID();

        if (m_Desc.hWnd == 0)
        {
            const DWORD exStyleWindow = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
            const DWORD exStyleFullscreen = WS_EX_APPWINDOW;
            const DWORD exStyleFullscreenWindow = WS_EX_APPWINDOW;
            const DWORD styleWindow = WS_OVERLAPPEDWINDOW;
            const DWORD styleFullscreen = WS_POPUP;
            const DWORD styleFullscreenWindow = WS_POPUP;

            DWORD exStyle = 0;
            DWORD style = 0;
            m_Desc.mode = WindowModes::WINDOW;

            RECT desktop;
            const HWND hDesktop = ::GetDesktopWindow();

            MONITORINFO MonitorInfo = { 0 };
            MonitorInfo.cbSize = sizeof(MonitorInfo);

            GetWindowRect(hDesktop, &desktop);

            {
                auto &Mode = m_pPrivate->aWindowModes[ WindowModes::FULLSCREEN ];
                Mode.exStyle = exStyleFullscreen;
                Mode.style = styleFullscreen;
                Mode.Size = m_Desc.Size;
            }
            {
                auto &Mode = m_pPrivate->aWindowModes[ WindowModes::WINDOW ];
                Mode.exStyle = exStyleWindow;
                Mode.style = styleWindow;
                Mode.Size = m_Desc.Size;
            }
            {
                auto &Mode = m_pPrivate->aWindowModes[ WindowModes::FULLSCREEN_WINDOW ];
                Mode.exStyle = exStyleFullscreenWindow;
                Mode.style = styleFullscreenWindow;
                Mode.Size.width = static_cast< uint16_t >( desktop.right );
                Mode.Size.height = static_cast< uint16_t >( desktop.bottom );
            }

            WNDCLASS wc = { 0 };
            RECT rect;

            const char* title = m_Desc.pTitle;

            wc.lpfnWndProc = VKE::WndProc;
            wc.hInstance = GetModuleHandle(NULL);
            wc.hCursor = LoadCursor(NULL, IDC_ARROW);
            wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
            wc.lpszClassName = title;
            wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
            if (!RegisterClass(&wc)) return VKE_FAIL;

            if (Info.Size.width == UNDEFINED_U16)
            {
                const uint16_t scrWidth = (uint16_t)GetSystemMetrics(SM_CXFULLSCREEN);
                m_Desc.Position.width = scrWidth;
            }
            if (Info.Size.height == UNDEFINED_U16)
            {
                const uint16_t scrHeight = (uint16_t)GetSystemMetrics(SM_CYFULLSCREEN);
                m_Desc.Position.height = scrHeight;
            }

            if (Info.Position.x == UNDEFINED_U16)
            {
                const uint16_t scrWidth = (uint16_t)GetSystemMetrics(SM_CXFULLSCREEN);
                m_Desc.Position.x = scrWidth / 2 - m_Desc.Size.width / 2;
            }
            if (Info.Position.y == UNDEFINED_U16)
            {
                const uint16_t scrHeight = (uint16_t)GetSystemMetrics(SM_CYFULLSCREEN);
                m_Desc.Position.y = scrHeight / 2 - m_Desc.Size.height / 2;
            }

            if (!SetRect(&rect, m_Desc.Position.x, m_Desc.Position.y, m_Desc.Size.width, m_Desc.Size.height)) return VKE_FAIL;
            if (!AdjustWindowRectEx(&rect, style, FALSE, exStyle)) return VKE_FAIL;

            HWND hWnd = CreateWindowExA( exStyle, title, title, style, Info.Position.x, Info.Position.y,
                                         m_Desc.Size.width, m_Desc.Size.height, NULL, NULL, wc.hInstance, 0 );
            if (!hWnd)
            {
                VKE_LOG_ERR("Unable to create window: " << title);
                goto ERR;
            }

            m_pPrivate->hDC = GetDC(hWnd);
            if (!m_pPrivate->hDC )
            {
                VKE_LOG_ERR("Unable to create device context for window: " << title);
                goto ERR;
            }

            ::SetFocus(hWnd);
            ::UpdateWindow(hWnd);
            ::SetForegroundWindow(hWnd);

            m_Desc.hWnd = reinterpret_cast<handle_t>(hWnd);
            m_Desc.hProcess = reinterpret_cast<handle_t>(wc.hInstance);

            m_pPrivate->hWnd = hWnd;

            IsVisible(false);
            SetMode(m_Desc.mode, m_Desc.Size.width, m_Desc.Size.height);

            ::GetClientRect( hWnd, &rect );
            m_ClientSize.width = ( image_dimm_t )(rect.right - rect.left);
            m_ClientSize.height = (image_dimm_t)( rect.bottom - rect.top );

            m_NewSize = m_Desc.Size;

            ret = VKE_OK;
        }
        else
        {
            m_isCustomWindow = true;
            m_pPrivate->hWnd = reinterpret_cast<HWND>(m_Desc.hWnd);
            m_pPrivate->hDC = ::GetDC(m_pPrivate->hWnd);
            m_Desc.hProcess = reinterpret_cast<handle_t>(::GetModuleHandle(nullptr));
            ret = VKE_OK;
        }

        if(VKE_SUCCEEDED(ret))
        {
            ret = VKE_FAIL;
            /*if (VKE_FAILED(Memory::CreateObject(&HeapAllocator, &m_pInputSystem)))
            {
                VKE_LOG_ERR("Unable to create memory for CInputSystem.");
                goto ERR;
            }*/
            Input::SInputSystemDesc InputDesc;
            InputDesc.hWnd = m_Desc.hWnd;
            if (VKE_FAILED(m_InputSystem._Create(InputDesc)))
            {
                goto ERR;
            }
            ret = VKE_OK;
        }

        return ret;

    ERR:
        return ret;
    }

    void CWindow::_SendMessage(uint32_t msg)
    {
        if( m_needDestroy || m_isDestroyed )
            return;
        Threads::ScopedLock l( m_MsgQueueSyncObj );
        m_pPrivate->qMessages.push_back( static_cast<WINDOW_MSG>(msg) );
    }

    void CWindow::SetMode(WINDOW_MODE mode, uint16_t width, uint16_t height)
    {
        m_Desc.Size.width = width;
        m_Desc.Size.height = height;
        m_Desc.mode = mode;
        _SendMessage( WindowMessages::SET_MODE );
    }

    bool CWindow::_OnSetMode(WINDOW_MODE mode, uint16_t width, uint16_t height)
    {
        DWORD style = m_pPrivate->aWindowModes[ mode ].style;
        DWORD exStyle = m_pPrivate->aWindowModes[ mode ].exStyle;

        ::SetWindowLong(m_pPrivate->hWnd, GWL_STYLE, style);
        ::SetWindowLong(m_pPrivate->hWnd, GWL_EXSTYLE, exStyle);

        HMONITOR hMon = ::MonitorFromWindow(m_pPrivate->hWnd, MONITOR_DEFAULTTONEAREST);
        MONITORINFO mi = { 0 };
        mi.cbSize = sizeof(mi);
        if( !::GetMonitorInfoA(hMon, &mi) )
        {
            return false;
        }

        auto w = mi.rcMonitor.right - mi.rcMonitor.left;
        auto h = mi.rcMonitor.bottom - mi.rcMonitor.top;
        if( width == 0 )
        {
            width = static_cast< uint16_t >( w );
        }
        if( height == 0 )
        {
            height = static_cast< uint16_t >( h );
        }

        m_Desc.Size.width = width;
        m_Desc.Size.height = height;
        m_Desc.mode = mode;

        auto swpFlags = SWP_FLAGS;
        if( m_isVisible )
        {
            swpFlags |= SWP_SHOWWINDOW;
        }

        switch( mode )
        {
            case WindowModes::FULLSCREEN:
            {
                m_Desc.Position.x = static_cast< uint16_t >( mi.rcMonitor.left );
                m_Desc.Position.y = static_cast< uint16_t >( mi.rcMonitor.top );

                ::SetWindowPos(m_pPrivate->hWnd, HWND_TOP, mi.rcMonitor.left, mi.rcMonitor.top, width, height,
                               swpFlags);

                DEVMODE ScreenSettings = { 0 };
                ScreenSettings.dmSize = sizeof(ScreenSettings);
                ScreenSettings.dmPelsWidth = m_Desc.Size.width;
                ScreenSettings.dmPelsHeight = m_Desc.Size.height;
                ScreenSettings.dmBitsPerPel = 32;
                ScreenSettings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
                auto res = ::ChangeDisplaySettingsA(&ScreenSettings, CDS_FULLSCREEN);
                if( res != DISP_CHANGE_SUCCESSFUL )
                {
                    return false;
                }
                ::InvalidateRect(m_pPrivate->hWnd, nullptr, true);
                return true;
            }
            break;
            case WindowModes::FULLSCREEN_WINDOW:
            {
                m_Desc.Position.x = static_cast< uint16_t >( mi.rcMonitor.left );
                m_Desc.Position.y = static_cast< uint16_t >( mi.rcMonitor.top );

                ::SetWindowPos(m_pPrivate->hWnd, NULL, mi.rcMonitor.left, mi.rcMonitor.top, w, h, swpFlags);
                ::InvalidateRect(m_pPrivate->hWnd, nullptr, true);

                m_Desc.Position.x = static_cast< uint16_t >( mi.rcMonitor.left );
                m_Desc.Position.y = static_cast< uint16_t >( mi.rcMonitor.top );
                return true;
            }
            break;
            case WindowModes::WINDOW:
            {
                auto posX = ( w - m_Desc.Size.width ) / 2;
                auto posY = ( h - m_Desc.Size.height ) / 2;
                if( m_Desc.Position.x == UNDEFINED_U32 )
                {
                    m_Desc.Position.x = static_cast< uint16_t >( posX );
                }
                if( m_Desc.Position.y == UNDEFINED_U32 )
                {
                    m_Desc.Position.y = static_cast< uint16_t >( posY );
                }

                ::RECT rect;
                if( !SetRect(&rect, 0, 0, m_Desc.Size.width, m_Desc.Size.height) ) return false;
                if( !AdjustWindowRectEx(&rect, style, FALSE, exStyle) ) return false;

                int wndWidth = rect.right - rect.left;
                int wndHeight = rect.bottom - rect.top;

                ::SetWindowPos( m_pPrivate->hWnd, NULL, m_Desc.Position.x, m_Desc.Position.y,
                    wndWidth, wndHeight, swpFlags );
                ::InvalidateRect( m_pPrivate->hWnd, nullptr, true );
                return true;
            }
            break;
        }
        return false;
    }

    void CWindow::IsVisible(bool isVisible)
    {
        m_isVisible = isVisible;
        //::ShowWindow((HWND)m_Desc.hWnd, m_isVisible);
        _SendMessage( WindowMessages::SHOW );
    }

    bool CWindow::NeedUpdate()
    {
        return IsVisible() && !NeedQuit() && !IsCustomWindow() && !m_isDestroyed;
    }

    bool CWindow::NeedDestroy()
    {
        return m_needDestroy;
    }

    bool CWindow::NeedQuit()
    {
        bool need;
        {
            //Threads::ScopedLock l(m_SyncObj);
            need = m_needDestroy;
        }
        return need;
    }

    void CWindow::SetText( cstr_t pText )
    {
        //Threads::ScopedLock l( m_SyncObj );
        if( !m_needDestroy )
        {
            m_strText = pText;
            _SendMessage( WindowMessages::SET_TEXT );
            //::SetWindowTextA( m_pPrivate->hWnd, pText );
        }
    }

    bool CWindow::IsActive()
    {
        HWND hWnd = GetActiveWindow();
        return hWnd == m_pPrivate->hWnd;
    }

    bool CWindow::HasFocus()
    {
        ::RECT Rect;
        ::GetWindowRect( m_pPrivate->hWnd, &Rect );
        //const auto Pos = m_pEngine->GetInputSystem()->GetState().Mouse.Position;
        const auto& Pos = m_InputSystem.GetState().Mouse.Position;
        bool hasFocus = Rect.left <= Pos.x && Rect.right >= Pos.x && Rect.top <= Pos.y && Rect.bottom >= Pos.y;
        return hasFocus;
    }

    uint32_t CWindow::_PeekMessage()
    {
        assert(m_isDestroyed == false);
        assert(m_pPrivate);

        auto& qMsgs = m_pPrivate->qMessages;

        if( !qMsgs.empty() )
        {
            //VKE_LOG("WND lock before pop msg");
            m_MsgQueueSyncObj.Lock();
            auto msg = qMsgs.front();
            qMsgs.pop_front();
            m_MsgQueueSyncObj.Unlock();
            //VKE_LOG("WND pop msg: " << msg);


            switch( msg )
            {
                case WindowMessages::SET_TEXT:
                {
                    ::SetWindowTextA( m_pPrivate->hWnd, m_strText.c_str() );
                }
                break;
                case WindowMessages::SHOW:
                {
                    //VKE_LOG( "Before showWindow" );
                    ::ShowWindow( m_pPrivate->hWnd, m_isVisible );
                    //VKE_LOG( "Before OnShow");
                    _OnShow();
                }
                break;
                case WindowMessages::CLOSE:
                {
                    m_needDestroy = true;
                    for( auto& Callback : m_pPrivate->Callbacks.vDestroyCallbacks )
                    {
                        Callback(this);
                    }
                    std::clog << "WND CLOSE";
                    if( m_pPrivate )
                    {
                        if( m_pPrivate->hWnd )
                            ::CloseWindow( m_pPrivate->hWnd );
                        if( m_pPrivate->hDC )
                            ::ReleaseDC( m_pPrivate->hWnd, m_pPrivate->hDC );
                        //::SendMessageA(m_pPrivate->hWnd, WM_DESTROY, 0, 0);
                        if( m_pPrivate->hWnd )
                            ::DestroyWindow( m_pPrivate->hWnd );
                        m_pPrivate->hWnd = nullptr;
                        m_pPrivate->hDC = nullptr;
                    }
                    m_MsgQueueSyncObj.Lock();
                    qMsgs.clear();
                    m_MsgQueueSyncObj.Unlock();
                }
                break;
                case WindowMessages::RESIZE:
                {
                    ::SetWindowPos( m_pPrivate->hWnd, nullptr,
                                    m_Desc.Position.x, m_Desc.Position.y,
                                    m_Desc.Size.width, m_Desc.Size.height,
                                    SWP_FLAGS );
                }
                break;
                case WindowMessages::SET_MODE:
                {
                    _OnSetMode( m_Desc.mode, m_Desc.Size.width, m_Desc.Size.height );
                }
                break;
            }
            return msg;
        }
        return 0;
    }

    bool CWindow::Update()
    {
        m_needUpdate = true;
        return true;
    }

    static const TaskState g_aTaskResults[] =
    {
        TaskStateBits::OK,
        TaskStateBits::REMOVE, // if m_needQuit == true
        TaskStateBits::OK,
        TaskStateBits::OK
    };

    static const TASK_RESULT g_aTaskResults2[] =
    {
        TaskResults::WAIT,
        TaskResults::OK, // if m_needQuit == true
        TaskResults::FAIL,
        TaskResults::FAIL
    };

    TASK_RESULT CWindow::_UpdateTask(void*)
    {
        //Threads::ScopedLock l(m_SyncObj);
        const bool needDestroy = NeedDestroy();
        const bool needUpdate = !needDestroy && m_needUpdate;
        if(needDestroy)
        {
            bool b = false;
            b = b;
        }
        if( needUpdate )
        {
            assert(m_isDestroyed == false);
            MSG msg = { 0 };
            HWND hWnd = m_pPrivate->hWnd;
            // Peek all messages except WM_INPUT
            //VKE_LOG("before peek1: " << hWnd);
            while( ::PeekMessageA( &msg, hWnd, 0, WM_INPUT - 1, PM_REMOVE ) != 0 )
            {
                ::TranslateMessage(&msg);
                ::DispatchMessage(&msg);
                //VKE_LOG( msg.message );
            }
            //VKE_LOG( "before peek2: " << hWnd );
            while( ::PeekMessageA( &msg, hWnd, WM_INPUT+1, (UINT)-1, PM_REMOVE ) != 0 )
            {
                ::TranslateMessage( &msg );
                ::DispatchMessage( &msg );
                //VKE_LOG( msg.message );
            }

            // Update Inputs
            m_InputSystem.Update();
            //if( _PeekMessage() == 0 )
            //VKE_LOG( "before peek3: " << hWnd );
            while(_PeekMessage())
            {
                //else
                {
                    
                }
            }
            //VKE_LOG( "need update: " << NeedUpdate() );
            if( NeedUpdate() )
            {
                // m_InputSystem.Update();
                //VKE_LOG( "before update: " << hWnd );
                _Update();
                // Threads::ScopedLock l(m_SyncObj);
                for( auto& Func: m_pPrivate->Callbacks.vUpdateCallbacks )
                {
                    Func( this );
                }
                // assert(m_pSwapChain);
                // m_pSwapChain->SwapBuffers();
            }
            m_needUpdate = false;
        }
        return g_aTaskResults2[ needDestroy ]; // if need destroy remove this task
    }

    void CWindow::_Update()
    {
        if( m_NewSize != m_Desc.Size )
        {
            m_checkSizeUpdateCount++;
        }
        if( m_checkSizeUpdateCount > 100 )
        {
            m_checkSizeUpdateCount = 0;
            m_Desc.Size = m_NewSize;
            _OnResize( m_Desc.Size.width, m_Desc.Size.height );
        }
    }

    void CWindow::AddDestroyCallback(DestroyCallback&& Func)
    {
        m_pPrivate->Callbacks.vDestroyCallbacks.push_back(Func);
    }

    void CWindow::AddPaintCallback(PaintCallback&& Func)
    {
        m_pPrivate->Callbacks.vPaintCallbacks.push_back(Func);
    }

    void CWindow::AddResizeCallback(ResizeCallback&& Func)
    {
        m_pPrivate->Callbacks.vResizeCallbacks.push_back(Func);
    }

    void CWindow::AddShowCallback(ShowCallback&& Func)
    {
        m_pPrivate->Callbacks.vShowCallbacks.push_back(Func);
    }

    void CWindow::SetSwapChain(RenderSystem::CSwapChain* pSwapChain)
    {
        m_pSwapChain = pSwapChain;
    }

    void CWindow::AddUpdateCallback(UpdateCallback&& Func)
    {
        m_pPrivate->Callbacks.vUpdateCallbacks.push_back(Func);
    }

    void CWindow::OnPaint()
    {
        for (auto& Func : m_pPrivate->Callbacks.vPaintCallbacks)
        {
            Func(this);
        }
    }

    void CWindow::Resize( uint16_t w, uint16_t h )
    {
        m_Desc.Size.width = w;
        m_Desc.Size.height = h;
        m_pPrivate->qMessages.push_back( WindowMessages::RESIZE );
    }

    void CWindow::_OnShow()
    {
        auto& vCallbacks = m_pPrivate->Callbacks.vShowCallbacks;
        for( auto& Callback : vCallbacks )
        {
            Callback(this);
        }
    }

    void CWindow::_OnResize(uint16_t w, uint16_t h)
    {
        ::RECT rect;
        ::GetClientRect( m_pPrivate->hWnd, &rect );
        m_ClientSize.width = (image_dimm_t)( rect.right - rect.left );
        m_ClientSize.height = ( image_dimm_t )( rect.top - rect.bottom );

        if( w > 0 && h > 0 )
        {
            for( auto& Func : m_pPrivate->Callbacks.vResizeCallbacks )
            {
                Func( this, w, h );
            }
        }
    }

    Platform::Thread::ID CWindow::GetThreadId()
    {
        return m_pPrivate->osThreadId;
    }

    void CWindow::Close()
    {
        if( !m_needDestroy )
        {
            _SendMessage( WindowMessages::CLOSE );
        }
    }

    Input::KEY ConvertVirtualKeyToInput( const uint64_t& idx )
    {
        return SKeyMapping::GetInstance()[idx];
    }

    uint64_t CWindow::WndProc(void* pWnd, uint32_t msg, uint64_t wParam, uint64_t lParam)
    {
        //if(msg != 15 ) printf("msg: %d, %p\n", msg, hWnd);
        if( m_isDestroyed )
            return 0;

        ::HWND hWnd = reinterpret_cast<::HWND>(pWnd);

        switch( msg )
        {
            case WM_INPUT:
            {
                //LPARAM p = ( LPARAM )lParam;
                //m_pEngine->GetInputSystem()->_ProcessWindowInput( ( void* )p );
            }
            break;

            case WM_DESTROY:
            {
                //PostQuitMessage(0);
                printf("Destroy: %p\n", hWnd);
                PostQuitMessage(0);
            }
            break;
            case WM_QUIT:
            {
                printf("Quit: %p\n", hWnd);
            }
            break;
            case WM_CLOSE:
            {
                Close();
                printf("Close: %p\n", hWnd);
                return 0;
            }
            break;
            case WM_SIZE:
            {
                uint16_t h = static_cast< uint16_t >( HIWORD( lParam ) );
                uint16_t w = static_cast< uint16_t >( LOWORD( lParam ) );
                m_NewSize = ExtentU16( w, h );
            }
            break;
            case WM_PAINT:
            {
                ::PAINTSTRUCT Ps;
                ::BeginPaint( hWnd, &Ps );
                OnPaint();
                ::EndPaint( hWnd, &Ps );
            }
            break;
            default:
                return DefWindowProc(reinterpret_cast< HWND >( hWnd ), msg, (WPARAM)wParam, (LPARAM)lParam);
        }
        //return DefWindowProc(hWnd, msg, wParam, lParam);
        return 0;
    }

    LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wparam, LPARAM lparam)
    {
        auto pEngine = VKEGetEngine();
        handle_t handle = reinterpret_cast<handle_t>( hWnd );
        WindowPtr pWnd = pEngine->FindWindowTS(handle);

        if( pWnd.IsValid() )
        {
            return pWnd->WndProc(hWnd, msg, (uint64_t)wparam, (uint64_t)lparam);
        }
        return DefWindowProc(hWnd, msg, wparam, lparam);
    }
}

#endif // WINDOWS
