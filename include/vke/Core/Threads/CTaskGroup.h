#pragma once

#include "Core/Utils/TCDynamicArray.h"
#include "ITask.h"

namespace VKE
{
    namespace Threads
    {
        class CTaskGroup;
        class CSchedulerTask final : public ITask
        {
            friend class CTaskGroup;
            struct SDefaultUserTask : public ITask
            {
                CTaskGroup* pGroup = nullptr;
                TaskResult _OnStart(uint32_t threadId);
            };
            static SDefaultUserTask DefaultTask;
            ITask* pUserTask = &DefaultTask;
            CTaskGroup* pGroup = nullptr;

            protected:

                CSchedulerTask();

                TaskResult _OnStart(uint32_t threadId) override;
        };

        class VKE_API CTaskGroup final
        {
            friend class CThreadPool;
            friend class CSchedulerTask;
            using TaskVec = Utils::TCDynamicArray< ITask*, 32 >;

            public:

                CTaskGroup();

                void AddTask(ITask* pTask);
                void AddSchedulerTask(ITask* pTask);
                void Pause();
                void Restart();

            protected:

                TaskVec         m_vpTasks;
                CSchedulerTask  m_Scheduler;
                uint32_t        m_id;
        };
    } // Threads
} // VKE