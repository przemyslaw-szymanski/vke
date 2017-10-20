#pragma once

#include "Core/VKECommon.h"
#include "Common.h"

namespace VKE
{
    namespace Threads
    {
        enum _THREAD_SAFE
        {
            NO_THREAD_SAFE = 0,
            THREAD_SAFE = 1
        };

        class ITask
        {
            friend class CThreadPool;
            friend class CThreadWorker;
            public:

                struct StateBits
                {
                    enum : uint8_t
                    {
                        OK          = 0,
                        FAIL        = VKE_BIT(1),
                        NEXT_TASK   = VKE_BIT(2),
                        NOT_ACTIVE  = VKE_BIT(3),
                        FINISHED    = VKE_BIT(4),
                        REMOVE      = VKE_BIT(5)
                    };
                };
                using State = uint8_t;

            public:

                ITask()
                {}
                
                virtual     ~ITask()
                {
                    //m_needEnd = true;
                    //m_isActive = false;
                    //Wait();
                };

                uint32_t Start(uint32_t threadId)
                {
                    ScopedLock l( m_SyncObj );
                    m_isFinished = false;
                    SetState<NO_THREAD_SAFE>( StateBits::FINISHED, false );
                    m_state |= _OnStart(threadId);
                    SetState<NO_THREAD_SAFE>( StateBits::FINISHED, true );
                    m_isFinished = true;
                    return m_state;
                }

                template<_THREAD_SAFE IsThreadSafe>
                void SetState(State state, bool set)
                {
                    if( IsThreadSafe )
                    {
                        ScopedLock l( m_SyncObj );
                        if( set )
                        {
                            m_state |= state;
                        }
                        else
                        {
                            m_state &= ~state;
                        }
                        if( m_pState )
                        {
                            *m_pState = m_state;
                        }
                    }
                    else
                    {
                        m_state |= state;
                        if( m_pState )
                        {
                            *m_pState = m_state;
                        }
                    }
                }

                template<_THREAD_SAFE IsThreadSafe>
                bool IsStateSet(State state)
                {
                    bool is;
                    if( IsThreadSafe )
                    {
                        ScopedLock l( m_SyncObj );
                        is = m_state & state;
                    }
                    else
                    {
                        is = m_state & state;
                    }
                    return is;
                }

                template<_THREAD_SAFE IsThreadSafe = THREAD_SAFE>
                bool        IsFinished()
                {
                    IsStateSet< IsThreadSafe >( StateBits::FINISHED );
                    return m_isFinished;
                }

                template<_THREAD_SAFE IsThreadSafe = THREAD_SAFE>
                void        IsFinished(bool is)
                {
                    SetState< IsThreadSafe >( StateBits::FINISHED, is );
                    m_isFinished = is;
                }

                template<_THREAD_SAFE ThreadSafe = THREAD_SAFE>
                State GetResult()
                {
                    State res;
                    if( ThreadSafe )
                    {
                        ScopedLock l( m_SyncObj );
                        res = m_state;
                    }
                    else
                    {
                        res = m_state;
                    }
                    return res;
                }

                void    Wait()
                {
                    while( !IsFinished<NO_THREAD_SAFE>() )
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
                    _OnGet( reinterpret_cast<void**>(pOut) );
                }

                void SetNextTask(ITask* pTask, bool isActive = false)
                {
                    IsActive<NO_THREAD_SAFE>(isActive);
                    m_pNextTask = pTask;
                }

                template<_THREAD_SAFE IsThreadSafe = THREAD_SAFE>
                bool IsActive()
                {
                    return IsStateSet< IsThreadSafe >( StateBits::NOT_ACTIVE ) == false;
                }

                template<_THREAD_SAFE IsThreadSafe = THREAD_SAFE>
                void IsActive(bool is)
                {
                    SetState< IsThreadSafe >( StateBits::NOT_ACTIVE, !is );
                }


                void SetDbgType(uint32_t type)
                {
#ifdef _DEBUG
                    m_dbgType = type;
#endif
                }

                uint32_t GetDbgType() const
                {
#ifdef _DEBUG
                    return m_dbgType;
#endif
                    return 0;
                }

                template<bool WaitForFinish = true>
                void Remove()
                {
                    //m_needEnd = true;
                    if( WaitForFinish )
                    {
                        Wait();
                    }
                }

            protected:

                virtual
                State _OnStart(uint32_t /*threadId*/)
                {
                    return StateBits::OK;
                }

                virtual
                void _OnGet(void** /*ppOut*/)
                {}

                void _ActivateNextTask()
                {
                    assert(m_pNextTask);
                    IsActive(false);
                    m_pNextTask->IsActive(true);
                }

            protected:

                SyncObject      m_SyncObj;
                ITask*          m_pNextTask = this;
                State           m_state = StateBits::OK;
                bool            m_isFinished = false;
                //bool            m_isActive = false;
                //bool            m_needEnd = false;
                bool*           m_pIsActive = nullptr;
                State*          m_pState = nullptr;
                
#ifdef _DEBUG
                uint32_t        m_dbgType = 0;
                vke_string      m_strDbgName;
#endif
        };
    } // Threads
    using TaskState = Threads::ITask::State;
    using TaskStateBits = Threads::ITask::StateBits;
} // vke
