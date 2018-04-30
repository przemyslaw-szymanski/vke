#include "Core/Platform/CWindow.h"
#if VKE_WINDOWS
#include <windows.h>
#include <iostream>

#include "Core/Utils/CLogger.h"
#include "CVkEngine.h"
#include "Core/Threads/CThreadPool.h"

#include "RenderSystem/CGraphicsContext.h"
#include "RenderSystem/CSwapChain.h"
#include "Core/Platform/CPlatform.h"

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
            _MAX_COUNT
        };
    };
    using WINDOW_MSG = WindowMessages::MSG;

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
            DWORD style;
            DWORD exStyle;
            ExtentU32 Size;
        };

        SWindowMode aWindowModes[ WindowModes::_MAX_COUNT ];

        SWindowInternal()
        {
        }
    };

    LRESULT CALLBACK WndProc(HWND window, UINT msg, WPARAM wparam, LPARAM lparam);

    CWindow::CWindow(CVkEngine* pEngine) :
        m_pEngine(pEngine)
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
            printf("WND %p waiting for messages\n", this);
            WaitForMessages();
            printf("WND messages processed\n");
            Threads::ScopedLock l(m_SyncObj);
            VKE_DELETE(m_pPrivate);
            printf("WND private deleted\n");
            m_pPrivate = nullptr;
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
        m_Desc = Info;
        m_pPrivate = VKE_NEW SWindowInternal;
        m_pPrivate->osThreadId = Platform::ThisThread::GetID();

        if (m_Desc.hWnd == 0)
        {
            int posX = 0;
            int posY = 0;
            
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
                Mode.Size.width = desktop.right;
                Mode.Size.height = desktop.bottom;
            }

            //SET_MODE:
            //if (m_Desc.mode == WindowModes::FULLSCREEN)
            //{
            //    style = styleFullscreen;
            //    exStyle = exStyleFullscreen;

            //    DEVMODE ScreenSettings = { 0 };
            //    ScreenSettings.dmSize = sizeof(ScreenSettings);
            //    ScreenSettings.dmPelsWidth = m_Desc.Size.width;
            //    ScreenSettings.dmPelsHeight = m_Desc.Size.height;
            //    ScreenSettings.dmBitsPerPel = 32;
            //    ScreenSettings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
            //    if( ::ChangeDisplaySettingsA(&ScreenSettings, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL )
            //    {
            //        // If fullscreen is not possible run in window mode
            //        m_Desc.mode = WindowModes::FULLSCREEN_WINDOW;
            //        goto SET_MODE;
            //    }

            //    ::SetCursor(nullptr);
            //    ::ShowCursor(false);
            //}
            //else if( m_Desc.mode == WindowModes::FULLSCREEN_WINDOW )
            //{
            //    m_Desc.Size.width = desktop.right;
            //    m_Desc.Size.height = desktop.bottom;
            //    posX = 0;
            //    posY = 0;
            //    exStyle = exStyleFullscreenWindow;
            //    style = styleFullscreenWindow;

            //    ::SetCursor(nullptr);
            //    ::ShowCursor(false);
            //}
            //else
            //{
            //    exStyle = exStyleWindow;
            //    style = styleWindow;
            //    posX = (GetSystemMetrics(SM_CXSCREEN) - m_Desc.Size.width) / 2;
            //    posY = (GetSystemMetrics(SM_CYSCREEN) - m_Desc.Size.height) / 2;
            //    if( m_Desc.Position.x == 0 )
            //        m_Desc.Position.x = posX;
            //    if( m_Desc.Position.y == 0 )
            //        m_Desc.Position.y = posY;
            //}

            WNDCLASS wc = { 0 };
            RECT rect;

            const char* title = m_Desc.pTitle;

            wc.lpfnWndProc = VKE::WndProc;
            wc.hInstance = GetModuleHandle(NULL);
            wc.hCursor = LoadCursor(NULL, IDC_ARROW);
            wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
            wc.lpszClassName = title;
            wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
            if (!RegisterClass(&wc)) return VKE_FAIL;

            if (!SetRect(&rect, 0, 0, m_Desc.Size.width, m_Desc.Size.height)) return VKE_FAIL;
            if (!AdjustWindowRectEx(&rect, style, FALSE, exStyle)) return VKE_FAIL;
            int wndWidth = rect.right - rect.left;
            int wndHeight = rect.bottom - rect.top;
            
            HWND hWnd = CreateWindowExA(exStyle, title, title, style, posX, posY, wndWidth, wndHeight, NULL, NULL,
                                        wc.hInstance, 0);
            if (!hWnd)
            {
                VKE_LOG_ERR("Unable to create window: " << title);
                return VKE_FAIL;
            }

            m_pPrivate->hDC = GetDC(hWnd);
            if (!m_pPrivate->hDC )
            {
                VKE_LOG_ERR("Unable to create device context for window: " << title);
                return VKE_FAIL;
            }
            
            ::SetFocus(hWnd);
            ::UpdateWindow(hWnd);
            ::SetForegroundWindow(hWnd);
            
            m_Desc.hWnd = reinterpret_cast<handle_t>(hWnd);
            m_Desc.hProcess = reinterpret_cast<handle_t>(wc.hInstance);

            m_pPrivate->hWnd = hWnd;

            IsVisible(false);
            SetMode(m_Desc.mode, m_Desc.Size.width, m_Desc.Size.height);         

            return VKE_OK;
        }
        else
        {
            m_isCustomWindow = true;
            m_pPrivate->hWnd = reinterpret_cast<HWND>(m_Desc.hWnd);
            m_pPrivate->hDC = ::GetDC(m_pPrivate->hWnd);
            m_Desc.hProcess = reinterpret_cast<handle_t>(::GetModuleHandle(nullptr));
            return VKE_OK;
        }
    }

    void CWindow::_SendMessage(uint32_t msg)
    {
        if( m_needDestroy || m_isDestroyed )
            return;
        Threads::ScopedLock l( m_MsgQueueSyncObj );
        m_pPrivate->qMessages.push_back( static_cast<WINDOW_MSG>(msg) );
    }

    void CWindow::SetMode(WINDOW_MODE mode, uint32_t width, uint32_t height)
    {
        m_Desc.Size.width = width;
        m_Desc.Size.height = height;
        m_Desc.mode = mode;
        _SendMessage( WindowMessages::SET_MODE );
    }

    bool CWindow::_OnSetMode(WINDOW_MODE mode, uint32_t width, uint32_t height)
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
            width = w;
        }
        if( height == 0 )
        {
            height = h;
        }

        m_Desc.Size.width = width;
        m_Desc.Size.height = height;
        m_Desc.mode = mode;
        m_Desc.Position.x = mi.rcMonitor.left;
        m_Desc.Position.y = mi.rcMonitor.top;

        auto swpFlags = SWP_FLAGS;
        if( m_isVisible )
        {
            swpFlags |= SWP_SHOWWINDOW;
        }

        switch( mode )
        {
            case WindowModes::FULLSCREEN:
            {
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
                ::SetWindowPos(m_pPrivate->hWnd, NULL, mi.rcMonitor.left, mi.rcMonitor.top, w, h, swpFlags);
                ::InvalidateRect(m_pPrivate->hWnd, nullptr, true);
                m_Desc.Size.width = w;
                m_Desc.Size.height = h;
                m_Desc.Position.x = mi.rcMonitor.left;
                m_Desc.Position.y = mi.rcMonitor.top;
                return true;
            }
            break;
            case WindowModes::WINDOW:
            {
                auto posX = ( w - m_Desc.Size.width ) / 2;
                auto posY = ( h - m_Desc.Size.height ) / 2;
                m_Desc.Position.x = posX;
                m_Desc.Position.y = posY;

                ::RECT rect;
                if( !SetRect(&rect, 0, 0, m_Desc.Size.width, m_Desc.Size.height) ) return false;
                if( !AdjustWindowRectEx(&rect, style, FALSE, exStyle) ) return false;
                
                int wndWidth = rect.right - rect.left;
                int wndHeight = rect.bottom - rect.top;

                m_Desc.Size.width = wndWidth;
                m_Desc.Size.height = wndHeight;

                ::SetWindowPos(m_pPrivate->hWnd, NULL, m_Desc.Position.x, m_Desc.Position.y,
                               m_Desc.Size.width, m_Desc.Size.height, swpFlags);
                ::InvalidateRect(m_pPrivate->hWnd, nullptr, true);
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

    void CWindow::NeedQuit(bool need)
    {
        if( !m_needQuit )
        {
            m_needQuit = need;
            _SendMessage(WindowMessages::CLOSE);
        }
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
            need = m_needQuit;
        }
        return need;
    }

    uint32_t CWindow::_PeekMessage()
    {
        assert(m_isDestroyed == false);
        assert(m_pPrivate);

        auto& qMsgs = m_pPrivate->qMessages;

        if( !qMsgs.empty() )
        {
            printf("WND lock before pop msg\n");
            m_MsgQueueSyncObj.Lock();
            auto msg = qMsgs.front();
            qMsgs.pop_front();
            printf("WND pop msg: %d\n", msg);
            m_MsgQueueSyncObj.Unlock();

            switch( msg )
            {
                case WindowMessages::SHOW:
                {
                    ::ShowWindow( m_pPrivate->hWnd, m_isVisible );
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
                    if( m_pPrivate->hWnd )
                        ::CloseWindow( m_pPrivate->hWnd );
                    if( m_pPrivate->hDC )
                        ::ReleaseDC(m_pPrivate->hWnd, m_pPrivate->hDC);
                    //::SendMessageA(m_pPrivate->hWnd, WM_DESTROY, 0, 0);
                    if( m_pPrivate->hWnd )
                        ::DestroyWindow(m_pPrivate->hWnd);
                    m_pPrivate->hWnd = nullptr;
                    m_pPrivate->hDC = nullptr;
                    
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
                    _OnResize( m_Desc.Size.width, m_Desc.Size.height );
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
        return true;
    }

    static const TaskState g_aTaskResults[] =
    {
        TaskStateBits::OK,
        TaskStateBits::REMOVE, // if m_needQuit == true
        TaskStateBits::OK,
        TaskStateBits::OK
    };

    TaskState CWindow::_UpdateTask()
    {
        //Threads::ScopedLock l(m_SyncObj);
        const bool needDestroy = NeedDestroy();
        const bool needUpdate = !needDestroy;
        if( needUpdate )
        {
            assert(m_isDestroyed == false);
            MSG msg = { 0 };
            HWND hWnd = m_pPrivate->hWnd;
            if( ::PeekMessage(&msg, hWnd, 0, 0, PM_REMOVE) )
            {
                {
                    ::TranslateMessage(&msg);
                    ::DispatchMessage(&msg);
                    //if(msg.message != 15 ) printf("translate %d : %d\n", msg.hwnd, msg.message);
                }
            }
            Threads::ScopedLock l(m_SyncObj);
            // Process messages from the application
            if( _PeekMessage() == 0 )
            {
                //else
                {
                    if( NeedUpdate() )
                    {
                        //Threads::ScopedLock l(m_SyncObj);
                        for( auto& Func : m_pPrivate->Callbacks.vUpdateCallbacks )
                        {
                            Func(this);
                        }

                        //assert(m_pSwapChain);
                        //m_pSwapChain->SwapBuffers();
                    }
                }
            }
        }
        return g_aTaskResults[ needDestroy ]; // if need destroy remove this task
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

    void CWindow::Resize( uint32_t w, uint32_t h )
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

    void CWindow::_OnResize(uint32_t w, uint32_t h)
    {
        for( auto& Func : m_pPrivate->Callbacks.vResizeCallbacks )
        {
            Func(this, w, h);
        }
        /*if( m_pSwapChain )
            m_pSwapChain->Resize(w, h);*/
    }

    Platform::Thread::ID CWindow::GetThreadId()
    {
        return m_pPrivate->osThreadId;
    }

    void CWindow::Close()
    {
        NeedQuit(true);
    }

    uint64_t CWindow::WndProc(void* hWnd, uint32_t msg, uint64_t wParam, uint64_t lParam)
    {
        //if(msg != 15 ) printf("msg: %d, %p\n", msg, hWnd);
        if( m_isDestroyed )
            return 0;
        switch( msg )
        {
            case WM_KEYDOWN:
            case WM_SYSKEYDOWN:
            {
                if( wParam == VK_ESCAPE )
                {
                    //PostQuitMessage(0);
                    NeedQuit(true);
                    printf("ESCAPE\n");
                }
                switch( wParam )
                {
                    case 'F':
                    case 'f':
                    {
                        SetMode( WindowModes::FULLSCREEN_WINDOW, 00, 00 );
                    }
                    break;
                    case 'W':
                    case 'w':
                    {
                        SetMode(WindowModes::WINDOW, 800, 600);
                    }
                    break;
                }
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
                NeedQuit(true);
                printf("Close: %p\n", hWnd);
                return 0;
            }
            break;
            case WM_SIZE:
            {
                uint32_t h = HIWORD(lParam);
                uint32_t w = LOWORD(lParam);
                _OnResize(w, h);
            }
            break;
            case WM_PAINT:
            {
                OnPaint();
            }
            break;
            default:
                return DefWindowProc(reinterpret_cast< HWND >( hWnd ), msg, wParam, lParam);
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
            return pWnd->WndProc(hWnd, msg, wparam, lparam);
        }
        return DefWindowProc(hWnd, msg, wparam, lparam);
    }
}

#endif // WINDOWS
