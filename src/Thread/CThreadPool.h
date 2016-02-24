#pragma once

#include "Thread/Common.h"

namespace VKE
{
    class CThreadWorker;

    class CThreadPool
    {
        public:

            using ThreadVec = std::vector< std::thread >;
            using PtrStack = std::stack< uint8_t* >;

            static const size_t PAGE_SIZE = 1024;

        public:

                        CThreadPool();
            virtual     ~CThreadPool();
                
            Result       Create(const SThreadPoolInfo& Info);
            void        Destroy();

            Result       AddTask(int32_t iThreadId, const STaskParams& Params, TaskFunction&& Func);

            size_t      GetThreadCount() const { return m_vThreads.size(); }

        protected:

            SThreadPoolInfo m_Info;
            ThreadVec       m_vThreads;
            CThreadWorker*  m_aWorkers = nullptr;
            memptr_t        m_pMemPool = nullptr;
            size_t          m_memPoolSize = 0;
            size_t          m_threadMemSize = 0;
    };

} // VKE
