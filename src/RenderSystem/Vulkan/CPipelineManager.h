#pragma once

#include "RenderSystem/Vulkan/Common.h"
#include "Core/Resource/TCManager.h"

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