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
            public:

                struct ResultBits
                {
                    enum : uint8_t
                    {
                        OK = 0x00000000,
                        FAIL = 0x00000001,
                        NEXT_TASK = 0x00000002,
                        NOT_ACTIVE = 0x00000004,
                        REMOVE = 0x00000008
                    };
                };
                using Result = uint8_t;

            public:

                ITask()
                {}
                
                virtual     ~ITask()
                {
                    m_needEnd = true;
                    m_isActive = false;
                    //Wait();
                };

                uint32_t Start(uint32_t threadId)
                {
                    Result res = ResultBits::REMOVE;
                    ScopedLock l( m_SyncObj );
                    if( !m_needEnd )
                    {
                        res = ResultBits::NOT_ACTIVE;
                        if( IsActive() )
                        {
                            m_isFinished = false;
                            res = _OnStart(threadId);
                            m_isFinished = true;
                            m_isActive = !( res & ResultBits::NOT_ACTIVE );
                            if( res & ResultBits::NEXT_TASK )
                            {
                                _ActivateNextTask();
                            }
                        }
                    }
                    return res;
                }

                template<_THREAD_SAFE _THREAD_SAFE_ = THREAD_SAFE>
                bool        IsFinished()
                {
                    bool is;
                    if( _THREAD_SAFE_ )
                    {
                        ScopedLock l(m_SyncObj);
                        is = m_isFinished;
                    }
                    else
                    {
                        is = m_isFinished;
                    }
                    return is;
                }

                template<_THREAD_SAFE _THREAD_SAFE_ = THREAD_SAFE>
                void        IsFinished(bool is)
                {
                    if( _THREAD_SAFE_ )
                    {
                        ScopedLock l( m_SyncObj );
                        m_isFinished = is;
                    }
                    else
                    {
                        m_isFinished = is;
                    }
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
                    IsActive(isActive);
                    m_pNextTask = pTask;
                }

                bool IsActive()
                {
                    bool isActive = m_isActive;
                    return isActive;
                }

                void IsActive(bool is)
                {
                    m_isActive = is;
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
                    m_needEnd = true;
                    if( WaitForFinish )
                    {
                        Wait();
                    }
                }

            protected:

                virtual
                Result _OnStart(uint32_t /*threadId*/)
                {
                    return ResultBits::OK;
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
                bool            m_isFinished = false;
                bool            m_isActive = true;
                bool            m_needEnd = false;
                
#ifdef _DEBUG
                uint32_t        m_dbgType = 0;
                vke_string      m_strDbgName;
#endif
        };
    } // Threads
    using TaskResult = Threads::ITask::Result;
    using TaskResultBits = Threads::ITask::ResultBits;
} // vke
