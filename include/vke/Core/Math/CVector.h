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
            CVector3() = default;
            vke_force_inline constexpr CVector3( float f );
            vke_force_inline constexpr CVector3( float x, float y, float z );
            vke_force_inline CVector3( const CVector3& Other ) = default;
            vke_force_inline CVector3( CVector3&& )            = default;

            vke_force_inline explicit constexpr CVector3( const NativeVector3& V ) : _Native{ V }
            {
            }

            vke_force_inline explicit CVector3( const CVector4& Other );
            ~CVector3() = default;

            // vke_force_inline void vke_vectorcall operator=( const CVector3& Other ) { _Native = Other._Native; }
            CVector3& operator=( const CVector3& Other )
            {
                x = Other.x;
                y = Other.y;
                z = Other.z;
                return *this;
            }

            CVector3& operator=( CVector3&& Other )
            {
                x = Other.x;
                y = Other.y;
                z = Other.z;
                return *this;
            }

            CVector3 vke_force_inline            operator+( const CVector3& Right ) const;
            CVector3 vke_force_inline            operator-( const CVector3& Right ) const;
            CVector3 vke_force_inline            operator*( const CVector3& Right ) const;
            CVector3 vke_force_inline            operator/( const CVector3& Right ) const;
            CVector3 vke_force_inline            operator-() const;
            CVector3 vke_force_inline            operator+() const;
            vke_force_inline void vke_vectorcall operator+=( const CVector3& Right );
            vke_force_inline void vke_vectorcall operator-=( const CVector3& Right );
            vke_force_inline void vke_vectorcall operator*=( const CVector3& Right );
            vke_force_inline void vke_vectorcall operator/=( const CVector3& Right );

            vke_force_inline bool vke_vectorcall operator==( const CVector3& Other ) const
            {
                return Equals( *this, Other );
            }

            vke_force_inline bool vke_vectorcall operator!=( const CVector3& Other ) const
            {
                return !Equals( *this, Other );
            }

            vke_force_inline bool vke_vectorcall operator<( const CVector3& Other ) const
            {
                return Less( *this, Other );
            }

            vke_force_inline bool vke_vectorcall operator>( const CVector3& Other ) const
            {
                return Greater( *this, Other );
            }

            vke_force_inline bool vke_vectorcall operator<=( const CVector3& Other ) const
            {
                return LessOrEquals( *this, Other );
            }

            vke_force_inline bool vke_vectorcall operator>=( const CVector3& Other ) const
            {
                return GreaterOrEquals( *this, Other );
            }

            vke_force_inline bool vke_vectorcall IsZero() const;
            vke_force_inline void vke_vectorcall ConvertToVector4( CVector4* pOut ) const;
            vke_force_inline void vke_vectorcall ConvertCompareToBools( bool** ppOut ) const;
            vke_force_inline void vke_vectorcall ConvertToRadians( CVector3* pOut ) const;
            vke_force_inline void vke_vectorcall ConvertToDegrees( CVector3* pOut ) const;
            vke_force_inline void vke_vectorcall Normalize( CVector3* pOut ) const;
            vke_force_inline void vke_vectorcall Normalize();
            vke_force_inline void vke_vectorcall Sin( CVector3* pOut ) const;
            vke_force_inline void vke_vectorcall Cos( CVector3* pOut ) const;
            float vke_force_inline               Dot( const CVector3& Other ) const;

            vke_force_inline static bool vke_vectorcall Equals( const CVector3& Left, const CVector3& Right );
            vke_force_inline static bool vke_vectorcall Less( const CVector3& Left, const CVector3& Right );
            vke_force_inline static bool vke_vectorcall Greater( const CVector3& Left, const CVector3& Right );
            vke_force_inline static bool vke_vectorcall LessOrEquals( const CVector3& Left, const CVector3& Right );
            vke_force_inline static bool vke_vectorcall GreaterOrEquals( const CVector3& Left, const CVector3& Right );

            vke_force_inline static void vke_vectorcall Set( const float v, CVector3* pOut );
            vke_force_inline static void vke_vectorcall Set( const float x, const float y, const float z,
                                                             CVector3* pOut );
            vke_force_inline static void vke_vectorcall Add( const CVector3& Left, const CVector3& Right,
                                                             CVector3* pOut );
            static vke_force_inline CVector3            Add( const CVector3& Left, const CVector3& Right );
            vke_force_inline static void vke_vectorcall Sub( const CVector3& Left, const CVector3& Right,
                                                             CVector3* pOut );
            static vke_force_inline CVector3            Sub( const CVector3& Left, const CVector3& Right );
            vke_force_inline static void vke_vectorcall Mul( const CVector3& Left, const CVector3& Right,
                                                             CVector3* pOut );
            static vke_force_inline CVector3            Mul( const CVector3& Left, const CVector3& Right );
            vke_force_inline static void vke_vectorcall Div( const CVector3& Left, const CVector3& Right,
                                                             CVector3* pOut );
            static vke_force_inline CVector3            Div( const CVector3& Left, const CVector3& Right );
            vke_force_inline static void vke_vectorcall Mad( const CVector3& V1, const CVector3& V2, const CVector3& V3,
                                                             CVector3* pOut );
            vke_force_inline static void vke_vectorcall Mad( const CVector4& V1, const CVector4& V2, const CVector4& V3,
                                                             CVector3* pOut );

            vke_force_inline static void vke_vectorcall Less( const CVector3& Left, const CVector3& Right,
                                                              CVector3* pOut );
            vke_force_inline static void vke_vectorcall LessOrEquals( const CVector3& Left, const CVector3& Right,
                                                                      CVector3* pOut );
            vke_force_inline static void vke_vectorcall Greater( const CVector3& Left, const CVector3& Right,
                                                                 CVector3* pOut );
            vke_force_inline static void vke_vectorcall GreaterOrEquals( const CVector3& Left, const CVector3& Right,
                                                                         CVector3* pOut );

            vke_force_inline static void vke_vectorcall vke_vectorcall Cross( const CVector3& V1, const CVector3& V2,
                                                                              CVector3* pOut );
            vke_force_inline static void vke_vectorcall vke_vectorcall Dot( const CVector3& V1, const CVector3& V2,
                                                                            CVector3* pOut );
            static float vke_force_inline                              Dot( const CVector3& V1, const CVector3& V2 );

            static float vke_force_inline Length( const CVector3& V );
            static float vke_force_inline Distance( const CVector3& V1, const CVector3& V2 );

            static vke_force_inline const CVector3& _ONE()
            {
                return ONE;
            }

            static vke_force_inline const CVector3& _NEGATIVE_ONE()
            {
                return NEGATIVE_ONE;
            }

            static vke_force_inline const CVector3& _ZERO()
            {
                return ZERO;
            }

            static vke_force_inline const CVector3& _X()
            {
                return X;
            }

            static vke_force_inline const CVector3& _Y()
            {
                return Y;
            }

            static vke_force_inline const CVector3& _Z()
            {
                return Z;
            }

            static vke_force_inline const CVector3& _NEGATIVE_X()
            {
                return NEGATIVE_X;
            }

            static vke_force_inline const CVector3& _NEGATIVE_Y()
            {
                return NEGATIVE_Y;
            }

            static vke_force_inline const CVector3& _NEGATIVE_Z()
            {
                return NEGATIVE_Z;
            }

        public:
            static const CVector3 ONE;
            static const CVector3 NEGATIVE_ONE;
            static const CVector3 ZERO;
            static const CVector3 X;
            static const CVector3 Y;
            static const CVector3 Z;
            static const CVector3 NEGATIVE_X;
            static const CVector3 NEGATIVE_Y;
            static const CVector3 NEGATIVE_Z;

            union
            {
                struct
                {
                    float x, y, z;
                };

                struct
                {
                    float width, height, depth;
                };

                float         floats[ 3 ];
                int32_t       ints[ 3 ];
                uint32_t      uints[ 3 ];
                NativeVector3 _Native;
            };
        };

        class alignas( 16 ) VKE_API CVector4
        {
        public:
#if defined( VKE_SIMD )
            using CVector4Ref = CVector4;
#else
            using CVector4In = CVector4Ref;
#endif
        public:
            CVector4()
            {
            }

            explicit vke_force_inline CVector4( float f );
            vke_force_inline constexpr CVector4( float x, float y, float z, float w );
            vke_force_inline constexpr CVector4( const CVector4& Other );
            explicit vke_force_inline constexpr CVector4( NativeVector4Ref Other );
            explicit vke_force_inline constexpr CVector4( const CVector3& Other );

            ~CVector4()
            {
            }

            vke_force_inline void vke_vectorcall operator=( CVector4Ref Other )
            {
                _Native = Other._Native;
            }

            vke_force_inline void vke_vectorcall operator=( const float v );
            vke_force_inline void vke_vectorcall operator=( const CVector3& V );

            vke_force_inline bool vke_vectorcall     operator==( CVector4Ref Other ) const;
            vke_force_inline bool vke_vectorcall     operator!=( CVector4Ref Other ) const;
            vke_force_inline bool vke_vectorcall     operator<( CVector4Ref Other ) const;
            vke_force_inline bool vke_vectorcall     operator>( CVector4Ref Other ) const;
            vke_force_inline bool vke_vectorcall     operator<=( CVector4Ref Other ) const;
            vke_force_inline bool vke_vectorcall     operator>=( CVector4Ref Other ) const;
            vke_force_inline CVector4 vke_vectorcall operator+( CVector4Ref Right ) const;
            vke_force_inline CVector4 vke_vectorcall operator-( CVector4Ref Right ) const;
            vke_force_inline CVector4 vke_vectorcall operator*( CVector4Ref Right ) const;
            vke_force_inline CVector4 vke_vectorcall operator/( CVector4Ref Right ) const;
            vke_force_inline void vke_vectorcall     operator+=( CVector4Ref Right );
            vke_force_inline void vke_vectorcall     operator-=( CVector4Ref Right );
            vke_force_inline void vke_vectorcall     operator*=( CVector4Ref Right );
            vke_force_inline void vke_vectorcall     operator/=( CVector4Ref Right );
            vke_force_inline CVector4 vke_vectorcall operator+( const float Right ) const;
            vke_force_inline CVector4 vke_vectorcall operator-( const float Right ) const;
            vke_force_inline CVector4 vke_vectorcall operator*( const float Right ) const;
            vke_force_inline CVector4 vke_vectorcall operator/( const float Right ) const;
            vke_force_inline void vke_vectorcall     operator+=( const float Right );
            vke_force_inline void vke_vectorcall     operator-=( const float Right );
            vke_force_inline void vke_vectorcall     operator*=( const float Right );
            vke_force_inline void vke_vectorcall     operator/=( const float Right );
            vke_force_inline CVector4 vke_vectorcall operator&( CVector4Ref Other ) const;

            vke_force_inline void vke_vectorcall Normalize();

            vke_force_inline void vke_vectorcall ConvertToInts( int32_t* pInts ) const;
            vke_force_inline void vke_vectorcall ConvertToUInts( uint32_t* pUInts ) const;

            vke_force_inline bool vke_vectorcall IsZero() const;
            vke_force_inline void vke_vectorcall ConvertToVector3( CVector3* pOut ) const;
            vke_force_inline void vke_vectorcall ConvertCompareToBools( bool* pOut ) const;

            vke_force_inline static CVector4 vke_vectorcall Equals( CVector4Ref Left, CVector4Ref Right );
            vke_force_inline static CVector4 vke_vectorcall Less( CVector4Ref Left, CVector4Ref Right );
            vke_force_inline static CVector4 vke_vectorcall Greater( CVector4Ref Left, CVector4Ref Right );
            vke_force_inline static CVector4 vke_vectorcall LessEquals( CVector4Ref Left, CVector4Ref Right );
            vke_force_inline static CVector4 vke_vectorcall GreaterEquals( CVector4Ref Left, CVector4Ref Right );

            vke_force_inline static void vke_vectorcall     Set( const float v, CVector4* pOut );
            vke_force_inline static void vke_vectorcall     Set( const float x, const float y, const float z,
                                                                 CVector4* pOut );
            vke_force_inline static void vke_vectorcall     Add( CVector4Ref Left, CVector4Ref Right, CVector4* pOut );
            vke_force_inline static CVector4 vke_vectorcall Add( CVector4Ref Left, CVector4Ref Right );
            vke_force_inline static void vke_vectorcall     Sub( CVector4Ref Left, CVector4Ref Right, CVector4* pOut );
            vke_force_inline static CVector4 vke_vectorcall Sub( CVector4Ref Left, CVector4Ref Right );
            vke_force_inline static void vke_vectorcall     Mul( CVector4Ref Left, CVector4Ref Right, CVector4* pOut );
            vke_force_inline static CVector4 vke_vectorcall Mul( CVector4Ref Left, CVector4Ref Right );
            vke_force_inline static void vke_vectorcall     Div( CVector4Ref Left, CVector4Ref Right, CVector4* pOut );
            vke_force_inline static CVector4 vke_vectorcall Div( CVector4Ref Left, CVector4Ref Right );
            vke_force_inline static void vke_vectorcall     Less( CVector4Ref Left, CVector4Ref Right, CVector4* pOut );
            vke_force_inline static void vke_vectorcall     LessOrEquals( CVector4Ref Left, CVector4Ref Right,
                                                                          CVector4* pOut );
            vke_force_inline static void vke_vectorcall Greater( CVector4Ref Left, CVector4Ref Right, CVector4* pOut );
            vke_force_inline static void vke_vectorcall GreaterOrEquals( CVector4Ref Left, CVector4Ref Right,
                                                                         CVector4* pOut );
            vke_force_inline static void vke_vectorcall Mad( CVector4Ref V1, CVector4Ref V2, CVector4Ref V3,
                                                             CVector4* pOut );
            vke_force_inline static void vke_vectorcall Max( CVector4Ref V1, CVector4Ref V2, CVector4* pOut );
            vke_force_inline static void vke_vectorcall Min( CVector4Ref V1, CVector4Ref V2, CVector4* pOut );

            vke_force_inline static void vke_vectorcall Normalize( CVector4Ref V, CVector4* pOut );
            vke_force_inline static void vke_vectorcall Cross( CVector4Ref V1, CVector4Ref V2, CVector4* pOut );

            vke_force_inline static void vke_vectorcall    And( CVector4Ref Left, CVector4Ref Right, CVector4* pOut );
            vke_force_inline static int32_t vke_vectorcall MoveMask( CVector4Ref Vec );

            vke_force_inline static void vke_vectorcall Clamp( CVector4Ref V, CVector4Ref Min, CVector4Ref Max,
                                                               CVector4* pOut );
            vke_force_inline static void vke_vectorcall Saturate( CVector4Ref V, CVector4* pOut );

            vke_force_inline static float vke_vectorcall Length( CVector4Ref V );
            vke_force_inline static float vke_vectorcall Distance( CVector4Ref V1, CVector4Ref V2 );

            vke_force_inline static void vke_vectorcall     Abs( CVector4Ref V, CVector4* pOut );
            vke_force_inline static CVector4 vke_vectorcall Abs( CVector4Ref V );

            vke_force_inline static void vke_vectorcall vke_vectorcall Sqrt( CVector4Ref V, CVector4* pOut );

            vke_force_inline static void vke_vectorcall vke_vectorcall Dot( CVector4Ref V1, CVector4Ref V2,
                                                                            CVector4* pOut );
            static float vke_force_inline                              Dot( CVector4Ref V1, CVector4Ref V2 );

            template< uint32_t DivExponent = 16 >
            vke_force_inline static void vke_vectorcall ConvertUintToFloat( CVector4Ref V, CVector4* pOut );
            template< uint32_t MulExponent = 16 >
            vke_force_inline static void vke_vectorcall ConvertFloatToUInt( CVector4Ref V, CVector4* pOut );

            vke_force_inline static CVector4Ref vke_vectorcall _ONE()
            {
                return ONE;
            }

            vke_force_inline static CVector4Ref vke_vectorcall _NEGATIVE_ONE()
            {
                return NEGATIVE_ONE;
            }

            vke_force_inline static CVector4Ref vke_vectorcall _ZERO()
            {
                return ZERO;
            }

            vke_force_inline static CVector4Ref vke_vectorcall _X()
            {
                return X;
            }

            vke_force_inline static CVector4Ref vke_vectorcall _Y()
            {
                return Y;
            }

            vke_force_inline static CVector4Ref vke_vectorcall _Z()
            {
                return Z;
            }

            vke_force_inline static CVector4Ref vke_vectorcall _W()
            {
                return W;
            }

            vke_force_inline static CVector4Ref vke_vectorcall _NEGATIVE_X()
            {
                return NEGATIVE_X;
            }

            vke_force_inline static CVector4Ref vke_vectorcall _NEGATIVE_Y()
            {
                return NEGATIVE_Y;
            }

            vke_force_inline static CVector4Ref vke_vectorcall _NEGATIVE_Z()
            {
                return NEGATIVE_Z;
            }

            vke_force_inline static CVector4Ref vke_vectorcall _NEGATIVE_W()
            {
                return NEGATIVE_W;
            }

            vke_force_inline static void vke_vectorcall Load( const float* ptr, CVector4* pOut );
            vke_force_inline static void vke_vectorcall Load( const float* ptr, CVector4* pOut1, CVector4* pOut2,
                                                              CVector4* pOut3 );
            vke_force_inline static void vke_vectorcall Load( const float* ptr, CVector4* pOut1, CVector4* pOut2,
                                                              CVector4* pOut3, CVector4* pOut4 );

            vke_force_inline static CVector4Ref vke_vectorcall Load( float v );
            vke_force_inline static CVector4Ref vke_vectorcall Load( float x, float y, float z, float w );
            vke_force_inline static CVector4Ref vke_vectorcall Load( CVector4Ref V );
            vke_force_inline static CVector4Ref vke_vectorcall LoadXYToZW( float x, float y );
            /// <summary>
            /// Loads single float from ptr[ index ] to all vector4[ index ] components.
            /// </summary>
            /// <param name="ptr"></param>
            /// <param name="pOut"></param>
            /// <returns></returns>
            template< uint32_t Count >
            vke_force_inline static void vke_vectorcall Load( const float* ptr, CVector4* pOut );

        public:
            static const CVector4 ONE;
            static const CVector4 NEGATIVE_ONE;
            static const CVector4 ZERO;
            static const CVector4 X;
            static const CVector4 Y;
            static const CVector4 Z;
            static const CVector4 W;
            static const CVector4 NEGATIVE_X;
            static const CVector4 NEGATIVE_Y;
            static const CVector4 NEGATIVE_Z;
            static const CVector4 NEGATIVE_W;
            static const CVector4 TRUE_INT;
            static const CVector4 FALSE_INT;

            union alignas( 16 )
            {
                struct alignas( 16 )
                {
                    float x, y, z, w;
                };

                float alignas( 16 ) floats[ 4 ];
                int32_t alignas( 16 ) ints[ 4 ];
                uint32_t alignas( 16 ) uints[ 4 ];
                NativeVector4 _Native;
            };
        };

        using CVector4Ref = CVector4::CVector4Ref;

    } // namespace Math

} // namespace VKE

// #include "DirectX/CVector.inl"