#pragma once

#include "Core/Thread/Common.h"
#include "Core/Thread/ITask.h"

namespace VKE
{
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
            using TaskQueue = std::deque< Thread::ITask* >;

        public:

            CThreadWorker();
            CThreadWorker(const CThreadWorker&);
            CThreadWorker(CThreadWorker&&);
            virtual     ~CThreadWorker();
                
            void operator()() { Start(); }
            void operator=(const CThreadWorker&);
            void operator=(CThreadWorker&&);

            Result Create(uint32_t id, uint16_t taskMemSize, uint16_t taskCount, memptr_t pMemPool);

            void Start();
            void Stop();
            void Pause(bool bPause);
            bool IsPaused();
            void WaitForStop();

            Result AddWork(const WorkFunc& Func, const STaskParams& Params, int32_t threadId);
            Result AddConstantWork(const WorkFunc2& Func, void* pPtr);

            Result AddTask(Thread::ITask* pTask);

            size_t GetWorkCount() const { return m_qWorks.size(); }

            SWorkerData* GetFreeData();
            void FreeData(SWorkerData* pData);

        protected:

            WorkVec         m_vConstantWorks;
            WorkQueue       m_qWorks;
            TaskQueue       m_qTasks;
            WorkDataPool    m_vDataPool;
            Stack           m_vFreeIds;
            std::mutex      m_Mutex;
            memptr_t        m_pMemPool = nullptr;
            size_t          m_memPoolSize = 0;
            size_t          m_taskMemSize = 0;
            uint32_t        m_id;
            bool            m_bNeedStop = false;
            bool            m_bPaused = false;
            bool            m_bIsEnd = false;
    };
} // VKE
