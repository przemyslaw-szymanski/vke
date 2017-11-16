#include "Core/Threads/CTaskGroup.h"
#include "Core/Platform/CPlatform.h"

namespace VKE
{
    namespace Threads
    {

        TaskState CSchedulerTask::SDefaultUserTask::_OnStart(uint32_t)
        {
            pGroup->Restart();
            return TaskStateBits::OK;
        }

        CSchedulerTask::CSchedulerTask(CTaskGroup* pOwner) :
            m_DefaultTask{ pOwner },
            pUserTask{ &m_DefaultTask },
            pGroup{ pOwner }
        {

        }

        TaskState CSchedulerTask::_OnStart(uint32_t threadId)
        {
            TaskState ret = TaskStateBits::OK;
            {
                ScopedLock l(pGroup->m_SyncObj);
                uint32_t finished = 0;
                const uint32_t count = pGroup->m_vpTasks.GetCount();

                for( uint32_t i = 0; i < count; ++i )
                {
                    ITask* pTask = pGroup->m_vpTasks[ i ];
                    bool a = pTask->IsActive();
                    bool f = pTask->IsFinished<THREAD_SAFE>();
                    finished += (a == false && f == true);
                }

                pGroup->m_tasksFinished = finished == count;
            }
            if( pGroup->m_tasksFinished )
            {
                pGroup->m_tasksFinished = false;
                ret = static_cast< TaskState >( pUserTask->Start( threadId ) );
            }
            return ret;
        }

        CTaskGroup::CTaskGroup() :
            m_Scheduler{ this }
        {
            /*m_Scheduler.pGroup = this;
            auto pUserTask = reinterpret_cast< CSchedulerTask::SDefaultUserTask* >( m_Scheduler.pUserTask );
            pUserTask->pGroup = this;*/
        }

        void CTaskGroup::AddTask(ITask* pTask)
        {
            m_vpTasks.PushBack(pTask);
        }

        void CTaskGroup::AddSchedulerTask(ITask* pTask)
        {
            m_Scheduler.pUserTask = pTask;
        }

        void CTaskGroup::Remove()
        {
            m_Scheduler.Remove();
            for( uint32_t i = 0; i < m_vpTasks.GetCount(); ++i )
            {
                m_vpTasks[ i ]->Remove<false /*wait for finish*/>();
            }
            Wait();
            m_vpTasks.Clear();
        }

        void CTaskGroup::Wait()
        {
            while( !m_tasksFinished )
            {
                Platform::ThisThread::Pause();
            }
        }

        void CTaskGroup::Pause()
        {
            //ScopedLock l(m_SyncObj);
            m_Scheduler.IsActive(false);
            for( uint32_t i = 0; i < m_vpTasks.GetCount(); ++i )
            {
                m_vpTasks[ i ]->IsActive(false);
            }
        }

        void CTaskGroup::Restart()
        {
            ScopedLock l( m_SyncObj );
            for( uint32_t i = 0; i < m_vpTasks.GetCount(); ++i )
            {
                //m_vpTasks[ i ]->IsFinished<THREAD_SAFE>(false);
                m_vpTasks[ i ]->IsActive(true);
            }
            m_Scheduler.IsActive(true);
        }

    } // Threads
} // VKE