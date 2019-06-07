#pragma once

#include "CVector.h"

namespace VKE
{
    namespace Math
    {
        class VKE_API CQuaternion : public CVector4
        {
            public:

                CQuaternion() = default;
                CQuaternion( const CQuaternion& ) = default;
                CQuaternion( CQuaternion&& ) = default;
                vke_force_inline explicit constexpr CQuaternion( const float x, const float y, const float z, const float w );
                vke_force_inline explicit constexpr CQuaternion( const CVector4& V );
                vke_force_inline explicit constexpr CQuaternion( const NativeQuaternion& Q );
                ~CQuaternion() {}

                CQuaternion& operator=( const CQuaternion& Right ) { this->_Native = Right._Native; return *this; }
                CQuaternion& operator=( CQuaternion&& Right ) { this->_Native = Right._Native; return *this; }

                CVector3 vke_force_inline operator*( const CVector3& V ) const;
                CQuaternion vke_force_inline operator*( const CQuaternion& Q ) const;
                void vke_force_inline operator*=( const CQuaternion& Q );

                void vke_force_inline Rotate( const CVector3& vecAxis, const float angleRadians ) { Rotate( vecAxis, angleRadians, this ); }
                void vke_force_inline Rotate( const CVector4& vecAxis, const float angleRadians ) { Rotate( vecAxis, angleRadians, this ); }
                void vke_force_inline Rotate( const CVector3& V, CVector3* pOut ) const;
                void vke_force_inline Rotate( const CVector4& V, CVector4* pOut ) const;

                static void vke_force_inline Mul( const CQuaternion& Q, const CVector3& V, CVector3* pOut );
                static void vke_force_inline Mul( const CQuaternion& Q1, const CQuaternion& Q2, CQuaternion* pOut );
                static void vke_force_inline Rotate( const CVector3& vecAxis, const float angleRadians, CQuaternion* pOut ) { Rotate( Math::CVector4( vecAxis ), angleRadians, pOut ); }
                static void vke_force_inline Rotate( const CVector4& vecAxis, const float angleRadians, CQuaternion* pOut );
                static void vke_force_inline Rotate( const float pitch, const float yaw, const float roll, CQuaternion* pOut );

                static const CQuaternion    UNIT;
                static const vke_force_inline CQuaternion&  _Unit() { return UNIT; }
        };
    } // Math
} // VKE

#include "DirectX/CQuaternion.inl"