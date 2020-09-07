#pragma once

#include "Types.h"
#include "CVector.h"

namespace VKE
{
    namespace Math
    {
        class VKE_API CBoundingSphere
        {
            public:

                CBoundingSphere() {};
                ~CBoundingSphere() {}
                CBoundingSphere( const CVector3& Center, float radius ) :
                    Position( Center ), radius( radius )
                {}
                //CBoundingSphere(const CAABB& AABB);

                CBoundingSphere& operator=( const CBoundingSphere& Other ) { _Native = Other._Native; return *this; }

                /*CBoundingSphere( const CBoundingSphere& Other );
                CBoundingSphere( CBoundingSphere&& Other );

                CBoundingSphere& operator=( CBoundingSphere&& Other );
                CBoundingSphere& operator=( const CBoundingSphere& Other );

                bool operator==( const CBoundingSphere& Other ) const;
                bool operator!=( const CBoundingSphere& Other ) const;
                bool operator<( const CBoundingSphere& Other ) const;
                bool operator>( const CBoundingSphere& Other ) const;
                bool operator<=( const CBoundingSphere& Other ) const;
                bool operator>=( const CBoundingSphere& Other ) const;*/

                static vke_force_inline void CreateFromAABB(const CAABB& AABB, CBoundingSphere* pOut);
                void vke_force_inline CreateFromAABB(const CAABB& AABB);

                static const CBoundingSphere ONE;
                static const vke_force_inline CBoundingSphere& _One() { return ONE; }

                static vke_force_inline bool Contains(const Math::CVector3& vecCenter, const float radius, const Math::CVector4& vecPoint);

                union
                {
                    struct
                    {
                        CVector3    Position;
                        float       radius;
                    };
                    NativeBoundingSphere    _Native;
                };
        };

    } // Math
} // VKE