#pragma once

#include "Types.h"

namespace VKE
{
    namespace Math
    {
        class VKE_API CVector
        {
            friend class CMatrix4x4;
            friend class CMatrix3x3;

            public:

                CVector() {}
                vke_force_inline CVector( float f );
                vke_force_inline CVector( float x, float y, float z );
                vke_force_inline CVector( const CVector& Other );
                ~CVector() {}

                void vke_force_inline operator=( const CVector& Other ) { _Native = Other._Native; }
                void vke_force_inline operator=( const float v );
                bool vke_force_inline operator==( const CVector& Other ) const { return Equals( *this, Other ); }
                bool vke_force_inline operator!=( const CVector& Other ) const { return !Equals( *this, Other ); }
                bool vke_force_inline operator<( const CVector& Other ) const { return Less( *this, Other ); }
                bool vke_force_inline operator>( const CVector& Other ) const { return Greater( *this, Other ); }
                bool vke_force_inline operator<=( const CVector& Other ) const { return LessEquals( *this, Other ); }
                bool vke_force_inline operator>=( const CVector& Other ) const { return GreaterEquals( *this, Other ); }

                bool vke_force_inline IsZero() const;

                static vke_force_inline bool    Equals( const CVector& Left, const CVector& Right );
                static vke_force_inline bool    Less( const CVector& Left, const CVector& Right );
                static vke_force_inline bool    Greater( const CVector& Left, const CVector& Right );
                static vke_force_inline bool    LessEquals( const CVector& Left, const CVector& Right );
                static vke_force_inline bool    GreaterEquals( const CVector& Left, const CVector& Right );

                static vke_force_inline void    Set( const float v, CVector* pOut );
                static vke_force_inline void    Set( const float x, const float y, const float z, CVector* pOut );
                static vke_force_inline void    Add( const CVector& Left, const CVector& Right, CVector* pOut );
                static vke_force_inline CVector Add( const CVector& Left, const CVector& Right );
                static vke_force_inline void    Sub( const CVector& Left, const CVector& Right, CVector* pOut );
                static vke_force_inline CVector Sub( const CVector& Left, const CVector& Right );
                static vke_force_inline void    Mul( const CVector& Left, const CVector& Right, CVector* pOut );
                static vke_force_inline CVector Mul( const CVector& Left, const CVector& Right );
                static vke_force_inline void    Div( const CVector& Left, const CVector& Right, CVector* pOut );
                static vke_force_inline CVector Div( const CVector& Left, const CVector& Right );

                static vke_force_inline const CVector& _ONE() { return ONE; }
                static vke_force_inline const CVector& _ZERO() { return ZERO; }
                static vke_force_inline const CVector& _X() { return X; }
                static vke_force_inline const CVector& _Y() { return Y; }
                static vke_force_inline const CVector& _Z() { return Z; }

            public:

                static const CVector    ONE;
                static const CVector    ZERO;
                static const CVector    X;
                static const CVector    Y;
                static const CVector    Z;

                union
                {
                    struct
                    {
                        float       x, y, z;
                    };
                    float           data[3];
                    NativeVector3   _Native;
                };
        };
    } // Math

} // VKE

#include "DirectX/CVector.inl"