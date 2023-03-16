#include "Core/Threads/CThreadPool.h"
#include "Core/Utils/CLogger.h"
#include "Core/Threads/CTaskGroup.h"

namespace VKE
{
    namespace Threads
    {
        struct SPageAllocatorDesc
        {
            uint32_t poolSize;
            uint32_t pageSize;
            uint8_t alignment = 8; // bytes
        };
        class CPageAllocator
        {
            struct SPool
            {
                uint8_t* pData;
                std::vector<bool> vFreePages;
            };

            using PoolVector = Utils::TCDynamicArray< SPool >;
            using UintVector = Utils::TCDynamicArray< uint16_t >;
            using BoolVector = std::vector< bool >;
            using BoolVectorVector = Utils::TCDynamicArray< BoolVector >;
            using PtrVector = Utils::TCDynamicArray< uint8_t* >;
            using Uint8Vector = Utils::TCDynamicArray< uint8_t >;
            using Uint8VectorVector = Utils::TCDynamicArray< Uint8Vector >;

            struct PageStates
            {
                enum STATE : uint8_t
                {
                    FREE,
                    ALLOCATED
                };
            };

            union UHandle
            {
                struct
                {
                    uint32_t poolIdx : 8;
                    uint32_t pageIdx : 12;
                    uint32_t pageCount : 12;
                };
                uint32_t handle;
            };

            public:

                using MemoryHandle = uint32_t;
                
                using HandleVector = Utils::TCDynamicArray< MemoryHandle >;

            Result Create(const SPageAllocatorDesc& Desc)
            {
                Result ret = VKE_FAIL;
                m_Desc = Desc;

                m_pageCount = m_Desc.poolSize / m_Desc.pageSize;
                
                return ret;
            }

            void Free( MemoryHandle hMem );
            void Free( void* pData );

            MemoryHandle Allocate( uint32_t size );
            void* GetPtr(MemoryHandle hMem)
            {
                UHandle handle = { .handle = hMem };
                VKE_ASSERT( m_vpMemoryData[ handle.poolIdx ] + handle.pageIdx * m_Desc.pageSize <
                            (m_vpMemoryData[ handle.poolIdx ] + m_pageCount * m_Desc.pageSize) );
                return m_vpMemoryData[ handle.poolIdx ] + handle.pageIdx * m_Desc.pageSize;
            }

            protected:

                MemoryHandle _CreatePool();
              MemoryHandle _AllocateMemory( uint32_t size );
                void _PrintStatus(uint32_t poolIdx);

            protected:

                SPageAllocatorDesc m_Desc;
              uint32_t m_pageCount;

                UintVector m_vTotalFreePageCount; // per pool total free pages
              BoolVectorVector m_vvPageStates; // per pool free pages
                PtrVector m_vpMemoryData; // per pool memory data
              Uint8VectorVector m_vvAllocatedPageCount; // per pool allocated pages started at page idx
        };

        CPageAllocator::MemoryHandle CPageAllocator::_CreatePool()
        {
            UHandle ret;
            uint8_t* pData;
            auto pPtr = Memory::AllocMemory( &HeapAllocator, &pData, m_Desc.poolSize );
            if (pPtr != nullptr)
            {
                // Init free pages
                uint32_t idx1 = m_vTotalFreePageCount.PushBack( (uint16_t)m_pageCount );
                uint32_t idx2 = m_vvPageStates.PushBack( std::move( BoolVector( m_pageCount, PageStates::FREE ) ) );
                uint32_t idx3 = m_vpMemoryData.PushBack( pPtr );
                m_vvAllocatedPageCount.PushBack( std::move( Uint8Vector( m_pageCount, 0 ) ) );
                VKE_ASSERT( idx1 == idx2 && idx2 == idx3 );
                VKE_ASSERT( idx1 < UINT8_MAX );
                ret.pageCount = m_pageCount;
                ret.poolIdx = (uint8_t)idx1;
                ret.pageIdx = 0;
            }
            else
            {
                ret.poolIdx = UNDEFINED_U8;
                VKE_LOG_ERR( "Unable to create memory pool." );
            }
            return ret.handle;
        }

        void CPageAllocator::Free(MemoryHandle hMem)
        {
            UHandle handle = { .handle = hMem };
            auto& vPages = m_vvPageStates[ handle.poolIdx ];
            for(uint32_t i = handle.pageIdx; i < handle.pageIdx + handle.pageCount; ++i)
            {
                VKE_ASSERT( vPages[ i ] != PageStates::FREE );
                vPages[ i ] = PageStates::FREE;
            }
            m_vvAllocatedPageCount[ handle.poolIdx ][ handle.pageIdx ] = 0;
            //_PrintStatus( handle.poolIdx );
        }

        void CPageAllocator::Free(void* pData)
        {
            // Calc pool index
            uint8_t* pPtr = (uint8_t*)pData;
            for (uint32_t p = 0; p < m_vpMemoryData.GetCount(); ++p)
            {
                const uint8_t* pCurrPoolPtr = m_vpMemoryData[ p ];
                if( pPtr >= pCurrPoolPtr && pPtr < pCurrPoolPtr + m_Desc.poolSize )
                {
                    // Calc page index
                    auto byteOffset = pPtr - pCurrPoolPtr;
                    VKE_ASSERT( byteOffset % m_Desc.pageSize == 0 );
                    UHandle handle;
                    handle.poolIdx = p;
                    handle.pageIdx = byteOffset / m_Desc.pageSize;
                    handle.pageCount = m_vvAllocatedPageCount[ p ][ handle.pageIdx ];
                    Free( handle.handle );
                    break;
                }
            }
        }

        CPageAllocator::MemoryHandle CPageAllocator::Allocate( uint32_t size )
        {
            return _AllocateMemory( size );
        }

        CPageAllocator::MemoryHandle CPageAllocator::_AllocateMemory(uint32_t size)
        {
            UHandle ret = {.handle = (uint32_t)INVALID_HANDLE};
            // Align size
            size = Memory::CalcAlignedSize( size, (uint32_t)m_Desc.alignment );
            uint32_t reminder = (size % m_Desc.pageSize) > 0; // add one page more if needed
            uint32_t pageCount = size / m_Desc.pageSize + reminder;
            // Find free slots
            uint32_t poolIdx = INVALID_POSITION;
            for (uint32_t i = 0; i < m_vTotalFreePageCount.GetCount(); ++i)
            {
                if( m_vTotalFreePageCount[ i ] >= pageCount )
                {
                    poolIdx = i;
                    // Find free pages in this pool
                    auto& vPageStates = m_vvPageStates[i];
                    uint32_t freePagesFoundIdx = INVALID_POSITION;
                    const uint32_t count = (uint32_t)vPageStates.size() - pageCount;
                    for (uint32_t p = 0; p < count; ++p)
                    {
                        if( vPageStates[p] == PageStates::FREE )
                        {
                            uint32_t freePagesFoundCount = 0;
                            for (uint32_t c = 0; c < pageCount; ++c)
                            {
                                bool isFree = vPageStates[ p + c ] == PageStates::FREE;
                                freePagesFoundCount += isFree;
                                if( !isFree )
                                {
                                    // If there is not enough free pages in a range
                                    // skip these pages
                                    p += pageCount;
                                    break; // stop to search in this range
                                }
                            }
                            if(freePagesFoundCount < pageCount)
                            {
                                continue; // go for p+1 to find
                            }
                            else
                            {
                                freePagesFoundIdx = p;
                                break;
                            }
                        }
                    }
                    if(freePagesFoundIdx != INVALID_POSITION)
                    {
                        ret.pageIdx = freePagesFoundIdx;
                        ret.poolIdx = poolIdx;
                        ret.pageCount = pageCount;
                        for (uint32_t p = freePagesFoundIdx; p < freePagesFoundIdx + pageCount; ++p)
                        {
                            vPageStates[ p ] = PageStates::ALLOCATED;
                        }
                        break;
                    }
                }
            }
            if (ret.handle == INVALID_HANDLE)
            {
                _CreatePool();
                return _AllocateMemory( size );
            }
            //_PrintStatus( ret.poolIdx );
            return ret.handle;
        }

        void CPageAllocator::_PrintStatus(uint32_t poolIdx)
        {
            if( poolIdx < m_vpMemoryData.GetCount() )
            {
                std::stringstream ss;
                for( uint32_t i = 0; i < m_pageCount; ++i )
                {
                    ss << m_vvPageStates[ poolIdx ][ i ];
                }
                VKE_LOG( ss.str() );
            }
        }

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
            Memory::DestroyObject( &HeapAllocator, &m_pMemAllocator );
        }

        vke_string ThreadUsageBitsToString( ThreadUsages Usages )
        {
            vke_string ret = std::to_string(Usages.Get());
            if( Usages == ( ThreadUsageBits::MAIN_THREAD ) )
                ret += " | main";
            if( Usages == ( ThreadUsageBits::GENERAL ) )
                ret += " | general";
            if( Usages == ( ThreadUsageBits::GRAPHICS ) )
                ret += " | graphics";
            if( Usages == ( ThreadUsageBits::LOGIC ) )
                ret += " | logic";
            if( Usages == ( ThreadUsageBits::PHYSICS ) )
                ret += " | physics";
            if( Usages == ( ThreadUsageBits::SOUND ) )
                ret += " | sound";
            if( Usages == ( ThreadUsageBits::NETWORK ) )
                ret += " | network";
            if( Usages == ( ThreadUsageBits::FILE_IO ) )
                ret += " | file_io";
            if( Usages == ( ThreadUsageBits::COMPILE ) )
                ret += " | compile";
            if( Usages == ( ThreadUsageBits::RESOURCE_PREPARE ) )
                ret += " | resource_prepare";
            if( Usages == ( ThreadUsageBits::CUSTOM_1 ) )
                ret += " | custom_1";

            return ret;
        }

        Result CThreadPool::Create( const SThreadPoolInfo& Info )
        {
            m_Desc = Info;
            bool res = false;
            Result ret = VKE_OK;
            
            Memory::CreateObject( &HeapAllocator, &m_pMemAllocator );
            SPageAllocatorDesc MemAllocDesc;
            MemAllocDesc.pageSize = VKE_KILOBYTES( 1 );
            MemAllocDesc.poolSize = 4096 * VKE_KILOBYTES(1);
            m_pMemAllocator->Create( MemAllocDesc );

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
                        m_Desc.vThreadDescs[ threadIdx ].Usages = Info.vThreadDescs[ i ].Usages;
                    }
                }
                else
                {
                    static const Threads::ThreadUsages aUsages[] =
                    {
                        ThreadUsageBits::GRAPHICS | ThreadUsageBits::LOGIC | ThreadUsageBits::PHYSICS | ThreadUsageBits::MAIN_THREAD,
                        ThreadUsageBits::FILE_IO | ThreadUsageBits::NETWORK | ThreadUsageBits::GENERAL | ThreadUsageBits::MAIN_THREAD,
                        ThreadUsageBits::RESOURCE_PREPARE | ThreadUsageBits::COMPILE | ThreadUsageBits::GRAPHICS,
                        ThreadUsageBits::NETWORK | ThreadUsageBits::MAIN_THREAD,
                        ThreadUsageBits::SOUND | ThreadUsageBits::MAIN_THREAD,
                        ThreadUsageBits::GRAPHICS | ThreadUsageBits::LOGIC | ThreadUsageBits::PHYSICS,
                        ThreadUsageBits::FILE_IO, ThreadUsageBits::RESOURCE_PREPARE | ThreadUsageBits::COMPILE,
                        ThreadUsageBits::GRAPHICS | ThreadUsageBits::LOGIC | ThreadUsageBits::PHYSICS,
                    };
                    uint32_t threadCount = sizeof( aUsages ) / sizeof( aUsages[ 0 ] );
                    m_Desc.vThreadDescs.Resize( threadCount );
                    for( uint32_t i = 0; i < threadCount; ++i )
                    {
                        uint32_t threadIdx = i;//% m_Desc.vThreadDescs.GetCount();
                        m_Desc.vThreadDescs[ threadIdx ].Usages = aUsages[ i ];
                        m_Desc.vThreadDescs[ threadIdx ].Name = ThreadUsageBitsToString( aUsages[ i ] ).c_str();
                    }
                }

                m_vqTasks.Resize( m_Desc.vThreadDescs.GetCount() );
                m_vThreadUsageSyncObjs.Resize( m_Desc.vThreadDescs.GetCount());
                m_anyThreadQueueIdx = m_vqTasks.GetCount();
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
                    Desc.Usages = ThreadDesc.Usages;
                    if( VKE_SUCCEEDED( m_vWorkers[ i ].Create( this, Desc ) ) )
                    {
                        m_vThreads[ i ] = std::thread( std::ref( m_vWorkers[ i ] ) );
                             
                        VKE_LOG( "Create thread: " << i << ", tid: " << m_vThreads[ i ].get_id() << ", "
                                                   << ThreadUsageBitsToString( Desc.Usages ) );
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
        SThreadWorkerID CThreadPool::_CalcQueueIndex( const SCalcWorkerIDDesc& Desc ) const
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

        uint32_t CThreadPool::_CalcQueueIndex( ThreadUsages TaskUsages ) const
        {
            uint32_t ret = m_anyThreadQueueIdx;
            const auto& Descs = m_Desc.vThreadDescs;
            uint32_t foundCount = 0;
            // If selected is ANY_THREAD
            if( ( TaskUsages != ThreadUsageBits::MAIN_THREAD && TaskUsages != ThreadUsageBits::ANY_EXCEPT_MAIN ) )
            {
                ret = { m_anyThreadQueueIdx };
            }
            else // find proper type of thread
            {
                for(uint32_t i = 0; i < Descs.GetCount(); ++i )
                {
                    const auto& WorkerUsages = Descs[ i ].Usages;
                    if( WorkerUsages.Contains(TaskUsages) )
                    {
                        foundCount++;
                        if( foundCount >= 1 && TaskUsages == ThreadUsageBits::ANY_THREAD )
                        {
                            ret = { i };
                            break;
                        }
                        else if( foundCount == 1 && TaskUsages == ThreadUsageBits::MAIN_THREAD )
                        {
                            ret = { i };
                            break;
                        }
                        else if( foundCount > 1 && TaskUsages == ThreadUsageBits::ANY_EXCEPT_MAIN )
                        {
                            ret = { i };
                            break;
                        }
                    }
                }
            }
            return ret;
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


        CThreadPool::WorkerID CThreadPool::GetThisThreadID() const
        {
            const auto& threadId = std::this_thread::get_id();
            for( uint32_t i = 0; i < m_vThreads.size(); ++i )
            {
                if( m_vThreads[ i ].get_id() == threadId )
                {
                    return WorkerID(i);
                }
            }
            return WorkerID((uint32_t)INVALID_HANDLE);
        }


        bool CThreadPool::_PopTaskFromQueue( uint32_t idx, SThreadPoolTask* pTask )
        {
            bool ret = false;
            auto& SyncObj = m_vThreadUsageSyncObjs[ idx ];
            {
                Threads::ScopedLock l( SyncObj );
                auto& qTasks2 = m_vqTasks[ idx ];
                ret = !qTasks2.empty();
                if( ret )
                {
                    *pTask = qTasks2.front();
                    qTasks2.pop_front();
                }
            }
            return ret;
        }

        bool CThreadPool::_PopTask(ThreadUsages WorkerUsages, SThreadPoolTask* pTask)
        {
            bool ret = false;
            // Clear thread type bits
            //if (Usages != ThreadUsageBits::MAIN_THREAD)
            //{
            //    // Find matching task
            //    for (uint32_t i = 0; i < m_vTasksUsages.GetCount(); ++i)
            //    {
            //        if( m_vTasksUsages[ i ] == Usages )
            //        {
            //            ret = true;
            //            ScopedLock l( m_TasksSyncObj );
            //            m_vTasksUsages.RemoveFast( i );
            //            *pTask = m_vTasks[i];
            //            m_vTasks.RemoveFast( i );
            //            break;
            //        }
            //    }
            //}
            //else // pop from main
            //{
            //    for (uint32_t i = 0; i < m_Desc.vThreadDescs.GetCount(); ++i)
            //    {
            //        const auto& Desc = m_Desc.vThreadDescs[ i ];
            //        if( Desc.Usages == Usages )
            //        {
            //            ScopedLock l( m_vThreadUsageSyncObjs[ i ] );
            //            auto& qTasks = m_vqTasks[ i ];
            //            ret = !qTasks.empty();
            //            if(ret)
            //            {
            //                *pTask = qTasks.front();
            //                qTasks.pop_front();
            //                break;
            //            }
            //        }
            //    }
            //}
            ScopedLock l( m_TasksSyncObj );
            for( uint32_t i = 0; i < m_vTasksUsages.GetCount(); ++i )
            {
                auto res = WorkerUsages.Contains( m_vTasksUsages[ i ] );
                if( res )
                {
                    ret = true;
                    m_vTasksUsages.RemoveFast( i );
                    *pTask = m_vTasks[ i ];
                    m_vTasks.RemoveFast( i );
                    break;
                }
            }
            return ret;
        }

        void CThreadPool::_AddTaskToQueue( uint32_t id, SThreadPoolTask& Task )
        {
            if(id == m_anyThreadQueueIdx)
            {
                ScopedLock l( m_TasksSyncObj );
                m_vTasksUsages.PushBack( Task.Usages );
                m_vTasks.PushBack( Task );
            }
            else
            {
                Threads::ScopedLock l( m_vThreadUsageSyncObjs[ id ] );
                m_vqTasks[ id ].push_back( Task );
            }
        }

        TASK_RESULT CThreadPool::_RunTask( const SThreadPoolTask& Task)
        {
            void* pData = nullptr;
            if( Task.hMemory != INVALID_HANDLE )
            {
                pData = m_pMemAllocator->GetPtr( Task.hMemory );
            }
            // VKE_LOG( "Running task: " << Task.GetDebugText() );
            TASK_RESULT Result = Task.Func( pData );
            if( Result != TaskResults::WAIT )
            {
                // VKE_LOG( "remove task: " << Task.GetDebugText() );
                m_pMemAllocator->Free( Task.hMemory );
            }
            return Result;
        }

        TASK_RESULT CThreadPool::_RunTaskForWorker( uint32_t workerIdx, SThreadPoolTask& Task )
        {
            void* pData = nullptr;
            if( Task.hMemory != INVALID_HANDLE )
            {
                pData = m_pMemAllocator->GetPtr( Task.hMemory );
            }
            
            //VKE_LOG( "Running task: " << Task.GetDebugText() );
            TASK_RESULT Result = Task.Func( pData );
            if( Result == TaskResults::WAIT )
            {
                //VKE_LOG( "Re-Add task: " << Task.GetDebugText() );
                //VKE_ASSERT( workerIdx == Task.workerIdx );
                _AddTask( workerIdx, Task );
                //m_vWorkers[ workerIdx ]._AddTask( std::move( Task ) );
            }
            else
            {
                //VKE_LOG( "remove task: " << Task.GetDebugText() );
                m_pMemAllocator->Free( Task.hMemory );
            }
            return Result;
        }

        TASK_RESULT CThreadPool::_RunTaskForWorker( uint32_t workerIdx, ThreadUsages usages )
        {
            TASK_RESULT ret = TaskResults::NONE;
            SThreadPoolTask Task;
            if( _PopTaskFromQueue( workerIdx, &Task) )
            {
                ret = _RunTaskForWorker( workerIdx, Task );
            }
            // Pop any task for thread usage
            else if( _PopTask( usages, &Task ) )
            {
                ret = _RunTaskForWorker( workerIdx, Task );
            }
            return ret;
        }

        void* CThreadPool::_AllocateTaskDataMemory( uint32_t size, uint32_t* pHandleOut )
        {
            *pHandleOut = m_pMemAllocator->Allocate( size );
            return m_pMemAllocator->GetPtr( *pHandleOut );
        }

    } // namespace Threads
} // VKE