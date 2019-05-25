#pragma once

#include "Types.h"

namespace VKE
{
    namespace Math
    {
        class CAABB
        {
            public:

                union
                {
                    NativeAABB  _Native;
                };
        };
    } // Math
} // VKE