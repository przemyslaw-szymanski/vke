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
                CBoundingSphere( const CVector& Center, float radius ) :
                    Position( Center ), radius( radius )
                {}

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

                static const CBoundingSphere ONE;
                static const vke_force_inline CBoundingSphere& _One() { return ONE; }

                union
                {
                    struct
                    {
                        CVector Position;
                        float   radius;
                    };
                    NativeBoundingSphere    _Native;
                };
        };

    } // Math
} // VKE

#include "DirectX/CBoundingSphere.inl"