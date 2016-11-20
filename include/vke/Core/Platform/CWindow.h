#pragma once

#include "Core/Platform/Common.h"
#include "Core/Utils/TCSmartPtr.h"
#include "Core/Threads/Common.h"

#if VKE_WINDOWS
#include <windows.h>
#endif // VKE_WINDOWS

namespace VKE
{
    struct SWindowInternal;
    class CVkEngine;

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

    class VKE_API CWindow
    {
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
            void NeedQuit(bool need);
            bool NeedDestroy();

            void IsVisible(bool bShow);
            bool IsVisible() const { return m_isVisible; }

            bool IsCustomWindow() const { return m_isCustomWindow; }

            bool NeedUpdate();

            void SetSwapChain(RenderSystem::CSwapChain*);

            void SetMode(WINDOW_MODE mode, uint32_t width, uint32_t height);

            void OnPaint();
            void AddPaintCallback(PaintCallback&& Func);
            void AddResizeCallback(ResizeCallback&& Func);
            void AddDestroyCallback(DestroyCallback&& Func);
            void AddKeyboardCallback(KeyboardCallback&& Func);
            void AddMouseCallback(MouseCallback&& Func);
            void AddUpdateCallback(UpdateCallback&& Func);

            void Resize(uint32_t width, uint32_t height);

            RenderSystem::CGraphicsContext* GetGraphicsContext() const;

            std::thread::id GetThreadId();

#if VKE_WINDOWS
            LRESULT WndProc(HWND, UINT, WPARAM, LPARAM);
#endif

        protected:

            uint32_t    _PeekMessage();
            void        _OnResize(uint32_t width, uint32_t height);
            bool        _OnSetMode(WINDOW_MODE mode, uint32_t width, uint32_t height);
            void        _SendMessage(uint32_t msg);

        protected:

            SWindowDesc                 m_Desc;
            SWindowInternal*            m_pPrivate = nullptr;
            CVkEngine*                  m_pEngine = nullptr;
            RenderSystem::CSwapChain*   m_pSwapChain = nullptr;
            Threads::SyncObject         m_SyncObj;
            Threads::SyncObject         m_MsgQueueSyncObj;
            bool                        m_needQuit = false;
            bool                        m_isVisible = false;
            bool                        m_isCustomWindow = false;
            bool                        m_isDestroyed = false;
            bool                        m_needDestroy = false;
    };

    using WindowPtr = Utils::TCWeakPtr< CWindow >;
    using WindowOwnPtr = Utils::TCUniquePtr< CWindow >;

} // vke
