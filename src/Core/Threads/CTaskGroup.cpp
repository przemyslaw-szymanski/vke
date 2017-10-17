#include "Core/Threads/CTaskGroup.h"
#include "Core/Platform/CPlatform.h"

namespace VKE
{
    namespace Threads
    {
        CSchedulerTask::SDefaultUserTask CSchedulerTask::DefaultTask;
        TaskResult CSchedulerTask::SDefaultUserTask::_OnStart(uint32_t threadId)
        {
            pGroup->Restart();
            return TaskResultBits::OK;
        }

        CSchedulerTask::CSchedulerTask()
        {
        }

        TaskResult CSchedulerTask::_OnStart(uint32_t threadId)
        {
            uint32_t finished = 0;
            const uint32_t count = pGroup->m_vpTasks.GetCount();
            TaskResult ret = TaskResultBits::OK;
            for( uint32_t i = 0; i < count; ++i )
            {
                finished += pGroup->m_vpTasks[ i ]->IsFinished();
            }
            pGroup->m_tasksFinished = finished == count;
            if( pGroup->m_tasksFinished )
            {
                ret = pUserTask->Start(threadId);
            }
            return ret;
        }

        CTaskGroup::CTaskGroup()
        {
            m_Scheduler.pGroup = this;
            auto pUserTask = reinterpret_cast< CSchedulerTask::SDefaultUserTask* >( m_Scheduler.pUserTask );
            pUserTask->pGroup = this;
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
            m_vpTasks.FastClear();
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
            m_Scheduler.IsActive(false);
            for( uint32_t i = 0; i < m_vpTasks.GetCount(); ++i )
            {
                m_vpTasks[ i ]->IsActive(false);
            }
        }

        void CTaskGroup::Restart()
        {
            m_Scheduler.IsActive(true);
            for( uint32_t i = 0; i < m_vpTasks.GetCount(); ++i )
            {
                m_vpTasks[ i ]->IsActive(true);
            }
        }

    } // Threads
} // VKE