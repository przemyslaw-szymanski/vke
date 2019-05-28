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

                CAABB() {}
                constexpr CAABB( const CVector& Center, const CVector& Extents );
                CAABB( const CAABB& ) = default;
                CAABB( CAABB&& ) = default;
                ~CAABB() {}

                CAABB& operator=( const CAABB& ) = default;
                CAABB& operator=( CAABB&& ) = default;

                void vke_force_inline CalcMinMax( CVector* pMinOut, CVector* pMaxOut );
                void vke_force_inline CalcCorners( CVector* pOut );

                static void vke_force_inline Transform( const CMatrix4x4& Matrix, CAABB* pOut );
                static void vke_force_inline Transform( const float scale, const CVector& Rotation,
                                                        const CVector& Translation, CAABB* pOut );

                static const CAABB ONE;

                static const vke_force_inline CAABB& _One() { return ONE; }

                union
                {
                    NativeAABB  _Native;
                };
        };

#if VKE_USE_DIRECTX_MATH
        constexpr CAABB::CAABB( const CVector& Center, const CVector& Extents ) :
            _Native{ Center._Native, Extents._Native }
        {}
#endif
    } // Math
} // VKE

#include "DirectX/CAABB.inl"