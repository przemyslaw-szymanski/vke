#pragma once

#include "Core/Platform/Common.h"
#include "Core/Utils/TCSmartPtr.h"
#include "Core/Threads/Common.h"
#include "Core/Threads/ITask.h"
#include "Core/Input/CInputSystem.h"

namespace VKE
{
    struct SWindowInternal;
    class CVkEngine;
    class CWindow;

    namespace RenderSystem
    {
        class CRenderSystem;
        class CGraphicsContext;
        class CSwapChain;
    }

    struct KeyModes
    {
        enum MODE
        {
            UNKNOWN,
            UP,
            DOWN,
            PRESS,
            _ENUM_COUNT
        };
    };
    using KEY_MODE = KeyModes::MODE;

    struct MouseModes
    {
        enum MODE
        {
            UNKNOWN,
            UP,
            DOWN,
            PRESS,
            _ENUM_COUNT
        };
    };
    using MOUSE_MODE = MouseModes::MODE;

    namespace Tasks
    {
        struct Window;
    } // Tasks

    namespace Input
    {
        namespace EventListeners
        {
            class IInput;
        } // EventListeners
    } // Input

    class VKE_API CWindow
    {
        friend struct Tasks::Window;

        public:

            using VoidFunc = std::function<void(CWindow*)>;
            using ResizeCallback = std::function<void(CWindow*, uint32_t, uint32_t)>;
            using PaintCallback = VoidFunc;
            using DestroyCallback = VoidFunc;
            using CloseCallback = VoidFunc;
            using MoveCallback = VoidFunc;
            using KeyboardCallback = std::function<void(CWindow*, int, KEY_MODE)>;
            using MouseCallback = std::function<void(CWindow*, int, int, MOUSE_MODE)>;
            using UpdateCallback = VoidFunc;
            using ShowCallback = VoidFunc;

        public:

            CWindow(CVkEngine* pEngine);
            ~CWindow();

            Result Create(const SWindowDesc& Info);
            void Destroy();

            bool Update();

            const SWindowDesc& GetDesc() const
            {
                return m_Desc;
            }

            void Close();
            bool NeedQuit();
            bool NeedDestroy();

            void IsVisible(bool bShow);
            bool IsVisible() const { return m_isVisible; }

            void SetText( cstr_t pText );

            bool IsCustomWindow() const { return m_isCustomWindow; }

            bool NeedUpdate();

            void SetSwapChain(RenderSystem::CSwapChain*);

            void SetMode(WINDOW_MODE mode, uint16_t width, uint16_t height);

            void OnPaint();
            void AddPaintCallback(PaintCallback&& Func);
            void AddResizeCallback(ResizeCallback&& Func);
            void AddDestroyCallback(DestroyCallback&& Func);
            void AddKeyboardCallback(KeyboardCallback&& Func);
            void AddMouseCallback(MouseCallback&& Func);
            void AddUpdateCallback(UpdateCallback&& Func);
            void AddShowCallback(ShowCallback&& Func);

            void SetInputListener( Input::EventListeners::IInput* pListener ) { m_pInputListener = pListener; }

            void Resize(uint16_t width, uint16_t height);

            RenderSystem::CSwapChain* GetSwapChain() const { return m_pSwapChain; }

            Platform::Thread::ID GetThreadId();

            void WaitForMessages();

            bool HasFocus();
            bool IsActive();

            uint64_t WndProc(void*, uint32_t, uint64_t, uint64_t);

            const Input::CInputSystem& GetInputSystem() const { return m_InputSystem; }
            Input::CInputSystem& GetInputSystem() { return m_InputSystem; }

            const ImageSize& GetSize() const { return m_Desc.Size; }
            const ImageSize& GetClientSize() const { return m_ClientSize; }

        protected:

            uint32_t    _PeekMessage();
            void        _OnResize(uint16_t width, uint16_t height);
            bool        _OnSetMode(WINDOW_MODE mode, uint16_t width, uint16_t height);
            void        _SendMessage(uint32_t msg);
            void        _OnShow();

            void        _Update();

            TASK_RESULT   _UpdateTask(void*);
            friend Threads::TaskFunction;
            friend class CVkEngine;

        protected:

            SWindowDesc                 m_Desc;
            SWindowInternal*            m_pPrivate = nullptr;
            CVkEngine*                  m_pEngine = nullptr;
            RenderSystem::CSwapChain*   m_pSwapChain = nullptr;
            Input::CInputSystem         m_InputSystem;
            ImageSize                   m_NewSize;
            ImageSize                   m_ClientSize;
            uint16_t                    m_checkSizeUpdateCount = 0;
            Threads::SyncObject         m_SyncObj;
            Threads::SyncObject         m_MsgQueueSyncObj;
            Input::EventListeners::IInput*   m_pInputListener = nullptr;
            vke_string                  m_strText;
            bool                        m_isVisible = false;
            bool                        m_isCustomWindow = false;
            bool                        m_isDestroyed = false;
            bool                        m_needDestroy = false;
            bool                        m_needUpdate = true;
    };

    using WindowPtr = Utils::TCWeakPtr< CWindow >;
    using WindowOwnPtr = Utils::TCUniquePtr< CWindow >;
    using WindowRefPtr = Utils::TCObjectSmartPtr< CWindow >;

    namespace Tasks
    {
        struct Window
        {
            struct SUpdate : public Threads::ITask
            {
                CWindow* pWnd;
                TaskState _OnStart(uint32_t)
                {
                    //return pWnd->_UpdateTask(nullptr);
                    return TaskStateBits::FAIL;
                }
            };
        };
    } // Tasks

} // vke
