#include "Scene/CCamera.h"

namespace VKE
{
    namespace Scene
    {
        

        void CCamera::Update()
        {
            Math:SphericalToCartesian( m_vecAngleRadians.x - DirectX::XM_PIDIV2, -m_vecAngleRadians.y, 1.0f, &m_LookAt );
            m_LookAt.Normalize( &m_vecDirection );

            if( m_needProjUpdate )
            {
                m_needProjUpdate = false;
                const float aspectRatio = m_Viewport.width / m_Viewport.height;
                m_ProjMatrix.SetPerspective( m_Viewport, m_ClippingPlanes );
                m_ProjMatrix.SetPerspectiveFOV( m_fovAngle, aspectRatio, m_ClippingPlanes );
                
            }

            // Can't look at direction of 0,0,0
            if( Math::CVector3::Sub( m_LookAt, m_Position ).IsZero() )
            {
                m_LookAt.z -= 1.0f;
            }
            m_ViewMatrix.SetLookAt( m_Position, m_LookAt + m_Position, m_Up );

            CalcViewProjectionMatrix( &m_ViewProjMatrix );

            //m_Frustum.Transform( m_ViewMatrix );
            //m_Frustum.Transform( m_Position, Math::CVector4(0,0,0,1), 1.0f );
            m_Frustum.CreateFromMatrix( m_ViewProjMatrix );
            //m_Frustum.SetOrientation( m_Position, Math::CVector4( 0, 0, 0, 1 ) );
        }

        void CCamera::SetLookAt( const Math::CVector3& vecPoint )
        {
            m_LookAt = vecPoint;
        }

        void CCamera::SetPosition( const Math::CVector3& vecPosition )
        {
            m_Position = vecPosition;
            //m_LookAt
        }

        void CCamera::SetUp( const Math::CVector3& Up )
        {
            m_Up = Up;
        }

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
            m_Position += vecDistance;
        }

        void CCamera::Rotate( const Math::CVector3& vecAxis, const float angleRadians )
        {
            const float halfAngle = angleRadians * 0.5f;
            const float sin = std::sinf( halfAngle );
            const float cos = std::cosf( halfAngle );
            Math::CVector4 vecQuaternion;
            vecQuaternion.x = sin * vecAxis.x;
            vecQuaternion.y = sin * vecAxis.y;
            vecQuaternion.z = sin * vecAxis.z;
            vecQuaternion.w = cos;
            Math::CVector4 vecQuatNorm;
            Math::CVector4::Normalize( vecQuaternion, &vecQuatNorm );
            m_vecOrientation *= vecQuatNorm;
        }
  

        void CCamera::SetAngleX( const float angleRadians )
        {
            m_vecAngleRadians.x = Math::NormalizeAngle( angleRadians );
            Math::CVector3 vecXAxis = Mul( m_vecOrientation, Math::CVector3::X );
            Rotate( vecXAxis, angleRadians );
        }     

        void CCamera::SetAngleY( const float angleRadians )
        {            
            m_vecAngleRadians.y = Math::Clamp( angleRadians, -Math::PI_DIV_2 + 1e-3f, Math::PI_DIV_2 + 1e-3f );
            Math::CVector3 vecYAxis = Mul( m_vecOrientation, Math::CVector3::Y );
            Rotate( vecYAxis, angleRadians );
        }

    } // Scene
} // VKE
