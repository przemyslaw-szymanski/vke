#pragma once

#include "Core/VKECommon.h"
#include "Common.h"

namespace VKE
{
    namespace Threads
    {
        class ITask
        {
            friend class CThreadPool;
            friend class CThreadWorker;
            public:

                using JobFunc = std::function< uint32_t(ITask*) >;

                struct StateBits
                {
                    enum : uint8_t
                    {
                        OK          = 0x00000000,
                        FAIL        = VKE_BIT(0),
                        NEXT_TASK   = VKE_BIT(1),
                        NOT_ACTIVE  = VKE_BIT(2),
                        FINISHED    = VKE_BIT(3),
                        REMOVE      = VKE_BIT(4),
                        WAIT        = VKE_BIT(5)
                    };
                };
                using State = uint8_t;

            public:

                ITask()
                {}

                virtual     ~ITask()
                {
                };

                uint32_t Start(uint32_t threadId)
                {
                    ScopedLock l( m_SyncObj );
                    /*if( m_needEnd )
                    {
                        m_state = StateBits::REMOVE;
                        return m_state;
                    }*/
                    //m_isFinished = false;

                    VKE_UNSET_MASK( m_state, StateBits::FINISHED );
                    VKE_SET_MASK( m_state, _OnStart( threadId ) );
                    if( Func )
                    {
                        VKE_SET_MASK( m_state, Func( this ) );
                    }
                    VKE_SET_MASK( m_state, StateBits::FINISHED );
                    //m_isFinished = true;
                    return m_state;
                }

                template<_THREAD_SAFE IsThreadSafe>
                bool IsStateSet(State state)
                {
                    State res;
                    if( IsThreadSafe )
                    {
                        ScopedLock l( m_SyncObj );
                        res = ( m_state & state );
                    }
                    else
                    {
                        res = ( m_state & state );
                    }
                    return res != 0;
                }

                template<_THREAD_SAFE IsThreadSafe = THREAD_SAFE>
                bool        IsFinished()
                {
                    //return m_isFinished;
                    return IsStateSet< IsThreadSafe >( StateBits::FINISHED );
                }

                /*template<_THREAD_SAFE IsThreadSafe = THREAD_SAFE>
                void        IsFinished(bool is)
                {
                    if( IsThreadSafe )
                    {
                        ScopedLock l( m_SyncObj );
                        m_isFinished = is;
                    }
                    else
                    {
                        m_isFinished = is;
                    }
                    if( is )
                    {
                        SetState< IsThreadSafe >( StateBits::FINISHED );
                    }
                    else
                    {
                        UnsetState< IsThreadSafe >( StateBits::FINISHED );
                    }
                }*/

                template<_THREAD_SAFE IsThreadSafe>
                State GetState()
                {
                    State state = m_state;
                    if( IsThreadSafe )
                    {
                        ScopedLock l( m_SyncObj );
                        state = m_state;
                    }
                    return state;
                }

                void    Wait()
                {
                    while( true )
                    {
                        bool needBreak = false;
                        {
                            ScopedLock l( m_SyncObj );
                            needBreak = IsFinished<NO_THREAD_SAFE>() || !IsActive<NO_THREAD_SAFE>();
                        }
                        if( needBreak )
                        {
                            break;
                        }
                        Platform::ThisThread::Pause();
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

                void SetNextTask(ITask* pTask)
                {
                    m_pNextTask = pTask;
                }

                template<_THREAD_SAFE IsThreadSafe = THREAD_SAFE>
                bool IsActive()
                {
                    return !IsStateSet< IsThreadSafe >(StateBits::NOT_ACTIVE);
                }

                template<_THREAD_SAFE IsThreadSafe = THREAD_SAFE>
                void IsActive(bool is)
                {
                    if( !is )
                    {
                        _SetState<IsThreadSafe>( StateBits::NOT_ACTIVE );
                    }
                    else
                    {
                        _UnsetState<IsThreadSafe>( StateBits::NOT_ACTIVE );
                    }
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

                template<bool WaitForFinish = true, _THREAD_SAFE IsThreadSafe = THREAD_SAFE>
                void Remove()
                {
                    if( WaitForFinish )
                    {
                        Wait();
                    }
                    if( IsThreadSafe )
                    {
                        ScopedLock l( m_SyncObj );
                        m_needEnd = true;
                        VKE_SET_MASK( m_state, StateBits::REMOVE /*| StateBits::NOT_ACTIVE*/ );
                        *m_pState = m_state;
                    }
                    else
                    {
                        m_needEnd = true;
                        VKE_SET_MASK( m_state, StateBits::REMOVE /*| StateBits::NOT_ACTIVE*/ );
                        *m_pState = m_state;
                    }
                }

                void SetTaskPriority(uint8_t priority) { m_priority = priority; }
                void SetTaskWeight(uint8_t weight) { m_weight = weight; }
                uint8_t GetTaskPriority() const { return m_priority; }
                uint8_t GetTaskWeight() const { return m_weight; }

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

                template<_THREAD_SAFE IsThreadSafe>
                void _SetState( State state )
                {
                    if( IsThreadSafe )
                    {
                        ScopedLock l( m_SyncObj );

                        VKE_SET_MASK( m_state, state );
                        //if( m_pState )
                        {
                            *m_pState = m_state;
                        }
                    }
                    else
                    {
                        VKE_SET_MASK( m_state, state );
                        //if( m_pState )
                        {
                            *m_pState = m_state;
                        }
                    }
                }

                template<_THREAD_SAFE IsThreadSafe>
                void _UnsetState( State state )
                {
                    if( IsThreadSafe )
                    {
                        ScopedLock l( m_SyncObj );

                        VKE_UNSET_MASK( m_state, state );
                        //if( m_pState )
                        {
                            *m_pState = m_state;
                        }
                    }
                    else
                    {
                        VKE_UNSET_MASK( m_state, state );
                        //if( m_pState )
                        {
                            *m_pState = m_state;
                        }
                    }
                }

            public:

                JobFunc         Func;

            protected:

                SyncObject      m_SyncObj;

            private:

                ITask*          m_pNextTask = this;
                State*          m_pState = &m_state;
                State           m_state = StateBits::OK;
                uint8_t         m_priority = 0;
                uint8_t         m_weight = 1;
                bool            m_isFinished = false;
                bool            m_needEnd = false;

#ifdef _DEBUG
            protected:
                uint32_t        m_dbgType = 0;
                vke_string      m_strDbgName;
#endif
        };

        template<class T>
        struct TSDataTypedTask : public ITask
        {
            T TaskData;
        };

        template<uint32_t Size>
        struct TSTask : ITask
        {
            uint8_t m_TaskData[Size];
        };
    } // Threads
    using TaskState = Threads::ITask::State;
    using TaskStateBits = Threads::ITask::StateBits;
} // vke
