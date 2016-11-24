#pragma once

#include "Core/Threads/Common.h"
#include "Core/Threads/ITask.h"

namespace VKE
{
    class CThreadWorker;

    class CThreadPool
    {
        friend class CThreadWorker;

        public:

            using ThreadVec = std::vector< std::thread >;
            using PtrStack = std::stack< uint8_t* >;
            using ThreadIdVec = std::vector< std::thread::id >;
            using NativeThreadID = Platform::Thread::ID;
            using WorkerID = int32_t;

            static const size_t PAGE_SIZE = 1024;

        public:

                        CThreadPool();
            virtual     ~CThreadPool();
                
            Result      Create(const SThreadPoolInfo& Info);
            void        Destroy();

            Result		AddTask(NativeThreadID threadId, Threads::ITask* pTask);
            Result      AddTask(WorkerID wokerId, const STaskParams& Params, TaskFunction&& Func);
            Result      AddTask(WorkerID wokerId, Threads::ITask* pTask);
            Result      AddConstantTask(WorkerID wokerId, void* pData, TaskFunction2&& Func);
            Result		AddConstantTask(WorkerID workerId, Threads::ITask* pTask);
            Result		AddConstantTask(NativeThreadID threadId, Threads::ITask* pTask);

            size_t      GetWorkerCount() const { return m_vThreads.size(); }
            const
            NativeThreadID&   GetOSThreadId(uint32_t id)
            { return Platform::Thread::GetID(m_vThreads[ id ].native_handle()); }

            int32_t    GetThisThreadID() const;

        protected:

            WorkerID        _CalcWorkerID(WorkerID id, bool forConstantTask) const;

            Threads::ITask*	_PopTask();

            WorkerID		_FindThread(NativeThreadID id);

        protected:

            SThreadPoolInfo m_Desc;
            ThreadVec       m_vThreads;
            CThreadWorker*  m_aWorkers = nullptr;
            memptr_t        m_pMemPool = nullptr;
            TaskQueue		m_qTasks;
            std::mutex		m_Mutex;
            size_t          m_memPoolSize = 0;
            size_t          m_threadMemSize = 0;
    };

} // VKE
