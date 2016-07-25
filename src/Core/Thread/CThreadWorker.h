#pragma once

#include "Thread/Common.h"

namespace VKE
{
    class CThreadWorker
    {
        public:

            using WorkFunc = TaskFunction;

            struct SWorkerData
            {
                memptr_t        pData;
                STaskResult*    pResult;
                size_t          dataSize;
                uint16_t        handle;
                WorkFunc        Func;
            };

            using WorkQueue = std::deque< SWorkerData* >;
            using WorkDataPool = std::vector< SWorkerData >;
            using Stack = std::vector< uint16_t >;

        public:

            CThreadWorker();
            CThreadWorker(const CThreadWorker&);
            CThreadWorker(CThreadWorker&&);
            virtual     ~CThreadWorker();
                
            void operator()() { Start(); }
            void operator=(const CThreadWorker&);
            void operator=(CThreadWorker&&);

            Result Create(uint16_t taskMemSize, uint16_t taskCount, memptr_t pMemPool);

            void Start();
            void Stop();
            void Pause(bool bPause);
            bool IsPaused();
            void WaitForStop();

            Result AddWork(const WorkFunc& Func, const STaskParams& Params);

            size_t GetWorkCount() const { return m_qWorks.size(); }

            SWorkerData* GetFreeData();
            void FreeData(SWorkerData* pData);

        protected:

            WorkQueue       m_qWorks;
            WorkDataPool    m_vDataPool;
            Stack           m_vFreeIds;
            std::mutex      m_Mutex;
            memptr_t        m_pMemPool = nullptr;
            size_t          m_memPoolSize = 0;
            size_t          m_taskMemSize = 0;
            bool            m_bNeedStop = false;
            bool            m_bPaused = false;
            bool            m_bIsEnd = false;
    };
} // VKE
