#pragma once

#include "Core/VKEPreprocessor.h"
#include "Core/VKETypes.h"

#ifndef VKE_USE_DIRECTX_MATH
#   define VKE_USE_DIRECTX_MATH 1
#endif

#if VKE_USE_DIRECTX_MATH
#   include "ThirdParty/DirectX/DirectXMath.h"
#   include "ThirdParty/DirectX/DirectXCollision.h"
#endif // #if VKE_USE_DIRECTX_MATH

namespace VKE
{
    namespace Math
    {
#if VKE_USE_DIRECTX_MATH
        using NativeVector4         = DirectX::XMVECTOR;
        using NativeVector3         = DirectX::XMFLOAT3;
        using NativeMatrix4x4       = DirectX::XMFLOAT4X4;
        using NativeMatrix3x3       = DirectX::XMFLOAT3X3;
        using NativeFrustum         = DirectX::BoundingFrustum;
        using NativeAABB            = DirectX::BoundingBox;
        using NativeBoundingSphere  = DirectX::BoundingSphere;
#endif // #if VKE_USE_DIRECTX_MATH
    } // Math
} // VKE