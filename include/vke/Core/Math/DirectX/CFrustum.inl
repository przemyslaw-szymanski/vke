#include "../Types.h"
#if VKE_USE_DIRECTX_MATH
namespace VKE
{
    namespace Math
    {
#define XMVEC(_vec) DirectX::XMLoadFloat3(&(_vec._Native))
#define XMMTX(_mtx) DirectX::XMLoadFloat4x4(&(_mtx)._Native)
        
        void CFrustum::Transform( const CMatrix4x4& Matrix )
        {
            NativeFrustum Tmp = _Native;
            Tmp.Transform( _Native, XMMTX( Matrix ) );
        }

        void CFrustum::Transform( const CVector3& Translation, const CVector3& Rotation, float scale )
        {
            NativeFrustum Tmp = _Native;
            Tmp.Transform( _Native, scale, XMVEC( Rotation ), XMVEC( Translation ) );
        }

        bool CFrustum::Intersects( const CAABB& AABB ) const
        {
            return _Native.Intersects( AABB._Native );
        }

        bool CFrustum::Intersects( const CBoundingSphere& Sphere ) const
        {
            return _Native.Intersects( Sphere._Native );
        }

    } // Math
} // VKE
#endif // VKE_USE_DIRECTX_MATH