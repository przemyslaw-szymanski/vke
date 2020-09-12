#pragma once

#include "Types.h"
#include "CVector.h"
#include "CMatrix.h"
#include "CBoundingSphere.h"

namespace VKE
{
    namespace Math
    {
        class VKE_API CAABB
        {
            public:

                struct SMinMax
                {
                    CVector3    vec3Min;
                    CVector3    vec3Max;
                };

            public:

                CAABB() {};
                constexpr CAABB( const CVector3& Center, const CVector3& Extents );
                CAABB( const CAABB& Other ) : _Native{ Other._Native } {}
                CAABB( CAABB&& Other ) : _Native{ Other._Native } {}
                ~CAABB() {}

                CAABB& operator=( const CAABB& Other ) { _Native = Other._Native;  return *this; }
                CAABB& operator=( CAABB&& Other ) { _Native = Other._Native;  return *this; }

                void vke_force_inline CalcMinMax( SMinMax* pOut ) const;
                void vke_force_inline CalcCorners( CVector3* pOut ) const;
                void vke_force_inline CalcSphere( Math::CBoundingSphere* pOut ) const;
                INTERSECT_RESULT vke_force_inline Contains( const CAABB& Other ) const;
                INTERSECT_RESULT vke_force_inline Contains( const CBoundingSphere& Sphere ) const;
                INTERSECT_RESULT vke_force_inline Contains( const CVector3& Point ) const;
                INTERSECT_RESULT vke_force_inline Contains( const CVector4& Point ) const;
                INTERSECT_RESULT vke_force_inline Contains( const CVector3& Vec1, const CVector3& Vec2, const CVector3& Vec3 ) const;
                void vke_force_inline CalcCenter( CVector3* pOut ) const;
                void vke_force_inline CalcCenter( CVector4* pOut ) const;
                void vke_force_inline CalcExtents( CVector3* pOut ) const;
                void vke_force_inline CalcExtents( CVector4* pOut ) const;
                bool vke_force_inline IsZero() const { return Extents == ZERO.Extents; }

                static void vke_force_inline Transform( const CMatrix4x4& Matrix, CAABB* pOut );
                static void vke_force_inline Transform( const float scale, const CVector3& Translation, CAABB* pOut );

                static const CAABB ONE;
                static const CAABB ZERO;

                static const vke_force_inline CAABB& _One() { return ONE; }
                static const vke_force_inline CAABB& _Zero() { return ZERO; }

                struct Corners
                {
                    enum CORNER
                    {
                        LEFT_BOTTOM_FAR,
                        RIGHT_BOTTOM_FAR,
                        RIGHT_TOP_FAR,
                        LEFT_TOP_FAR,

                        LEFT_BOTTOM_NEAR,
                        RIGHT_BOTTOM_NEAR,
                        RIGHT_TOP_NEAR,
                        LEFT_TOP_NEAR
                    };
                };

                union
                {
                    struct
                    {
                        CVector3    Center;
                        CVector3    Extents;
                    };
                    NativeAABB  _Native;
                };
        };

#if VKE_USE_DIRECTX_MATH
        constexpr CAABB::CAABB( const CVector3& Center, const CVector3& Extents ) :
            _Native{ Center._Native, Extents._Native }
        {}
#endif
    } // Math
} // VKE

//#include "DirectX/CAABB.inl"