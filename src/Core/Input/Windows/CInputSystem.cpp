#include "Core/Input/CInputSystem.h"
#include "Core/Platform/CPlatform.h"
#include "Core/Utils/CLogger.h"
#if VKE_WINDOWS
#include <windef.h>
#include <basetsd.h>
#include <winnt.h>
#include <hidusage.h>
namespace VKE
{
    namespace Input
    {
        using QWORD = uint64_t;

        static Input::KEY vke_force_inline ConvertRawKeyToKey( const uint16_t virtualKey )
        {
            return static_cast<Input::KEY>(virtualKey);
        }

        struct SDefaultInputListener : public Input::EventListeners::IInput
        {

        };
        static SDefaultInputListener    g_DefaultInputListener;

        struct SRawInputData
        {
            using RawBuffer = Utils::TCDynamicArray< uint8_t, 1024 >;

            ::PRAWINPUT pRawInput = nullptr;
            uint32_t    rawInputSize = 0;
            RawBuffer   vBuffer;
        };
        static SRawInputData g_sRAwInputData;

        void CInputSystem::_Destroy()
        {
            m_vDevices.Destroy();

            SRawInputData* pRawInputData = reinterpret_cast< SRawInputData* >( m_pData );
            Memory::DestroyObject( &HeapAllocator, &pRawInputData );
        }

        Result CInputSystem::_QueryDevices()
        {
            Result ret = VKE_FAIL;
            uint32_t count = 0;
            auto res = ::GetRawInputDeviceList( nullptr, &count, sizeof( ::RAWINPUTDEVICELIST ) );
            if( res == 0 )
            {
                Utils::TCDynamicArray<::RAWINPUTDEVICELIST> vDevices( count );
                res = ::GetRawInputDeviceList( &vDevices[ 0 ], &count, sizeof( ::RAWINPUTDEVICELIST ) );
                if( res != ((UINT)-1) )
                {
                    count = res;
                    bool err = false;
                    ::RID_DEVICE_INFO Rdi;
                    Rdi.cbSize = sizeof( ::RID_DEVICE_INFO );

                    for( uint32_t i = 0; i < count; ++i )
                    {
                        const auto& Curr = vDevices[i];
                        SDevice Device;
                        Device.hDevice = Curr.hDevice;
                        uint32_t size = sizeof(Device.pName);

                        res = ::GetRawInputDeviceInfoA( Curr.hDevice, RIDI_DEVICENAME, Device.pName, &size );
                        if( res < 0 )
                        {
                            err = true;
                            break;
                        }

                        size = Rdi.cbSize;
                        res = ::GetRawInputDeviceInfoA( Curr.hDevice, RIDI_DEVICEINFO, &Rdi, &size );
                        if( res < 0 )
                        {
                            err = true;
                            break;
                        }

                        switch( Rdi.dwType )
                        {
                            case RIM_TYPEMOUSE:
                            {
                                Device.type = DeviceTypes::MOUSE;
                                Device.Mouse.buttonCount = static_cast< uint16_t >( Rdi.mouse.dwNumberOfButtons );
                                Device.Mouse.sampleRate = static_cast< uint16_t >( Rdi.mouse.dwSampleRate );
                                Device.Mouse.hasWheel = Rdi.mouse.fHasHorizontalWheel;
                                Device.Mouse.id = Rdi.mouse.dwId;
                                m_vDevices.PushBack( Device );
                            }
                            break;
                            case RIM_TYPEKEYBOARD:
                            {
                                Device.type = DeviceTypes::KEYBOARD;
                                Device.Keyboard.funcKeyCount = static_cast< uint16_t >( Rdi.keyboard.dwNumberOfFunctionKeys );
                                Device.Keyboard.keyCount = static_cast< uint16_t >( Rdi.keyboard.dwNumberOfKeysTotal );
                                Device.Keyboard.mode = Rdi.keyboard.dwKeyboardMode;
                                Device.Keyboard.subType = Rdi.keyboard.dwSubType;
                                Device.Keyboard.type = Rdi.keyboard.dwType;
                                m_vDevices.PushBack( Device );
                            }
                            break;
                            case RIM_TYPEHID:
                            {
                                Device.type = DeviceTypes::HUMAN_INTERFACE_DEVICE;
                                Device.HID.id = Rdi.hid.dwProductId;
                                Device.HID.usage = Rdi.hid.usUsage;
                                Device.HID.usagePage = Rdi.hid.usUsagePage;
                                if( Rdi.hid.usUsage > 0 && Rdi.hid.usUsagePage > 0 )
                                {
                                    m_vDevices.PushBack( Device );
                                }
                            }
                            break;
                        }
                    }
                    if( !err )
                    {
                        ret = VKE_OK;
                    }
                }
            }
            return ret;
        }

        Result CInputSystem::_Create( const SInputSystemDesc& Desc )
        {
            Result ret = VKE_FAIL;
            m_pListener = &g_DefaultInputListener;
            m_Timer.Start();

            SRawInputData* pRawInputData;
            Memory::CreateObject( &HeapAllocator, &pRawInputData );
            m_pData = pRawInputData;

            Memory::Zero( &m_InputState );

            //m_Keyboard.usUsagePage = 0x01;
            //m_Keyboard.usUsage = 0x02;
            //m_Keyboard.dwFlags =0 /*RIDEV_NOLEGACY*/;
            //m_Keyboard.hwndTarget = 0;

            //m_Mouse.usUsagePage = 0x01;
            //m_Mouse.usUsage = 0x06;
            //m_Mouse.dwFlags = 0/*RIDEV_NOLEGACY*/;
            //m_Mouse.hwndTarget = 0;

            //m_GamePad.usUsagePage = 0x01;
            //m_GamePad.usUsage = 0x05;
            //m_GamePad.dwFlags = 0;
            //m_GamePad.hwndTarget = 0;

            //m_Joystick.usUsagePage = 0x01;
            //m_Joystick.usUsage = 0x04;
            //m_Joystick.dwFlags = 0;
            //m_Joystick.hwndTarget = 0;

            const ::RAWINPUTDEVICE aDevices[ 2 ] =
            {
                m_Keyboard,
                m_Mouse
            };

            if( VKE_SUCCEEDED( _QueryDevices() ) )
            {
                Utils::TCDynamicArray< ::RAWINPUTDEVICE > vRawDevices( m_vDevices.GetCount() );
                for( uint32_t i = 0; i < m_vDevices.GetCount(); ++i )
                {
                    const auto& Curr = m_vDevices[i];
                    ::RAWINPUTDEVICE Device;
                    Device.usUsagePage = HID_USAGE_PAGE_GENERIC;
                    Device.dwFlags = 0;
                    Device.hwndTarget = 0;

                    switch( Curr.type )
                    {
                        case DeviceTypes::MOUSE:
                        {
                            Device.usUsage = HID_USAGE_GENERIC_MOUSE;
                        }
                        break;
                        case DeviceTypes::KEYBOARD:
                        {
                            Device.usUsage = HID_USAGE_GENERIC_KEYBOARD;
                        }
                        break;
                        case DeviceTypes::HUMAN_INTERFACE_DEVICE:
                        {
                            Device.usUsage = static_cast<uint16_t>(Curr.HID.usage);
                            Device.usUsagePage = static_cast<uint16_t>(Curr.HID.usagePage);
                        }
                    };
                    vRawDevices[i] = Device;
                }

                auto res = ::RegisterRawInputDevices( vRawDevices.GetData(), vRawDevices.GetCount(),
                                                      sizeof( ::RAWINPUTDEVICE ) );
                if( res != FALSE )
                {
                    ret = VKE_OK;
                }
                else
                {
                    const auto err = ::GetLastError();
                    char buff[ 2048 ];
                    Platform::Debug::ConvertErrorCodeToText( err, buff, sizeof( buff ) );
                    VKE_LOG_ERR( "Unable to register input devices. Error: " << buff );
                }
            }
            ret = VKE_OK;
            return ret;
        }

        bool CInputSystem::NeedUpdate()
        {
            const auto elapsedTime = m_Timer.GetElapsedTime();
            return !m_isPaused && elapsedTime >= m_Desc.updateFrequencyUS;
        }

        void CInputSystem::Update()
        {
            if( NeedUpdate() )
            {
                m_Timer.Start();

                uint32_t size = 0;
                static const size_t scHeaderSize = sizeof( ::RAWINPUTHEADER );
                auto res = ::GetRawInputBuffer( nullptr, &size, scHeaderSize );
                if( res == 0 )
                {
                    SRawInputData* pData = reinterpret_cast< SRawInputData* >( m_pData );
                    pData->rawInputSize = size * 16;
                    size = pData->rawInputSize;
                    pData->vBuffer.Resize( pData->rawInputSize );
                    ::RAWINPUT* pRawInput = reinterpret_cast< ::RAWINPUT* >( pData->vBuffer.GetData() );
                    auto count = ::GetRawInputBuffer( pRawInput, &size, scHeaderSize );
                    if( count != ( ( UINT )-1 ) && count > 0 )
                    {
                        ::PRAWINPUT pCurr = pRawInput;
                        for( uint32_t i = 0; i < count; ++i )
                        {
                            switch( pCurr->header.dwType )
                            {
                                case RIM_TYPEMOUSE:
                                {
                                    _ProcessMouse( &pCurr->data.mouse, &m_InputState.Mouse );
                                }
                                break;
                                case RIM_TYPEKEYBOARD:
                                {
                                    _ProcessKeyboard( &pCurr->data.keyboard, &m_InputState.Keyboard );
                                }
                                break;
                            }
                            pCurr = NEXTRAWINPUTBLOCK( pCurr );
                        }
                        ::DefRawInputProc( &pRawInput, count, scHeaderSize );
                    }
                }
            }
        }

        void CInputSystem::_ProcessWindowInput( void* pParam )
        {
            ::LPARAM lParam = (LPARAM)pParam;
            static const size_t scHeaderSize = sizeof( ::RAWINPUTHEADER );
            uint32_t size;

            auto res = ::GetRawInputData( ( ::HRAWINPUT )lParam, RID_INPUT, nullptr, &size, scHeaderSize );
            if( res == 0 )
            {
                SRawInputData* pData = reinterpret_cast< SRawInputData* >( m_pData );
                pData->vBuffer.Resize( size );
                res = ::GetRawInputData( ( ::HRAWINPUT )lParam, RID_INPUT, &pData->vBuffer[ 0 ], &size, scHeaderSize );
                if( res == size )
                {
                    ::RAWINPUT* pRawInput = ( ::RAWINPUT* )pData->vBuffer.GetData();
                    switch( pRawInput->header.dwType )
                    {
                        case RIM_TYPEMOUSE:
                        {
                            _ProcessMouse( &pRawInput->data.mouse, &m_InputState.Mouse );
                        }
                        break;
                        case RIM_TYPEKEYBOARD:
                        {
                            _ProcessKeyboard( &pRawInput->data.keyboard, &m_InputState.Keyboard );
                        }
                        break;
                    }
                    ::DefRawInputProc( &pRawInput, 1, scHeaderSize );
                }
            }
        }

        void CInputSystem::_ProcessMouse( void* pData, SMouseState* pOut )
        {
            ::RAWMOUSE* pMouse = reinterpret_cast< ::RAWMOUSE* >( pData );
            const bool isVirtualDestkop = (pMouse->usFlags & MOUSE_VIRTUAL_DESKTOP) == MOUSE_VIRTUAL_DESKTOP;

            pOut->LastMove = pOut->Move;
            pOut->LastPosition = pOut->Position;

            if( (pMouse->usFlags & MOUSE_MOVE_ABSOLUTE) == MOUSE_MOVE_ABSOLUTE )
            {
                const int width = GetSystemMetrics(isVirtualDestkop ? SM_CXVIRTUALSCREEN : SM_CXSCREEN);
                const int height = GetSystemMetrics(isVirtualDestkop ? SM_CYVIRTUALSCREEN : SM_CYSCREEN);

                pOut->Position.x = static_cast< int16_t >(( pMouse->lLastX / 65535.0f ) * width);
                pOut->Position.y = static_cast< int16_t >(( pMouse->lLastY / 65535.0f ) * height);
                //printf("ab %d, %d\n", pOut->Position.x, pOut->Position.y);

                pOut->Move = pOut->LastPosition - pOut->Position;
            }
            else if ((pMouse->usFlags & MOUSE_MOVE_RELATIVE) == MOUSE_MOVE_RELATIVE)
            {
                /*pOut->Position.x = static_cast<int16_t>(pMouse->lLastX);
                pOut->Position.y = static_cast<int16_t>(pMouse->lLastY);*/

                ::POINT Point;
                ::GetCursorPos( &Point );
                pOut->Position.x = static_cast<int16_t>(Point.x);
                pOut->Position.y = static_cast<int16_t>(Point.y);
                pOut->Move = { (int16_t)pMouse->lLastX, ( int16_t )-pMouse->lLastY };
                printf("rel %d, %d / %d, %d\n", pOut->Position.x, pOut->Position.y, pMouse->lLastX, pMouse->lLastY);
            }

            //::POINT Point;
            //::GetCursorPos( &Point );
            //pOut->Position.x = static_cast<uint16_t>(Point.x);
            //pOut->Position.y = static_cast<uint16_t>(Point.y);
            //printf("p %d, %d\n", pOut->Position.x, pOut->Position.y);
            printf("m %d, %d\n", pOut->Move.x, pOut->Move.y);

            if( pMouse->usButtonFlags )
            {
                pOut->buttonState = pMouse->usButtonFlags;
            }
            //if( pMouse->usFlags & RI_MOUSE_WHEEL )
            {
                pOut->wheelMove = static_cast< int16_t >( pMouse->usButtonData );
            }
            if( pOut->buttonState & MouseButtonStates::LEFT_BUTTON_DOWN )
            {
                m_pListener->OnMouseButtonDown( *pOut );
                //VKE_LOG( "state: " << pOut->buttonState );
            }
            else if( pOut->buttonState & MouseButtonStates::LEFT_BUTTON_UP )
            {
                m_pListener->OnMouseButtonUp( *pOut );
            }

            m_pListener->OnMouseMove( *pOut );
        }

        void CInputSystem::_ProcessKeyboard( void* pData, SKeyboardState* pOut )
        {
            ::RAWKEYBOARD* pKeyboard = reinterpret_cast<::RAWKEYBOARD*>(pData);
            //Memory::Zero<SKeyboardState>( pOut );
            //const bool keyDown = (pKeyboard->Flags & RI_KEY_MAKE) == 0;
            const bool keyUp = (pKeyboard->Flags == RI_KEY_BREAK);
            const bool keyDown = !keyUp;
            const Input::KEY key = ConvertRawKeyToKey( pKeyboard->VKey );
            pOut->aKeys[ key ] = keyDown;
            if( keyDown )
            {
                m_pListener->OnKeyDown( *pOut, key );
            }
            else
            {
                m_pListener->OnKeyUp( *pOut, key );
            }
        }

    } // Input
} // VKE
#endif // VKE_WINDOWS