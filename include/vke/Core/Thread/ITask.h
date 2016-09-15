#ifndef __VKE_ITASK_H__
#define __VKE_ITASK_H__

#include "Core/VKECommon.h"

namespace VKE
{
    class ITask
    {
        public:

                        ITask(){}
            virtual     ~ITask() {};

            void        Start() { _OnStart(); Thread::LockGuard l(m_Mutex); m_bIsFinished = true; }

            bool        IsFinished() const { return m_bIsFinished; }

        protected:

            virtual
            void        _OnStart() {}

        protected:

            std::mutex  m_Mutex;
            bool        m_bIsFinished = false;
    };
} // vke

#endif // __VKE_ITASK_H__