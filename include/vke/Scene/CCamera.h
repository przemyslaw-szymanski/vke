#pragma once

#include "Core/VKECommon.h"
#include "Core/Utils/TCSmartPtr.h"
#include "Core/Math/Math.h"

namespace VKE
{
    struct SSceneInfo
    {

    };

    namespace RenderSystem
    {
        class CDrawcall;
    }

    namespace Scene
    {
        class VKE_API CCamera
        {
            public:

                CCamera() {}

                void Update();

                void    SetFOV( const float angle );
                void    SetClippingPlanes( const ExtentF32& Planes );
                void    SetViewport( const ExtentF32& Viewport );
                void    SetPosition( const Math::CVector3& Position );
                void    SetUp( const Math::CVector3& Up );
                void    SetLookAt( const Math::CVector3& Position );
                void    Move( const Math::CVector3& vecDistance );
                void    Rotate( const Math::CVector3& vecAxis, const float angleRadians );
                void    SetAngleX( const float angleRadians );
                void    SetAngleY( const float angleRadians );
                void    SetYaw( const float angleRadians ) { SetAngleY( angleRadians ); }
                void    SetPitch( const float angleRadians ) { SetAngleX( angleRadians ); }
                void    RotateX( const float angleRadians ) { SetAngleX( m_vecAngleRadians.x + angleRadians ); }
                void    RotateY( const float angleRadians ) { SetAngleY( m_vecAngleRadians.y + angleRadians ); }

                const Math::CVector3&    GetPosition() const { return m_Position; }
                const Math::CVector3&    GetLookAt() const { return m_LookAt; }
                const Math::CVector3&    GetUp() const { return m_Up; }

                const Math::CMatrix4x4& GetViewMatrix() const { return m_ViewMatrix; }
                const Math::CMatrix4x4& GetProjectionMatrix() const { return m_ProjMatrix; }
                const Math::CMatrix4x4& GetViewProjectionMatrix() const { return m_ViewProjMatrix; }
                const Math::CFrustum& GetFrustum() const { return m_Frustum; }

                void CalcViewProjectionMatrix( Math::CMatrix4x4* pOut )
                {
                    Math::CMatrix4x4::Mul( m_ViewMatrix, m_ProjMatrix, pOut );
                }

            protected:
                
                Math::CVector3      m_Position = Math::CVector3::ZERO;
                Math::CVector3      m_LookAt = Math::CVector3::ZERO;
                Math::CVector3      m_Up = Math::CVector3::Y;
                Math::CVector3      m_vecAngleRadians;
                Math::CVector3      m_vecDirection = Math::CVector3::Z;
                Math::CVector4      m_vecOrientation = Math::CVector4::W;
                ExtentF32           m_ClippingPlanes = { 1.0f, 1000.0f };
                ExtentF32           m_Viewport = { 800, 600 };
                float               m_fovAngle = 45.0f;
                uint32_t            m_handle;
                
                bool                m_needProjUpdate = true;

                Math::CMatrix4x4    m_ViewMatrix;
                Math::CMatrix4x4    m_ProjMatrix;
                Math::CMatrix4x4    m_ViewProjMatrix;
                Math::CFrustum      m_Frustum;
        };

        using CameraPtr = CCamera*;
    } // Scene
} // VKE
