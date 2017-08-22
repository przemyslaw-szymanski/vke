#pragma once

#include "Core/VKECommon.h"

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
            public:

                enum class Status : uint8_t
                {
                    OK,
                    FAIL,
                    NOT_ACTIVE,
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
                    Status res = Status::REMOVE;
                    if( !m_needEnd )
                    {
                        res = Status::NOT_ACTIVE;
                        if( IsActive() )
                        {
                            IsFinished<THREAD_SAFE>(false);
                            res = _OnStart(threadId);
                            _ActivateNextTask();
                        }
                    }
                    IsFinished<THREAD_SAFE>(true);
                    return res;
                }

                template<_THREAD_SAFE _THREAD_SAFE_ = THREAD_SAFE>
                bool        IsFinished()
                {
                    /*if( _THREAD_SAFE_ )
                    {
                        Threads::UniqueLock l( m_Mutex );
                        return m_bIsFinished;
                    }
                    return m_bIsFinished;*/
                    const bool is = m_isFinished;
                    return is;
                }

                template<_THREAD_SAFE _THREAD_SAFE_ = THREAD_SAFE>
                void        IsFinished(bool is)
                {
                    /*if( _THREAD_SAFE_ )
                    {
                        Threads::UniqueLock l( m_Mutex );
                        m_bIsFinished = is;
                        return;
                    }
                    m_bIsFinished = is;*/
                    m_isFinished = is;
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
                    _OnGet( pOut );
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



            protected:

                virtual
                Status _OnStart(uint32_t /*threadId*/)
                {
                    return Status::OK;
                }

                virtual
                void _OnGet(void* /*pOut*/)
                {}

                void _ActivateNextTask()
                {
                    assert(m_pNextTask);
                    IsActive(false);
                    m_pNextTask->IsActive(true);
                }

            protected:

                std::mutex      m_Mutex;
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
} // vke
