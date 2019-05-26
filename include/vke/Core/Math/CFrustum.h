#pragma once

#include "Types.h"

namespace VKE
{
    namespace Math
    {
        class CFrustum
        {
            public:

                CFrustum() {}

                union
                {
                    NativeFrustum   _Native;
                };
        };
    } // Math
} // VKE