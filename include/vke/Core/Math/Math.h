#pragma once

#include "Core/VKEPreprocessor.h"
#include "CMatrix.h"
#include "CAABB.h"
#include "CBoundingSphere.h"
#include "CFrustum.h"
#include "CQuaternion.h"

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

        template<typename T>
        T Clamp( const T& v, const T& min, const T& max )
        {
            return std::min( std::max( min, v ), max );
        }

        static
        float vke_force_inline NormalizeAngle( const float angleRadians )
        {
            float ret = angleRadians / Math::PI_MUL_2;
            float intPart;
            ret = std::modff( ret, &intPart ) * Math::PI_MUL_2;
            return ret;
        }

        static
        Math::CVector3 vke_force_inline Mul( const Math::CVector4& quat, const Math::CVector3& vec )
        {
            Math::CVector3 uv, uuv, qvec( quat.x, quat.y, quat.z );
            Math::CVector3::Cross( qvec, vec, &uv );
            Math::CVector3::Cross( qvec, uv, &uuv );
            uv *= (2.0f * quat.w);
            uuv *= 2.0f;
            return vec + uv + uuv;
        }

        static
        void vke_force_inline SphericalToCartesian( float yaw, float pitch, float r, Math::CVector3* pvecOut )
        {
            float fSy = sinf( yaw ), fCy = cosf( yaw );
            float fSp = sinf( pitch ), fCp = cosf( pitch );
            pvecOut->x = r * fCp * fCy;
            pvecOut->y = r * fSp;
            pvecOut->z = -r * fCp * fSy;

            DirectX::XMVECTOR vecSin;
            DirectX::XMVECTOR vecCos;
            DirectX::XMVECTOR vecSinCos = { pitch, yaw, 0, 0 };
            DirectX::XMVectorSinCos( &vecSin, &vecCos, vecSinCos );

        }

    } // Math
} // VKE