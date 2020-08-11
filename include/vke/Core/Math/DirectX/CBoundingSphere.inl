#include "../Types.h"
#if VKE_USE_DIRECTX_MATH
namespace VKE
{
    namespace Math
    {
        /*constexpr CBoundingSphere::CBoundingSphere( const CVector3& Center, float radius ) :
            _Native{ Center._Native, radius }
        {}

        CBoundingSphere::CBoundingSphere( const CBoundingSphere& Other ) :
            _Native{ Other._Native }
        {}

        CBoundingSphere::CBoundingSphere( CBoundingSphere&& Other ) :
            _Native{ Other._Native }
        {}

        CBoundingSphere& CBoundingSphere::operator=( CBoundingSphere&& Other )
        {
            _Native = Other._Native;
            return *this;
        }

        CBoundingSphere& CBoundingSphere::operator=( const CBoundingSphere& Other )
        {
            _Native = Other._Native;
            return *this;
        }

        bool CBoundingSphere::operator==( const CBoundingSphere& Other ) const
        {
            return Position == Other.Position && radius == Other.radius;
        }

        bool CBoundingSphere::operator!=( const CBoundingSphere& Other ) const
        {
            return Position != Other.Position || radius != Other.radius;
        }

        bool CBoundingSphere::operator<( const CBoundingSphere& Other ) const
        {
            return radius < Other.radius;
        }

        bool CBoundingSphere::operator<=( const CBoundingSphere& Other ) const
        {
            return radius <= Other.radius;
        }

        bool CBoundingSphere::operator>( const CBoundingSphere& Other ) const
        {
            return radius > Other.radius;
        }

        bool CBoundingSphere::operator>=( const CBoundingSphere& Other ) const
        {
            return radius >= Other.radius;
        }*/

        bool CBoundingSphere::Contains(const Math::CVector3& vecCenter, const float radius, const Math::CVector4& vecPoint)
        {
            NativeBoundingSphere Native(vecCenter._Native, radius);
            return Native.Contains(vecPoint._Native);
        }
    } // VKE
} // Math
#endif // VKE_USE_DIRECTX_MATH