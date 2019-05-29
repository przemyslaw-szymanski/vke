#pragma once

#include "Types.h"
#include "CBoundingSphere.h"
#include "CAABB.h"
#include "CVector.h"
#include "CMatrix.h"

namespace VKE
{
    namespace Math
    {
        class CFrustum
        {
            public:

                CFrustum() {}

                void vke_force_inline   Transform( const CMatrix4x4& Matrix );
                void vke_force_inline   Transform( const CVector3& Translation, const CVector3& Rotation, float scale );

                bool vke_force_inline   Intersects( const CBoundingSphere& Sphere ) const;
                bool vke_force_inline   Intersects( const CAABB& AABB ) const;

                CVector4    Planes[6];
                
                union
                {
                    NativeFrustum   _Native;
                };
        };
    } // Math
} // VKE

#include "DirectX/CFrustum.inl"