#pragma once

#include "Core/VKEPreprocessor.h"
#include "CMatrix.h"
#include "CAABB.h"
#include "CBoundingSphere.h"

namespace VKE
{
    namespace Math
    {
        template<typename T>
        T Round( const T& value, const T& multiplier )
        {
            if( multiplier == 0 )
            {
                return value;
            }

            const T remainder = value % multiplier;
            if( remainder == 0 )
            {
                return value;
            }

            return value + multiplier - remainder;
        }
    } // Math
} // VKE