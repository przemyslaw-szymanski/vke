#pragma once

#include "Core/Threads/Common.h"
#include "Core/Threads/ITask.h"
#include "Core/Utils/TCDynamicArray.h"
#include "Core/Utils/CTimer.h"

namespace VKE
{
    namespace Threads
    {
        class CThreadPool;
        struct SThreadPoolTask;

        class CThreadWorker
        {
          public:
            using WorkFunc = TaskFunction;

            struct SWorkerData
            {
                memptr_t pData;
                STaskResult* pResult;
                size_t dataSize;
                uint16_t handle;
                uint8_t weight;
                uint8_t priority;
                WorkFunc Func;
            };

            using WorkQueue = std::deque<SWorkerData*>;

            using WorkDataPool = std::vector<SWorkerData>;
            using Stack = std::vector<uint16_t>;
            // using TaskVec = std::vector< Threads::ITask* >;
            using TaskVec = Utils::TCDynamicArray<Threads::ITask*>;
            using BoolVec = Utils::TCDynamicArray<bool>;
            using StateVec = Utils::TCDynamicArray<TaskState>;
            using UsageVec = Utils::TCDynamicArray<THREAD_USAGE>;
            struct SConstantTaskData
            {
                StateVec vStates;
                TaskVec vpTasks;
                Threads::SyncObject SyncObj;
            };

            struct SDesc
            {
                SThreadDesc Desc;
                memptr_t pMemPool;
                uint32_t id;
                uint16_t taskMemSize;
                uint16_t taskCount;
                THREAD_USAGES usages;
            };

          public:
            CThreadWorker();
            CThreadWorker( const CThreadWorker& );
            CThreadWorker( CThreadWorker&& );
            virtual ~CThreadWorker();
            void operator()()
            {
                Start();
            }
            void operator=( const CThreadWorker& );
            void operator=( CThreadWorker&& );
            Result Create( CThreadPool* pPool, const SDesc& Desc );
            void Start();
            void Stop();
            void Pause( bool bPause );
            bool IsPaused();
            void WaitForStop();
            Result AddWork( const WorkFunc& Func, const STaskParams& Params, uint8_t weight, uint8_t priority,
                            int32_t threadId );
            //std::thread::id AddConstantWork( const WorkFunc2& Func, void* pPtr );
            std::thread::id AddConstantTask( Threads::ITask* pTask, TaskState state );
            std::thread::id AddTask( Threads::ITask* pTask );
            uint32_t GetWorkCount() const
            {
                return static_cast<uint32_t>( m_qWorks.size() );
            }
            std::thread::id GetThreadID() const
            {
                return m_ThreadId;
            }
            SWorkerData* GetFreeData();
            void FreeData( SWorkerData* pData );
            uint32_t GetConstantTaskCount() const
            {
                return m_vConstantTasks.GetCount();
            }
            uint32_t GetTotalTaskWeight() const
            {
                return m_totalTaskWeight;
            }

          protected:
            Threads::ITask* _StealTask();
            THREAD_USAGE _StealTask2( SThreadPoolTask* );
            uint32_t _RunConstantTasks();
            std::pair<uint8_t, uint8_t> _CalcStealTaskPriorityAndWeightIndices( uint8_t level );

          protected:
            SDesc m_Desc;
            bool m_bNeedStop = false;
            bool m_bPaused = false;
            Threads::ITask::FlagBits m_Flags = Threads::TaskFlags::DEFAULT;
            Utils::CTimer m_TotalTimer;
            Utils::CTimer m_ConstantTaskTimer;
            //WorkVec m_vConstantWorks;
            TaskVec m_vConstantTasks;
            SConstantTaskData m_ConstantTasks;
            WorkQueue m_qWorks;
            TaskQueue m_qTasks;
            WorkDataPool m_vDataPool;
            Stack m_vFreeIds;
            Threads::SyncObject m_TaskSyncObj;
            Threads::SyncObject m_ConstantTaskSyncObj;

            UsageVec m_vUsages;
            CThreadPool* m_pPool = nullptr;
            uint32_t m_memPoolSize = 0;
            std::thread::id m_ThreadId = std::this_thread::get_id();
            uint32_t m_totalTaskWeight = 0;
            uint32_t m_totalTaskPriority = 0;
            float m_totalTimeUS = 0.0f;
            float m_totalContantTaskTimeUS = 0.0f;
            bool m_bIsEnd = false;
        };
    } // namespace Threads
} // VKE
