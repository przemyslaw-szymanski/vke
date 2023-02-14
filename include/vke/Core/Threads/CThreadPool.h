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
        class CPageAllocator;

        struct SThreadPoolTask
        {
            TaskFunction Func;
            uint32_t hMemory = INVALID_HANDLE;
            VKE_DEBUG_TEXT;
        };

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

            using TaskQueue2 = std::deque< SThreadPoolTask >;

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

            template<class T>
            Result AddTask( THREAD_USAGE usage, Threads::TSSimpleTask< T >& Task );

            std::thread::id AddConstantTask( THREAD_USAGES usages, THREAD_TYPE_INDEX index, Threads::ITask* pTask,
                                             TaskState state );
            //std::thread::id AddConstantTask( WorkerID wokerId, void* pData, TaskFunction2&& Func );
            std::thread::id AddConstantTask( WorkerID workerId, Threads::ITask* pTask, TaskState state );
            std::thread::id AddConstantTask( NativeThreadID threadId, Threads::ITask* pTask, TaskState state );
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
            WorkerID _CalcWorkerID( THREAD_USAGES usage, THREAD_TYPE_INDEX index ) const;

            // Threads::ITask* _PopTask(uint8_t priority, uint8_t weight);
            Threads::ITask* _PopTask(THREAD_USAGE usage);
            bool _PopTask2( THREAD_USAGE usage, SThreadPoolTask* pTask );
            WorkerID _FindThread( NativeThreadID id );

            TaskResult _RunTask( THREAD_USAGE usage, SThreadPoolTask& );
            /// <summary>
            /// Adds simple task without task data allocation
            /// </summary>
            /// <param name="usage"></param>
            /// <param name=""></param>
            void _AddTask( THREAD_USAGE usage, SThreadPoolTask& );

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
            CPageAllocator* m_pMemAllocator;
            /// <summary>
            /// Map indexed by THREAD_USAGE single bit with values of all threads
            /// where such bit is present.
            /// </summary>
            UintVec m_avThreadIds[ ThreadUsages::_MAX_COUNT ];
            uint32_t m_aThreadTaskCounts[ ThreadUsages::_MAX_COUNT ] = { 0 };
            SyncObject m_aThreadUsageSyncObjs[ ThreadUsages::_MAX_COUNT ];
            TaskQueue m_aqTasks[ ThreadUsages::_MAX_COUNT ];
            TaskQueue2 m_aqTasks2[ ThreadUsages::_MAX_COUNT ];
        };

        template<class T>
        Result CThreadPool::AddTask(THREAD_USAGE usage, Threads::TSSimpleTask<T>& Task)
        {
            SThreadPoolTask InternalTask;
            InternalTask.Func = Task.Task;
            InternalTask.SetDebugText( Task.GetDebugText() );
            VKE_ASSERT( Task.IsDebugTextSet() );
            if( Task.pData != nullptr )
            {
                Threads::ScopedLock l( m_aThreadUsageSyncObjs[ usage ] );
                uint32_t dataSize = sizeof( T ) * Task.dataElementCount;
                //InternalTask.hMemory = m_pMemAllocator->Allocate( dataSize );
                //T* pPtr = static_cast<T*>( m_pMemAllocator->GetPtr( InternalTask.hMemory ) );
                void* pPtr = ( _AllocateTaskDataMemory( dataSize, &InternalTask.hMemory ) );
                for (uint32_t i = 0; i < Task.dataElementCount; ++i)
                {
                    uint8_t* pTmpPtr = ( uint8_t* )pPtr + sizeof(T)*i;
                    // Do the placement new to default initialize memory
                    // It is needed for any smart ptr like functionality
                    // where a current data must be deleted before assignment
                    /// TODO: optimize this
                    T* pTmp = new( pTmpPtr ) T{};
                    *pTmp = Task.pData[i];
                }
            }
            _AddTask( usage, InternalTask );
            return VKE_OK;
        }
    } // namespace Threads
} // VKE
