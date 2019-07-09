#pragma once

#include "Core/VKEPreprocessor.h"
#include "Core/VKETypes.h"

#ifndef VKE_USE_DIRECTX_MATH
#   define VKE_USE_DIRECTX_MATH 1
#endif

#if VKE_WINDOWS || VKE_LINUX
#   ifndef VKE_USE_SSE
#       define VKE_USE_SSE 1
#   endif
#elif VKE_ANDROID || VKE_IOS
#   ifndef VKE_USE_NEON
#       define VKE_USE_NEON 1
#   endif // USE_NEON
#endif // WINDOWS

#if VKE_USE_DIRECTX_MATH
#   include "ThirdParty/DirectX/DirectXMath.h"
#   include "ThirdParty/DirectX/DirectXCollision.h"
#endif // #if VKE_USE_DIRECTX_MATH

namespace VKE
{
    namespace Math
    {
        struct IntersectResults
        {
            enum RESULT
            {
                OUTSIDE,
                INTERSECTS,
                CONTAINS,
                _MAX_COUNT
            };
        };
        using INTERSECT_RESULT = IntersectResults::RESULT;

#if VKE_USE_DIRECTX_MATH
        using NativeVector4         = DirectX::XMVECTOR;
        using NativeVector3         = DirectX::XMFLOAT3;
        using NativeMatrix4x4       = DirectX::XMFLOAT4X4;
        using NativeMatrix3x3       = DirectX::XMFLOAT3X3;
        using NativeFrustum         = DirectX::BoundingFrustum;
        using NativeAABB            = DirectX::BoundingBox;
        using NativeBoundingSphere  = DirectX::BoundingSphere;
        using NativeQuaternion      = DirectX::XMVECTOR;

        static const float PI       = DirectX::XM_PI;
        static const float PI_DIV_2 = DirectX::XM_PIDIV2;
        static const float PI_MUL_2 = DirectX::XM_2PI;

#define VKE_XMLOADF3(_float3) DirectX::XMLoadFloat3(&(_float3))
#define VKE_XMVEC3(_vec) VKE_XMLOADF3((_vec)._Native)
#define VKE_XMVEC4(_vec) ((_vec)._Native)

#define VKE_XMMTX4(_mtx) DirectX::XMLoadFloat4x4(&(_mtx)._Native)
#define VKE_XMSTORE44(_dst, _xmmatrix) DirectX::XMStoreFloat4x4( &_dst, (_xmmatrix) )
#define VKE_XMSTOREMTX(_dst, _xmmatrix) VKE_XMSTORE44( ((_dst)._Native), (_xmmatrix) )
#define VKE_XMSTOREPMTX(_dst, _xmmatrix) VKE_XMSTORE44( ((_dst)->_Native), (_xmmatrix) )

        // Impl in Math.cpp
        static vke_force_inline INTERSECT_RESULT ConvertFromNative( DirectX::ContainmentType type )
        {
            static const INTERSECT_RESULT aRets[] =
            {
                IntersectResults::OUTSIDE,
                IntersectResults::INTERSECTS,
                IntersectResults::CONTAINS
            };
            return aRets[type];
        }

#endif // #if VKE_USE_DIRECTX_MATH
        
    } // Math
} // VKE