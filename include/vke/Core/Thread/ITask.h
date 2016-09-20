#pragma once

#include "Core/VKECommon.h"

namespace VKE
{
    namespace Thread
    {
        class ITask
        {
            public:

            ITask()
            {}
            virtual     ~ITask()
            {};

            void        Start(uint32_t threadId)
            {
                IsFinished( false );
                _OnStart( threadId );
                IsFinished( true );
            }

            template<bool _THREAD_SAFE_ = true>
            bool        IsFinished()
            {
                if( _THREAD_SAFE_ )
                {
                    Thread::LockGuard l( m_Mutex );
                    return m_bIsFinished;
                }
                return m_bIsFinished;
            }

            template<bool _THREAD_SAFE_ = true>
            void        IsFinished(bool is)
            {
                if( _THREAD_SAFE_ )
                {
                    Thread::LockGuard l( m_Mutex );
                    m_bIsFinished = is;
                    return;
                }
                m_bIsFinished = is;
            }

            void    Wait()
            {
                while( !IsFinished() )
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
                void        _OnStart(uint32_t)
            {}

            void _OnGet(void* pOut) {}

            protected:

            std::mutex  m_Mutex;
            bool        m_bIsFinished = false;
        };
    } // Thread
} // vke
