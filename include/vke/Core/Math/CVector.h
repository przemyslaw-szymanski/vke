#pragma once

#include "Types.h"

namespace VKE
{
    namespace Math
    {
        class CVector4;

        class VKE_API CVector3
        {
            friend class CMatrix4x4;
            friend class CMatrix3x3;

            public:

                CVector3() {}
                vke_force_inline constexpr CVector3( float f );
                vke_force_inline constexpr CVector3( float x, float y, float z );
                vke_force_inline CVector3( const CVector3& Other ) = default;
                vke_force_inline CVector3( CVector3&& ) = default;
                vke_force_inline explicit CVector3( const CVector4& Other );
                ~CVector3() {}

                //void vke_force_inline operator=( const CVector3& Other ) { _Native = Other._Native; }
                CVector3& operator=( const CVector3& ) = default;
                CVector3& operator=( CVector3&& ) = default;

                CVector3 vke_force_inline operator+( const CVector3& Right ) const;
                CVector3 vke_force_inline operator-( const CVector3& Right ) const;
                CVector3 vke_force_inline operator*( const CVector3& Right ) const;
                CVector3 vke_force_inline operator/( const CVector3& Right ) const;
                CVector3 vke_force_inline operator-() const;
                CVector3 vke_force_inline operator+() const;
                void vke_force_inline operator+=( const CVector3& Right );
                void vke_force_inline operator-=( const CVector3& Right );
                void vke_force_inline operator*=( const CVector3& Right );
                void vke_force_inline operator/=( const CVector3& Right );

                bool vke_force_inline operator==( const CVector3& Other ) const { return Equals( *this, Other ); }
                bool vke_force_inline operator!=( const CVector3& Other ) const { return !Equals( *this, Other ); }
                bool vke_force_inline operator<( const CVector3& Other ) const { return Less( *this, Other ); }
                bool vke_force_inline operator>( const CVector3& Other ) const { return Greater( *this, Other ); }
                bool vke_force_inline operator<=( const CVector3& Other ) const { return LessOrEquals( *this, Other ); }
                bool vke_force_inline operator>=( const CVector3& Other ) const { return GreaterOrEquals( *this, Other ); }

                bool vke_force_inline IsZero() const;
                void vke_force_inline ConvertToVector4( CVector4* pOut ) const;
                void vke_force_inline ConvertCompareToBools( bool** ppOut ) const;
                void vke_force_inline ConvertToRadians( CVector3* pOut ) const;
                void vke_force_inline ConvertToDegrees( CVector3* pOut ) const;
                void vke_force_inline Normalize( CVector3* pOut ) const;
                void vke_force_inline Normalize();
                void vke_force_inline Sin( CVector3* pOut ) const;
                void vke_force_inline Cos( CVector3* pOut ) const;
                float vke_force_inline Dot( const CVector3& Other ) const;
                

                static vke_force_inline bool    Equals( const CVector3& Left, const CVector3& Right );
                static vke_force_inline bool    Less( const CVector3& Left, const CVector3& Right );
                static vke_force_inline bool    Greater( const CVector3& Left, const CVector3& Right );
                static vke_force_inline bool    LessOrEquals( const CVector3& Left, const CVector3& Right );
                static vke_force_inline bool    GreaterOrEquals( const CVector3& Left, const CVector3& Right );

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

                static vke_force_inline void    Less( const CVector3& Left, const CVector3& Right, CVector3* pOut );
                static vke_force_inline void    LessOrEquals( const CVector3& Left, const CVector3& Right, CVector3* pOut );
                static vke_force_inline void    Greater( const CVector3& Left, const CVector3& Right, CVector3* pOut );
                static vke_force_inline void    GreaterOrEquals( const CVector3& Left, const CVector3& Right, CVector3* pOut );

                static void vke_force_inline    Cross( const CVector3& V1, const CVector3& V2, CVector3* pOut );
                static void vke_force_inline    Dot( const CVector3& V1, const CVector3& V2, CVector3* pOut );
                static float vke_force_inline   Dot( const CVector3& V1, const CVector3& V2 );

                static vke_force_inline const CVector3& _ONE() { return ONE; }
                static vke_force_inline const CVector3& _NEGATIVE_ONE() { return NEGATIVE_ONE; }
                static vke_force_inline const CVector3& _ZERO() { return ZERO; }
                static vke_force_inline const CVector3& _X() { return X; }
                static vke_force_inline const CVector3& _Y() { return Y; }
                static vke_force_inline const CVector3& _Z() { return Z; }
                static vke_force_inline const CVector3& _NEGATIVE_X() { return NEGATIVE_X; }
                static vke_force_inline const CVector3& _NEGATIVE_Y() { return NEGATIVE_Y; }
                static vke_force_inline const CVector3& _NEGATIVE_Z() { return NEGATIVE_Z; }

            public:

                static const CVector3    ONE;
                static const CVector3    NEGATIVE_ONE;
                static const CVector3    ZERO;
                static const CVector3    X;
                static const CVector3    Y;
                static const CVector3    Z;
                static const CVector3    NEGATIVE_X;
                static const CVector3    NEGATIVE_Y;
                static const CVector3    NEGATIVE_Z;

                union
                {
                    struct
                    {
                        float       x, y, z;
                    };
                    float           floats[3];
                    int32_t         ints[ 3 ];
                    uint32_t        uints[ 3 ];
                    NativeVector3   _Native;
                };
        };
    
        VKE_ALIGN(16) class VKE_API CVector4
        {
            public:

                CVector4() {}
                explicit vke_force_inline CVector4( float f );
                vke_force_inline CVector4( float x, float y, float z, float w );
                vke_force_inline CVector4( const CVector4& Other );
                explicit vke_force_inline CVector4( const NativeVector4& Other );
                explicit vke_force_inline constexpr CVector4( const CVector3& Other );
                ~CVector4() {}

                void vke_force_inline operator=( const CVector4& Other ) { _Native = Other._Native; }
                void vke_force_inline operator=( const float v );
                bool vke_force_inline operator==( const CVector4& Other ) const { return Equals( *this, Other ); }
                bool vke_force_inline operator!=( const CVector4& Other ) const { return !Equals( *this, Other ); }
                bool vke_force_inline operator<( const CVector4& Other ) const { return Less( *this, Other ); }
                bool vke_force_inline operator>( const CVector4& Other ) const { return Greater( *this, Other ); }
                bool vke_force_inline operator<=( const CVector4& Other ) const { return LessEquals( *this, Other ); }
                bool vke_force_inline operator>=( const CVector4& Other ) const { return GreaterEquals( *this, Other ); }
                CVector4 vke_force_inline operator+( const CVector4& Right ) const;
                CVector4 vke_force_inline operator-( const CVector4& Right ) const;
                CVector4 vke_force_inline operator*( const CVector4& Right ) const;
                CVector4 vke_force_inline operator/( const CVector4& Right ) const;
                void vke_force_inline operator+=( const CVector4& Right );
                void vke_force_inline operator-=( const CVector4& Right );
                void vke_force_inline operator*=( const CVector4& Right );
                void vke_force_inline operator/=( const CVector4& Right );
                CVector4 vke_force_inline operator&( const CVector4& Other ) const;

                void vke_force_inline Normalize();
                
                void vke_force_inline ConvertToInts( int32_t* pInts ) const;
                void vke_force_inline ConvertToUInts( uint32_t* pUInts ) const;

                bool vke_force_inline IsZero() const;
                void vke_force_inline ConvertToVector3( CVector3* pOut ) const;
                void vke_force_inline ConvertCompareToBools( bool* pOut ) const;

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
                static vke_force_inline void    Less( const CVector4& Left, const CVector4& Right, CVector4* pOut );
                static vke_force_inline void    LessOrEquals( const CVector4& Left, const CVector4& Right, CVector4* pOut );
                static vke_force_inline void    Greater( const CVector4& Left, const CVector4& Right, CVector4* pOut );
                static vke_force_inline void    GreaterOrEquals( const CVector4& Left, const CVector4& Right, CVector4* pOut );
                static vke_force_inline void    Mad( const CVector4& V1, const CVector4& V2, const CVector4& V3, CVector4* pOut );
                static vke_force_inline void    Max( const CVector4& V1, const CVector4& V2, CVector4* pOut );
                static vke_force_inline void    Min( const CVector4& V1, const CVector4& V2, CVector4* pOut );

                static vke_force_inline void    Normalize( const CVector4& V, CVector4* pOut );

                static vke_force_inline void    And( const CVector4& Left, const CVector4& Right, CVector4* pOut );
                static vke_force_inline int32_t MoveMask( const CVector4& Vec );

                static vke_force_inline void    Clamp( const CVector4& V, const CVector4& Min, const CVector4& Max, CVector4* pOut );
                static vke_force_inline void    Saturate( const CVector4& V, CVector4* pOut );

                template<uint32_t DivExponent = 16>
                static vke_force_inline void    ConvertUintToFloat( const CVector4& V, CVector4* pOut );
                template<uint32_t MulExponent = 16>
                static vke_force_inline void    ConvertFloatToUInt( const CVector4& V, CVector4* pOut );

                static vke_force_inline const CVector4& _ONE() { return ONE; }
                static vke_force_inline const CVector4& _NEGATIVE_ONE() { return NEGATIVE_ONE; }
                static vke_force_inline const CVector4& _ZERO() { return ZERO; }
                static vke_force_inline const CVector4& _X() { return X; }
                static vke_force_inline const CVector4& _Y() { return Y; }
                static vke_force_inline const CVector4& _Z() { return Z; }
                static vke_force_inline const CVector4& _W() { return W; }
                static vke_force_inline const CVector4& _NEGATIVE_X() { return NEGATIVE_X; }
                static vke_force_inline const CVector4& _NEGATIVE_Y() { return NEGATIVE_Y; }
                static vke_force_inline const CVector4& _NEGATIVE_Z() { return NEGATIVE_Z; }
                static vke_force_inline const CVector4& _NEGATIVE_W() { return NEGATIVE_W; }

            public:

                static const CVector4    ONE;
                static const CVector4    NEGATIVE_ONE;
                static const CVector4    ZERO;
                static const CVector4    X;
                static const CVector4    Y;
                static const CVector4    Z;
                static const CVector4    W;
                static const CVector4    NEGATIVE_X;
                static const CVector4    NEGATIVE_Y;
                static const CVector4    NEGATIVE_Z;
                static const CVector4    NEGATIVE_W;

                VKE_ALIGN( 16 ) union
                {
                    VKE_ALIGN( 16 ) struct
                    {
                        float       x, y, z, w;
                    };
                    VKE_ALIGN( 16 ) float       floats[ 4 ];
                    VKE_ALIGN( 16 ) int32_t     ints[ 4 ];
                    VKE_ALIGN( 16 ) uint32_t    uints[ 4 ];
                    NativeVector4               _Native;
                };
        };
    } // Math

} // VKE

#include "DirectX/CVector.inl"