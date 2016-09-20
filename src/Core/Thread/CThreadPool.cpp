#include "CThreadPool.h"
#include "CThreadWorker.h"
#include "Core/Utils/CLogger.h"

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
            m_aWorkers[i].WaitForStop();
            m_vThreads[i].join();
        }
        m_vThreads.clear();
        VKE_DELETE_ARRAY(m_aWorkers);
        VKE_DELETE_ARRAY(m_pMemPool);
    }

    Result CThreadPool::Create(const SThreadPoolInfo& Info)
    {
        m_Info = Info;
        if( m_Info.threadCount == Constants::Thread::COUNT_OPTIMAL )
            m_Info.threadCount = static_cast<uint16_t>(std::thread::hardware_concurrency() - 1);

        m_aWorkers = VKE_NEW CThreadWorker[ m_Info.threadCount ];
        if(!m_aWorkers)
        {
            VKE_LOG_ERR_RET(VKE_ENOMEMORY, "No memory for thread workers");
        }

        m_threadMemSize = m_Info.taskMemSize * m_Info.maxTaskCount;
        m_memPoolSize = m_threadMemSize * m_Info.threadCount;
        m_pMemPool = VKE_NEW mem_t[ m_memPoolSize ];
        if(!m_pMemPool)
        {
            return VKE_ENOMEMORY;
        }

        m_vThreads.resize( m_Info.threadCount );
        for(size_t i = 0; i < m_vThreads.size(); ++i)
        {
            memptr_t pMem = m_pMemPool + i * m_threadMemSize;
            if( VKE_SUCCEEDED( m_aWorkers[ i ].Create( i, m_Info.taskMemSize, m_Info.maxTaskCount, pMem) ) )
            {
                m_vThreads[i] = std::thread(std::ref(m_aWorkers[i]));
            }
        }
 
        return VKE_OK;
    }

    int32_t CThreadPool::_CalcThreadId(int32_t threadId) const
    {
        if (threadId < 0)
        {
            size_t uMin = std::numeric_limits<size_t>::max();
            size_t uId = uMin;
            for (uint32_t i = 0; i < m_vThreads.size(); ++i)
            {
                auto uCount = m_aWorkers[i].GetWorkCount();
                if (uCount < uMin)
                {
                    uMin = uCount;
                    uId = i;
                }
            }
            threadId = (int32_t)uId;
        }
        return threadId;
    }

    Result CThreadPool::AddTask(int32_t threadId, const STaskParams& Params, TaskFunction&& Func)
    {
        if(GetThreadCount())
        {
            threadId = _CalcThreadId(threadId);
            return m_aWorkers[threadId].AddWork(Func, Params, threadId);
        }
        return VKE_FAIL;
    }

    Result CThreadPool::AddTask(int32_t threadId, Thread::ITask* pTask)
    {
        if( GetThreadCount() )
        {
            threadId = _CalcThreadId( threadId );
            return m_aWorkers[ threadId ].AddTask( pTask );
        }
        return VKE_FAIL;
    }

    Result CThreadPool::AddConstantTask(int32_t threadId, void* pData, TaskFunction2&& Func)
    {
        if (GetThreadCount())
        {
            threadId = _CalcThreadId(threadId);
            return m_aWorkers[threadId].AddConstantWork(Func, pData);
        }
        return VKE_FAIL;
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

} // VKE