#pragma once

#include "Core/Input/Common.h"
#include "EventListeners.h"
#include "Core/Utils/TCDynamicArray.h"
#include "Core/Utils/CTimer.h"

namespace VKE
{
    class CVKEngine;

    namespace Input
    {
        struct SInputSystemDesc
        {
            uint32_t    updateFrequencyUS = 1; // microseconds
        };

        struct SDevice
        {
            SDevice() {}

            struct SMouseInfo
            {
                uint32_t    id = 0;
                uint16_t    buttonCount = 0;
                uint16_t    sampleRate = 0;
                bool        hasWheel = false;
            };

            struct SKeyboardInfo
            {
                uint32_t    mode;
                uint32_t    type;
                uint32_t    subType;
                uint16_t    funcKeyCount;
                uint16_t    keyCount;
            };

            struct SHID
            {
                uint32_t    usage = 0;
                uint32_t    usagePage = 0;
                uint32_t    id = 0;
            };

            DeviceHandle        hDevice;
            vke_string          strName;
            DeviceTypes::TYPE   type;
            union
            {
                SMouseInfo      Mouse;
                SKeyboardInfo   Keyboard;
                SHID            HID;
            };
        };

        struct MouseButtonStates
        {
            enum STATE
            {
                
                BUTTON_1_DOWN       = VKE_BIT( 0 ),
                BUTTON_1_UP         = VKE_BIT( 1 ),
                BUTTON_2_DOWN       = VKE_BIT( 2 ),
                BUTTON_2_UP         = VKE_BIT( 3 ),
                BUTTON_3_DOWN       = VKE_BIT( 4 ),
                BUTTON_3_UP         = VKE_BIT( 5 ),
                BUTTON_4_DOWN       = VKE_BIT( 6 ),
                BUTTON_4_UP         = VKE_BIT( 7 ),
                BUTTON_5_DOWN       = VKE_BIT( 8 ),
                BUTTON_5_UP         = VKE_BIT( 9 ),
                WHEEL_MOVE          = VKE_BIT( 10 ),
                HWHEEL_MOVE         = VKE_BIT( 11 ),
                LEFT_BUTTON_DOWN    = BUTTON_1_DOWN,
                LEFT_BUTTON_UP      = BUTTON_1_UP,
                RIGHT_BUTTON_DOWN   = BUTTON_2_DOWN,
                RIGHT_BUTTON_UP     = BUTTON_2_UP,
                MIDDLE_BUTTON_DOWN  = BUTTON_3_DOWN,
                MIDDLE_BUTTON_UP    = BUTTON_3_UP,
                _MAX_COUNT          = 12
            };
        };
        using MOUSE_BUTTON_STATE = uint16_t;

        struct SMouseState
        {
            ExtentU16           Position = { 0,0 };
            ExtentI32           Move = { 0,0 };
            MOUSE_BUTTON_STATE  buttonState = 0;
            int16_t             wheelMove;
        };

        struct SKeyboardState
        {

        };

        struct SInputState
        {
            SMouseState      Mouse;
            SKeyboardState   Keyboard;
        };

        class VKE_API CInputSystem
        {
            friend class CVKEngine;
            friend class CWindow;

            using DeviceArray = Utils::TCDynamicArray< SDevice >;

            public:

                void    Update();
                void    SetListener( EventListeners::IInput* pListener ) { m_pListener = pListener; }

                Result  _Create( const SInputSystemDesc& Desc );
                void    _Destroy();

                Result  GetState( SInputState* pOut );

                bool    NeedUpdate();

                bool    IsPaused() const { return m_isPaused; }
                void    IsPaused(const bool is) { m_isPaused = is; }

            protected:

                void    _ProcessMouse( void* pMouseState, SMouseState* pOut );
                void    _ProcessKeyboard( void* pKeyboardState, SKeyboardState* pOut );
                void    _ProcessWindowInput(void*);

                Result  _QueryDevices();

            protected:

                SInputSystemDesc    m_Desc;
                DeviceArray         m_vDevices;

                Keyboard            m_Keyboard;
                Mouse               m_Mouse;
                GamePad             m_GamePad;
                Joystick            m_Joystick;

                Utils::CTimer   m_Timer;
                bool            m_isPaused = false;
                SInputState     m_InputState;
                void*           m_pData = nullptr;

                EventListeners::IInput*  m_pListener = nullptr;
        };
    } // Input
} // VKE