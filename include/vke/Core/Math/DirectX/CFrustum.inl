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

    } // Math
} // VKE
#endif // VKE_USE_DIRECTX_MATH