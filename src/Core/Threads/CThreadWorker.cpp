#include "CThreadWorker.h"
#include "CThreadPool.h"

namespace VKE
{
    CThreadWorker::CThreadWorker()
    {
        
    }

    CThreadWorker::CThreadWorker(const CThreadWorker& Other)
    {
        m_qWorks = Other.m_qWorks;
    }

    CThreadWorker::CThreadWorker(CThreadWorker&& Other)
    {
        m_qWorks = std::move(Other.m_qWorks);
    }

    CThreadWorker::~CThreadWorker()
    {
        m_bNeedStop = true;
    }

    void CThreadWorker::operator=(const CThreadWorker& Other)
    {
        m_qWorks = Other.m_qWorks;
    }

    void CThreadWorker::operator=(CThreadWorker&& Other)
    {
        m_qWorks = std::move(Other.m_qWorks);
    }

    Result CThreadWorker::Create(CThreadPool* pPool, uint32_t id, uint16_t taskMemSize, 
        uint16_t taskCount, memptr_t pMemPool)
    {
        m_pPool = pPool;
        m_id = id;
        m_vDataPool.resize(taskCount);
        m_vFreeIds.reserve(m_vDataPool.size());
        m_pMemPool = pMemPool;
        m_memPoolSize = taskMemSize * taskCount;
        m_taskMemSize = taskMemSize;
        uint16_t count = (uint16_t)m_vDataPool.size();
        for(uint16_t i = 0; i < count; ++i)
        {
            m_vFreeIds.push_back(i);
        }
        
        return VKE_OK;
    }

    void CThreadWorker::_RunConstantTasks()
    {
        Threads::ScopedLock l( m_ConstantTasks.SyncObj );
        int32_t taskCount = static_cast< int32_t >( m_ConstantTasks.vStates.GetCount() );
        for( int32_t i = 0; i < taskCount; ++i )
        {
            TaskState& state = m_ConstantTasks.vStates[ i ];
            
            bool isActive = ( state & TaskStateBits::NOT_ACTIVE ) == 0;
            bool needRemove = ( state & TaskStateBits::REMOVE ) != 0;
            bool fail = (state & TaskStateBits::FAIL) != 0;
            bool finished = (state & TaskStateBits::FINISHED) != 0;
            bool next = (state & TaskStateBits::NEXT_TASK) != 0;
            bool ok = (state & TaskStateBits::OK) != 0;
            
            if( isActive && !needRemove )
            {
                Threads::ITask* pTask = m_ConstantTasks.vpTasks[ i ];
                state = static_cast< TaskState >( pTask->Start( m_id ) );
                
                if( state & TaskStateBits::NOT_ACTIVE )
                {
                    
                }
                if( state & TaskStateBits::NEXT_TASK )
                {
                    pTask->IsActive( false );
                    pTask->m_pNextTask->IsActive( true );
                }
                /*if( ( state & TaskStateBits::REMOVE ) )
                {
                    m_ConstantTasks.vpTasks.Remove( i );
                    m_ConstantTasks.vStates.Remove( i );
                    --i;
                    taskCount = static_cast< int32_t >( m_ConstantTasks.vStates.GetCount() );
                }*/
            }
            else
            {

            }
        }
    }

    void CThreadWorker::Start()
    {
        //uint32_t loop = 0;
        while(!m_bNeedStop)
        {
            if(!m_bPaused)
            {
                _RunConstantTasks();

                Threads::ITask* pTask = nullptr;
                {
                    //Threads::UniqueLock l( m_Mutex );
                    if( !m_qTasks.empty() )
                    {
                        Threads::ScopedLock l( m_TaskSyncObj );
                        pTask = m_qTasks.front();
                        m_qTasks.pop_front();
                    }
                    else
                    {
                        _StealTask();
                    }
                }
                if( pTask )
                {
                    pTask->Start( m_id );
                }
            }
            std::this_thread::yield();
        }
        m_bIsEnd = true;
    }

    Result CThreadWorker::AddWork(const WorkFunc& Func, const STaskParams& Params, int32_t threadId)
    {
        (void)threadId;
        if(Params.inputParamSize >= m_taskMemSize)
        {
            return VKE_FAIL;
        }
        
        SWorkerData* pData = GetFreeData();
        if(pData)
        {
            memcpy(pData->pData, Params.pInputParam, Params.inputParamSize);
            pData->dataSize = Params.inputParamSize;
            pData->Func = Func;
            pData->pResult = Params.pResult;
            if(pData->pResult)
                pData->pResult->m_ready = false;

            Threads::ScopedLock l( m_TaskSyncObj );
            m_qWorks.push_back(pData);
            return VKE_OK;
        }
        return VKE_FAIL;
    }

    Result CThreadWorker::AddTask(Threads::ITask* pTask)
    {
        Threads::ScopedLock l( m_TaskSyncObj );
        m_qTasks.push_back( pTask );
        return VKE_OK;
    }

    Result CThreadWorker::AddConstantWork(const WorkFunc2& Func, void* pPtr)
    {
        m_ConstantTaskSyncObj.Lock();
        m_vConstantWorks.push_back({pPtr, Func});
        m_ConstantTaskSyncObj.Unlock();
        return VKE_OK;
    }

    Result CThreadWorker::AddConstantTask(Threads::ITask* pTask, TaskState state)
    {
        Threads::ScopedLock l(m_ConstantTaskSyncObj);
        m_vConstantTasks.PushBack(pTask);
        
        m_ConstantTasks.vpTasks.PushBack(pTask);

        uint32_t id = m_ConstantTasks.vStates.PushBack( state );

        pTask->m_state = state;
        pTask->m_pState = &m_ConstantTasks.vStates[ id ];

        return VKE_OK;
    }

    void CThreadWorker::Stop()
    {
        //LockGuard l(m_Mutex);
        m_bNeedStop = true;
    }

    void CThreadWorker::Pause(bool bPause)
    {
        m_bPaused = bPause;
    }

    bool CThreadWorker::IsPaused()
    {
        return m_bPaused;
    }

    void CThreadWorker::WaitForStop()
    {
        while(!m_bIsEnd)
        {
            std::this_thread::yield();
        }
    }

    CThreadWorker::SWorkerData* CThreadWorker::GetFreeData()
    {
        Threads::ScopedLock l( m_TaskSyncObj );
        if(!m_vFreeIds.empty())
        {
            auto id = m_vFreeIds.back();
            m_vFreeIds.pop_back();
            auto* pData = &m_vDataPool[ id ];
            pData->pData = m_pMemPool + id * m_taskMemSize;
            pData->handle = id;
            return pData;
        }
        return nullptr;
    }

    void CThreadWorker::FreeData(SWorkerData* pData)
    {
        assert(pData);
        Threads::ScopedLock l( m_TaskSyncObj );
        m_vFreeIds.push_back(pData->handle);
    }

    void CThreadWorker::_StealTask()
    {
        auto pTask = m_pPool->_PopTask();
        if( pTask )
        {
            Threads::ScopedLock l( m_TaskSyncObj );
            m_qTasks.push_back(pTask);
        }
    }

} // VKE