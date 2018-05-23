#pragma once

#include "Core/Threads/Common.h"
#include "Core/Threads/ITask.h"
#include "Core/Utils/TCDynamicArray.h"

namespace VKE
{
    class CThreadPool;
    class CThreadWorker
    {
        public:

            using WorkFunc = TaskFunction;
            using WorkFunc2 = TaskFunction2;

            struct SWorkerData
            {
                memptr_t        pData;
                STaskResult*    pResult;
                size_t          dataSize;
                uint16_t        handle;
                WorkFunc        Func;
            };

            struct SConstantWorkerData
            {
                void*           pData;
                WorkFunc2       Func;
            };

            using WorkQueue = std::deque< SWorkerData* >;
            using WorkVec = std::vector< SConstantWorkerData >;
            using WorkDataPool = std::vector< SWorkerData >;
            using Stack = std::vector< uint16_t >;
            //using TaskVec = std::vector< Threads::ITask* >;
            using TaskVec = Utils::TCDynamicArray< Threads::ITask* >;
            using BoolVec = Utils::TCDynamicArray< bool >;
            using StateVec = Utils::TCDynamicArray< TaskState >;

            struct SConstantTaskData
            {
                StateVec            vStates;
                TaskVec             vpTasks;
                Threads::SyncObject SyncObj;
            };

        public:

            CThreadWorker();
            CThreadWorker(const CThreadWorker&);
            CThreadWorker(CThreadWorker&&);
            virtual     ~CThreadWorker();
                
            void operator()() { Start(); }
            void operator=(const CThreadWorker&);
            void operator=(CThreadWorker&&);

            Result Create(CThreadPool* pPool, uint32_t id, uint16_t taskMemSize, uint16_t taskCount, memptr_t pMemPool);

            void Start();
            void Stop();
            void Pause(bool bPause);
            bool IsPaused();
            void WaitForStop();

            Result AddWork(const WorkFunc& Func, const STaskParams& Params, int32_t threadId);
            Result AddConstantWork(const WorkFunc2& Func, void* pPtr);

            Result AddConstantTask(Threads::ITask* pTask, TaskState state);

            Result AddTask(Threads::ITask* pTask);

            uint32_t GetWorkCount() const { return static_cast<uint32_t>(m_qWorks.size()); }

            std::thread::id GetThreadID() const { return m_ThreadId; }

            SWorkerData* GetFreeData();
            void FreeData(SWorkerData* pData);

            uint32_t GetConstantTaskCount() const { return m_vConstantTasks.GetCount(); }

        protected:

            void	_StealTask();
            void    _RunConstantTasks();

        protected:

            WorkVec         m_vConstantWorks;
            TaskVec			m_vConstantTasks;
            SConstantTaskData   m_ConstantTasks;
            WorkQueue       m_qWorks;
            TaskQueue       m_qTasks;
            WorkDataPool    m_vDataPool;
            Stack           m_vFreeIds;
            Threads::SyncObject m_TaskSyncObj;
            Threads::SyncObject m_ConstantTaskSyncObj;
            memptr_t        m_pMemPool = nullptr;
            CThreadPool*	m_pPool = nullptr;
            size_t          m_memPoolSize = 0;
            size_t          m_taskMemSize = 0;
            uint32_t        m_id;
            std::thread::id	m_ThreadId = std::this_thread::get_id();
            uint32_t        m_totalTaskWeight = 0;
            bool            m_bNeedStop = false;
            bool            m_bPaused = false;
            bool            m_bIsEnd = false;
    };
} // VKE
