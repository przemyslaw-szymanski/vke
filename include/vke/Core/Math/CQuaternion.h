#pragma once

#include "CVector.h"

namespace VKE
{
    namespace Math
    {
        class CQuaternion : public CVector4
        {
            public:

                CQuaternion() = default;
                CQuaternion( const CQuaternion& ) = default;
                CQuaternion( CQuaternion&& ) = default;
                vke_force_inline explicit constexpr CQuaternion( const float x, const float y, const float z, const float w );
                vke_force_inline explicit constexpr CQuaternion( const CVector4& V );
                ~CQuaternion() {}

                CQuaternion& operator=( const CQuaternion& ) = default;
                CQuaternion& operator=( CQuaternion&& ) = default;

                CQuaternion vke_force_inline operator*( const CVector3& V ) const;
                void vke_force_inline operator*=( const CVector3& V );

                static void vke_force_inline Mul( const CQuaternion& Q, const CVector3& V, CQuaternion* pOut );

                static const CQuaternion    UNIT;
                static const vke_force_inline CQuaternion&  _Unit() { return UNIT; }
        };
    } // Math
} // VKE

#include "DirectX/CQuaternion.inl"