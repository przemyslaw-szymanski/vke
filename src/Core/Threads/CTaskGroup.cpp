#include "Core/Threads/CTaskGroup.h"

namespace VKE
{
    namespace Threads
    {
        TaskResult CSchedulerTask::SDefaultUserTask::_OnStart(uint32_t threadId)
        {
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
            if( finished == count )
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
    } // Threads
} // VKE