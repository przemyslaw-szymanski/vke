#include "../Types.h"
#if VKE_USE_DIRECTX_MATH
namespace VKE
{
    namespace Math
    {
#define VKE_XMMTX4(_mtx) DirectX::XMLoadFloat4x4(&(_mtx)._Native)

        void CFrustum::Update()
        {
            // Build the frustum planes.
            aPlanes[0]._Native = DirectX::XMVectorSet(0.0f, 0.0f, -1.0f, _Native.Near);
            aPlanes[1]._Native = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, -_Native.Far);
            aPlanes[2]._Native = DirectX::XMVectorSet(1.0f, 0.0f, -_Native.RightSlope, 0.0f);
            aPlanes[3]._Native = DirectX::XMVectorSet(-1.0f, 0.0f, _Native.LeftSlope, 0.0f);
            aPlanes[4]._Native = DirectX::XMVectorSet(0.0f, 1.0f, -_Native.TopSlope, 0.0f);
            aPlanes[5]._Native = DirectX::XMVectorSet(0.0f, -1.0f, _Native.BottomSlope, 0.0f);

            // Normalize the planes so we can compare to the sphere radius.
            aPlanes[2]._Native = DirectX::XMVector3Normalize(aPlanes[2]._Native);
            aPlanes[3]._Native = DirectX::XMVector3Normalize(aPlanes[3]._Native);
            aPlanes[4]._Native = DirectX::XMVector3Normalize(aPlanes[4]._Native);
            aPlanes[5]._Native = DirectX::XMVector3Normalize(aPlanes[5]._Native);

            // Build the corners of the frustum.
            DirectX::XMVECTOR vRightTop = DirectX::XMVectorSet(_Native.RightSlope, _Native.TopSlope, 1.0f, 0.0f);
            DirectX::XMVECTOR vRightBottom = DirectX::XMVectorSet(_Native.RightSlope, _Native.BottomSlope, 1.0f, 0.0f);
            DirectX::XMVECTOR vLeftTop = DirectX::XMVectorSet(_Native.LeftSlope, _Native.TopSlope, 1.0f, 0.0f);
            DirectX::XMVECTOR vLeftBottom = DirectX::XMVectorSet(_Native.LeftSlope, _Native.BottomSlope, 1.0f, 0.0f);
            DirectX::XMVECTOR vNear = DirectX::XMVectorReplicatePtr(&_Native.Near);
            DirectX::XMVECTOR vFar = DirectX::XMVectorReplicatePtr(&_Native.Far);

            aCorners[0]._Native = DirectX::XMVectorMultiply(vRightTop, vNear);
            aCorners[1]._Native = DirectX::XMVectorMultiply(vRightBottom, vNear);
            aCorners[2]._Native = DirectX::XMVectorMultiply(vLeftTop, vNear);
            aCorners[3]._Native = DirectX::XMVectorMultiply(vLeftBottom, vNear);
            aCorners[4]._Native = DirectX::XMVectorMultiply(vRightTop, vFar);
            aCorners[5]._Native = DirectX::XMVectorMultiply(vRightBottom, vFar);
            aCorners[6]._Native = DirectX::XMVectorMultiply(vLeftTop, vFar);
            aCorners[7]._Native = DirectX::XMVectorMultiply(vLeftBottom, vFar);
        }

        template<bool DoUpdate>
        void CFrustum::CreateFromMatrix( const CMatrix4x4& Matrix )
        {
            NativeFrustum::CreateFromMatrix( _Native, VKE_XMMTX4( Matrix ) );

            if constexpr(DoUpdate)
            {
                Update();
            }
        }

        template<bool DoUpdate>
        void CFrustum::SetFar(const float far)
        {
            _Native.Far = far;
            if constexpr(DoUpdate)
            {
                Update();
            }
        }

        template<bool DoUpdate>
        void CFrustum::SetNear(const float near)
        {
            _Native.Near = near;
            if constexpr(DoUpdate)
            {
                Update();
            }
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

        // Copied from DirectXCollision.inl
        bool CFrustum::Intersects( const CBoundingSphere& Sphere ) const
        {
            // Load origin and orientation of the frustum.
            DirectX::XMVECTOR vOrigin = DirectX::XMLoadFloat3(&_Native.Origin);
            DirectX::XMVECTOR vOrientation = DirectX::XMLoadFloat4(&_Native.Orientation);
            DirectX::XMVECTOR Zero = DirectX::XMVectorZero();

            assert(DirectX::Internal::XMQuaternionIsUnit(vOrientation));

            // Load the sphere.
            DirectX::XMVECTOR vCenter = DirectX::XMLoadFloat3( &Sphere.Position._Native );
            DirectX::XMVECTOR vRadius = DirectX::XMVectorReplicatePtr(&Sphere.radius);

            // Transform the center of the sphere into the local space of frustum.
            vCenter = DirectX::XMVector3InverseRotate(DirectX::XMVectorSubtract(vCenter, vOrigin), vOrientation);

            // Set w of the center to one so we can dot4 with the plane.
            vCenter = DirectX::XMVectorInsert<0, 0, 0, 0, 1>(vCenter, DirectX::XMVectorSplatOne());

            // Check against each plane of the frustum.
            DirectX::XMVECTOR Outside = DirectX::XMVectorFalseInt();
            DirectX::XMVECTOR InsideAll = DirectX::XMVectorTrueInt();
            DirectX::XMVECTOR CenterInsideAll = DirectX::XMVectorTrueInt();

            DirectX::XMVECTOR Dist[6];

            for (size_t i = 0; i < 6; ++i)
            {
                Dist[i] = DirectX::XMVector4Dot(vCenter, aPlanes[i]._Native);

                // Outside the plane?
                Outside = DirectX::XMVectorOrInt(Outside, DirectX::XMVectorGreater(Dist[i], vRadius));

                // Fully inside the plane?
                InsideAll = DirectX::XMVectorAndInt(InsideAll, DirectX::XMVectorLessOrEqual(Dist[i], DirectX::XMVectorNegate(vRadius)));

                // Check if the center is inside the plane.
                CenterInsideAll = DirectX::XMVectorAndInt(CenterInsideAll, DirectX::XMVectorLessOrEqual(Dist[i], Zero));
            }

            // If the sphere is outside any of the planes it is outside.
            if (DirectX::XMVector4EqualInt(Outside, DirectX::XMVectorTrueInt()))
            {
                return false;
            }

            // If the sphere is inside all planes it is fully inside.
            if (DirectX::XMVector4EqualInt(InsideAll, DirectX::XMVectorTrueInt()))
            {
                return true;
            }

            // If the center of the sphere is inside all planes and the sphere intersects
            // one or more planes then it must intersect.
            if (DirectX::XMVector4EqualInt(CenterInsideAll, DirectX::XMVectorTrueInt()))
            {
                return true;
            }

            // The sphere may be outside the frustum or intersecting the frustum.
            // Find the nearest feature (face, edge, or corner) on the frustum
            // to the sphere.

            // The faces adjacent to each face are:
            static const size_t adjacent_faces[6][4] =
            {
                {2, 3, 4, 5},    // 0
            {2, 3, 4, 5},    // 1
            {0, 1, 4, 5},    // 2
            {0, 1, 4, 5},    // 3
            {0, 1, 2, 3},    // 4
            {0, 1, 2, 3}
            };  // 5

            DirectX::XMVECTOR Intersects = DirectX::XMVectorFalseInt();

            // Check to see if the nearest feature is one of the planes.
            for (size_t i = 0; i < 6; ++i)
            {
                // Find the nearest point on the plane to the center of the sphere.
                DirectX::XMVECTOR Point = DirectX::XMVectorNegativeMultiplySubtract(aPlanes[i]._Native, Dist[i], vCenter);

                // Set w of the point to one.
                Point = DirectX::XMVectorInsert<0, 0, 0, 0, 1>(Point, DirectX::XMVectorSplatOne());

                // If the point is inside the face (inside the adjacent planes) then
                // this plane is the nearest feature.
                DirectX::XMVECTOR InsideFace = DirectX::XMVectorTrueInt();

                for (size_t j = 0; j < 4; j++)
                {
                    size_t plane_index = adjacent_faces[i][j];

                    InsideFace = DirectX::XMVectorAndInt(InsideFace,
                        DirectX::XMVectorLessOrEqual(DirectX::XMVector4Dot(Point, aPlanes[plane_index]._Native), Zero));
                }

                // Since we have already checked distance from the plane we know that the
                // sphere must intersect if this plane is the nearest feature.
                Intersects = DirectX::XMVectorOrInt(Intersects,
                    DirectX::XMVectorAndInt(DirectX::XMVectorGreater(Dist[i], Zero), InsideFace));
            }

            if (DirectX::XMVector4EqualInt(Intersects, DirectX::XMVectorTrueInt()))
            {
                return true;
            }

            // The Edges are:
            static const size_t edges[12][2] =
            {
                {0, 1}, {2, 3}, {0, 2}, {1, 3},    // Near plane
            {4, 5}, {6, 7}, {4, 6}, {5, 7},    // Far plane
            {0, 4}, {1, 5}, {2, 6}, {3, 7},
            }; // Near to far

            DirectX::XMVECTOR RadiusSq = DirectX::XMVectorMultiply(vRadius, vRadius);

            // Check to see if the nearest feature is one of the edges (or corners).
            for (size_t i = 0; i < 12; ++i)
            {
                size_t ei0 = edges[i][0];
                size_t ei1 = edges[i][1];

                // Find the nearest point on the edge to the center of the sphere.
                // The corners of the frustum are included as the endpoints of the edges.
                DirectX::XMVECTOR Point = DirectX::Internal::PointOnLineSegmentNearestPoint(aCorners[ei0]._Native,
                    aCorners[ei1]._Native, vCenter);

                DirectX::XMVECTOR Delta = DirectX::XMVectorSubtract(vCenter, Point);

                DirectX::XMVECTOR DistSq = DirectX::XMVector3Dot(Delta, Delta);

                // If the distance to the center of the sphere to the point is less than
                // the radius of the sphere then it must intersect.
                Intersects = DirectX::XMVectorOrInt(Intersects, DirectX::XMVectorLessOrEqual(DistSq, RadiusSq));
            }

            if (DirectX::XMVector4EqualInt(Intersects, DirectX::XMVectorTrueInt()))
            {
                return true;
            }
            // The sphere must be outside the frustum.
            return false;
        }

        template<bool DoUpdate>
        void CFrustum::SetOrientation( const CVector3& vecPosition, const CQuaternion& quatRotation )
        {
            _Native.Origin = vecPosition._Native;
            DirectX::XMStoreFloat4( &_Native.Orientation, quatRotation._Native );
            if constexpr(DoUpdate)
            {
                Update();
            }
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