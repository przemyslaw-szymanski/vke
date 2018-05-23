#pragma once

#include "Core/Threads/Common.h"
#include "Core/Threads/ITask.h"
#include "Core/Utils/TCDynamicArray.h"
#include "Core/Threads/CThreadWorker.h"

namespace VKE
{
    class CThreadWorker;
    
    namespace Threads
    {
        class CTaskGroup;
    } // Threads

    template<typename T, T DefaultValue>
    struct TSThreadID
    {
        T id = DefaultValue;

        TSThreadID() {}
        explicit TSThreadID(const T& idx) : id{ idx } {}
    };

    using SThreadWorkerID = TSThreadID< int32_t, -1 >;
    using SNativeThreadID = TSThreadID< Platform::Thread::ID, 0 >;

    class CThreadPool
    {
        friend class CThreadWorker;

        public:

            using ThreadVec = std::vector< std::thread >;
            using PtrStack = std::stack< uint8_t* >;
            using ThreadIdVec = std::vector< std::thread::id >;
            using NativeThreadID = SNativeThreadID;
            using WorkerID = SThreadWorkerID;
            using TaskGroupVec = Utils::TCDynamicArray< Threads::CTaskGroup*, 64 >;
            using WorkerVec = Utils::TCDynamicArray< CThreadWorker, 16 >;

            static const size_t PAGE_SIZE = 1024;

            struct SCalcWorkerIDDesc
            {
                WorkerID    threadId;
                uint8_t     taskWeight = 0;
                uint8_t     taskPriority = 0;
                bool        isConstantTask = false;
            };

        public:

                        CThreadPool();
            virtual     ~CThreadPool();
                
            Result      Create(const SThreadPoolInfo& Info);
            void        Destroy();

            Result		AddTask(NativeThreadID threadId, Threads::ITask* pTask);
            Result      AddTask(WorkerID wokerId, const STaskParams& Params, TaskFunction&& Func);
            Result      AddTask(WorkerID wokerId, Threads::ITask* pTask);
            Result      AddTask(Threads::ITask* pTask);

            Result		AddConstantTask(Threads::ITask* pTask, TaskState state );
            Result      AddConstantTask(WorkerID wokerId, void* pData, TaskFunction2&& Func);
            Result		AddConstantTask(WorkerID workerId, Threads::ITask* pTask, TaskState state);
            Result		AddConstantTask(NativeThreadID threadId, Threads::ITask* pTask, TaskState state);
            
            Result      AddConstantTaskGroup(Threads::CTaskGroup* pGroup);
            //Result      AddTaskGroup(Threads::CTaskGroup* pGroup);

            size_t      GetWorkerCount() const { return m_vThreads.size(); }
            const
            NativeThreadID   GetOSThreadId(uint32_t id)
            {
                return NativeThreadID( Platform::Thread::GetID( m_vThreads[ id ].native_handle() ) );
            }

            int32_t    GetThisThreadID() const;

        protected:

            WorkerID        _CalcWorkerID(const SCalcWorkerIDDesc& Desc) const;

            Threads::ITask*	_PopTask();

            WorkerID		_FindThread(NativeThreadID id);

        protected:

            SThreadPoolInfo m_Desc;
            ThreadVec       m_vThreads;
            TaskGroupVec    m_vpTaskGroups;
            WorkerVec       m_vWorkers;
            memptr_t        m_pMemPool = nullptr;
            TaskQueue		m_qTasks;
            Threads::SyncObject m_TaskSyncObj;
            size_t          m_memPoolSize = 0;
            size_t          m_threadMemSize = 0;
    };

} // VKE
