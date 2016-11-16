#include "Core/Platform/CWindow.h"
#if VKE_WINDOWS
#include <windows.h>

#include "Core/Utils/CLogger.h"
#include "CVkEngine.h"
#include "Core/Threads/CThreadPool.h"

#include "RenderSystem/CGraphicsContext.h"
#include "RenderSystem/CSwapChain.h"

namespace VKE
{
    using ResizeCallbackVec = vke_vector< CWindow::ResizeCallback >;
    using PaintCallbackVec = vke_vector< CWindow::PaintCallback >;
    using DestroyCallbackVec = vke_vector< CWindow::DestroyCallback >;
    using KeyboardCallbackVec = vke_vector< CWindow::KeyboardCallback >;
    using MouseCallbackVec = vke_vector< CWindow::MouseCallback >;
    using UpdateCallbackVec = vke_vector< CWindow::UpdateCallback >;

    struct SWindowInternal
    {
        HWND    hWnd;
        HDC     hDC;
        HDC     hCompatibleDC;
        RenderSystem::CGraphicsContext* pCtx = nullptr;
        RenderSystem::CRenderSystem*          pRenderSystem = nullptr;
        std::mutex              mutex;
        std::thread::id         osThreadId;

        struct
        {
            ResizeCallbackVec   vResizeCallbacks;
            PaintCallbackVec    vPaintCallbacks;
            DestroyCallbackVec  vDestroyCallbacks;
            KeyboardCallbackVec vKeyboardCallbacks;
            MouseCallbackVec    vMouseCallbacks;
            UpdateCallbackVec	vUpdateCallbacks;
        } Callbacks;

        struct  
        {
            struct SIsVisible : public VKE::Threads::ITask
            {
                CWindow* pWnd = nullptr;
                bool isVisible = false;

                void _OnStart(uint32_t) override
                {
                    pWnd->IsVisible(isVisible);
                }
            };
            SIsVisible IsVisible;

            struct SUpdate : public VKE::Threads::ITask
            {
                CWindow* pWnd = nullptr;
                void _OnStart(uint32_t)
                {
                    pWnd->Update();
                }
            };
            SUpdate Update;
        } Tasks;
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
        VKE_DELETE( m_pPrivate );
    }

    Result CWindow::Create(const SWindowInfo& Info)
    {
        m_Desc = Info;
        m_pPrivate = VKE_NEW SWindowInternal;
        m_pPrivate->osThreadId = std::this_thread::get_id();

        if (m_Desc.wndHandle == 0)
        {
            int posX = 0;
            int posY = 0;

            if (m_Desc.fullScreen)
            {
                RECT desktop;
                const HWND hDesktop = GetDesktopWindow();
                GetWindowRect(hDesktop, &desktop);
                m_Desc.Size.width = desktop.right;
                m_Desc.Size.height = desktop.bottom;
            }
            else
            {
                posX = (GetSystemMetrics(SM_CXSCREEN) - m_Desc.Size.width) / 2;
                posY = (GetSystemMetrics(SM_CYSCREEN) - m_Desc.Size.height) / 2;
            }

            WNDCLASS wc = { 0 };
            RECT rect;

            const char* title = m_Desc.pTitle;

            wc.lpfnWndProc = WndProc;
            wc.hInstance = GetModuleHandle(NULL);
            wc.hCursor = LoadCursor(NULL, IDC_ARROW);
            wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
            wc.lpszClassName = title;
            if (!RegisterClass(&wc)) return VKE_FAIL;

            if (!SetRect(&rect, 0, 0, m_Desc.Size.width, m_Desc.Size.height)) return VKE_FAIL;
            if (!AdjustWindowRect(&rect, WS_OVERLAPPED | WS_SYSMENU | WS_CAPTION | WS_MINIMIZEBOX, FALSE)) return VKE_FAIL;
            int wndWidth = rect.right - rect.left;
            int wndHeight = rect.bottom - rect.top;
            HWND hWnd = CreateWindowA(title, title,
                WS_OVERLAPPED | WS_SYSMENU | WS_CAPTION | WS_MINIMIZEBOX | WS_VISIBLE,
                posX, posY, wndWidth, wndHeight,
                NULL, NULL, NULL, 0);
            if (!hWnd)
            {
                VKE_LOG_ERR("Unable to create window: " << title);
                return VKE_FAIL;
            }
            HDC hDC = GetDC(hWnd);
            if (!hDC)
            {
                VKE_LOG_ERR("Unable to create device context for window: " << title);
                return VKE_FAIL;
            }

            m_Desc.wndHandle = reinterpret_cast<handle_t>(hWnd);
            m_Desc.platformHandle = reinterpret_cast<handle_t>(wc.hInstance);

            PIXELFORMATDESCRIPTOR pfd = {};
            // 807C 1000000001111100
            //pfd.dwFlags = PFD_SUPPORT_COMPOSITION | PFD_DRAW_TO_WINDOW | PFD_DRAW_TO_BITMAP |
            //    PFD_SUPPORT_GDI | PFD_SUPPORT_OPENGL | PFD_GENERIC_FORMAT | PFD_GENERIC_ACCELERATED;
            pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER | PFD_TYPE_RGBA;
            pfd.cColorBits = 32;
            pfd.cRedShift = 16;
            pfd.cRedBits = 8;
            pfd.cGreenBits = 8;
            pfd.cBlueBits = 8;
            pfd.cGreenShift = 8;
            pfd.cAccumBits = 64;
            pfd.cAccumRedBits = 16;
            pfd.cAccumGreenBits = 16;
            pfd.cAccumBlueBits = 16;
            pfd.cDepthBits = 32;
            pfd.cStencilBits = 8;
            pfd.nSize = sizeof(pfd);
            pfd.nVersion = 1;
            pfd.iLayerType = PFD_MAIN_PLANE;
            auto pf = ChoosePixelFormat(hDC, &pfd);

            m_pPrivate->hWnd = hWnd;
            m_pPrivate->hDC = hDC;
            IsVisible(false);
            return VKE_OK;
        }
        else
        {
            m_isCustomWindow = true;
            m_pPrivate->hWnd = reinterpret_cast<HWND>(m_Desc.wndHandle);
            m_pPrivate->hDC = ::GetDC(m_pPrivate->hWnd);
            m_Desc.platformHandle = reinterpret_cast<handle_t>(::GetModuleHandle(nullptr));
            return VKE_OK;
        }
    }

    void CWindow::IsVisible(bool isVisible)
    {
        m_isVisible = isVisible;
        ::ShowWindow((HWND)m_Desc.wndHandle, m_isVisible);
    }

    void CWindow::IsVisibleAsync(bool bShow)
    {
        auto pThreadPool = VKEGetEngine()->GetThreadPool();
        m_pPrivate->Tasks.IsVisible.isVisible = bShow;
        m_pPrivate->Tasks.IsVisible.pWnd = this;
        pThreadPool->AddTask(m_pPrivate->osThreadId, &m_pPrivate->Tasks.IsVisible);
    }

    void CWindow::NeedQuit(bool need)
    {
        //Threads::LockGuard l(m_pPrivate->mutex);
        m_needQuit = need;
    }

    bool CWindow::NeedUpdate() const
    {
        return IsVisible() && !NeedQuit() && !IsCustomWindow();
    }

    bool CWindow::NeedQuit() const
    {
        bool need;
        {
            //Threads::LockGuard l(m_pPrivate->mutex);
            need = m_needQuit;
        }
        return need;
    }

    void CWindow::_Update()
    {
        
    }

    void CWindow::Update()
    {
        if(NeedUpdate())
        {
            MSG msg = { 0 };
            if(::PeekMessage(&msg, m_pPrivate->hWnd, 0, 0, PM_REMOVE))
            {
                if(msg.message == WM_QUIT)
                {
                    NeedQuit(true);
                }
                else
                {
                    ::TranslateMessage(&msg);
                    ::DispatchMessage(&msg);
                }
            }
            //else
            {
                if (!NeedQuit())
                {
                    for (auto& Func : m_pPrivate->Callbacks.vUpdateCallbacks)
                    {
                        Func(this);
                    }

                    //assert(m_pSwapChain);
                    //m_pSwapChain->SwapBuffers();
                }
            }
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

    void CWindow::SetSwapChain(RenderSystem::CSwapChain* pSwapChain)
    {
        m_pSwapChain = pSwapChain;
    }

    void CWindow::OnPaint()
    {
        for (auto& Func : m_pPrivate->Callbacks.vPaintCallbacks)
        {
            Func(this);
        }
    }

    void CWindow::Resize(uint32_t w, uint32_t h)
    {
        for (auto& Func : m_pPrivate->Callbacks.vResizeCallbacks)
        {
            Func(this, w, h);
        }
    }

    std::thread::id CWindow::GetThreadId()
    {
        return m_pPrivate->osThreadId;
    }

    void CWindow::AddUpdateCallback(UpdateCallback&& Func)
    {
        m_pPrivate->Callbacks.vUpdateCallbacks.push_back(Func);
    }

    RenderSystem::CGraphicsContext* CWindow::GetRenderingContext() const
    {
        return m_pSwapChain->GetGraphicsContext();
    }

    LRESULT CALLBACK WndProc(HWND window, UINT msg, WPARAM wparam, LPARAM lparam)
    {
        auto pEngine = VKEGetEngine();
        handle_t hWnd = reinterpret_cast<handle_t>(window);
        WindowPtr pWnd = pEngine->FindWindowTS(hWnd);
        if (pWnd.IsNull())
            return DefWindowProc(window, msg, wparam, lparam);;

        switch (msg)
        {
        case WM_KEYDOWN:
            if (wparam == VK_ESCAPE)
            {
                //PostQuitMessage(0);
            }
            break;
        case WM_DESTROY:
            pWnd->NeedQuit(true);
            pEngine->DestroyWindow(pWnd);
            //PostQuitMessage(0);
            break;
        case WM_SIZE:
        {
            uint32_t h = HIWORD(lparam);
            uint32_t w = LOWORD(lparam);
            pWnd->Resize(w, h);
        }
        break;
        case WM_PAINT:
        {           
            pWnd->OnPaint();
        }
        break;
   
        default:
            return DefWindowProc(window, msg, wparam, lparam);
        }
        return 0;
    }
}

#endif // WINDOWS
