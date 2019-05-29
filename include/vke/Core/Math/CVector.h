#pragma once

#include "Types.h"

namespace VKE
{
    namespace Math
    {
        class VKE_API CVector3
        {
            friend class CMatrix4x4;
            friend class CMatrix3x3;

            public:

                CVector3() {}
                vke_force_inline CVector3( float f );
                vke_force_inline CVector3( float x, float y, float z );
                vke_force_inline CVector3( const CVector3& Other );
                ~CVector3() {}

                void vke_force_inline operator=( const CVector3& Other ) { _Native = Other._Native; }
                void vke_force_inline operator=( const float v );
                bool vke_force_inline operator==( const CVector3& Other ) const { return Equals( *this, Other ); }
                bool vke_force_inline operator!=( const CVector3& Other ) const { return !Equals( *this, Other ); }
                bool vke_force_inline operator<( const CVector3& Other ) const { return Less( *this, Other ); }
                bool vke_force_inline operator>( const CVector3& Other ) const { return Greater( *this, Other ); }
                bool vke_force_inline operator<=( const CVector3& Other ) const { return LessEquals( *this, Other ); }
                bool vke_force_inline operator>=( const CVector3& Other ) const { return GreaterEquals( *this, Other ); }

                bool vke_force_inline IsZero() const;

                static vke_force_inline bool    Equals( const CVector3& Left, const CVector3& Right );
                static vke_force_inline bool    Less( const CVector3& Left, const CVector3& Right );
                static vke_force_inline bool    Greater( const CVector3& Left, const CVector3& Right );
                static vke_force_inline bool    LessEquals( const CVector3& Left, const CVector3& Right );
                static vke_force_inline bool    GreaterEquals( const CVector3& Left, const CVector3& Right );

                static vke_force_inline void    Set( const float v, CVector3* pOut );
                static vke_force_inline void    Set( const float x, const float y, const float z, CVector3* pOut );
                static vke_force_inline void    Add( const CVector3& Left, const CVector3& Right, CVector3* pOut );
                static vke_force_inline CVector3 Add( const CVector3& Left, const CVector3& Right );
                static vke_force_inline void    Sub( const CVector3& Left, const CVector3& Right, CVector3* pOut );
                static vke_force_inline CVector3 Sub( const CVector3& Left, const CVector3& Right );
                static vke_force_inline void    Mul( const CVector3& Left, const CVector3& Right, CVector3* pOut );
                static vke_force_inline CVector3 Mul( const CVector3& Left, const CVector3& Right );
                static vke_force_inline void    Div( const CVector3& Left, const CVector3& Right, CVector3* pOut );
                static vke_force_inline CVector3 Div( const CVector3& Left, const CVector3& Right );

                static vke_force_inline const CVector3& _ONE() { return ONE; }
                static vke_force_inline const CVector3& _ZERO() { return ZERO; }
                static vke_force_inline const CVector3& _X() { return X; }
                static vke_force_inline const CVector3& _Y() { return Y; }
                static vke_force_inline const CVector3& _Z() { return Z; }

            public:

                static const CVector3    ONE;
                static const CVector3    ZERO;
                static const CVector3    X;
                static const CVector3    Y;
                static const CVector3    Z;

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
    
        class VKE_API CVector4
        {
            public:

                CVector4() {}
                vke_force_inline CVector4( float f );
                vke_force_inline CVector4( float x, float y, float z, float w );
                vke_force_inline CVector4( const CVector4& Other );
                ~CVector4() {}

                void vke_force_inline operator=( const CVector4& Other ) { _Native = Other._Native; }
                void vke_force_inline operator=( const float v );
                bool vke_force_inline operator==( const CVector4& Other ) const { return Equals( *this, Other ); }
                bool vke_force_inline operator!=( const CVector4& Other ) const { return !Equals( *this, Other ); }
                bool vke_force_inline operator<( const CVector4& Other ) const { return Less( *this, Other ); }
                bool vke_force_inline operator>( const CVector4& Other ) const { return Greater( *this, Other ); }
                bool vke_force_inline operator<=( const CVector4& Other ) const { return LessEquals( *this, Other ); }
                bool vke_force_inline operator>=( const CVector4& Other ) const { return GreaterEquals( *this, Other ); }

                bool vke_force_inline IsZero() const;

                static vke_force_inline bool    Equals( const CVector4& Left, const CVector4& Right );
                static vke_force_inline bool    Less( const CVector4& Left, const CVector4& Right );
                static vke_force_inline bool    Greater( const CVector4& Left, const CVector4& Right );
                static vke_force_inline bool    LessEquals( const CVector4& Left, const CVector4& Right );
                static vke_force_inline bool    GreaterEquals( const CVector4& Left, const CVector4& Right );

                static vke_force_inline void    Set( const float v, CVector4* pOut );
                static vke_force_inline void    Set( const float x, const float y, const float z, CVector4* pOut );
                static vke_force_inline void    Add( const CVector4& Left, const CVector4& Right, CVector4* pOut );
                static vke_force_inline CVector4 Add( const CVector4& Left, const CVector4& Right );
                static vke_force_inline void    Sub( const CVector4& Left, const CVector4& Right, CVector4* pOut );
                static vke_force_inline CVector4 Sub( const CVector4& Left, const CVector4& Right );
                static vke_force_inline void    Mul( const CVector4& Left, const CVector4& Right, CVector4* pOut );
                static vke_force_inline CVector4 Mul( const CVector4& Left, const CVector4& Right );
                static vke_force_inline void    Div( const CVector4& Left, const CVector4& Right, CVector4* pOut );
                static vke_force_inline CVector4 Div( const CVector4& Left, const CVector4& Right );

                static vke_force_inline const CVector4& _ONE() { return ONE; }
                static vke_force_inline const CVector4& _ZERO() { return ZERO; }
                static vke_force_inline const CVector4& _X() { return X; }
                static vke_force_inline const CVector4& _Y() { return Y; }
                static vke_force_inline const CVector4& _Z() { return Z; }
                static vke_force_inline const CVector4& _W() { return W; }

            public:

                static const CVector4    ONE;
                static const CVector4    ZERO;
                static const CVector4    X;
                static const CVector4    Y;
                static const CVector4    Z;
                static const CVector4    W;

                union
                {
                    float       x, y, z, w;
                };
                float           data[4];
                NativeVector4   _Native;
        };
    } // Math

} // VKE

#include "DirectX/CVector.inl"