#pragma once

#include "Core/VKECommon.h"

namespace VKE
{
    namespace Config
    {
        struct Scene
        {
            static const uint32_t MAX_DRAWCALL_COUNT = 100000;

            struct Debug
            {
                static const uint32_t DEFAULT_AABB_VIEW_COUNT = 1000;
                static const uint32_t DEFAULT_SPHERE_VIEW_COUNT = 2000;
                static const uint32_t DEFAULT_FRUSTUM_VIEW_COUNT = 100;
            };
        };
    } // Config
} // VKE