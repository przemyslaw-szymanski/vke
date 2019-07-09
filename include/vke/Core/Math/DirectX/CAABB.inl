#include "../Types.h"
#if VKE_USE_DIRECTX_MATH
namespace VKE
{
    namespace Math
    {
        void CAABB::CalcMinMax( SMinMax* pOut ) const
        {
            DirectX::XMVECTOR BoxMax = DirectX::XMVectorAdd( VKE_XMLOADF3( _Native.Center ), VKE_XMLOADF3( _Native.Extents ) );
            DirectX::XMVECTOR BoxMin = DirectX::XMVectorSubtract( VKE_XMLOADF3( _Native.Center ), VKE_XMLOADF3( _Native.Extents ) );
            DirectX::XMStoreFloat3( &pOut->vec3Min._Native, BoxMin );
            DirectX::XMStoreFloat3( &pOut->vec3Max._Native, BoxMax );
        }

        void CAABB::CalcCorners( CVector3* pOut ) const
        {
            DirectX::XMFLOAT3 aCorners[8];
            _Native.GetCorners( aCorners );
            for( uint32_t i = 0; i < 8; ++i )
            {
                pOut[i]._Native = aCorners[i];
            }
        }

        void CAABB::CalcSphere( Math::CBoundingSphere* pOut ) const
        {
            DirectX::XMVECTOR BoxMax = DirectX::XMVectorAdd( VKE_XMLOADF3(_Native.Center), VKE_XMLOADF3(_Native.Extents) );
            DirectX::XMVECTOR BoxMin = DirectX::XMVectorSubtract( VKE_XMLOADF3( _Native.Center ), VKE_XMLOADF3( _Native.Extents ) );
            DirectX::XMVECTOR Distance = DirectX::XMVector3Length( DirectX::XMVectorSubtract( BoxMax, BoxMin ) );
            
            pOut->_Native.Radius = DirectX::XMVectorGetX( Distance );
            pOut->_Native.Center = _Native.Center;   
        }

        INTERSECT_RESULT CAABB::Contains( const CAABB& Other ) const
        {
            const DirectX::ContainmentType res = _Native.Contains( Other._Native );
            return ConvertFromNative( res );
        }

        INTERSECT_RESULT CAABB::Contains( const CBoundingSphere& Sphere ) const
        {
            const DirectX::ContainmentType res = _Native.Contains( Sphere._Native );
            return ConvertFromNative( res );
        }

        INTERSECT_RESULT CAABB::Contains( const CVector3& Point ) const
        {
            const DirectX::ContainmentType res = _Native.Contains( VKE_XMVEC3( Point ) );
            return ConvertFromNative( res );
        }

        INTERSECT_RESULT CAABB::Contains( const CVector4& Point ) const
        {
            const DirectX::ContainmentType res = _Native.Contains( VKE_XMVEC4( Point ) );
            return ConvertFromNative( res );
        }

        INTERSECT_RESULT CAABB::Contains( const CVector3& Vec1, const CVector3& Vec2, const CVector3& Vec3 ) const
        {
            const DirectX::ContainmentType res = _Native.Contains( VKE_XMVEC3( Vec1 ), VKE_XMVEC3( Vec2 ), VKE_XMVEC3( Vec3 ) );
            return ConvertFromNative( res );
        }

        void CAABB::CalcCenter( CVector3* pOut ) const
        {
            pOut->_Native = _Native.Center;
        }

        void CAABB::CalcCenter( CVector4* pOut ) const
        {
            pOut->_Native = DirectX::XMLoadFloat3( &_Native.Center );
        }

        void CAABB::CalcExtents( CVector3* pOut ) const
        {
            pOut->_Native = _Native.Extents;
        }

        void CAABB::CalcExtents( CVector4* pOut ) const
        {
            pOut->_Native = DirectX::XMLoadFloat3( &_Native.Extents );
        }

        void CAABB::Transform( const CMatrix4x4& Matrix, CAABB* pOut )
        {
            pOut->_Native.Transform( pOut->_Native, VKE_XMMTX4( Matrix ) );
        }

        void CAABB::Transform( const float scale, const CVector3& Translation, CAABB* pOut )
        {  
            pOut->_Native.Transform( pOut->_Native, scale, DirectX::XMQuaternionIdentity(), VKE_XMVEC3( Translation ) );
        }

    } // Math
} // VKE
#endif // VKE_USE_DIRECTX_MATH