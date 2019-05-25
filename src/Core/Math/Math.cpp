#include "Core/Math/Math.h"
#include "Core/Math/CVector.h"
#include "Core/Math/CMatrix.h"

namespace VKE
{
    namespace Math
    {
        const CVector CVector::ONE = CVector( 1.0f );
        const CVector CVector::ZERO = CVector( 0.0f );
        const CVector CVector::X = CVector( 1.0f, 0.0f, 0.0f );
        const CVector CVector::Y = CVector( 0.0f, 1.0f, 0.0f );
        const CVector CVector::Z = CVector( 0.0f, 0.0f, 1.0f );

        const CMatrix4x4 CMatrix4x4::IDENTITY = CMatrix4x4::Identity();

    } // Math
} // VKE