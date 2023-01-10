#include "Core/Threads/CThreadPool.h"
#include "Core/Utils/CLogger.h"
#include "Core/Threads/CTaskGroup.h"

namespace VKE
{
    
    namespace Threads
    {
        CThreadPool::CThreadPool()
        {
        }
        CThreadPool::~CThreadPool()
        {
            Destroy();
        }
        void CThreadPool::Destroy()
        {
            for( size_t i = 0; i < m_vThreads.size(); ++i )
            {
                m_vWorkers[ i ].Stop();
            }
            for( size_t i = 0; i < m_vThreads.size(); ++i )
            {
                // m_vWorkers[ i ].WaitForStop();
                m_vThreads[ i ].join();
            }
            m_vThreads.clear();
            m_vWorkers.ClearFull();
            VKE_DELETE_ARRAY( m_pMemPool );
        }
        Result CThreadPool::Create( const SThreadPoolInfo& Info )
        {
            m_Desc = Info;
            bool res = false;
            Result ret = VKE_OK;

            m_Desc.vThreadDescs.Resize( Info.threadCount );
            if( Info.threadCount == Constants::Threads::COUNT_OPTIMAL )
            {
                res = m_Desc.vThreadDescs.Resize( Platform::Thread::GetMaxConcurrentThreadCount() - 1 );
            }
            {
                if( Info.vThreadDescs.GetCount() > 0 )
                {
                    for( uint32_t i = 0; i < Info.vThreadDescs.GetCount(); ++i )
                    {
                        uint32_t threadIdx = i % m_Desc.vThreadDescs.GetCount();
                        m_Desc.vThreadDescs[ threadIdx ].usages = Info.vThreadDescs[ i ].usages;
                    }
                }
                else
                {
                    static const SThreadDesc aDescs[] =
                    {
                        ThreadUsageBits::GRAPHICS | ThreadUsageBits::LOGIC | ThreadUsageBits::PHYSICS,
                        ThreadUsageBits::FILE_IO | ThreadUsageBits::NETWORK | ThreadUsageBits::GENERAL,
                        ThreadUsageBits::RESOURCE_PREPARE | ThreadUsageBits::COMPILE,
                        ThreadUsageBits::NETWORK,
                        ThreadUsageBits::SOUND,
                        ThreadUsageBits::GRAPHICS | ThreadUsageBits::LOGIC | ThreadUsageBits::PHYSICS,
                        ThreadUsageBits::FILE_IO, ThreadUsageBits::RESOURCE_PREPARE | ThreadUsageBits::COMPILE,
                        ThreadUsageBits::GRAPHICS | ThreadUsageBits::LOGIC | ThreadUsageBits::PHYSICS,
                    };
                    uint32_t threadCount = sizeof( aDescs ) / sizeof( aDescs[ 0 ] );
                    m_Desc.vThreadDescs.Resize( threadCount );
                    for( uint32_t i = 0; i < threadCount; ++i )
                    {
                        uint32_t threadIdx = i;//% m_Desc.vThreadDescs.GetCount();
                        m_Desc.vThreadDescs[ threadIdx ].usages = aDescs[i].usages;
                    }
                }
            }
            if( res )
            {
                res = m_vWorkers.Resize( m_Desc.vThreadDescs.GetCount() );
                if( !res )
                {
                    VKE_LOG_ERR_RET( VKE_ENOMEMORY, "No memory for thread workers" );
                }
                m_threadMemSize = m_Desc.taskMemSize * m_Desc.maxTaskCount;
                m_memPoolSize = m_threadMemSize * m_Desc.vThreadDescs.GetCount();
                m_pMemPool = VKE_NEW mem_t[ m_memPoolSize ];
                if( !m_pMemPool )
                {
                    return VKE_ENOMEMORY;
                }
                m_vThreads.resize( m_Desc.vThreadDescs.GetCount() );
                for( uint32_t i = 0; i < m_vThreads.size(); ++i )
                {
                    const auto& ThreadDesc = m_Desc.vThreadDescs[ i ];
                    memptr_t pMem = m_pMemPool + i * m_threadMemSize;
                    CThreadWorker::SDesc Desc;
                    Desc.Desc = ThreadDesc;
                    Desc.id = ( uint32_t )i;
                    Desc.taskMemSize = m_Desc.taskMemSize;
                    Desc.taskCount = m_Desc.maxTaskCount;
                    Desc.pMemPool = pMem;
                    Desc.usages = ThreadDesc.usages;
                    if( VKE_SUCCEEDED( m_vWorkers[ i ].Create( this, Desc ) ) )
                    {
                        m_vThreads[ i ] = std::thread( std::ref( m_vWorkers[ i ] ) );
                    }

                    for(uint32_t bit = 0; bit < ThreadUsages::_MAX_COUNT; ++bit)
                    {
                        // Calc which bit == 1
                        auto num = Desc.usages >> bit;
                        if( (num & 1) == 1 )
                        {
                            m_avThreadIds[ bit ].PushBack( i );
                        }
                    }
                }
                
            }
            else
            {
                VKE_LOG_ERR( "No memory for array allocation." );
                ret = VKE_ENOMEMORY;
            }
            return ret;
        }
        SThreadWorkerID CThreadPool::_CalcWorkerID( const SCalcWorkerIDDesc& Desc ) const
        {
            SThreadWorkerID threadId;
            if( Desc.threadId.id < 0 )
            {
                size_t minTaskCount = std::numeric_limits<size_t>::max();
                size_t minTaskCountId = minTaskCount;
                uint32_t taskCount = 0;
                size_t minWeight = minTaskCountId;
                size_t minWeightId = minWeight;
                // Calculate min of tasks scheduled in threads
                for( uint32_t i = 0; i < m_vThreads.size(); ++i )
                {
                    const auto& Worker = m_vWorkers[ i ];
                    if( Desc.isConstantTask )
                    {
                        taskCount = Worker.GetConstantTaskCount();
                    }
                    else
                    {
                        taskCount = Worker.GetWorkCount();
                    }
                    if( taskCount < minTaskCount )
                    {
                        minTaskCount = taskCount;
                        minTaskCountId = i;
                    }
                    if( minWeight > Worker.GetTotalTaskWeight() )
                    {
                        minWeight = Worker.GetTotalTaskWeight();
                        minWeightId = i;
                    }
                }
                // Weight 1 is default minimal value
                if( Desc.taskWeight > 1 )
                {
                    threadId.id = static_cast<uint32_t>( minWeightId );
                }
                else
                {
                    threadId.id = static_cast<uint32_t>( minTaskCountId );
                }
            }
            return threadId;
        }

//        Result CThreadPool::AddTask( WorkerID threadId, const STaskParams& Params, TaskFunction&& Func )
//        {
//            if( GetWorkerCount() )
//            {
//                SCalcWorkerIDDesc Desc;
//                Desc.threadId = threadId;
//                Desc.isConstantTask = false;
//                threadId = _CalcWorkerID( Desc );
//                return m_vWorkers[ threadId.id ].AddWork( Func, Params, 1, 1, threadId.id );
//            }
//            return VKE_FAIL;
//        }
//        Result CThreadPool::AddTask( WorkerID threadId, Threads::ITask* pTask )
//        {
//            // if( GetWorkerCount() )
//            {
//                // threadId = _CalcWorkerID( threadId );
//                if( threadId.id < 0 )
//                {
//                    return AddTask( ThreadUsages::ANY, pTask );
//                }
//                VKE_ASSERT2( ( size_t )threadId.id < GetWorkerCount(), "Thread id out of bounds." );
//#if VKE_DEBUG
//                VKE_ASSERT2( pTask->m_strDbgName != "", "" );
//#endif
//                return m_vWorkers[ threadId.id ].AddTask( pTask );
//            }
//            // return VKE_FAIL;
//        }
        Result CThreadPool::AddTask( THREAD_USAGE usage, Threads::ITask* pTask )
        {
            // if( GetWorkerCount() )
            {
                // threadId = _CalcWorkerID( threadId );
                {
                    Threads::ScopedLock l( m_aThreadUsageSyncObjs[usage] );
#if VKE_DEBUG
                    VKE_ASSERT2( pTask->m_strDbgName != "", "" );
#endif
                    m_aqTasks[ usage ].push_back( pTask );
                    return VKE_OK;
                }
            }
            // return VKE_FAIL;
        }

        Result CThreadPool::AddTask(THREAD_USAGE usage, THREAD_TYPE_INDEX index, Threads::ITask* pTask)
        {
            return VKE_OK;
        }

        CThreadPool::WorkerID CThreadPool::_FindThread( NativeThreadID id )
        {
            for( decltype( m_Desc.vThreadDescs.GetCount() ) i = 0; i < m_Desc.vThreadDescs.GetCount(); ++i )
            {
                NativeThreadID ID = NativeThreadID( Platform::Thread::GetID( m_vThreads[ i ].native_handle() ) );
                if( ID.id == id.id )
                {
                    return WorkerID( i );
                }
            }
            return WorkerID( -1 );
        }
//        Result CThreadPool::AddTask( NativeThreadID threadId, Threads::ITask* pTask )
//        {
//            // Find thread
//            auto id = _FindThread( threadId );
//            VKE_ASSERT2( id.id >= 0, "Wrong threadId." );
//#if VKE_DEBUG
//            VKE_ASSERT2( pTask->m_strDbgName != "", "" );
//#endif
//            return m_vWorkers[ id.id ].AddTask( pTask );
//        }
//        Result CThreadPool::AddConstantTask( WorkerID workerId, void* pData, TaskFunction2&& Func )
//        {
//            if( GetWorkerCount() )
//            {
//                SCalcWorkerIDDesc Desc;
//                Desc.threadId = workerId;
//                Desc.isConstantTask = true;
//                workerId = _CalcWorkerID( Desc );
//                return m_vWorkers[ workerId.id ].AddConstantWork( Func, pData );
//            }
//            return VKE_FAIL;
//        }
        Result CThreadPool::AddConstantTask( WorkerID workerId, Threads::ITask* pTask, TaskState state )
        {
            if( GetWorkerCount() )
            {
                SCalcWorkerIDDesc Desc;
                Desc.threadId = workerId;
                Desc.isConstantTask = true;
                workerId = _CalcWorkerID( Desc );
#if VKE_DEBUG
                VKE_ASSERT2( pTask->m_strDbgName != "", "" );
#endif
                return m_vWorkers[ workerId.id ].AddConstantTask( pTask, state );
            }
            return VKE_FAIL;
        }
        Result CThreadPool::AddConstantTask( THREAD_USAGE usage, THREAD_TYPE_INDEX index,
            Threads::ITask* pTask, TaskState state )
        {
            return m_vWorkers[ usage ].AddConstantTask( pTask, state );
        }
        Result CThreadPool::AddConstantTask( NativeThreadID threadId, Threads::ITask* pTask, TaskState state )
        {
            auto id = _FindThread( threadId );
            return m_vWorkers[ id.id ].AddConstantTask( pTask, state );
        }
        int32_t CThreadPool::GetThisThreadID() const
        {
            const auto& threadId = std::this_thread::get_id();
            for( uint32_t i = 0; i < m_vThreads.size(); ++i )
            {
                if( m_vThreads[ i ].get_id() == threadId )
                {
                    return i;
                }
            }
            return -1;
        }
        /*Threads::ITask* CThreadPool::_PopTask( uint8_t priority, uint8_t weight )
        {
            VKE_ASSERT2( priority < 3 && weight < 3, "" );
            Threads::ITask* pTask = nullptr;
            auto& SyncObj = m_aaTaskSyncObjs[ priority ][ weight ];
            {
                auto& qTasks = m_aaqTasks[ priority ][ weight ];
                Threads::ScopedLock l( SyncObj );
                if( !qTasks.empty() )
                {
                    pTask = qTasks.front();
                    qTasks.pop_front();
                }
            }
            return pTask;
        }*/
        Threads::ITask* CThreadPool::_PopTask(THREAD_USAGE usage)
        {
            Threads::ITask* pTask = nullptr;
            auto& SyncObj = m_aThreadUsageSyncObjs[usage];
            {
                auto& qTasks = m_aqTasks[ usage ];
                Threads::ScopedLock l( SyncObj );
                if( !qTasks.empty() )
                {
                    pTask = qTasks.front();
                    qTasks.pop_front();
                }
            }
            return pTask;
        }
        /*Result CThreadPool::AddConstantTaskGroup( Threads::CTaskGroup* pGroup )
        {
            auto id = m_vpTaskGroups.PushBack( pGroup );
            pGroup->m_id = id;
            for( uint32_t i = 0; i < pGroup->m_vpTasks.GetCount(); ++i )
            {
                WorkerID threadId = WorkerID( i % m_vThreads.size() );
                AddConstantTask( threadId, pGroup->m_vpTasks[ i ], TaskStateBits::NOT_ACTIVE );
            }
            AddConstantTask( &pGroup->m_Scheduler, TaskStateBits::NOT_ACTIVE );
            return VKE_OK;
        }*/
        // Result CThreadPool::AddTaskGroup(Threads::CTaskGroup* pGroup)
        //{
        //     auto id = m_vpTaskGroups.PushBack( pGroup );
        //     pGroup->m_id = id;
        //     for( uint32_t i = 0; i < pGroup->m_vpTasks.GetCount(); ++i )
        //     {
        //         WorkerID threadId = i % m_vThreads.size();
        //         AddTask( threadId, pGroup->m_vpTasks[ i ], TaskStateBits::NOT_ACTIVE );
        //     }
        //     AddTask( Constants::Threads::ID_BALANCED, &pGroup->m_Scheduler, TaskStateBits::NOT_ACTIVE );
        //     return VKE_OK;
        // }
    } // namespace Threads
} // VKE