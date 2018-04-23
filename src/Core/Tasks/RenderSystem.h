#ifndef __VKE_TASKS_RENDER_SYSTEM_H__
#define __VKE_TASKS_RENDER_SYSTEM_H__

#include "ITask.h"

namespace VKE
{
    namespace Tasks
    {
        class RenderSystemCommon : public ITask
        {
            public:

            protected:

                void _OnStart();
        };
    } // tasks
} // vke

#endif // __VKE_TASKS_RENDER_SYSTEM_H__