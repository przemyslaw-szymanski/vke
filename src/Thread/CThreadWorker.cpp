#include "CThreadWorker.h"

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

    Result CThreadWorker::Create(uint16_t taskMemSize, uint16_t taskCount, memptr_t pMemPool)
    {
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
                SWorkerData* pData = nullptr;
                {
                    Thread::LockGuard l(m_Mutex);
                    if(!m_qWorks.empty())
                    {
                        pData = m_qWorks.front();
                        m_qWorks.pop_front();
                    }
                }
                if(pData && pData->Func)
                {
                    printf("thread: %d, loop: %d\n", std::this_thread::get_id(), loop++);
                    pData->Func(pData->pData, pData->pResult);
                    if(pData->pResult)
                        pData->pResult->NotifyReady();
                    FreeData(pData);
                }
            }
            std::this_thread::yield();
        }
        m_bIsEnd = true;
    }

    Result CThreadWorker::AddWork(const WorkFunc& Func, const STaskParams& Params)
    {
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

} // VKE