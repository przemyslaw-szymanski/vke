#include "Scene/CCamera.h"
namespace VKE
{
    namespace Scene
    {
        void CCamera::Reset()
        {
            m_vecPosition = Math::CVector3::ZERO;
            m_vecDirection = Math::CVector3::Z;
            m_vecUp = Math::CVector3::Y;
            m_vecRight = Math::CVector3::X;
        }
        void CCamera::_UpdateViewMatrix()
        {
            m_ViewMatrix.SetLookTo( m_vecPosition, m_vecDirection, m_vecUp );
        }
        void CCamera::_UpdateProjMatrix()
        {
            const float aspectRatio = m_Viewport.width / m_Viewport.height;
            m_ProjMatrix.SetPerspectiveFOV( m_fovAngle, aspectRatio,
                                            m_ClippingPlanes );
        }
        void CCamera::_ApplyRotation( const Math::CMatrix4x4& mtxTransform )
        {
            Math::CVector4 vecDir( m_vecDirection );
            Math::CMatrix4x4::Transform( vecDir, mtxTransform, &vecDir );
            vecDir.Normalize();
            Math::CVector4 vecUp( m_vecUp );
            Math::CMatrix4x4::Transform( vecUp, mtxTransform, &vecUp );
            vecUp.Normalize();
            Math::CVector4 vecRight;
            Math::CVector4::Cross( vecDir, vecUp, &vecRight );
            Math::CVector4::Cross( vecRight, vecDir, &vecUp );
            m_vecDirection = Math::CVector3( vecDir );
            m_vecUp = Math::CVector3( vecUp );
            m_vecRight = Math::CVector3( vecRight );
        }
        void CCamera::Update( float time )
        {
            // Math:SphericalToCartesian( m_vecAngleRadians.x -
            // DirectX::XM_PIDIV2, -m_vecAngleRadians.y, 1.0f, &m_LookAt );
            // m_LookAt.Normalize( &m_vecDirection );
            // if( m_needProjUpdate )
            {
                m_needProjUpdate = false;
                const float aspectRatio = m_Viewport.width / m_Viewport.height;
                m_ProjMatrix.SetPerspective( m_Viewport, m_ClippingPlanes );
                m_ProjMatrix.SetPerspectiveFOV( m_fovAngle, aspectRatio,
                                                m_ClippingPlanes );
                m_Frustum.CreateFromMatrix( m_ProjMatrix );
                m_frustumWidth = CalcFrustumWidth( m_ClippingPlanes.end );
            }
            auto tmp = m_LookAt + m_vecPosition;
            _UpdateViewMatrix();
            // m_ViewMatrix.SetLookAt( m_vecPosition, m_LookAt /*+ m_Position +
            // Math::CVector3::ONE*/, m_vecUp );
            CalcViewProjectionMatrix( &m_ViewProjMatrix );
            Math::CQuaternion quatRotation;
            quatRotation.Rotate( m_ViewMatrix );
            m_Frustum.SetOrientation( m_vecPosition, quatRotation );
            // m_Frustum.Transform( m_ViewMatrix );
            // m_Frustum.Transform( m_Position, Math::CVector4(0,0,0,1), 1.0f );
            // m_Frustum.SetOrientation( m_Position, Math::CVector4( 0, 0, 0, 1
            // ) );
            if( m_hDbgView != INVALID_HANDLE ) {
            }
        }
        void CCamera::SetLookAt( const Math::CVector3& vecPoint )
        {
            m_LookAt = vecPoint;
        }
        void CCamera::SetPosition( const Math::CVector3& vecPosition )
        {
            m_vecPosition = vecPosition;
            // m_LookAt
        }
        void CCamera::SetUp( const Math::CVector3& Up ) { m_vecUp = Up; }
        void CCamera::SetFOV( const float angle )
        {
            m_fovAngle = angle;
            m_needProjUpdate = true;
        }
        void CCamera::SetClippingPlanes( const ExtentF32& Planes )
        {
            m_ClippingPlanes = Planes;
            m_needProjUpdate = true;
        }
        void CCamera::SetViewport( const ExtentF32& Viewport )
        {
            m_Viewport = Viewport;
            m_needProjUpdate = true;
        }
        void CCamera::Move( const Math::CVector3& vecDistance )
        {
            m_vecPosition += vecDistance;
        }
        void CCamera::Rotate( const Math::CVector3& vecAxis,
                              const float angleRadians )
        {
            /*const float halfAngle = angleRadians * 0.5f;
            const float sin = std::sinf( halfAngle );
            const float cos = std::cosf( halfAngle );
            Math::CQuaternion quatTmp;
            quatTmp.x = sin * vecAxis.x;
            quatTmp.y = sin * vecAxis.y;
            quatTmp.z = sin * vecAxis.z;
            quatTmp.w = cos;
            Math::CQuaternion quatNormalized;
            Math::CQuaternion::Normalize( quatTmp, &quatNormalized );
            m_quatOrientation *= quatNormalized;*/
        }
        void CCamera::Rotate( const float pitch, const float yaw,
                              const float roll )
        {
            /*DirectX::XMMATRIX rot, rotp, roty, rotr;
            Math::CVector4 look( m_LookAt );
            rotp = DirectX::XMMatrixRotationAxis( VKE_XMVEC4(
            Math::CVector4::NEGATIVE_X ), pitch ); roty =
            DirectX::XMMatrixRotationAxis( VKE_XMVEC4( Math::CVector4::Y ), yaw
            ); rotr = DirectX::XMMatrixRotationAxis( VKE_XMVEC4( look ), roll );
            rot = rotp * roty * rotr;
            look._Native = DirectX::XMVector3Normalize(
            DirectX::XMVector3Transform( look._Native, rot ) ); m_LookAt +=
            Math::CVector3( look );*/
            Math::CMatrix4x4 mtxPitch, mtxYaw, mtxRot;
            Math::CMatrix4x4::Rotation( Math::CVector4( m_vecRight ), yaw,
                                        &mtxPitch );
            Math::CMatrix4x4::RotationY( pitch, &mtxYaw );
            Math::CMatrix4x4::Mul( mtxPitch, mtxYaw, &mtxRot );
            _ApplyRotation( mtxRot );
        }
        void CCamera::SetAngleX( const float angleRadians )
        {
            /*m_vecAngleRadians.x = Math::NormalizeAngle( angleRadians );
            Math::CVector3 vecXAxis = Mul( m_quatOrientation, Math::CVector3::X
            ); Rotate( vecXAxis, angleRadians );*/
            Math::CQuaternion quatTmp;
            Math::CQuaternion::Rotate( angleRadians, 0.0f, 0.0f, &quatTmp );
            m_quatOrientation *= quatTmp;
        }
        void CCamera::SetAngleY( const float angleRadians )
        {
            /*m_vecAngleRadians.y = Math::Clamp( angleRadians, -Math::PI_DIV_2 +
            1e-3f, Math::PI_DIV_2 + 1e-3f ); Math::CVector3 vecYAxis = Mul(
            m_quatOrientation, Math::CVector3::Y ); Rotate( vecYAxis,
            angleRadians );*/
            Math::CQuaternion quatTmp;
            Math::CQuaternion::Rotate( 0.0f, angleRadians, 0.0f, &quatTmp );
            m_quatOrientation *= quatTmp;
        }
        float CCamera::CalcFrustumWidth( const float distance ) const
        {
            return 2.0f * distance * tanf( m_fovAngle * 0.5f );
        }
    } // namespace Scene
} // namespace VKE
