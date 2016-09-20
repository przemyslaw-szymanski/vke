#pragma once

#include "Core/VKEPreprocessor.h"
#include "Core/Math/VectorBase.h"
#if VKE_USE_DIRECTX_MATH
#include "ThirdParty/DirectX/DirectXMath.h"

namespace VKE
{
    namespace Math
    {       
        class CVector
        {
            public:

                DirectX::XMVECTOR vec;

            public:

                CVector() {}
                ~CVector() {}

                vke_force_inline
                    float GetX() const;

                vke_force_inline
                    float GetY() const;

                vke_force_inline
                    float GetZ() const;

                vke_force_inline
                    float GetW() const;

                vke_force_inline
                    float*& Get();

                vke_force_inline
                    void SetX( float );

                vke_force_inline
                    void SetY( float );

                vke_force_inline
                    void SetZ( float );

                vke_force_inline
                    void SetW( float );

                vke_force_inline
                    void Set( float, float, float, float );

                vke_force_inline
                    void Add( const CVector&, const CVector& );

                vke_force_inline
                    void Add( const CVector& );

                vke_force_inline
                    void Add( float );

                vke_force_inline
                    void Sub( const CVector&, const CVector& );

                vke_force_inline
                    void Sub( const CVector& );

                vke_force_inline
                    void Sub( float );

                vke_force_inline
                    void Div( const CVector&, const CVector& );

                vke_force_inline
                    void Div( const CVector& );

                vke_force_inline
                    void Div( float );

                vke_force_inline
                    void Mul( const CVector&, const CVector& );

                vke_force_inline
                    void Mul( const CVector& );

                vke_force_inline
                    void Mul( float );
        };

        void CVector::Add(const CVector& l, const CVector& r)
        {
            vec = DirectX::XMVectorAdd( l.vec, r.vec );
        }

        void CVector::Add(const CVector& v)
        {
            vec = DirectX::XMVectorAdd( vec, v.vec );
        }

        void CVector::Add(float v)
        {   
            vec = DirectX::XMVectorAdd(vec, DirectX::XMVectorSet( v, v, v, v ) );
        }

        void CVector::Sub( const CVector& l, const CVector& r )
        {
            vec = DirectX::XMVectorSubtract( l.vec, r.vec );
        }

        void CVector::Sub( const CVector& v )
        {
            vec = DirectX::XMVectorSubtract( vec, v.vec );
        }

        void CVector::Sub( float v )
        {
            vec = DirectX::XMVectorSubtract( vec, DirectX::XMVectorSet( v, v, v, v ) );
        }

        void CVector::Mul( const CVector& l, const CVector& r )
        {
            vec = DirectX::XMVectorMultiply( l.vec, r.vec );
        }

        void CVector::Mul( const CVector& v )
        {
            vec = DirectX::XMVectorMultiply( vec, v.vec );
        }

        void CVector::Mul( float v )
        {
            vec = DirectX::XMVectorMultiply( vec, DirectX::XMVectorSet( v, v, v, v ) );
        }

        void CVector::Div( const CVector& l, const CVector& r )
        {
            vec = DirectX::XMVectorDivide( l.vec, r.vec );
        }

        void CVector::Div( const CVector& v )
        {
            vec = DirectX::XMVectorDivide( vec, v.vec );
        }

        void CVector::Div( float v )
        {
            vec = DirectX::XMVectorDivide( vec, DirectX::XMVectorSet( v, v, v, v ) );
        }
    } // Math
} // VKE

#endif // VKE_USE_DIRECTX_MATH