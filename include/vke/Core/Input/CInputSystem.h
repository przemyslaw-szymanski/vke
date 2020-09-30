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
            handle_t    hWnd = 0;
            uint32_t    updateFrequencyUS = 1; // microseconds
        };

        struct SDevice
        {
            SDevice() {}
            ~SDevice() { }

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
            char                pName[256];
            DeviceTypes::TYPE   type;
            union
            {
                SMouseInfo      Mouse;
                SKeyboardInfo   Keyboard;
                SHID            HID;
            };
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

                const SInputState&  GetState() const { return m_InputState; }

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