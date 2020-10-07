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
            return v1 < (T)v2 ? v1 : (T)v2;
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

        static void vke_force_inline SphericalToCartesian( float yaw, float pitch, float r, Math::CVector3* pOut )
        {
#if VKE_USE_DIRECTX_MATH
            DirectX::XMVECTOR vecSin;
            DirectX::XMVECTOR vecCos;
            DirectX::XMVECTOR vecSinCos = { pitch, yaw, 0, 0 };
            DirectX::XMVectorSinCos( &vecSin, &vecCos, vecSinCos );
#else
            float fSy = sinf( yaw ), fCy = cosf( yaw );
            float fSp = sinf( pitch ), fCp = cosf( pitch );
            pvecOut->x = r * fCp * fCy;
            pvecOut->y = r * fSp;
            pvecOut->z = -r * fCp * fSy;
#endif // VKE_USE_DIRECTX_MATH
        }

        constexpr float vke_force_inline ConvertToRadians( const float degrees )
        {
#if VKE_USE_DIRECTX_MATH
            return DirectX::XMConvertToRadians( degrees );
#else
#error "implement"
#endif
        }

        constexpr float vke_force_inline ConvertToDegrees( const float radians )
        {
#if VKE_USE_DIRECTX_MATH
            return DirectX::XMConvertToDegrees( radians );
#else
#error "implement"
#endif
        }

        void vke_force_inline Transform( const CVector4& V, const CMatrix4x4& Matrix, CVector4* pOut )
        {
#if VKE_USE_DIRECTX_MATH
            pOut->_Native = DirectX::XMVector3Transform( VKE_XMVEC4( V ), VKE_XMMTX4( Matrix ) );
#else
#error "implement"
#endif
        }

        static uint32_t vke_force_inline CalcNextPow2( uint32_t v )
        {
            // https://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
            v--;
            v |= v >> 1;
            v |= v >> 2;
            v |= v >> 4;
            v |= v >> 8;
            v |= v >> 16;
            v++;
            return v;
        }

        template<typename T>
        T vke_force_inline CalcPow2(const T& factor)
        {
            return (T)( 1u << (uint8_t)factor );
        }

        static float vke_force_inline Cot( const float v )
        {
            return 1.0f / std::tanf( v );
        }

        static uint32_t vke_force_inline
        Map2DArrayIndexTo1DArrayIndex(const uint32_t& x, const uint32_t& y, const uint32_t& width)
        {
            return x + y * width;
        }

        static void vke_force_inline
        Map1DarrayIndexTo2DArrayIndex( const uint32_t& idx, const uint32_t& height,
                                       uint32_t* pXOut, uint32_t* pYOut )
        {
            *pYOut = idx / height;
            *pXOut = idx % height;
        }

        // Calculates number of 1 bits
        template<typename T>
        static uint16_t vke_force_inline CalcEnabledBitCount(const T& v)
        {
            return (uint16_t)Platform::PopCnt( v );
        }

        // Checks if number is power of 2
        template<typename T>
        static bool vke_force_inline IsPow2(const T& v)
        {
            return CalcEnabledBitCount( v ) == 1;
        }

    } // Math
} // VKE

#include "DirectX/CVector.inl"
#include "DirectX/CQuaternion.inl"
#include "DirectX/CMatrix.inl"
#include "DirectX/CFrustum.inl"
#include "DirectX/CAABB.inl"
#include "DirectX/CBoundingSphere.inl"