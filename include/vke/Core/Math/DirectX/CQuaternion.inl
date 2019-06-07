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

        CQuaternion CQuaternion::operator*( const CVector3& V ) const
        {
            CQuaternion ret;
            Mul( *this, V, &ret );
            return ret;
        }

        void CQuaternion::operator*=( const CVector3& V )
        {
            CQuaternion tmp;
            Mul( *this, V, &tmp );
            _Native = tmp._Native;
        }

        void CQuaternion::Mul( const CQuaternion& Q, const CVector3& V, CQuaternion* pOut )
        {
            const DirectX::XMVECTOR v = VKE_XMLOADF3( V._Native );

            const DirectX::XMVECTOR uv = DirectX::XMVector3Cross( VKE_XMVEC4( Q ), v );
            const DirectX::XMVECTOR uuv = DirectX::XMVector3Cross( VKE_XMVEC4( Q ), uv );

            const DirectX::XMVECTOR uv2 = DirectX::XMVectorScale( uv, 2.0f * Q.w );
            const DirectX::XMVECTOR uuv2 = DirectX::XMVectorScale( uuv, 2.0f );

            const DirectX::XMVECTOR sum = DirectX::XMVectorAdd( uv2, uuv2 );
            pOut->_Native = DirectX::XMVectorAdd( sum, v );
        }

    } // MAth
} // VKE
#endif // VKE_USE_DIRECTX_MATH