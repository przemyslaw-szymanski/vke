#pragma once

#include "RenderSystem/Common.h"
#include "Resource/TCManager.h"

namespace VKE
{
    namespace RenderSystem
    {
        class CPipeline;

        class CPipelineManager : public Resource::TCManager< CPipeline >
        {
            public:

            protected:
        };
    } // RenderSystem
} // VKE