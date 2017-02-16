#pragma once

#include "Core/VKECommon.h"

namespace VKE
{
    namespace Threads
    {
        class ITask
        {
            public:

                enum class Status
                {
                    OK,
                    FAIL,
                    REMOVE
                };

            public:

                ITask()
                {}
                
                virtual     ~ITask()
                {
                    m_needEnd = true;
                    //Wait();
                };

                Status        Start(uint32_t threadId)
                {
                    if( !m_needEnd )
                    {
                        IsFinished<false>(false);
                        Status res = _OnStart(threadId);
                        IsFinished<false>(true);
                        return res;
                    }
                    return Status::REMOVE;
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
                Status        _OnStart(uint32_t /*threadId*/)
                {
                    return Status::OK;
                }

                virtual
                void _OnGet(void* /*pOut*/)
                {}

            protected:

                std::mutex  m_Mutex;
                bool        m_bIsFinished = false;
                bool        m_needEnd = false;
                VKE_DEBUG_CODE(vke_string m_strDbgName;);
        };
    } // Threads
} // vke
