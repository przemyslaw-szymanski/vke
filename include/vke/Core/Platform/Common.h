#ifndef __PLATFORM_COMMON_H__
#define __PLATFORM_COMMON_H__

#include "Core/VKECommon.h"

namespace VKE
{
    
    struct WindowModes
    {
        enum MODE
        {
            WINDOW,
            FULLSCREEN_WINDOW,
            FULLSCREEN,
            _MAX_COUNT
        };
    };
    using WINDOW_MODE = WindowModes::MODE;

    struct SWindowDesc
    {
        ExtentU16   Size = { 800, 600 };
        ExtentU16   Position = { UNDEFINED_U16, UNDEFINED_U16 };
        handle_t    hWnd;
        handle_t    hProcess;
        cstr_t      pTitle;
        void*       pUserData;
        uint32_t    threadId;
        WINDOW_MODE mode;
        bool        vSync;
    };

    template<typename T> class CAtomicWrapper
    {
      public:
        using AtomicType = std::atomic<T>;

      public:
        CAtomicWrapper()
            : m_Atomic()
        {
        }
        CAtomicWrapper( const CAtomicWrapper& Other )
            : CAtomicWrapper( Other.m_Atomic )
        {
        }
        CAtomicWrapper( const AtomicType& Other )
        {
            m_Atomic.store( Other.load() );
        }
        CAtomicWrapper& operator=( const AtomicType& Other )
        {
            m_Atomic.store( Other.load() );
            return *this;
        }
        CAtomicWrapper& operator=( const CAtomicWrapper& Other )
        {
            return this->operator=( Other.m_Atomic );
        }
        CAtomicWrapper& operator++()
        {
            m_Atomic++;
            return *this;
        }
        T Load()
        {
            return m_Atomic.load();
        }
        const T Load() const
        {
            return m_Atomic.load();
        }
        void Store( T v )
        {
            m_Atomic.store( v );
        }

      protected:
        AtomicType m_Atomic;
    };
    
} // vke

#endif // __PLATFORM_COMMON_H__