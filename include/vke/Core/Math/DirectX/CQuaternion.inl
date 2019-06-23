#if VKE_USE_DIRECTX_MATH
namespace VKE
{

    namespace Math
    {
        constexpr CQuaternion::CQuaternion( const float x, const float y, const float z, const float w ) :
            CVector4( x, y, z, w )
        {}

        constexpr CQuaternion::CQuaternion(const CVector4& V) :
            CVector4( V )
        {}

        constexpr CQuaternion::CQuaternion( const NativeQuaternion& Q ) :
            CVector4( Q )
        {}

        CVector3 CQuaternion::operator*( const CVector3& V ) const
        {
            CVector3 ret;
            Mul( *this, V, &ret );
            return ret;
        }

        void CQuaternion::operator*=( const CQuaternion& Q )
        {
            Mul( *this, Q, this );
        }

        CQuaternion CQuaternion::operator*( const CQuaternion& Q ) const
        {
            return CQuaternion{ DirectX::XMQuaternionMultiply( _Native, VKE_XMVEC4( Q ) ) };
        }

        void CQuaternion::Mul( const CQuaternion& Q, const CVector3& V, CVector3* pOut )
        {
            const DirectX::XMVECTOR v = VKE_XMLOADF3( V._Native );

            const DirectX::XMVECTOR uv = DirectX::XMVector3Cross( VKE_XMVEC4( Q ), v );
            const DirectX::XMVECTOR uuv = DirectX::XMVector3Cross( VKE_XMVEC4( Q ), uv );

            const DirectX::XMVECTOR uv2 = DirectX::XMVectorScale( uv, 2.0f * Q.w );
            const DirectX::XMVECTOR uuv2 = DirectX::XMVectorScale( uuv, 2.0f );

            const DirectX::XMVECTOR sum = DirectX::XMVectorAdd( uv2, uuv2 );
            DirectX::XMStoreFloat3( &pOut->_Native, DirectX::XMVectorAdd( sum, v ) );
        }

        void CQuaternion::Mul( const CQuaternion& Q1, const CQuaternion& Q2, CQuaternion* pOut )
        {
            pOut->_Native = DirectX::XMQuaternionMultiply( VKE_XMVEC4( Q1 ), VKE_XMVEC4( Q2 ) );
        }

        void CQuaternion::Rotate( const CVector4& vecAxis, const float angleRadians, CQuaternion* pOut )
        {
            pOut->_Native = DirectX::XMQuaternionRotationAxis( VKE_XMVEC4( vecAxis ), angleRadians );
        }

        void CQuaternion::Rotate( const float pitch, const float yaw, const float roll, CQuaternion* pOut )
        {
            pOut->_Native = DirectX::XMQuaternionRotationRollPitchYaw( pitch, yaw, roll );
        }

        void CQuaternion::Rotate( const CVector4& V, CVector4* pOut ) const
        {
            pOut->_Native = DirectX::XMVector3Rotate( VKE_XMVEC4( V ), _Native );
        }

        void CQuaternion::Rotate( const CVector3& V, CVector3* pOut ) const
        {
            DirectX::XMStoreFloat3( &pOut->_Native, DirectX::XMVector3Rotate( VKE_XMVEC3( V ), _Native ) );
        }

        void CQuaternion::Rotate( const CMatrix4x4& mtxRotation )
        {
            _Native = DirectX::XMQuaternionRotationMatrix( VKE_XMMTX4( mtxRotation ) );
        }

    } // MAth
} // VKE
#endif // VKE_USE_DIRECTX_MATH