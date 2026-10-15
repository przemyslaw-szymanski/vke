#if VKE_LINUX

#include "Core/Input/CInputSystem.h"
#include "Core/Memory/Memory.h"
#include "Core/Platform/CPlatform.h"
#include "Core/Platform/Linux/CX11.h"
#include "Core/Utils/CLogger.h"

namespace VKE::Input
{
    namespace Internal
    {
        // X core protocol button numbers
        static constexpr uint32_t X_BUTTON_LEFT        = 1;
        static constexpr uint32_t X_BUTTON_MIDDLE      = 2;
        static constexpr uint32_t X_BUTTON_RIGHT       = 3;
        static constexpr uint32_t X_BUTTON_WHEEL_UP    = 4;
        static constexpr uint32_t X_BUTTON_WHEEL_DOWN  = 5;
        static constexpr uint32_t X_BUTTON_WHEEL_LEFT  = 6;
        static constexpr uint32_t X_BUTTON_WHEEL_RIGHT = 7;
        static constexpr uint32_t X_BUTTON_BACK        = 8;
        static constexpr uint32_t X_BUTTON_FORWARD     = 9;

        // Same unit as Windows WHEEL_DELTA
        static constexpr int16_t WHEEL_DELTA = 120;

        static MOUSE_BUTTON_STATE ConvertButtonToState( const uint32_t button, const bool isDown )
        {
            MOUSE_BUTTON_STATE ret = 0;
            switch( button )
            {
                case X_BUTTON_LEFT:
                    ret = isDown ? MouseButtonStates::LEFT_BUTTON_DOWN : MouseButtonStates::LEFT_BUTTON_UP;
                    break;
                case X_BUTTON_RIGHT:
                    ret = isDown ? MouseButtonStates::RIGHT_BUTTON_DOWN : MouseButtonStates::RIGHT_BUTTON_UP;
                    break;
                case X_BUTTON_MIDDLE:
                    ret = isDown ? MouseButtonStates::MIDDLE_BUTTON_DOWN : MouseButtonStates::MIDDLE_BUTTON_UP;
                    break;
                case X_BUTTON_BACK:
                    ret = isDown ? MouseButtonStates::BUTTON_4_DOWN : MouseButtonStates::BUTTON_4_UP;
                    break;
                case X_BUTTON_FORWARD:
                    ret = isDown ? MouseButtonStates::BUTTON_5_DOWN : MouseButtonStates::BUTTON_5_UP;
                    break;
                case X_BUTTON_WHEEL_UP:
                case X_BUTTON_WHEEL_DOWN:
                    ret = MouseButtonStates::WHEEL_MOVE;
                    break;
                case X_BUTTON_WHEEL_LEFT:
                case X_BUTTON_WHEEL_RIGHT:
                    ret = MouseButtonStates::HWHEEL_MOVE;
                    break;
                default:
                    break;
            }
            return ret;
        }

        struct SDefaultInputListener : public EventListeners::IInput
        {
        };

        static SDefaultInputListener DefaultInputListener;
    } // namespace Internal

    using SInputEvent     = Platform::Linux::SInputEvent;
    using InputEventArray = Platform::Linux::InputEventArray;

    CInputSystem::CInputSystem()
    {
    }

    CInputSystem::~CInputSystem()
    {
        _Destroy();
    }

    Result CInputSystem::_Create( const SInputSystemDesc& Desc )
    {
        Result ret  = VKE_FAIL;
        m_Desc      = Desc;
        m_pListener = &Internal::DefaultInputListener;
        Memory::Zero( &m_InputState );
        m_Timer.Start();

        InputEventArray* pEvents = nullptr;
        if( VKE_SUCCEEDED( Memory::CreateObject( &HeapAllocator, &pEvents ) ) )
        {
            m_pData = pEvents;
            // Missing XInput2 is not fatal: the engine runs without input
            _QueryDevices();
            ret = VKE_OK;
        }
        else
        {
            VKE_LOG_ERR( "Unable to allocate input event buffer." );
            ret = VKE_ENOMEMORY;
        }
        return ret;
    }

    void CInputSystem::_Destroy()
    {
        if( m_pData != nullptr )
        {
            m_vDevices.Destroy();
            InputEventArray* pEvents = reinterpret_cast< InputEventArray* >( m_pData );
            Memory::DestroyObject( &HeapAllocator, &pEvents );
            m_pData = nullptr;
        }
    }

    bool CInputSystem::NeedUpdate()
    {
        const auto elapsedTime = m_Timer.GetElapsedTime();
        return !m_isPaused && elapsedTime >= m_Desc.updateFrequencyUS;
    }

    void CInputSystem::Update()
    {
        if( m_pData != nullptr && NeedUpdate() )
        {
            m_Timer.Start();
            InputEventArray& vEvents = *reinterpret_cast< InputEventArray* >( m_pData );
            // Events are produced by the X11 pump on the window thread; only the queue is touched here
            Platform::Linux::CX11::ConsumeInputEvents( &vEvents );
            for( uint32_t i = 0; i < vEvents.GetCount(); ++i )
            {
                SInputEvent& Event = vEvents[ i ];
                if( Event.type == SInputEvent::RESET )
                {
                    _ResetState();
                }
                else if( Event.type == SInputEvent::KEY_DOWN || Event.type == SInputEvent::KEY_UP )
                {
                    _ProcessKeyboard( &Event, &m_InputState.Keyboard );
                }
                else
                {
                    _ProcessMouse( &Event, &m_InputState.Mouse );
                }
            }
        }
    }

    void CInputSystem::_ResetState()
    {
        SKeyboardState& Keyboard = m_InputState.Keyboard;
        if( !Keyboard.aKeys.IsEmpty() )
        {
            const auto Pressed = Keyboard.aKeys; // snapshot: listeners see consistent state while iterating
            Keyboard.aKeys.Reset();
            Pressed.ForEachSetBit( [ & ]( const size_t key ) {
                m_pListener->OnKeyUp( Keyboard, static_cast< KEY >( key ) );
            } );
        }

        SMouseState&       Mouse = m_InputState.Mouse;
        MOUSE_BUTTON_STATE held  = static_cast< MOUSE_BUTTON_STATE >( Mouse.buttonState & MOUSE_BUTTONS_DOWN_MASK );
        while( held != 0 )
        {
            // Lowest held button only: one Up event per button, same as a real release
            const MOUSE_BUTTON_STATE downBit = static_cast< MOUSE_BUTTON_STATE >( held & ( ~held + 1 ) );
            Mouse.buttonState                = ApplyMouseButtonTransitions( Mouse.buttonState, 0, downBit );
            m_pListener->OnMouseButtonUp( Mouse );
            held &= static_cast< MOUSE_BUTTON_STATE >( held - 1 );
        }
        Mouse.Move      = { 0, 0 };
        Mouse.wheelMove = 0;
    }

    void CInputSystem::_ProcessMouse( void* pData, SMouseState* pOut )
    {
        const SInputEvent* pEvent = reinterpret_cast< const SInputEvent* >( pData );

        pOut->LastMove     = pOut->Move;
        pOut->LastPosition = pOut->Position;
        pOut->Move         = { 0, 0 };
        pOut->wheelMove    = 0;
        // Wheel is an event, not a held state
        pOut->buttonState  = static_cast< MOUSE_BUTTON_STATE >( pOut->buttonState & ~MOUSE_WHEELS_MASK );

        switch( pEvent->type )
        {
            case SInputEvent::POSITION: {
                pOut->Position = { pEvent->x, pEvent->y };
                m_pListener->OnMouseMove( *pOut );
            }
            break;
            case SInputEvent::RAW_MOTION: {
                // Same sign convention as Windows raw input
                pOut->Move = { static_cast< int16_t >( pEvent->dx ), static_cast< int16_t >( -pEvent->dy ) };
                m_pListener->OnMouseMove( *pOut );
            }
            break;
            case SInputEvent::BUTTON_DOWN:
            case SInputEvent::BUTTON_UP: {
                const bool isDown = pEvent->type == SInputEvent::BUTTON_DOWN;
                // Wheel is reported as press+release pairs: count only the press
                if( pEvent->detail >= Internal::X_BUTTON_WHEEL_UP && pEvent->detail <= Internal::X_BUTTON_WHEEL_RIGHT )
                {
                    if( isDown )
                    {
                        const bool isPositive = pEvent->detail == Internal::X_BUTTON_WHEEL_UP ||
                                                pEvent->detail == Internal::X_BUTTON_WHEEL_RIGHT;
                        pOut->buttonState     = static_cast< MOUSE_BUTTON_STATE >(
                            pOut->buttonState | Internal::ConvertButtonToState( pEvent->detail, isDown ) );
                        pOut->wheelMove = isPositive;
                        m_pListener->OnMouseMove( *pOut );
                    }
                }
                else
                {
                    const MOUSE_BUTTON_STATE downBit = Internal::ConvertButtonToState( pEvent->detail, true );
                    if( downBit != 0 )
                    {
                        if( isDown )
                        {
                            pOut->buttonState = ApplyMouseButtonTransitions( pOut->buttonState, downBit, 0 );
                            m_pListener->OnMouseButtonDown( *pOut );
                        }
                        else
                        {
                            pOut->buttonState = ApplyMouseButtonTransitions( pOut->buttonState, 0, downBit );
                            m_pListener->OnMouseButtonUp( *pOut );
                        }
                    }
                }
            }
            break;
            default:
                break;
        }
    }

    void CInputSystem::_ProcessKeyboard( void* pData, SKeyboardState* pOut )
    {
        const SInputEvent* pEvent  = reinterpret_cast< const SInputEvent* >( pData );
        const bool         keyDown = pEvent->type == SInputEvent::KEY_DOWN;
        const Input::KEY   key     = Platform::KeyMap::GetMappedKey( static_cast< uint16_t >( pEvent->detail ) );
        pOut->aKeys[ key ]         = keyDown;
        if( keyDown )
        {
            m_pListener->OnKeyDown( *pOut, key );
        }
        else
        {
            m_pListener->OnKeyUp( *pOut, key );
        }
    }

    // Linux input arrives through the shared X11 event pump, not through window messages
    void CInputSystem::_ProcessWindowInput( void* )
    {
    }

    Result CInputSystem::_QueryDevices()
    {
        Result                ret = VKE_FAIL;
        Platform::Linux::SX11 X11Ref;
        // Keep a connection reference only for the duration of the query
        if( VKE_SUCCEEDED( Platform::Linux::CX11::Load( X11Ref ) ) )
        {
            if( X11Ref.pDisplay->hasXI2 )
            {
                xcb_connection_t*                  pConn  = X11Ref.pDisplay->pConnection;
                xcb_input_xi_query_device_reply_t* pReply = xcb_input_xi_query_device_reply(
                    pConn, xcb_input_xi_query_device( pConn, XCB_INPUT_DEVICE_ALL ), nullptr );
                if( pReply != nullptr )
                {
                    xcb_input_xi_device_info_iterator_t Itr = xcb_input_xi_query_device_infos_iterator( pReply );
                    for( ; Itr.rem > 0; xcb_input_xi_device_info_next( &Itr ) )
                    {
                        const xcb_input_xi_device_info_t* pInfo   = Itr.data;
                        const bool                        isMouse = pInfo->type == XCB_INPUT_DEVICE_TYPE_SLAVE_POINTER;
                        const bool             isKeyboard         = pInfo->type == XCB_INPUT_DEVICE_TYPE_SLAVE_KEYBOARD;
                        const char*            pName              = xcb_input_xi_device_info_name( Itr.data );
                        const std::string_view Name( pName, pInfo->name_len );
                        // XTEST devices are virtual devices created by the X server for synthetic input
                        if( ( isMouse || isKeyboard ) && Name.find( "XTEST" ) == std::string_view::npos )
                        {
                            SDevice Device;
                            Device.hDevice =
                                reinterpret_cast< DeviceHandle >( static_cast< uintptr_t >( pInfo->deviceid ) );
                            const size_t len = std::min( Name.size(), sizeof( Device.pName ) - 1 );
                            memcpy( Device.pName, Name.data(), len );
                            Device.pName[ len ] = '\0';
                            if( isMouse )
                            {
                                Device.type     = DeviceTypes::MOUSE;
                                Device.Mouse    = {};
                                Device.Mouse.id = pInfo->deviceid;
                                xcb_input_device_class_iterator_t ClassItr =
                                    xcb_input_xi_device_info_classes_iterator( pInfo );
                                for( ; ClassItr.rem > 0; xcb_input_device_class_next( &ClassItr ) )
                                {
                                    if( ClassItr.data->type == XCB_INPUT_DEVICE_CLASS_TYPE_BUTTON )
                                    {
                                        const auto* pButtons     = (const xcb_input_button_class_t*)ClassItr.data;
                                        Device.Mouse.buttonCount = pButtons->num_buttons;
                                        Device.Mouse.hasWheel = pButtons->num_buttons >= Internal::X_BUTTON_WHEEL_DOWN;
                                    }
                                }
                            }
                            else
                            {
                                Device.type     = DeviceTypes::KEYBOARD;
                                Device.Keyboard = {};
                            }
                            VKE_LOG( "Input device: " << Device.pName << " id: " << pInfo->deviceid
                                                      << ( isMouse ? " (mouse)" : " (keyboard)" ) );
                            m_vDevices.PushBack( Device );
                        }
                    }
                    free( pReply );
                    ret = VKE_OK;
                }
                else
                {
                    VKE_LOG_ERR( "xcb_input_xi_query_device() failed." );
                }
            }
            else
            {
                VKE_LOG_WARN( "XInput2 not available. No input devices registered." );
            }
            Platform::Linux::CX11::Unload( X11Ref );
        }
        return ret;
    }

    // TODO: Gamepad/joystick support via an optional evdev path (/dev/input), enabled only when accessible.
    // Not available under WSL.
}; // namespace VKE::Input

#endif
