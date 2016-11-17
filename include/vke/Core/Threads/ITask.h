#pragma once

#include "Core/VKECommon.h"

namespace VKE
{
    namespace Threads
    {
        class ITask
        {
            public:

            ITask()
            {}
            virtual     ~ITask()
            {};

            bool        Start(uint32_t threadId)
            {
                IsFinished<false>( false );
                bool res = _OnStart( threadId );
                IsFinished<false>( true );
                return res;
            }

            template<bool _THREAD_SAFE_ = true>
            bool        IsFinished()
            {
                if( _THREAD_SAFE_ )
                {
                    Threads::UniqueLock l( m_Mutex );
                    return m_bIsFinished;
                }
                return m_bIsFinished;
            }

            template<bool _THREAD_SAFE_ = true>
            void        IsFinished(bool is)
            {
                if( _THREAD_SAFE_ )
                {
                    Threads::UniqueLock l( m_Mutex );
                    m_bIsFinished = is;
                    return;
                }
                m_bIsFinished = is;
            }

            void    Wait()
            {
                while( !IsFinished<false>() )
                {
                    std::this_thread::yield();
                }
            }

            template<typename _T_, bool _WAIT_ = true>
            void Get(_T_* pOut)
            {
                if( _WAIT_ )
                {
                    Wait();
                }
                _OnGet( pOut );
            }

            protected:

            virtual
            bool        _OnStart(uint32_t) { return true; }

            void _OnGet(void*) {}

            protected:

            std::mutex  m_Mutex;
            bool        m_bIsFinished = false;
        };
    } // Threads
} // vke
