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
        assert( m_ConstantTasks.vpTasks.GetCount() == m_ConstantTasks.vActives.GetCount() );
        for( int32_t i = 0; i < m_ConstantTasks.vActives.GetCount(); ++i )
        {
            TaskResult res = TaskResultBits::NOT_ACTIVE;
            if( m_ConstantTasks.vActives[ i ] )
            {
                Threads::ITask* pTask = m_ConstantTasks.vpTasks[ i ];
                res = pTask->Start( m_id );
                if( res & TaskResultBits::NOT_ACTIVE )
                {
                    m_ConstantTasks.vActives[ i ] = false;
                }
                if( res & TaskResultBits::NEXT_TASK )
                {
                    m_ConstantTasks.vActives[ i ] = false;
                    pTask->m_pNextTask->IsActive( true );
                }
                if( res & TaskResultBits::REMOVE )
                {
                    m_ConstantTasks.vpTasks.Remove( i );
                    m_ConstantTasks.vActives.Remove( i );
                    m_ConstantTasks.vFinishes.Remove( i );
                    --i;
                }
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
                // Do constant works first
                /*for (auto& WorkData : m_vConstantWorks)
                {
                    WorkData.Func(0, WorkData.pData);
                }*/

                //{
                //    Threads::ScopedLock l(m_ConstantTaskSyncObj);
                //    //for (auto& pTask : m_vConstantTasks)
                //    //m_ConstantTaskSyncObj.Lock();
                //    for(int32_t i = 0; i < m_vConstantTasks.GetCount(); ++i)
                //    {
                //        auto pTask = m_vConstantTasks[i];
                //        assert(pTask);
                //        Threads::ITask::Result res = pTask->Start(m_id);
                //        if( res & TaskResultBits::REMOVE )
                //        {
                //            /// @todo optimize this code 
                //            //m_vConstantTasks.erase(std::find(m_vConstantTasks.begin(), m_vConstantTasks.end(), pTask));
                //            m_vConstantTasks.Remove(i);
                //            --i;
                //        }
                //    }
                //}
                _RunConstantTasks();

                /*SWorkerData* pData = nullptr;
                {
                    Threads::UniqueLock l(m_Mutex);
                    if(!m_qWorks.empty())
                    {
                        pData = m_qWorks.front();
                        m_qWorks.pop_front();
                    }
                    else
                    {
                        
                    }
                }
                if(pData && pData->Func)
                {
                    pData->Func(pData->pData, pData->pResult);
                    if(pData->pResult)
                        pData->pResult->NotifyReady();
                    FreeData(pData);
                }*/

                Threads::ITask* pTask = nullptr;
                {
                    //Threads::UniqueLock l( m_Mutex );
                    if( !m_qTasks.empty() )
                    {
                        m_TaskSyncObj.Lock();
                        pTask = m_qTasks.front();
                        m_qTasks.pop_front();
                        m_TaskSyncObj.Unlock();
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

            m_TaskSyncObj.Lock();
            m_qWorks.push_back(pData);
            m_TaskSyncObj.Unlock();
            return VKE_OK;
        }
        return VKE_FAIL;
    }

    Result CThreadWorker::AddTask(Threads::ITask* pTask)
    {
        m_TaskSyncObj.Lock();
        m_qTasks.push_back( pTask );
        m_TaskSyncObj.Unlock();
        return VKE_OK;
    }

    Result CThreadWorker::AddConstantWork(const WorkFunc2& Func, void* pPtr)
    {
        m_ConstantTaskSyncObj.Lock();
        m_vConstantWorks.push_back({pPtr, Func});
        m_ConstantTaskSyncObj.Unlock();
        return VKE_OK;
    }

    Result CThreadWorker::AddConstantTask(Threads::ITask* pTask)
    {
        Threads::ScopedLock l(m_ConstantTaskSyncObj);
        m_vConstantTasks.PushBack(pTask);
        m_ConstantTasks.vpTasks.PushBack(pTask);
        uint32_t activeId = m_ConstantTasks.vActives.PushBack(pTask->IsActive());
        m_ConstantTasks.vFinishes.PushBack(pTask->IsFinished());
        pTask->m_pIsActive = &m_ConstantTasks.vActives[ activeId ];
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
        m_TaskSyncObj.Lock();
        if(!m_vFreeIds.empty())
        {
            auto id = m_vFreeIds.back();
            m_vFreeIds.pop_back();
            auto* pData = &m_vDataPool[ id ];
            pData->pData = m_pMemPool + id * m_taskMemSize;
            pData->handle = id;
            m_TaskSyncObj.Unlock();
            return pData;
        }
        m_TaskSyncObj.Unlock();
        return nullptr;
    }

    void CThreadWorker::FreeData(SWorkerData* pData)
    {
        assert(pData);
        m_TaskSyncObj.Lock();
        m_vFreeIds.push_back(pData->handle);
        m_TaskSyncObj.Unlock();
    }

    void CThreadWorker::_StealTask()
    {
        auto pTask = m_pPool->_PopTask();
        if( pTask )
        {
            m_qTasks.push_back(pTask);
        }
    }

} // VKE