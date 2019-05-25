#pragma once

#include "Types.h"

namespace VKE
{
    namespace Math
    {
        class CFrustum
        {
            public:



                union
                {
                    NativeFrustum   _Native;
                };
        };
    } // Math
} // VKE