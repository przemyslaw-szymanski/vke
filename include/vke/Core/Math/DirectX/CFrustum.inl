#include "../Types.h"
#if VKE_USE_DIRECTX_MATH
namespace VKE
{
    namespace Math
    {
#define VKE_XMMTX4(_mtx) DirectX::XMLoadFloat4x4(&(_mtx)._Native)
        
        void CFrustum::CreateFromMatrix( const CMatrix4x4& Matrix )
        {
            NativeFrustum::CreateFromMatrix( _Native, VKE_XMMTX4( Matrix ) );

            // Build the frustum planes.
            aPlanes[ 0 ]._Native = DirectX::XMVectorSet( 0.0f, 0.0f, -1.0f, _Native.Near );
            aPlanes[ 1 ]._Native = DirectX::XMVectorSet( 0.0f, 0.0f, 1.0f, -_Native.Far );
            aPlanes[ 2 ]._Native = DirectX::XMVectorSet( 1.0f, 0.0f, -_Native.RightSlope, 0.0f );
            aPlanes[ 3 ]._Native = DirectX::XMVectorSet( -1.0f, 0.0f, _Native.LeftSlope, 0.0f );
            aPlanes[ 4 ]._Native = DirectX::XMVectorSet( 0.0f, 1.0f, -_Native.TopSlope, 0.0f );
            aPlanes[ 5 ]._Native = DirectX::XMVectorSet( 0.0f, -1.0f, _Native.BottomSlope, 0.0f );

            // Normalize the planes so we can compare to the sphere radius.
            aPlanes[ 2 ]._Native = DirectX::XMVector3Normalize( aPlanes[ 2 ]._Native );
            aPlanes[ 3 ]._Native = DirectX::XMVector3Normalize( aPlanes[ 3 ]._Native );
            aPlanes[ 4 ]._Native = DirectX::XMVector3Normalize( aPlanes[ 4 ]._Native );
            aPlanes[ 5 ]._Native = DirectX::XMVector3Normalize( aPlanes[ 5 ]._Native );

            // Build the corners of the frustum.
            DirectX::XMVECTOR vRightTop = DirectX::XMVectorSet( _Native.RightSlope, _Native.TopSlope, 1.0f, 0.0f );
            DirectX::XMVECTOR vRightBottom = DirectX::XMVectorSet( _Native.RightSlope, _Native.BottomSlope, 1.0f, 0.0f );
            DirectX::XMVECTOR vLeftTop = DirectX::XMVectorSet( _Native.LeftSlope, _Native.TopSlope, 1.0f, 0.0f );
            DirectX::XMVECTOR vLeftBottom = DirectX::XMVectorSet( _Native.LeftSlope, _Native.BottomSlope, 1.0f, 0.0f );
            DirectX::XMVECTOR vNear = DirectX::XMVectorReplicatePtr( &_Native.Near );
            DirectX::XMVECTOR vFar = DirectX::XMVectorReplicatePtr( &_Native.Far );

            aCorners[ 0 ]._Native = DirectX::XMVectorMultiply( vRightTop, vNear );
            aCorners[ 1 ]._Native = DirectX::XMVectorMultiply( vRightBottom, vNear );
            aCorners[ 2 ]._Native = DirectX::XMVectorMultiply( vLeftTop, vNear );
            aCorners[ 3 ]._Native = DirectX::XMVectorMultiply( vLeftBottom, vNear );
            aCorners[ 4 ]._Native = DirectX::XMVectorMultiply( vRightTop, vFar );
            aCorners[ 5 ]._Native = DirectX::XMVectorMultiply( vRightBottom, vFar );
            aCorners[ 6 ]._Native = DirectX::XMVectorMultiply( vLeftTop, vFar );
            aCorners[ 7 ]._Native = DirectX::XMVectorMultiply( vLeftBottom, vFar );
        }

        void CFrustum::Transform( const CMatrix4x4& Matrix )
        {
            //NativeFrustum Tmp = _Native;
            //Tmp.Transform( _Native, XMMTX( Matrix ) );
            _Native.Transform( _Native, VKE_XMMTX4( Matrix ) );
        }

        void CFrustum::Transform( const CVector3& Translation, const CVector4& Rotation, float scale )
        {
            NativeFrustum Tmp = _Native;
            Tmp.Transform( _Native, scale, VKE_XMVEC4( Rotation ), VKE_XMVEC3( Translation ) );
        }

        bool CFrustum::Intersects( const CAABB& AABB ) const
        {
            return _Native.Intersects( AABB._Native );
        }

        bool CFrustum::Intersects( const CBoundingSphere& Sphere ) const
        {
            return _Native.Intersects( Sphere._Native );
        }

        void CFrustum::SetOrientation( const CVector3& vecPosition, const CQuaternion& quatRotation )
        {
            _Native.Origin = vecPosition._Native;
            DirectX::XMStoreFloat4( &_Native.Orientation, quatRotation._Native );
        }

        void CFrustum::CalcCorners( CVector3* pOut ) const
        {
            DirectX::XMFLOAT3 aTmps[ 8 ];
            _Native.GetCorners( aTmps );
            for( uint32_t i = 0; i < 8; ++i )
            {
                pOut[ i ] = CVector3( aTmps[ i ] );
            }
        }

        vke_force_inline void XMFloat3ToXmFloat4( const DirectX::XMFLOAT3& In, DirectX::XMFLOAT4* pOut )
        {
            pOut->x = In.x;
            pOut->y = In.y;
            pOut->z = In.z;
            pOut->w = 0;
        }

        void CFrustum::CalcMatrix( CMatrix4x4* pOut ) const
        {
            DirectX::XMVECTOR Orientation = DirectX::XMLoadFloat4( &this->_Native.Orientation );
            DirectX::XMFLOAT4 Origin4;
            XMFloat3ToXmFloat4( _Native.Origin, &Origin4 );
            DirectX::XMVECTOR Origin = DirectX::XMLoadFloat4( &Origin4 );

            DirectX::XMStoreFloat4x4(
                &pOut->_Native,
                DirectX::XMMatrixAffineTransformation(
                Math::CVector4::ONE._Native, Math::CVector4::ONE._Native,
                Orientation, Origin )
            );
        }

    } // Math
} // VKE
#endif // VKE_USE_DIRECTX_MATH