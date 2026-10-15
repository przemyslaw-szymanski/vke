#include "Core/Platform/CWindow.h"

#if VKE_LINUX
#include "Core/Memory/Memory.h"
#include "Core/Utils/CLogger.h"

#include "Core/Platform/Linux/Backend/Backend.h"

namespace VKE
{
    // std::function is not trivially copyable, TCDynamicArray relocates with memcpy
    using ResizeCallbackVec   = vke_vector< CWindow::ResizeCallback >;
    using PaintCallbackVec    = vke_vector< CWindow::PaintCallback >;
    using DestroyCallbackVec  = vke_vector< CWindow::DestroyCallback >;
    using KeyboardCallbackVec = vke_vector< CWindow::KeyboardCallback >;
    using MouseCallbackVec    = vke_vector< CWindow::MouseCallback >;
    using UpdateCallbackVec   = vke_vector< CWindow::UpdateCallback >;
    using ShowCallbackVec     = vke_vector< CWindow::ShowCallback >;

    struct SWindowInternal
    {
        using WINDOW_SYSTEM = Platform::Backend::WINDOW_SYSTEM;

        const WINDOW_SYSTEM  windowSystem = Platform::Backend::GetWindowSystem();
        Platform::Thread::ID osThreadId   = 0;

        // Both kept (no union): small, trivially destructible, no manual lifetime management required
        Platform::Backend::SX11     X11;
        Platform::Backend::SWayland Wayland;

        struct
        {
            ResizeCallbackVec   vResizeCallbacks;
            PaintCallbackVec    vPaintCallbacks;
            DestroyCallbackVec  vDestroyCallbacks;
            KeyboardCallbackVec vKeyboardCallbacks;
            MouseCallbackVec    vMouseCallbacks;
            UpdateCallbackVec   vUpdateCallbacks;
            ShowCallbackVec     vShowCallbacks;
        } Callbacks;

        bool isLoaded = false;
    };

    // Static, non-virtual dispatch. Branch is perfectly predicted: windowSystem never changes.
#define VKE_WINDOW_CALL_BACKEND( _func, ... )                                                                          \
    ( m_pPrivate->windowSystem == Platform::Backend::WindowSystems::WAYLAND                                            \
          ? Platform::Backend::CWayland::_func( m_pPrivate->Wayland __VA_OPT__(, ) __VA_ARGS__ )                       \
          : Platform::Backend::CX11::_func( m_pPrivate->X11 __VA_OPT__(, ) __VA_ARGS__ ) )

#define VKE_WINDOW_BACKEND_EVENTS()                                                                                    \
    ( m_pPrivate->windowSystem == Platform::Backend::WindowSystems::WAYLAND ? m_pPrivate->Wayland.Events               \
                                                                             : m_pPrivate->X11.Events )

    struct SDefaultInputListener : public Input::EventListeners::IInput
    {
    };

    static SDefaultInputListener g_DefaultInputListener;

    CWindow::CWindow( CVkEngine* pEngine ) : m_pEngine( pEngine ), m_pInputListener{ &g_DefaultInputListener }
    {
    }

    CWindow::~CWindow()
    {
        Destroy();
    }

    Result CWindow::Create( const SWindowDesc& Info )
    {
        Result ret = VKE_FAIL;

        if( m_pPrivate != nullptr )
        {
            VKE_LOG_WARN( "Window already initialized" );
            ret = VKE_OK;
        }
        else if( VKE_FAILED( Memory::CreateObject( &HeapAllocator, &m_pPrivate ) ) )
        {
            VKE_LOG_ERR( "Out of memory" );
            ret = VKE_ENOMEMORY;
        }
        else
        {
            m_Desc                 = Info;
            m_pPrivate->osThreadId = Platform::ThisThread::GetID();

            ret = VKE_WINDOW_CALL_BACKEND( Load );
            if( VKE_SUCCEEDED( ret ) )
            {
                m_pPrivate->isLoaded = true;
                ret                  = VKE_WINDOW_CALL_BACKEND( Create, m_Desc );
            }
            else
            {
                VKE_LOG_ERR( "Unable to connect to the window system." );
            }

            if( VKE_SUCCEEDED( ret ) )
            {
                const auto& Events  = VKE_WINDOW_BACKEND_EVENTS();
                m_Desc.Size         = { Events.width, Events.height };
                m_Desc.hWnd         = m_pPrivate->X11.windowId;
                m_Desc.threadId     = static_cast< uint32_t >( m_pPrivate->osThreadId );
                m_ClientSize.width  = static_cast< image_dimm_t >( Events.width );
                m_ClientSize.height = static_cast< image_dimm_t >( Events.height );
                m_NewSize           = m_ClientSize;
                m_isDestroyed       = false;
                m_needDestroy       = false;
            }
            else
            {
                Destroy();
            }
        }

        return ret;
    }

    void CWindow::Destroy()
    {
        Threads::ScopedLock l( m_SyncObj );
        if( m_pPrivate != nullptr )
        {
            VKE_WINDOW_CALL_BACKEND( Destroy );
            if( m_pPrivate->isLoaded )
            {
                VKE_WINDOW_CALL_BACKEND( Unload );
            }
            Memory::DestroyObject( &HeapAllocator, &m_pPrivate );
            m_pPrivate = nullptr;
        }
        m_Desc.hWnd   = 0;
        m_isDestroyed = true;
    }

    bool CWindow::Update()
    {
        bool ret     = true;
        m_needUpdate = true;
        if( GetThreadId() == Platform::ThisThread::GetID() )
        {
            ret = _UpdateTask( nullptr ) != TaskResults::FAIL;
        }
        return ret;
    }

    void CWindow::Close()
    {
        if( !m_needDestroy && m_pPrivate != nullptr )
        {
            m_needDestroy = true;
            for( auto& Callback: m_pPrivate->Callbacks.vDestroyCallbacks )
            {
                Callback( this );
            }
        }
    }

    bool CWindow::NeedQuit()
    {
        return m_needDestroy;
    }

    bool CWindow::NeedDestroy()
    {
        return m_needDestroy;
    }

    void CWindow::IsVisible( bool isVisible )
    {
        m_isVisible = isVisible;
        if( m_pPrivate != nullptr )
        {
            VKE_WINDOW_CALL_BACKEND( IsVisible, m_isVisible );
            _OnShow();
        }
    }

    void CWindow::SetText( cstr_t pText )
    {
        if( !m_needDestroy && m_pPrivate != nullptr && pText != nullptr )
        {
            m_strText = pText;
            VKE_WINDOW_CALL_BACKEND( SetWindowTitle, m_strText.c_str() );
        }
    }

    bool CWindow::NeedUpdate()
    {
        return IsVisible() && !NeedQuit() && !IsCustomWindow() && !m_isDestroyed;
    }

    void CWindow::SetSwapChain( RenderSystem::CSwapChain* pSwapChain )
    {
        m_pSwapChain = pSwapChain;
    }

    void CWindow::SetMode( WINDOW_MODE mode, uint16_t width, uint16_t height )
    {
        _OnSetMode( mode, width, height );
    }

    void CWindow::OnPaint()
    {
        if( m_pPrivate != nullptr )
        {
            for( auto& Func: m_pPrivate->Callbacks.vPaintCallbacks )
            {
                Func( this );
            }
        }
    }

    void CWindow::AddPaintCallback( PaintCallback&& Func )
    {
        m_pPrivate->Callbacks.vPaintCallbacks.push_back( std::move( Func ) );
    }

    void CWindow::AddResizeCallback( ResizeCallback&& Func )
    {
        m_pPrivate->Callbacks.vResizeCallbacks.push_back( std::move( Func ) );
    }

    void CWindow::AddDestroyCallback( DestroyCallback&& Func )
    {
        m_pPrivate->Callbacks.vDestroyCallbacks.push_back( std::move( Func ) );
    }

    void CWindow::AddKeyboardCallback( KeyboardCallback&& Func )
    {
        m_pPrivate->Callbacks.vKeyboardCallbacks.push_back( std::move( Func ) );
    }

    void CWindow::AddMouseCallback( MouseCallback&& Func )
    {
        m_pPrivate->Callbacks.vMouseCallbacks.push_back( std::move( Func ) );
    }

    void CWindow::AddUpdateCallback( UpdateCallback&& Func )
    {
        m_pPrivate->Callbacks.vUpdateCallbacks.push_back( std::move( Func ) );
    }

    void CWindow::AddShowCallback( ShowCallback&& Func )
    {
        m_pPrivate->Callbacks.vShowCallbacks.push_back( std::move( Func ) );
    }

    Platform::Thread::ID CWindow::GetThreadId()
    {
        Platform::Thread::ID id = 0;
        if( m_pPrivate != nullptr )
        {
            id = m_pPrivate->osThreadId;
        }
        return id;
    }

    void CWindow::WaitForMessages()
    {
    }

    bool CWindow::HasFocus()
    {
        bool ret = false;
        if( m_pPrivate != nullptr )
        {
            ret = VKE_WINDOW_BACKEND_EVENTS().focused;
        }
        return ret;
    }

    bool CWindow::IsActive()
    {
        return HasFocus();
    }

    uint64_t CWindow::WndProc( void*, uint32_t, uint64_t, uint64_t )
    {
        return 0;
    }

    // X11: xcb_window_t. Use Platform::Backend::GetX11()/GetWayland() for full typed data.
    uint64_t CWindow::GetNativeHandle() const
    {
        uint64_t            ret = 0;
        Threads::ScopedLock l( m_SyncObj );
        if( m_pPrivate != nullptr && m_pPrivate->windowSystem == Platform::Backend::WindowSystems::X11 )
        {
            ret = static_cast< uint64_t >( m_pPrivate->X11.windowId );
        }
        return ret;
    }

    void CWindow::WaitForClose()
    {
        while( m_pPrivate != nullptr && !m_needDestroy )
        {
            Platform::ThisThread::Pause();
        }
    }

    uint32_t CWindow::_PeekMessage()
    {
        return 0;
    }

    void CWindow::_OnResize( uint16_t width, uint16_t height )
    {
        m_Desc.Size         = { width, height };
        m_ClientSize.width  = static_cast< image_dimm_t >( width );
        m_ClientSize.height = static_cast< image_dimm_t >( height );
        m_NewSize           = m_ClientSize;
        for( auto& Func: m_pPrivate->Callbacks.vResizeCallbacks )
        {
            Func( this, width, height );
        }
    }

    bool CWindow::_OnSetMode( WINDOW_MODE mode, uint16_t width, uint16_t height )
    {
        // TODO: runtime mode switch (_NET_WM_STATE client message). Initial mode is applied in Create.
        m_Desc.mode        = mode;
        m_Desc.Size.width  = width;
        m_Desc.Size.height = height;
        return false;
    }

    void CWindow::_SendMessage( uint32_t )
    {
    }

    void CWindow::_OnShow()
    {
        for( auto& Func: m_pPrivate->Callbacks.vShowCallbacks )
        {
            Func( this );
        }
    }

    void CWindow::_Update()
    {
        for( auto& Func: m_pPrivate->Callbacks.vUpdateCallbacks )
        {
            Func( this );
        }
    }

    TASK_RESULT CWindow::_UpdateTask( void* )
    {
        TASK_RESULT ret = TaskResults::OK;
        if( m_pPrivate != nullptr && !m_needDestroy && m_needUpdate )
        {
            // One shared connection: draining it here routes events to every window
            const bool isConnected = m_pPrivate->windowSystem == Platform::Backend::WindowSystems::WAYLAND
                                         ? Platform::Backend::CWayland::PumpEvents()
                                         : Platform::Backend::CX11::PumpEvents();
            auto&      Events      = VKE_WINDOW_BACKEND_EVENTS();
            if( !isConnected )
            {
                VKE_LOG_ERR( "Lost connection to the window system." );
                Events.closeRequested = true;
            }
            if( Events.resized )
            {
                Events.resized = false;
                _OnResize( Events.width, Events.height );
            }
            if( Events.closeRequested )
            {
                Close();
            }
            else
            {
                _Update();
            }
        }
        return ret;
    }

    namespace Platform::Backend
    {
        const SX11* GetX11( const CWindow* pWindow )
        {
            const SX11* pRet = nullptr;
            if( pWindow != nullptr && GetWindowSystem() == WindowSystems::X11 )
            {
                const auto windowId = static_cast< xcb_window_t >( pWindow->GetNativeHandle() );
                if( windowId != XCB_WINDOW_NONE )
                {
                    pRet = CX11::FindWindow( windowId );
                }
            }
            return pRet;
        }

        const SWayland* GetWayland( const CWindow* )
        {
            return nullptr;
        }
    } // namespace Platform::Backend

} // namespace VKE
#endif // VKE_LINUX
