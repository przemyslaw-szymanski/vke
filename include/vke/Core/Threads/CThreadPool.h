#pragma once

#include "Core/Threads/Common.h"
#include "Core/Threads/ITask.h"
#include "Core/Utils/TCDynamicArray.h"
#include "Core/Threads/CThreadWorker.h"
#include "Core/Utils/STLUtils.h"

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
        explicit TSThreadID( T idx) : id{ idx } {}
    };

    using SThreadWorkerID = TSThreadID< int32_t, -1 >;
    using SNativeThreadID = TSThreadID< Platform::Thread::ID, 0 >;

    

    namespace Threads
    {
        class CPageAllocator;

        struct SThreadPoolTask
        {
            TaskFunction Func;
            uint32_t hMemory = INVALID_HANDLE;
            ThreadUsages Usages = ThreadUsageBits::GENERAL;
            VKE_DEBUG_TEXT;
        };

        class CThreadPool
        {
            friend class CThreadWorker;

          public:
            using InternalTaskQueue = std::deque< SThreadPoolTask >;
            using ThreadVec = std::vector<std::thread>;
            using PtrStack = std::stack<uint8_t*>;
            using UintVec = Utils::TCDynamicArray<uint32_t>;
            using ThreadIdVec = std::vector<std::thread::id>;
            using NativeThreadID = SNativeThreadID;
            using WorkerID = SThreadWorkerID;
            using TaskGroupVec = Utils::TCDynamicArray<Threads::CTaskGroup*, 64>;
            using WorkerVec = Utils::TCDynamicArray<CThreadWorker, 16>;
            using TaskQueueArray = Utils::TCDynamicArray<InternalTaskQueue>;
            using TaskIndexMap = vke_hash_map<ThreadUsages, WorkerID>;
            using TaskQueueMap = vke_hash_map< ThreadUsages, TaskQueue >;
            using SyncObjectVec = Utils::TCDynamicArray< SyncObject >;
            using ThreadUsagesVec = Utils::TCDynamicArray< ThreadUsages >;
            using InternalTaskVec = Utils::TCDynamicArray< SThreadPoolTask >;
            static const size_t PAGE_SIZE = 1024;
            struct SCalcWorkerIDDesc
            {
                WorkerID threadId;
                uint8_t taskWeight = 0;
                uint8_t taskPriority = 0;
                bool isConstantTask = false;
            };

            using TaskQueue2 = std::deque< SThreadPoolTask >;

            using CustomCopyCallback = std::function<void(void*, const void*)>;

          public:
            CThreadPool();
            virtual ~CThreadPool();
            Result Create( const SThreadPoolInfo& Info );
            void Destroy();
            //Result AddTask( NativeThreadID threadId, Threads::ITask* pTask );
            //Result AddTask( WorkerID wokerId, const STaskParams& Params, TaskFunction&& Func );
            //Result AddTask( WorkerID wokerId, Threads::ITask* pTask );
            //Result AddTask( THREAD_USAGES usages, THREAD_TYPE_INDEX index, Threads::ITask* pTask );
            //Result AddTask( THREAD_USAGES usages, Threads::ITask* pTask );

            /*template<class T>
            Result AddTask( THREAD_USAGES usages, THREAD_TYPE_INDEX index,
                Threads::TSSimpleTask< T >& Task );

            Result AddTask( THREAD_USAGES usages, THREAD_TYPE_INDEX index,
                Threads::TSSimpleTask<void>& Task, CustomCopyCallback&& Copy );*/

            template<typename ... ArgsT>
            Result AddTask( ThreadUsages usages, cstr_t pDbgText,
                TaskFunction& Func, ArgsT&&... );
            template<typename... ArgsT>
            Result AddWorkerThreadTask( WorkerID workerIdx, ThreadUsages usages, cstr_t pDbgText, TaskFunction& Func,
                            ArgsT&&... );

            //std::thread::id AddConstantTask( THREAD_USAGES usages, THREAD_TYPE_INDEX index, Threads::ITask* pTask,TaskState state );
            //std::thread::id AddConstantTask( WorkerID wokerId, void* pData, TaskFunction2&& Func );
            //std::thread::id AddConstantTask( WorkerID workerId, Threads::ITask* pTask, TaskState state );
            //std::thread::id AddConstantTask( NativeThreadID threadId, Threads::ITask* pTask, TaskState state );
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
            WorkerID GetThisThreadID() const;

          protected:
            WorkerID _CalcQueueIndex( const SCalcWorkerIDDesc& Desc ) const;
            uint32_t _CalcQueueIndex( ThreadUsages Usags ) const;

            // Threads::ITask* _PopTask(uint8_t priority, uint8_t weight);
            //Threads::ITask* _PopTask(THREAD_USAGES usages);
            bool _PopTask( ThreadUsages usages, SThreadPoolTask* pTask );
            bool _PopTaskFromQueue( uint32_t workerIdx, SThreadPoolTask* pTask );
            WorkerID _FindThread( NativeThreadID id );

            TASK_RESULT _RunTask( ThreadUsages usages, SThreadPoolTask& Task )
            {
                return _RunTaskForWorker( _CalcQueueIndex( usages ), Task );
            }
            TASK_RESULT _RunTaskForWorker( uint32_t workerIdx, SThreadPoolTask& );
            TASK_RESULT _RunTaskForWorker( uint32_t workerIdx, ThreadUsages usages );
            TASK_RESULT _RunTask( const SThreadPoolTask& );
            /// <summary>
            /// Adds simple task without task data allocation
            /// </summary>
            /// <param name="usage"></param>
            /// <param name=""></param>
            void _AddTask( ThreadUsages usages, SThreadPoolTask& Task )
            {
                return _AddTaskToQueue( _CalcQueueIndex( usages ), Task );
            }
            void _AddTaskToQueue( uint32_t workerIdx, SThreadPoolTask& );

            void* _AllocateTaskDataMemory( uint32_t size, uint32_t* pHandleOut );

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
            SyncObject m_AllocatorSyncObj;
            CPageAllocator* m_pMemAllocator;
            /// <summary>
            /// Map indexed by THREAD_USAGE single bit with values of all threads
            /// where such bit is present.
            /// </summary>
           
            /// <summary>
            /// For last queue, which corresponds to ANY usage, this array contains only usages
            /// correspond to m_vqTasks[ m_anyTaskQueueIdex ]
            /// </summary>
            SyncObject m_TasksSyncObj;
            ThreadUsagesVec m_vTasksUsages;
            InternalTaskVec m_vTasks; // all tasks not allocated to a specific worker yet
            uint32_t m_anyThreadQueueIdx;
            SyncObjectVec m_vThreadUsageSyncObjs;
            TaskQueueArray m_vqTasks;
        };

        template<typename ... ArgsT>
        Result CThreadPool::AddTask( ThreadUsages TaskUsages, cstr_t pDbgText,
            TaskFunction& Func, ArgsT&&... args )
        {
            SThreadPoolTask InternalTask;
            InternalTask.Func = std::move( Func );
            InternalTask.SetDebugText( pDbgText );
            InternalTask.Usages = TaskUsages;
            VKE_ASSERT( InternalTask.IsDebugTextSet() );
            if constexpr( Utils::GetArgumentCount<ArgsT...>() > 0 )
            {
                constexpr auto totalSize = Utils::GetArgumentTotalSize<ArgsT...>();
                void* pPtr;
                {
                    Threads::ScopedLock l( m_AllocatorSyncObj );
                    pPtr = ( _AllocateTaskDataMemory( totalSize, &InternalTask.hMemory ) );
                }
                void* pLastPtr = pPtr;
                {
                    pLastPtr = Utils::StoreArguments( pPtr, std::forward<ArgsT>( args )... );
                }
                VKE_ASSERT( ( uint8_t* )pLastPtr <= ( uint8_t* )pPtr + totalSize );
            }
            _AddTaskToQueue( m_anyThreadQueueIdx, InternalTask );
            return VKE_OK;
        }

        template<typename ... ArgsT>
        Result CThreadPool::AddWorkerThreadTask( WorkerID workerIdx, ThreadUsages usages, cstr_t pDbgText,
            TaskFunction& Func, ArgsT&&... args)
        {
            SThreadPoolTask InternalTask;
            InternalTask.Func = std::move( Func );
            InternalTask.SetDebugText( pDbgText );
            InternalTask.Usages = usages;
            VKE_ASSERT( InternalTask.IsDebugTextSet() );
            if constexpr( Utils::GetArgumentCount<ArgsT...>() > 0 )
            {
                constexpr auto totalSize = Utils::GetArgumentTotalSize<ArgsT...>();
                void* pPtr;
                {
                    Threads::ScopedLock l( m_AllocatorSyncObj );
                    pPtr = ( _AllocateTaskDataMemory( totalSize, &InternalTask.hMemory ) );
                }
                void* pLastPtr = pPtr;
                {
                    pLastPtr = Utils::StoreArguments( pPtr, std::forward<ArgsT>( args )... );
                }
                VKE_ASSERT( ( uint8_t* )pLastPtr <= ( uint8_t* )pPtr + totalSize );
            }
            _AddTaskToQueue( workerIdx.id, InternalTask );
            return VKE_OK;
        }

    } // namespace Threads
} // VKE
