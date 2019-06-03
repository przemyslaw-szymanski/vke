#pragma once

#include "Core/VKEPreprocessor.h"
#include "CMatrix.h"
#include "CAABB.h"
#include "CBoundingSphere.h"
#include "CFrustum.h"

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

        template<typename T, typename T2>
        static vke_force_inline T Min( const T& v1, const T2& v2 )
        {
            return v1 < v2 ? v1 : v2;
        }

        template<typename T, typename T2>
        static vke_force_inline T Max( const T& v1, const T2& v2 )
        {
            return v1 > v2 ? v1 : v2;
        }

    } // Math
} // VKE