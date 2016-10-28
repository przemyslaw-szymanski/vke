#pragma once

#include "Core/Platform/Common.h"
#include "Core/Utils/TCSmartPtr.h"

namespace VKE
{
    struct SWindowInternal;
    class CVkEngine;

    namespace RenderSystem
    {
        class CContext;
    }

    class CRenderSystem;

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

            Result Create(const SWindowInfo& Info);
            void Destroy();

            void Update();

            const SWindowInfo& GetInfo() const
            {
                return m_Info; 
            }

            bool NeedQuit() const;
            void NeedQuit(bool need);

            void IsVisible(bool bShow);
            void IsVisibleAsync(bool bVisible);
            bool IsVisible() const { return m_isVisible; }

            bool IsCustomWindow() const { return m_isCustomWindow; }

            bool NeedUpdate() const;

            void SetRenderingContext(RenderSystem::CContext* pCtx);
            void SetRenderSystem(CRenderSystem* pRS);

            void OnPaint();
            void AddPaintCallback(PaintCallback&& Func);
            void AddResizeCallback(ResizeCallback&& Func);
            void AddDestroyCallback(DestroyCallback&& Func);
            void AddKeyboardCallback(KeyboardCallback&& Func);
            void AddMouseCallback(MouseCallback&& Func);
            void AddUpdateCallback(UpdateCallback&& Func);

            void Resize(uint32_t width, uint32_t height);

            RenderSystem::CContext* GetRenderingContext() const;

            std::thread::id GetThreadId();

        protected:

            void    _Update();

        protected:

            SWindowInfo         m_Info;
            SWindowInternal*    m_pInternal = nullptr;
            CVkEngine*			m_pEngine = nullptr;
            bool                m_needQuit = false;
            bool                m_isVisible = false;
            bool                m_isCustomWindow = false;
    };

    using WindowPtr = Utils::TCWeakPtr< CWindow >;
    using WindowOwnPtr = Utils::TCUniquePtr< CWindow >;

} // vke
