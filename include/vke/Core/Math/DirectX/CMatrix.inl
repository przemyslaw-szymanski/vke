
#if VKE_USE_DIRECTX_MATH
namespace VKE
{
    namespace Math
    {
        void CMatrix4x4::SetLookAt( const CVector3& Position, const CVector3& AtPosition, const CVector3& Up )
        {
#if VKE_USE_RIGHT_HANDED_COORDINATES
            //_Native = DirectX::XMMatrixLookAtRH( Position._Native, AtPosition._Native, Up._Native );
            DirectX::XMStoreFloat4x4( &_Native,
                DirectX::XMMatrixLookAtRH(
                    DirectX::XMLoadFloat3( &Position._Native ),
                    DirectX::XMLoadFloat3( &AtPosition._Native ),
                    DirectX::XMLoadFloat3( &Up._Native ) ) );
#else
            DirectX::XMStoreFloat4x4( &_Native,
                                      DirectX::XMMatrixLookAtLH(
                                      DirectX::XMLoadFloat3( &Position._Native ),
                                      DirectX::XMLoadFloat3( &AtPosition._Native ),
                                      DirectX::XMLoadFloat3( &Up._Native ) ) );
#endif
        }

        void CMatrix4x4::SetLookTo( const CVector3& Position, const CVector3& Direction, const CVector3& Up )
        {
#if VKE_USE_RIGHT_HANDED_COORDINATES
            DirectX::XMStoreFloat4x4( &_Native, DirectX::XMMatrixLookToRH( VKE_XMVEC3( Position ), VKE_XMVEC3( Direction ), VKE_XMVEC3( Up ) ) );
#else
            DirectX::XMStoreFloat4x4( &_Native, DirectX::XMMatrixLookToLH( VKE_XMVEC3( Position ), VKE_XMVEC3( Direction ), VKE_XMVEC3( Up ) ) );
#endif
        }

        void CMatrix4x4::SetPerspective( const ExtentF32& Viewport, const ExtentF32& Planes )
        {
#if VKE_USE_RIGHT_HANDED_COORDINATES
            VKE_XMSTORE44( _Native, DirectX::XMMatrixPerspectiveRH( Viewport.width, Viewport.height, Planes.min, Planes.max ) );
#else
            VKE_XMSTORE44( _Native, DirectX::XMMatrixPerspectiveLH( Viewport.width, Viewport.height, Planes.min, Planes.max ) );
#endif
        }

        void CMatrix4x4::SetPerspectiveFOV( float fovAngleY, float aspectRatio, const ExtentF32& Planes )
        {
#if VKE_USE_RIGHT_HANDED_COORDINATES
            VKE_XMSTORE44( _Native, DirectX::XMMatrixPerspectiveFovRH( fovAngleY, aspectRatio, Planes.min, Planes.max ) );
#else
            VKE_XMSTORE44( _Native, DirectX::XMMatrixPerspectiveFovLH( fovAngleY, aspectRatio, Planes.min, Planes.max ) );
#endif
        }

        void CMatrix4x4::Mul( const CMatrix4x4& Left, const CMatrix4x4& Right, CMatrix4x4* pOut )
        {
            //pOut->_Native = DirectX::XMMatrixMultiply( Left._Native, Right._Native );
            DirectX::XMStoreFloat4x4( &pOut->_Native,
                DirectX::XMMatrixMultiply(
                    DirectX::XMLoadFloat4x4( &Left._Native ),
                    DirectX::XMLoadFloat4x4( &Right._Native ) ) );
        }

        void CMatrix4x4::Transpose( const CMatrix4x4& Src, CMatrix4x4* pDst )
        {
            VKE_XMSTOREPMTX( pDst, DirectX::XMMatrixTranspose( VKE_XMMTX4( Src ) ) );
        }

        void CMatrix4x4::Transpose()
        {
            Transpose( *this, this );
        }

        void CMatrix4x4::Invert( const CMatrix4x4& Src, CMatrix4x4* pDst )
        {
            VKE_XMSTOREPMTX( pDst, DirectX::XMMatrixInverse( nullptr, VKE_XMMTX4( Src ) ) );
        }

        void CMatrix4x4::Invert()
        {
            Invert( *this, this );
        }

        void CMatrix4x4::Transform( const CVector4& vecScale, const CVector4& vecRotationOrigin,
            const CQuaternion& quatRotation, const CVector4& vecTranslate )
        {
            Transform( vecScale, vecRotationOrigin, quatRotation, vecTranslate, this );
        }

        void CMatrix4x4::Translate( const CVector3& Vec, CMatrix4x4* pOut )
        {
            DirectX::XMStoreFloat4x4( &pOut->_Native, DirectX::XMMatrixTranslation( Vec.x, Vec.y, Vec.z ) );
        }

        void CMatrix4x4::Scale( const CVector3& Scale, CMatrix4x4* pOut )
        {
            DirectX::XMStoreFloat4x4( &pOut->_Native, DirectX::XMMatrixScaling( Scale.x, Scale.y, Scale.z ) );
        }

        void CMatrix4x4::Transform( const CVector4& vecScale, const CVector4& vecRotationOrigin,
            const CQuaternion& quatRotation, const CVector4& vecTranslate, CMatrix4x4* pOut )
        {
            DirectX::XMStoreFloat4x4( &pOut->_Native,
                DirectX::XMMatrixAffineTransformation(
                    VKE_XMVEC4( vecScale ),
                    VKE_XMVEC4( vecRotationOrigin ),
                    VKE_XMVEC4( quatRotation ),
                    VKE_XMVEC4( vecTranslate ) ) );
        }

        void CMatrix4x4::Identity( CMatrix4x4* pOut )
        {
            DirectX::XMStoreFloat4x4( &pOut->_Native, DirectX::XMMatrixIdentity() );
        }

        CMatrix4x4 CMatrix4x4::Identity()
        {
            CMatrix4x4 tmp;
            Identity( &tmp );
            return tmp;
        }

        void CMatrix4x4::Rotation( const CVector4& vecAxis, const float radians, CMatrix4x4* pOut )
        {
            DirectX::XMStoreFloat4x4( &pOut->_Native, DirectX::XMMatrixRotationAxis( VKE_XMVEC4( vecAxis ), radians ) );
        }

        void CMatrix4x4::RotationY( const float angle, CMatrix4x4* pOut )
        {
            DirectX::XMStoreFloat4x4( &pOut->_Native, DirectX::XMMatrixRotationY( angle ) );
        }

        void CMatrix4x4::Transform( const CVector4& vecDirection, const CMatrix4x4& mtxTransform,
                                    CVector4* pOut )
        {
            pOut->_Native = DirectX::XMVector3TransformNormal( VKE_XMVEC4( vecDirection ), VKE_XMMTX4( mtxTransform ) );
        }

    } // Math
} // VKE
#endif // #if VKE_USE_DIRECTX_MATH