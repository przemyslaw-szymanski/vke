#pragma once

#include "Types.h"
#include "CBoundingSphere.h"
#include "CAABB.h"
#include "CVector.h"
#include "CMatrix.h"
#include "CQuaternion.h"

namespace VKE
{
    namespace Math
    {
        class CFrustum
        {
            public:

                CFrustum() {}

                CFrustum& operator=( const CFrustum& Other )
                {
                    this->_Native = Other._Native;
                    for( uint32_t i = 0; i < 6; ++i )
                    {
                        this->aPlanes[ i ] = Other.aPlanes[ i ];
                    }
                    for( uint32_t i = 0; i < 8; ++i )
                    {
                        this->aCorners[ i ] = Other.aCorners[ i ];
                    }
                    return *this;
                }
                //CFrustum& operator=( CFrustum&& Other );
                template<bool Update = true>
                void vke_force_inline   CreateFromMatrix( const CMatrix4x4& Matrix );

                void vke_force_inline   Transform( const CMatrix4x4& Matrix );
                void vke_force_inline   Transform( const CVector3& Translation, const CVector4& Rotation, float scale );

                bool vke_force_inline   Intersects( const CBoundingSphere& Sphere ) const;
                bool vke_force_inline   Intersects( const CAABB& AABB ) const;

                template<bool Update = true>
                void vke_force_inline   SetOrientation( const CVector3& vecPosition, const CQuaternion& quatRotation );
                template<bool Update = true>
                void vke_force_inline   SetFar(const float far);
                template<bool Update = true>
                void vke_force_inline   SetNear(const float near);

                void vke_force_inline   Update();

                void vke_force_inline   CalcCorners( CVector3* pOut ) const;

                void vke_force_inline   CalcMatrix( CMatrix4x4* pOut ) const;

                struct Corners
                {
                    enum CORNER
                    {
                        LEFT_TOP_NEAR,
                        RIGHT_TOP_NEAR,
                        RIGHT_BOTTOM_NEAR,
                        LEFT_BOTTOM_NEAR,
                        LEFT_TOP_FAR,
                        RIGHT_TOP_FAR,
                        RIGHT_BOTTOM_FAR,
                        LEFT_BOTTOM_FAR
                    };
                };

                CVector4 aPlanes[ 6 ];
                CVector4 aCorners[ 8 ];
                union
                {
                    NativeFrustum   _Native;
                };
        };
    } // Math
} // VKE

//#include "DirectX/CFrustum.inl"