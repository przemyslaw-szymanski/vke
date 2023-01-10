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

    namespace Threads
    {
        class CThreadPool
        {
            friend class CThreadWorker;

          public:
            using ThreadVec = std::vector<std::thread>;
            using PtrStack = std::stack<uint8_t*>;
            using UintVec = Utils::TCDynamicArray<uint32_t>;
            using ThreadIdVec = std::vector<std::thread::id>;
            using NativeThreadID = SNativeThreadID;
            using WorkerID = SThreadWorkerID;
            using TaskGroupVec = Utils::TCDynamicArray<Threads::CTaskGroup*, 64>;
            using WorkerVec = Utils::TCDynamicArray<CThreadWorker, 16>;
            using TaskQueueArray = Utils::TCDynamicArray<TaskQueue>;
            using TaskIndexMap = vke_hash_map<THREAD_USAGES, WorkerID>;
            using TaskQueueMap = vke_hash_map< THREAD_USAGES, TaskQueue >;
            static const size_t PAGE_SIZE = 1024;
            struct SCalcWorkerIDDesc
            {
                WorkerID threadId;
                uint8_t taskWeight = 0;
                uint8_t taskPriority = 0;
                bool isConstantTask = false;
            };

          public:
            CThreadPool();
            virtual ~CThreadPool();
            Result Create( const SThreadPoolInfo& Info );
            void Destroy();
            //Result AddTask( NativeThreadID threadId, Threads::ITask* pTask );
            //Result AddTask( WorkerID wokerId, const STaskParams& Params, TaskFunction&& Func );
            //Result AddTask( WorkerID wokerId, Threads::ITask* pTask );
            Result AddTask( THREAD_USAGE usage, THREAD_TYPE_INDEX index, Threads::ITask* pTask );
            Result AddTask( THREAD_USAGE usage, Threads::ITask* pTask );
            Result AddConstantTask( THREAD_USAGE usage, THREAD_TYPE_INDEX index, Threads::ITask* pTask, TaskState state );
            Result AddConstantTask( WorkerID wokerId, void* pData, TaskFunction2&& Func );
            Result AddConstantTask( WorkerID workerId, Threads::ITask* pTask, TaskState state );
            Result AddConstantTask( NativeThreadID threadId, Threads::ITask* pTask, TaskState state );
            //Result AddConstantTaskGroup( Threads::CTaskGroup* pGroup );
            // Result      AddTaskGroup(Threads::CTaskGroup* pGroup);
            size_t GetWorkerCount() const
            {
                return m_vThreads.size();
            }
            const NativeThreadID GetOSThreadId( uint32_t id )
            {
                return NativeThreadID( Platform::Thread::GetID( m_vThreads[ id ].native_handle() ) );
            }
            int32_t GetThisThreadID() const;

          protected:
            WorkerID _CalcWorkerID( const SCalcWorkerIDDesc& Desc ) const;

            // Threads::ITask* _PopTask(uint8_t priority, uint8_t weight);
            Threads::ITask* _PopTask(THREAD_USAGE usage);
            WorkerID _FindThread( NativeThreadID id );

          protected:
            SThreadPoolInfo m_Desc;
            ThreadVec m_vThreads;
            TaskGroupVec m_vpTaskGroups;
            WorkerVec m_vWorkers;
            memptr_t m_pMemPool = nullptr;
            /*TaskQueueMap m_mqTasks;
            TaskIndexMap m_mTaskIds;
            TaskQueueArray m_vqTasks;*/
            //Threads::SyncObject m_TaskSyncObj;
            // TaskQueue m_aaqTasks[ 3 ][ 3 ];
            // Threads::SyncObject m_aaTaskSyncObjs[ 3 ][ 3 ];
            size_t m_memPoolSize = 0;
            size_t m_threadMemSize = 0;
            /// <summary>
            /// Map indexed by THREAD_USAGE single bit with values of all threads
            /// where such bit is present.
            /// </summary>
            UintVec m_avThreadIds[ ThreadUsages::_MAX_COUNT ];
            uint32_t m_aThreadTaskCounts[ ThreadUsages::_MAX_COUNT ] = { 0 };
            SyncObject m_aThreadUsageSyncObjs[ ThreadUsages::_MAX_COUNT ];
            TaskQueue m_aqTasks[ ThreadUsages::_MAX_COUNT ];
        };
    } // namespace Threads
} // VKE
