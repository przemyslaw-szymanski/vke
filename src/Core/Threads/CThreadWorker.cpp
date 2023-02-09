#include "Core/Threads/CThreadWorker.h"
#include "Core/Threads/CThreadPool.h"
#include "Core/Math/Math.h"
#include <Core/Utils/CLogger.h>

namespace VKE
{
    namespace Threads
    {
        CThreadWorker::CThreadWorker()
        {
        }
        CThreadWorker::CThreadWorker( const CThreadWorker& Other )
        {
            m_qWorks = Other.m_qWorks;
        }
        CThreadWorker::CThreadWorker( CThreadWorker&& Other )
        {
            m_qWorks = std::move( Other.m_qWorks );
        }
        CThreadWorker::~CThreadWorker()
        {
            m_bNeedStop = true;
        }
        void CThreadWorker::operator=( const CThreadWorker& Other )
        {
            m_qWorks = Other.m_qWorks;
        }
        void CThreadWorker::operator=( CThreadWorker&& Other )
        {
            m_qWorks = std::move( Other.m_qWorks );
        }
        Result CThreadWorker::Create( CThreadPool* pPool, const SDesc& Desc )
        {
            m_Desc = Desc;
            m_pPool = pPool;

            m_vDataPool.resize( m_Desc.taskCount );
            m_vFreeIds.reserve( m_vDataPool.size() );

            m_memPoolSize = m_Desc.taskMemSize * m_Desc.taskCount;
            
            uint16_t count = ( uint16_t )m_vDataPool.size();
            for( uint16_t i = 0; i < count; ++i )
            {
                m_vFreeIds.push_back( i );
            }

            Utils::TCBitset<THREAD_USAGES> UsageBits(Desc.usages);
            for(uint8_t bit = 0; bit < ThreadUsages::_MAX_COUNT; ++bit)
            {
                if(UsageBits.IsBitSet(bit))
                {
                    m_vUsages.PushBack( (THREAD_USAGE)bit );
                }
            }

            return VKE_OK;
        }
        uint32_t CThreadWorker::_RunConstantTasks()
        {
            m_ConstantTaskTimer.Start();
            Threads::ScopedLock l( m_ConstantTasks.SyncObj );
            int32_t taskCount = static_cast<int32_t>( m_ConstantTasks.vStates.GetCount() );
            for( int32_t i = 0; i < taskCount; ++i )
            {
                TaskState& state = m_ConstantTasks.vStates[ i ];
                bool isActive = ( state & TaskStateBits::NOT_ACTIVE ) == 0;
                bool needRemove = ( state & TaskStateBits::REMOVE ) != 0;
                // bool fail = (state & TaskStateBits::FAIL) != 0;
                // bool finished = (state & TaskStateBits::FINISHED) != 0;
                // bool next = (state & TaskStateBits::NEXT_TASK) != 0;
                // bool ok = (state & TaskStateBits::OK) != 0;
                if( isActive && !needRemove )
                {
                    Threads::ITask* pTask = m_ConstantTasks.vpTasks[ i ];
                    state = static_cast<TaskState>( pTask->Start( m_Desc.id ) );
                    if( state & TaskStateBits::NOT_ACTIVE )
                    {
                    }
                    if( state & TaskStateBits::NEXT_TASK )
                    {
                        pTask->IsActive( false );
                        pTask->m_pNextTask->IsActive( true );
                    }
                    /*if( ( state & TaskStateBits::REMOVE ) )
                    {
                        m_ConstantTasks.vpTasks.Remove( i );
                        m_ConstantTasks.vStates.Remove( i );
                        --i;
                        taskCount = static_cast< int32_t >( m_ConstantTasks.vStates.GetCount() );
                    }*/
                }
                else
                {
                }
            }
            m_totalContantTaskTimeUS = m_ConstantTaskTimer.GetElapsedTime();
            return ( uint32_t )taskCount;
        }
        void CThreadWorker::Start()
        {
            // uint32_t loop = 0;
            while( !m_bNeedStop )
            {
                m_TotalTimer.Start();
                bool needPause = m_bPaused;
                if( !m_bPaused )
                {
                    uint32_t constantTaskCount = _RunConstantTasks();
                    needPause = constantTaskCount == 0;
                    // std::this_thread::yield();
                    // if( m_Flags != Threads::TaskFlags::RENDER_THREAD )
                    {
                        Threads::ITask* pTask = nullptr;
                        {
                            // Threads::UniqueLock l( m_Mutex );
                            if( !m_qTasks.empty() )
                            {
                                Threads::ScopedLock l( m_TaskSyncObj );
                                pTask = m_qTasks.front();
                                m_qTasks.pop_front();
                                m_totalTaskWeight -= pTask->GetTaskWeight();
                            }
                            else // if( m_totalTaskWeight < UINT8_MAX )
                            {
                                pTask = _StealTask();
                            }
                        }
                        if( pTask )
                        {
                            m_totalTaskWeight += pTask->GetTaskWeight();
                            auto result = pTask->Start( m_Desc.id );
                            m_totalTaskWeight -= pTask->GetTaskWeight();
                            auto waitBit = ( result & TaskStateBits::WAIT );
                            if( waitBit == TaskStateBits::WAIT )
                            {
                                VKE_UNSET_MASK( pTask->m_state, TaskStateBits::WAIT );
                                AddTask( pTask );
                                //m_pPool->AddTask( pTask );
                            }
                            else
                            {
                            }
                            needPause = false;
                        }
                    }
                }
                m_totalTimeUS = m_TotalTimer.GetElapsedTime();
                if( needPause )
                {
                    Platform::ThisThread::Sleep( 1000 );
                }
            }
            m_bIsEnd = true;
        }
        Result CThreadWorker::AddWork( const WorkFunc& Func, const STaskParams& Params, uint8_t weight,
                                       uint8_t priority, int32_t threadId )
        {
            ( void )threadId;
            if( Params.inputParamSize >= m_Desc.taskMemSize )
            {
                return VKE_FAIL;
            }
            SWorkerData* pData = GetFreeData();
            if( pData )
            {
                memcpy( pData->pData, Params.pInputParam, Params.inputParamSize );
                pData->dataSize = Params.inputParamSize;
                pData->Func = Func;
                pData->pResult = Params.pResult;
                pData->weight = weight;
                pData->priority = priority;
                if( pData->pResult )
                    pData->pResult->m_ready = false;
                Threads::ScopedLock l( m_TaskSyncObj );
                m_qWorks.push_back( pData );
                return VKE_OK;
            }
            return VKE_FAIL;
        }
        std::thread::id CThreadWorker::AddTask( Threads::ITask* pTask )
        {
            Threads::ScopedLock l( m_TaskSyncObj );
            m_qTasks.push_back( pTask );
            m_totalTaskWeight += pTask->GetTaskWeight();
            return GetThreadID();
        }
        std::thread::id CThreadWorker::AddConstantWork( const WorkFunc2& Func, void* pPtr )
        {
            m_ConstantTaskSyncObj.Lock();
            m_vConstantWorks.push_back( { pPtr, Func } );
            m_ConstantTaskSyncObj.Unlock();
            return GetThreadID();
        }
        std::thread::id CThreadWorker::AddConstantTask( Threads::ITask* pTask, TaskState state )
        {
            Threads::ScopedLock l( m_ConstantTaskSyncObj );
            VKE_ASSERT( strlen( pTask->GetName() ) > 0 );
            m_vConstantTasks.PushBack( pTask );
            m_ConstantTasks.vpTasks.PushBack( pTask );
            uint32_t id = m_ConstantTasks.vStates.PushBack( state );
            m_totalTaskWeight += pTask->GetTaskWeight();
            m_totalTaskPriority += pTask->GetTaskPriority();
            pTask->m_state = state;
            pTask->m_pState = &m_ConstantTasks.vStates[ id ];
            m_Flags |= pTask->Flags;
            return GetThreadID();
        }
        void CThreadWorker::Stop()
        {
            // LockGuard l(m_Mutex);
            m_bNeedStop = true;
        }
        void CThreadWorker::Pause( bool bPause )
        {
            m_bPaused = bPause;
        }
        bool CThreadWorker::IsPaused()
        {
            return m_bPaused;
        }
        void CThreadWorker::WaitForStop()
        {
            while( !m_bIsEnd )
            {
                std::this_thread::yield();
            }
        }
        CThreadWorker::SWorkerData* CThreadWorker::GetFreeData()
        {
            Threads::ScopedLock l( m_TaskSyncObj );
            if( !m_vFreeIds.empty() )
            {
                auto id = m_vFreeIds.back();
                m_vFreeIds.pop_back();
                auto* pData = &m_vDataPool[ id ];
                pData->pData = m_Desc.pMemPool + id * m_Desc.taskMemSize;
                pData->handle = id;
                return pData;
            }
            return nullptr;
        }
        void CThreadWorker::FreeData( SWorkerData* pData )
        {
            assert( pData );
            Threads::ScopedLock l( m_TaskSyncObj );
            m_vFreeIds.push_back( pData->handle );
        }
        std::pair<uint8_t, uint8_t> CThreadWorker::_CalcStealTaskPriorityAndWeightIndices( uint8_t level )
        {
            const uint8_t LIGHT = 0, MEDIUM = 1, HEAVY = 2, LOW = 0, HIGH = 2;
            // If current workder thread has heavy work to do try to find anything light
            static const std::pair<uint8_t, uint8_t> aaValues[ 3 ][ 3 ] = {
                // light,               medium,             heavy
                { { HIGH, HEAVY }, { MEDIUM, MEDIUM }, { LOW, LIGHT } },   // low priority
                { { MEDIUM, HEAVY }, { MEDIUM, MEDIUM }, { LOW, LIGHT } }, // medium priority
                { { LOW, HEAVY }, { MEDIUM, MEDIUM }, { LOW, LIGHT } }     // high priority
            };
            uint8_t p = Threads::ITask::ConvertTaskFlagsToPriorityIndex( m_Flags );
            uint8_t w = Threads::ITask::ConvertTaskFlagsToWeightIndex( m_Flags );
            p = Math::Min( ( uint8_t )3, p + level );
            w = Math::Min( ( uint8_t )3, w );
            VKE_ASSERT2( aaValues[ p ][ w ].first < 3 && aaValues[ p ][ w ].second < 3, "" );
            return aaValues[ p ][ w ];
        }
        Threads::ITask* CThreadWorker::_StealTask()
        {
            // Pick usage
            ITask* pTask = nullptr;
            for( uint32_t usage = 0; usage < m_vUsages.GetCount(); ++usage )
            {
                auto threadUsage = m_vUsages[ usage ];
                pTask = m_pPool->_PopTask( threadUsage );
                if(pTask != nullptr)
                {
                    break;
                }
            }
            return pTask;
        }
    } // namespace Threads
} // VKE