#include "Scene/CCamera.h"

namespace VKE
{
    namespace Scene
    {
        void CCamera::Update()
        {
            const float aspectRatio = m_Viewport.width / m_Viewport.height;

            m_ProjMatrix.SetPerspective( m_Viewport, m_ClippingPlanes );
            m_ProjMatrix.SetPerspectiveFOV( m_fovAngle, aspectRatio, m_ClippingPlanes );
            // Can't look at direction of 0,0,0
            if( Math::CVector3::Sub( m_LookAt, m_Position ).IsZero() )
            {
                m_LookAt.z -= 1.0f;
            }
            m_ViewMatrix.SetLookAt( m_Position, m_LookAt, m_Up );

            CalcViewProjectionMatrix( &m_ViewProjMatrix );
        }

        void CCamera::SetLookAt( const Math::CVector3& Position )
        {
            m_LookAt = Position;
        }

        void CCamera::SetPosition( const Math::CVector3& Position )
        {
            m_Position = Position;
        }

        void CCamera::SetUp( const Math::CVector3& Up )
        {
            m_Up = Up;
        }

    } // Scene
} // VKE
