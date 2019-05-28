#pragma once

#include "CVector.h"

namespace VKE
{
    namespace Math
    {
        VKE_ALIGN( 16 ) class VKE_API CMatrix4x4
        {
            friend class CVector;

            public:

                CMatrix4x4() {}
                CMatrix4x4( const CMatrix4x4& ) = default;
                CMatrix4x4( CMatrix4x4&& ) = default;
                ~CMatrix4x4() {}

                CMatrix4x4& operator=( const CMatrix4x4& ) = default;
                CMatrix4x4& operator=( CMatrix4x4&& ) = default;

                void vke_force_inline   SetLookAt(const CVector& Position, const CVector& AtPosition, const CVector& Up);
                void vke_force_inline   SetLookTo( const CVector& Position, const CVector& Direction, const CVector& Up );
                void vke_force_inline   SetPerspective(const ExtentF32& Viewport, const ExtentF32& Planes);
                void vke_force_inline   SetPerspectiveFOV( float fovAngleY, float aspectRatio, const ExtentF32& Planes );


                static void vke_force_inline Mul( const CMatrix4x4& Left, const CMatrix4x4& Right, CMatrix4x4* pOut );
                static void vke_force_inline Transpose( CMatrix4x4* pInOut );
                static void vke_force_inline Transpose( const CMatrix4x4& Src, CMatrix4x4* pDst );
                static void vke_force_inline Invert( CVector* pDeterminant, CMatrix4x4* pInOut );
                static void vke_force_inline Invert( CVector* pDeterminant, const CMatrix4x4& Src, CMatrix4x4* pDst );
                static void vke_force_inline Translate( const CVector& Vec, CMatrix4x4* pOut );
                static void vke_force_inline Identity( CMatrix4x4* pOut );
                
                static CMatrix4x4 vke_force_inline Identity();

                static vke_force_inline const CMatrix4x4& _Identity() { return IDENTITY; }

            public:

                static const CMatrix4x4 IDENTITY;

            public:

                VKE_ALIGN( 16 ) union
                {
                    VKE_ALIGN( 16 ) float m[16];
                    VKE_ALIGN( 16 ) float mm[4][4];
                    NativeMatrix4x4 _Native;
                };
        };
    } // Math
} // VKE

#include "DirectX/CMatrix.inl"
