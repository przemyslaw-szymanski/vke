#include "Platform/CWindow.h"
#if VKE_WINDOWS
#include <windows.h>

#include "Utils/CLogger.h"
#include "CVkEngine.h"

namespace VKE
{
    struct SWindowInternal
    {
        HWND    hWnd;
        HDC     hDC;
        HDC     hCompatibleDC;
        RenderSystem::CContext* pCtx = nullptr;
        CRenderSystem*          pRenderSystem = nullptr;
        std::mutex              mutex;
    };

    LRESULT CALLBACK WndProc(HWND window, UINT msg, WPARAM wparam, LPARAM lparam);

    CWindow::CWindow()
    {

    }

    CWindow::~CWindow()
    {
        Destroy();
    }

    void CWindow::Destroy()
    {
        VKE_DELETE( m_pInternal );
    }

    Result CWindow::Create(const SWindowInfo& Info)
    {
        m_Info = Info;
  
        if (m_Info.wndHandle == 0)
        {
            m_pInternal = VKE_NEW SWindowInternal;

            int posX = 0;
            int posY = 0;

            if (m_Info.fullScreen)
            {
                RECT desktop;
                const HWND hDesktop = GetDesktopWindow();
                GetWindowRect(hDesktop, &desktop);
                m_Info.Size.width = desktop.right;
                m_Info.Size.height = desktop.bottom;
            }
            else
            {
                posX = (GetSystemMetrics(SM_CXSCREEN) - m_Info.Size.width) / 2;
                posY = (GetSystemMetrics(SM_CYSCREEN) - m_Info.Size.height) / 2;
            }

            WNDCLASS wc = { 0 };
            RECT rect;

            const char* title = m_Info.pTitle;

            wc.lpfnWndProc = WndProc;
            wc.hInstance = GetModuleHandle(NULL);
            wc.hCursor = LoadCursor(NULL, IDC_ARROW);
            wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
            wc.lpszClassName = title;
            if (!RegisterClass(&wc)) return VKE_FAIL;

            if (!SetRect(&rect, 0, 0, m_Info.Size.width, m_Info.Size.height)) return VKE_FAIL;
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

            m_Info.wndHandle = reinterpret_cast<handle_t>(hWnd);
            m_Info.platformHandle = reinterpret_cast<handle_t>(wc.hInstance);

            /*bi = (BITMAPINFO *)malloc(sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * 256);
            if (!bi) goto label_error;

            memset(bi, 0, sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * 256);
            bi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            bi->bmiHeader.biPlanes = 1;
            bi->bmiHeader.biBitCount = 32;
            bi->bmiHeader.biCompression = BI_RGB;
            bi->bmiHeader.biWidth = m_Info.Size.width;
            bi->bmiHeader.biHeight = -m_Info.Size.height;
            bi->bmiHeader.biSizeImage = m_Info.Size.width * m_Info.Size.height;
            HDC hCompatibleDC = CreateCompatibleDC(hDC);
            if (!hCompatibleDC) goto label_error;
            HBITMAP hBMP = CreateDIBSection(hDC, bi, DIB_RGB_COLORS, (void **)&g_displayPtr, NULL, 0);
            if (!hBMP) goto label_error;
            if (!SelectObject(hCompatibleDC, g_hBMP)) goto label_error;*/

            m_pInternal->hWnd = hWnd;
            m_pInternal->hDC = hDC;

            return VKE_OK;
        }

        return VKE_FAIL;
    }

    void CWindow::Show(bool bShow)
    {
        ::ShowWindow((HWND)m_Info.wndHandle, bShow);
    }

    void CWindow::NeedQuit(bool need)
    {
        //Thread::LockGuard l(m_pInternal->mutex);
        m_needQuit = need;
    }

    bool CWindow::NeedQuit() const
    {
        bool need;
        {
            //Thread::LockGuard l(m_pInternal->mutex);
            need = m_needQuit;
        }
        return need;
    }

    void CWindow::Update()
    {
        if(!NeedQuit())
        {
            MSG msg = { 0 };
            if(::PeekMessage(&msg, m_pInternal->hWnd, 0, 0, PM_REMOVE))
            {
                if(msg.message == WM_QUIT)
                {
                    m_needQuit = true;
                }
                else
                {
                    ::TranslateMessage(&msg);
                    ::DispatchMessage(&msg);
                }
            }
        }
    }

    void CWindow::SetContext(RenderSystem::CContext* pCtx)
    {
        m_pInternal->pCtx = pCtx;
    }

    void CWindow::SetRenderSystem(CRenderSystem* pRS)
    {
        m_pInternal->pRenderSystem = pRS;
    }

    LRESULT CALLBACK WndProc(HWND window, UINT msg, WPARAM wparam, LPARAM lparam)
    {
        auto pEngine = VKEGetEngine();
        handle_t hWnd = reinterpret_cast<handle_t>(window);
        WindowPtr pWnd = pEngine->GetWindow(hWnd);

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
            PostQuitMessage(0);
            break;
        case WM_SIZE:
            break;
        default:
            return DefWindowProc(window, msg, wparam, lparam);
        }
        return 0;
    }
}

#endif // WINDOWS
