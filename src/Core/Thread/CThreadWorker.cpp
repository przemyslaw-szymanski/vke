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

    void CThreadWorker::Start()
    {
        uint32_t loop = 0;
        while(!m_bNeedStop)
        {
            if(!m_bPaused)
            {
                // Do constant works first
                for (auto& WorkData : m_vConstantWorks)
                {
                    WorkData.Func(0, WorkData.pData);
                }

				{
					Thread::LockGuard l(m_Mutex);
					//for (auto& pTask : m_vConstantTasks)
					auto size = m_vConstantTasks.size();
					for(decltype(size) i = 0; i < size; ++i)
					{
						auto pTask = m_vConstantTasks[i];
						assert(pTask);
						pTask->Start(m_id);
					}
				}

                SWorkerData* pData = nullptr;
                {
                    Thread::LockGuard l(m_Mutex);
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
                }

                Thread::ITask* pTask = nullptr;
                {
                    Thread::LockGuard l( m_Mutex );
                    if( !m_qTasks.empty() )
                    {
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

            Thread::LockGuard l(m_Mutex);
            m_qWorks.push_back(pData);
            return VKE_OK;
        }
        return VKE_FAIL;
    }

    Result CThreadWorker::AddTask(Thread::ITask* pTask)
    {
        Thread::LockGuard l( m_Mutex );
        m_qTasks.push_back( pTask );
        return VKE_OK;
    }

    Result CThreadWorker::AddConstantWork(const WorkFunc2& Func, void* pPtr)
    {
        m_vConstantWorks.push_back({pPtr, Func});
        return VKE_OK;
    }

	Result CThreadWorker::AddConstantTask(Thread::ITask* pTask)
	{
		m_vConstantTasks.push_back(pTask);
		return VKE_OK;
	}

    void CThreadWorker::Stop()
    {
        //LockGuard l(m_Mutex);
        m_bNeedStop = true;
    }

    void CThreadWorker::Pause(bool bPause)
    {
        Thread::LockGuard l(m_Mutex);
        m_bPaused = bPause;
    }

    bool CThreadWorker::IsPaused()
    {
        Thread::LockGuard l(m_Mutex);
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
        Thread::LockGuard l(m_Mutex);
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
        Thread::LockGuard l(m_Mutex);
        m_vFreeIds.push_back(pData->handle);
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