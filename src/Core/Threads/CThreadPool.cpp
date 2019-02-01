#include "Core/Threads/CThreadPool.h"
#include "Core/Utils/CLogger.h"
#include "Core/Threads/CTaskGroup.h"

namespace VKE
{
    CThreadPool::CThreadPool()
    {

    }

    CThreadPool::~CThreadPool()
    {
        Destroy();
    }

    void CThreadPool::Destroy()
    {
        for(size_t i = 0; i < m_vThreads.size(); ++i)
        {
            m_vWorkers[i].Stop();
        }
        for( size_t i = 0; i < m_vThreads.size(); ++i )
        {
            //m_vWorkers[ i ].WaitForStop();
            m_vThreads[ i ].join();
        }
        m_vThreads.clear();
        m_vWorkers.ClearFull();
        VKE_DELETE_ARRAY(m_pMemPool);
    }

    Result CThreadPool::Create(const SThreadPoolInfo& Info)
    {
        m_Desc = Info;
        if( m_Desc.threadCount == Constants::Threads::COUNT_OPTIMAL )
            m_Desc.threadCount = static_cast<uint16_t>(std::thread::hardware_concurrency() - 1);

        bool res = m_vWorkers.Resize( Platform::Thread::GetMaxConcurrentThreadCount() - 1 );
        if(!res)
        {
            VKE_LOG_ERR_RET(VKE_ENOMEMORY, "No memory for thread workers");
        }

        m_threadMemSize = m_Desc.taskMemSize * m_Desc.maxTaskCount;
        m_memPoolSize = m_threadMemSize * m_Desc.threadCount;
        m_pMemPool = VKE_NEW mem_t[ m_memPoolSize ];
        if(!m_pMemPool)
        {
            return VKE_ENOMEMORY;
        }

        m_vThreads.resize( m_Desc.threadCount );
        for(size_t i = 0; i < m_vThreads.size(); ++i)
        {
            memptr_t pMem = m_pMemPool + i * m_threadMemSize;
            if( VKE_SUCCEEDED( m_vWorkers[ i ].Create( this, static_cast<uint32_t>(i),
                m_Desc.taskMemSize, m_Desc.maxTaskCount, pMem) ) )
            {
                m_vThreads[i] = std::thread(std::ref(m_vWorkers[i]));
            }
        }
 
        return VKE_OK;
    }

    SThreadWorkerID CThreadPool::_CalcWorkerID(const SCalcWorkerIDDesc& Desc) const
    {
        SThreadWorkerID threadId;
        if (Desc.threadId.id < 0)
        {
            size_t minTaskCount = std::numeric_limits<size_t>::max();
            size_t minTaskCountId = minTaskCount;
            uint32_t taskCount = 0;
            size_t minWeight = minTaskCountId;
            size_t minWeightId = minWeight;
            // Calculate min of tasks scheduled in threads
            for (uint32_t i = 0; i < m_vThreads.size(); ++i)
            {
                const auto& Worker = m_vWorkers[ i ];
                if( Desc.isConstantTask )
                {
                    taskCount = Worker.GetConstantTaskCount();
                }
                else
                {
                    taskCount = Worker.GetWorkCount();
                }

                if (taskCount < minTaskCount)
                {
                    minTaskCount = taskCount;
                    minTaskCountId = i;
                }

                if( minWeight > Worker.GetTotalTaskWeight() )
                {
                    minWeight = Worker.GetTotalTaskWeight();
                    minWeightId = i;
                }
            }
            // Weight 1 is default minimal value
            if (Desc.taskWeight > 1)
            {
                threadId.id = minWeightId;
            }
            else
            {
                threadId.id = minTaskCountId;
            }
        }
        return threadId;
    }

    Result CThreadPool::AddTask(WorkerID threadId, const STaskParams& Params, TaskFunction&& Func)
    {
        if(GetWorkerCount())
        {
            SCalcWorkerIDDesc Desc;
            Desc.threadId = threadId;
            Desc.isConstantTask = false;
            threadId = _CalcWorkerID( Desc );
            return m_vWorkers[ threadId.id ].AddWork( Func, Params, threadId.id );
        }
        return VKE_FAIL;
    }

    Result CThreadPool::AddTask(WorkerID threadId, Threads::ITask* pTask)
    {
        //if( GetWorkerCount() )
        {
            //threadId = _CalcWorkerID( threadId );
            if (threadId.id < 0)
            {
                return AddTask( pTask );
            }
            VKE_ASSERT( threadId.id < GetWorkerCount(), "Thread id out of bounds." );
            return m_vWorkers[ threadId.id ].AddTask( pTask );
        }
        //return VKE_FAIL;
    }

    Result CThreadPool::AddTask(Threads::ITask* pTask)
    {
        //if( GetWorkerCount() )
        {
            //threadId = _CalcWorkerID( threadId );
            {
                Threads::ScopedLock l(m_TaskSyncObj);
                m_qTasks.push_back( pTask );
                return VKE_OK;
            }
        }
        //return VKE_FAIL;
    }

    CThreadPool::WorkerID CThreadPool::_FindThread(NativeThreadID id)
    {
        for (decltype(m_Desc.threadCount) i = 0; i < m_Desc.threadCount; ++i)
        {
            NativeThreadID ID = NativeThreadID( Platform::Thread::GetID( m_vThreads[ i ].native_handle() ) );
            if (ID.id == id.id)
            {
                return WorkerID( i );
            }
        }
        return WorkerID( -1 );
    }

    Result CThreadPool::AddTask(NativeThreadID threadId, Threads::ITask* pTask)
    {
        // Find thread
        auto id = _FindThread(threadId);
        VKE_ASSERT( id.id >= 0, "Wrong threadId." );
        return m_vWorkers[ id.id ].AddTask( pTask );
    }

    Result CThreadPool::AddConstantTask(WorkerID workerId, void* pData, TaskFunction2&& Func)
    {
        if (GetWorkerCount())
        {
            SCalcWorkerIDDesc Desc;
            Desc.threadId = workerId;
            Desc.isConstantTask = true;
            workerId = _CalcWorkerID( Desc );
            return m_vWorkers[ workerId.id ].AddConstantWork(Func, pData);
        }
        return VKE_FAIL;
    }

    Result CThreadPool::AddConstantTask(WorkerID workerId, Threads::ITask* pTask, TaskState state)
    {
        if( GetWorkerCount() )
        {
            SCalcWorkerIDDesc Desc;
            Desc.threadId = workerId;
            Desc.isConstantTask = true;
            workerId = _CalcWorkerID( Desc );
            return m_vWorkers[ workerId.id ].AddConstantTask(pTask, state);
        }
        return VKE_FAIL;
    }

    Result CThreadPool::AddConstantTask(Threads::ITask* pTask, TaskState state)
    {
        if( GetWorkerCount() )
        {
            SCalcWorkerIDDesc Desc;
            Desc.threadId = WorkerID();
            Desc.isConstantTask = true;
            Desc.taskPriority = pTask->GetTaskPriority();
            Desc.taskWeight = pTask->GetTaskWeight();
            WorkerID workerId = _CalcWorkerID( Desc );
            VKE_ASSERT( workerId.id >= 0, "Wrong thread workder id." );
            return m_vWorkers[ workerId.id ].AddConstantTask( pTask, state );
        }
        return VKE_FAIL;
    }

    Result CThreadPool::AddConstantTask(NativeThreadID threadId, Threads::ITask* pTask, TaskState state)
    {
        auto id = _FindThread(threadId);
        return m_vWorkers[ id.id ].AddConstantTask( pTask, state );
    }

    int32_t CThreadPool::GetThisThreadID() const
    {
        const auto& threadId = std::this_thread::get_id();
        for (uint32_t i = 0; i < m_vThreads.size(); ++i)
        {
            if (m_vThreads[i].get_id() == threadId)
            {
                return i;
            }
        }
        return -1;
    }

    Threads::ITask* CThreadPool::_PopTask()
    {
        //Threads::ScopedLock l(m_TaskSyncObj);
        m_TaskSyncObj.Lock();
        if (!m_qTasks.empty())
        {
            auto pTask = m_qTasks.front();
            m_qTasks.pop_front();
            m_TaskSyncObj.Unlock();
            return pTask;
        }
        m_TaskSyncObj.Unlock();
        return nullptr;
    }

    Result CThreadPool::AddConstantTaskGroup(Threads::CTaskGroup* pGroup)
    {
        auto id = m_vpTaskGroups.PushBack(pGroup);
        pGroup->m_id = id;
        for( uint32_t i = 0; i < pGroup->m_vpTasks.GetCount(); ++i )
        {
            WorkerID threadId = WorkerID( i % m_vThreads.size() );
            AddConstantTask(threadId, pGroup->m_vpTasks[ i ], TaskStateBits::NOT_ACTIVE);
        }
        AddConstantTask( &pGroup->m_Scheduler, TaskStateBits::NOT_ACTIVE );
        return VKE_OK;
    }

    //Result CThreadPool::AddTaskGroup(Threads::CTaskGroup* pGroup)
    //{
    //    auto id = m_vpTaskGroups.PushBack( pGroup );
    //    pGroup->m_id = id;
    //    for( uint32_t i = 0; i < pGroup->m_vpTasks.GetCount(); ++i )
    //    {
    //        WorkerID threadId = i % m_vThreads.size();
    //        AddTask( threadId, pGroup->m_vpTasks[ i ], TaskStateBits::NOT_ACTIVE );
    //    }
    //    AddTask( Constants::Threads::ID_BALANCED, &pGroup->m_Scheduler, TaskStateBits::NOT_ACTIVE );
    //    return VKE_OK;
    //}

} // VKE