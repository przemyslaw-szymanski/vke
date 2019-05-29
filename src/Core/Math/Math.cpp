#include "Core/Math/Math.h"
#include "Core/Math/CVector.h"
#include "Core/Math/CMatrix.h"
#include "Core/Math/CAABB.h"
#include "Core/Math/CFrustum.h"
#include "Core/Math/CBoundingSphere.h"

namespace VKE
{
    namespace Math
    {
        const CVector3 CVector3::ONE = CVector3( 1.0f );
        const CVector3 CVector3::ZERO = CVector3( 0.0f );
        const CVector3 CVector3::X = CVector3( 1.0f, 0.0f, 0.0f );
        const CVector3 CVector3::Y = CVector3( 0.0f, 1.0f, 0.0f );
        const CVector3 CVector3::Z = CVector3( 0.0f, 0.0f, 1.0f );

        const CVector4 CVector4::ONE = CVector4( 1.0f );
        const CVector4 CVector4::ZERO = CVector4( 0.0f );
        const CVector4 CVector4::X = CVector4( 1.0f, 0.0f, 0.0f, 0.0f );
        const CVector4 CVector4::Y = CVector4( 0.0f, 1.0f, 0.0f, 0.0f );
        const CVector4 CVector4::Z = CVector4( 0.0f, 0.0f, 1.0f, 0.0f );
        const CVector4 CVector4::W = CVector4( 0.0f, 0.0f, 0.0f, 1.0f );

        const CMatrix4x4 CMatrix4x4::IDENTITY = CMatrix4x4::Identity();

        const CAABB CAABB::ONE = CAABB( CVector3( 0.0f ), CVector3( 1.0f ) );

        const CBoundingSphere CBoundingSphere::ONE;

    } // Math
} // VKE