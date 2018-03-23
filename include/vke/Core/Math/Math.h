#pragma once

#include "Core/VKEPreprocessor.h"

#if VKE_USE_DIRECTX_MATH
#include <intrin.h>
#include "DirectX/CVector.h"
#include "DirectX/CMatrix.h"
#include "DirectX/CPlane.h"
#endif


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