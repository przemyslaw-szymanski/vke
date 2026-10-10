#include "Core/Platform/CWindow.h"

#if VKE_LINUX
#include <unistd.h>
#include <dlfcn.h>

#include "Core/Memory/Memory.h"
#include "Core/Utils/CLogger.h"

#include "Core/Platform/Linux/Backend/CWayland.h"
#include "Core/Platform/Linux/Backend/CX11.h"

namespace VKE
{
    inline bool IsWaylandSupported()
    {
        static std::optional< bool > isSupported;

        if( !isSupported.has_value() )
        {
            isSupported = Platform::Backend::CWayland::IsSupported();
        }

        return isSupported.value();
    }

#define VKE_WINDOW_BACKEND_CALL( _func, ... )                                                                          \
    ( IsWaylandSupported() ? Platform::Backend::CWayland::_func( __VA_ARGS__ )                                         \
                           : Platform::Backend::CX11::_func( __VA_ARGS__ ) )

    struct SWindowInternal
    {
    };

    struct SDefaultInputListener : public Input::EventListeners::IInput
    {
    };

    static SDefaultInputListener g_DefaultInputListener;

    CWindow::CWindow( CVkEngine* pEngine ) : m_pEngine( pEngine ), m_pInputListener{ &g_DefaultInputListener }
    {
        VKE_WINDOW_BACKEND_CALL( Load );
    }

    CWindow::~CWindow()
    {
    }

    Result CWindow::Create( const SWindowDesc& Info )
    {
        assert( m_pPrivate == nullptr );

        m_Desc = Info;
        if( VKE_FAILED( Memory::CreateObject( &HeapAllocator, &m_pPrivate ) ) )
        {
            VKE_LOG_ERR( "Unable to create internal struct data. Out of Memory." );
            return VKE_ENOMEMORY;
        }

        return VKE_WINDOW_BACKEND_CALL( Create, Info );
    }

    void CWindow::Destroy()
    {
        Memory::DestroyObject( &HeapAllocator, &m_pPrivate );
    }

    bool CWindow::Update()
    {
        return true;
    }

    void CWindow::Close()
    {
    }

    bool CWindow::NeedQuit()
    {
        return false;
    }

    bool CWindow::NeedDestroy()
    {
        return false;
    }

    void CWindow::IsVisible( bool isVisible )
    {
    }

    void CWindow::SetText( cstr_t pText )
    {
    }

    bool CWindow::NeedUpdate()
    {
        return false;
    }

    void CWindow::SetSwapChain( RenderSystem::CSwapChain* pSwapChain )
    {
    }

    void CWindow::SetMode( WINDOW_MODE mode, uint16_t width, uint16_t height )
    {
    }

    void CWindow::OnPaint()
    {
    }

    void CWindow::AddPaintCallback( PaintCallback&& Func )
    {
    }

    void CWindow::AddResizeCallback( ResizeCallback&& Func )
    {
    }

    void CWindow::AddDestroyCallback( DestroyCallback&& Func )
    {
    }

    void CWindow::AddKeyboardCallback( KeyboardCallback&& Func )
    {
    }

    void CWindow::AddMouseCallback( MouseCallback&& Func )
    {
    }

    void CWindow::AddUpdateCallback( UpdateCallback&& Func )
    {
    }

    void CWindow::AddShowCallback( ShowCallback&& Func )
    {
    }

    Platform::Thread::ID CWindow::GetThreadId()
    {
        Platform::Thread::ID id = 0;
        return id;
    }

    void CWindow::WaitForMessages()
    {
    }

    bool CWindow::HasFocus()
    {
        return true;
    }

    bool CWindow::IsActive()
    {
        return true;
    }

    uint64_t CWindow::WndProc( void* ptr, uint32_t p1, uint64_t p2, uint64_t p3 )
    {
        return 0;
    }

    uint64_t CWindow::GetNativeHandle()
    {
        return 0;
    }

    void CWindow::WaitForClose()
    {
    }

    uint32_t CWindow::_PeekMessage()
    {
        return 0;
    }

    void CWindow::_OnResize( uint16_t width, uint16_t height )
    {
    }

    bool CWindow::_OnSetMode( WINDOW_MODE mode, uint16_t width, uint16_t height )
    {
        return true;
    }

    void CWindow::_SendMessage( uint32_t msg )
    {
    }

    void CWindow::_OnShow()
    {
    }

    void CWindow::_Update()
    {
    }

    TASK_RESULT CWindow::_UpdateTask( void* )
    {
        return TASK_RESULT::OK;
    }

} // namespace VKE
#endif // VKE_LINUX