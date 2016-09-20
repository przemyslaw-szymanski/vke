#pragma once

#include "Core/Thread/Common.h"
#include "Core/Thread/ITask.h"

namespace VKE
{
    class CThreadWorker;

    class CThreadPool
    {
        public:

            using ThreadVec = std::vector< std::thread >;
            using PtrStack = std::stack< uint8_t* >;
            using ThreadIdVec = std::vector< std::thread::id >;
            using ThreadID = std::thread::id;

            static const size_t PAGE_SIZE = 1024;

        public:

                        CThreadPool();
            virtual     ~CThreadPool();
                
            Result      Create(const SThreadPoolInfo& Info);
            void        Destroy();

            Result      AddTask(int32_t iThreadId, const STaskParams& Params, TaskFunction&& Func);
            Result      AddTask(int32_t threadId, Thread::ITask* pTask);
            Result      AddConstantTask(int32_t threadId, void* pData, TaskFunction2&& Func);

            size_t      GetThreadCount() const { return m_vThreads.size(); }
            const
            ThreadID&   GetOSThreadId(uint32_t id) const{ return m_vThreads[id].get_id(); }

            int32_t    GetThisThreadID() const;

        protected:

            int32_t     _CalcThreadId(int32_t threadId) const;

        protected:

            SThreadPoolInfo m_Info;
            ThreadVec       m_vThreads;
            CThreadWorker*  m_aWorkers = nullptr;
            memptr_t        m_pMemPool = nullptr;
            size_t          m_memPoolSize = 0;
            size_t          m_threadMemSize = 0;
    };

} // VKE
