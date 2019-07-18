#pragma once

#include "Scene/Common.h"
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
            friend class CScene;

            VKE_DECL_SCENE_OBJECT_DEBUG();

        public:

            CCamera()
            {
                Reset();
            }

            void Update(float time);

            void    Reset();
            void    SetFOV(const float angle);
            void    SetClippingPlanes(const ExtentF32& Planes);
            void    SetViewport(const ExtentF32& Viewport);
            void    SetPosition(const Math::CVector3& Position);
            void    SetUp(const Math::CVector3& Up);
            void    SetLookAt(const Math::CVector3& Position);
            void    Move(const Math::CVector3& vecDistance);
            void    Rotate(const Math::CVector3& vecAxis, const float angleRadians);
            void    Rotate(const float pitch, const float yaw, const float roll);
            void    SetAngleX(const float angleRadians);
            void    SetAngleY(const float angleRadians);
            void    SetYaw(const float angleRadians)
            {
                SetAngleY(angleRadians);
            }
            void    SetPitch(const float angleRadians)
            {
                SetAngleX(angleRadians);
            }
            void    RotateX(const float angleRadians)
            {
                SetAngleX(m_vecAngleRadians.x + angleRadians);
            }
            void    RotateY(const float angleRadians)
            {
                SetAngleY(m_vecAngleRadians.y + angleRadians);
            }

            const Math::CVector3&   GetPosition() const
            {
                return m_vecPosition;
            }
            const Math::CVector3&   GetLookAt() const
            {
                return m_LookAt;
            }
            const Math::CVector3&   GetUp() const
            {
                return m_vecUp;
            }
            const Math::CVector3&   GetRight() const
            {
                return m_vecRight;
            }
            const Math::CVector3&   GetDirection() const
            {
                return m_vecDirection;
            }
            const ExtentF32&    GetClippingPlanes() const
            {
                return m_ClippingPlanes;
            }

            const Math::CMatrix4x4& GetViewMatrix() const
            {
                return m_ViewMatrix;
            }
            const Math::CMatrix4x4& GetProjectionMatrix() const
            {
                return m_ProjMatrix;
            }
            const Math::CMatrix4x4& GetViewProjectionMatrix() const
            {
                return m_ViewProjMatrix;
            }
            const Math::CFrustum& GetFrustum() const
            {
                return m_Frustum;
            }

            void CalcViewProjectionMatrix(Math::CMatrix4x4* pOut)
            {
                Math::CMatrix4x4::Mul(m_ViewMatrix, m_ProjMatrix, pOut);
            }

            float GetFOVAngleRadians() const { return m_fovAngle; }
            const ExtentF32 GetViewport() const { return m_Viewport; }

        protected:

            void    _UpdateViewMatrix();
            void    _UpdateProjMatrix();
            void    _ApplyRotation(const Math::CMatrix4x4& mtxTransform);

        protected:

            Math::CVector3      m_vecPosition;
            Math::CVector3      m_vecDirection;
            Math::CVector3      m_LookAt;
            Math::CVector3      m_vecUp;
            Math::CVector3      m_vecRight;
            Math::CVector3      m_vecAngleRadians;
            Math::CQuaternion   m_quatOrientation = Math::CQuaternion::UNIT;
            ExtentF32           m_ClippingPlanes = {1.0f, 1000.0f};
            ExtentF32           m_Viewport = {800, 600};
            float               m_fovAngle = 45.0f;
            uint32_t            m_handle;
            uint32_t            m_hDbgView = UNDEFINED_U32;

            bool                m_needProjUpdate = true;

            Math::CMatrix4x4    m_ViewMatrix;
            Math::CMatrix4x4    m_ProjMatrix;
            Math::CMatrix4x4    m_ViewProjMatrix;
            Math::CFrustum      m_Frustum;
        };

        using CameraPtr = CCamera * ;
    } // Scene
} // VKE
