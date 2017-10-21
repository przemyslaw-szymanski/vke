#include "CThreadPool.h"
#include "CThreadWorker.h"
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
            m_aWorkers[i].Stop();
        }
        for( size_t i = 0; i < m_vThreads.size(); ++i )
        {
            //m_aWorkers[ i ].WaitForStop();
            m_vThreads[ i ].join();
        }
        m_vThreads.clear();
        VKE_DELETE_ARRAY(m_aWorkers);
        VKE_DELETE_ARRAY(m_pMemPool);
    }

    Result CThreadPool::Create(const SThreadPoolInfo& Info)
    {
        m_Desc = Info;
        if( m_Desc.threadCount == Constants::Threads::COUNT_OPTIMAL )
            m_Desc.threadCount = static_cast<uint16_t>(std::thread::hardware_concurrency() - 1);

        m_aWorkers = VKE_NEW CThreadWorker[ m_Desc.threadCount ];
        if(!m_aWorkers)
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
            if( VKE_SUCCEEDED( m_aWorkers[ i ].Create( this, static_cast<uint32_t>(i),
                m_Desc.taskMemSize, m_Desc.maxTaskCount, pMem) ) )
            {
                m_vThreads[i] = std::thread(std::ref(m_aWorkers[i]));
            }
        }
 
        return VKE_OK;
    }

    int32_t CThreadPool::_CalcWorkerID(int32_t threadId, bool forConstantTask) const
    {
        if (threadId < 0)
        {
            size_t uMin = std::numeric_limits<size_t>::max();
            size_t uId = uMin;
            uint32_t count = 0;
            for (uint32_t i = 0; i < m_vThreads.size(); ++i)
            {
                if( forConstantTask )
                {
                    count = m_aWorkers[ i ].GetConstantTaskCount();
                }
                else
                {
                    count = m_aWorkers[ i ].GetWorkCount();
                }
                if (count < uMin)
                {
                    uMin = count;
                    uId = i;
                }
            }
            threadId = (int32_t)uId;
        }
        return threadId;
    }

    Result CThreadPool::AddTask(WorkerID threadId, const STaskParams& Params, TaskFunction&& Func)
    {
        if(GetWorkerCount())
        {
            threadId = _CalcWorkerID(threadId, false);
            return m_aWorkers[threadId].AddWork(Func, Params, threadId);
        }
        return VKE_FAIL;
    }

    Result CThreadPool::AddTask(WorkerID threadId, Threads::ITask* pTask)
    {
        if( GetWorkerCount() )
        {
            //threadId = _CalcWorkerID( threadId );
            if (threadId < 0)
            {
                //Threads::ScopedLock l(m_TaskSyncObj);
                m_TaskSyncObj.Lock();
                m_qTasks.push_back(pTask);
                m_TaskSyncObj.Unlock();
                return VKE_OK;
            }
            return m_aWorkers[ threadId ].AddTask( pTask );
        }
        return VKE_FAIL;
    }

    CThreadPool::WorkerID CThreadPool::_FindThread(NativeThreadID id)
    {
        for (decltype(m_Desc.threadCount) i = 0; i < m_Desc.threadCount; ++i)
        {
            if (Platform::Thread::GetID(m_vThreads[i].native_handle()) == id)
            {
                return i;
            }
        }
        return -1;
    }

    Result CThreadPool::AddTask(NativeThreadID threadId, Threads::ITask* pTask)
    {
        // Find thread
        auto id = _FindThread(threadId);
        return m_aWorkers[id].AddTask(pTask);
    }

    Result CThreadPool::AddConstantTask(WorkerID wokerId, void* pData, TaskFunction2&& Func)
    {
        if (GetWorkerCount())
        {
            wokerId = _CalcWorkerID(wokerId, true);
            return m_aWorkers[ wokerId ].AddConstantWork(Func, pData);
        }
        return VKE_FAIL;
    }

    Result CThreadPool::AddConstantTask(WorkerID wokerId, Threads::ITask* pTask, TaskState state)
    {
        if( GetWorkerCount() )
        {
            wokerId = _CalcWorkerID(wokerId, true);
            return m_aWorkers[ wokerId ].AddConstantTask(pTask, state);
        }
        return VKE_FAIL;
    }

    Result CThreadPool::AddConstantTask(NativeThreadID threadId, Threads::ITask* pTask, TaskState state)
    {
        auto id = _FindThread(threadId);
        return m_aWorkers[id].AddConstantTask(pTask, state);
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
            WorkerID threadId = i % m_vThreads.size();
            AddConstantTask(threadId, pGroup->m_vpTasks[ i ], TaskStateBits::NOT_ACTIVE);
        }
        AddConstantTask(Constants::Threads::ID_BALANCED, &pGroup->m_Scheduler, TaskStateBits::NOT_ACTIVE);
        return VKE_OK;
    }

} // VKE