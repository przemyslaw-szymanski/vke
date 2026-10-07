#pragma once

#include "Core/VKECommon.h"

namespace VKE
{
    namespace Config
    {
        struct Scene
        {
            static constexpr uint32_t MAX_DRAWCALL_COUNT = 100000;

            struct Debug
            {
                static constexpr uint32_t DEFAULT_AABB_VIEW_COUNT    = 1000;
                static constexpr uint32_t DEFAULT_SPHERE_VIEW_COUNT  = 2000;
                static constexpr uint32_t DEFAULT_FRUSTUM_VIEW_COUNT = 100;
            };
        };
    } // namespace Config
} // namespace VKE