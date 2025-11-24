#pragma once
#include "Core/Math/Math.h"
#include "Core/Utils/TCSmartPtr.h"
#include "Scene/Common.h"

namespace VKE
{
    struct SSceneInfo
    {
    };

    namespace RenderSystem
    {
        class CDrawcall;
    } // namespace RenderSystem

    namespace Scene
    {
        class VKE_API CCamera
        {
            friend class CScene;
            VKE_DECL_SCENE_OBJECT_DEBUG();

        public:
            struct SData
            {
                Math::CMatrix4x4       mtxViewProj;
                Math::CVector3         vec3Position;
                float                  pad1;
                Math::CVector3         vec3Direction;
                float                  pad2;
                Math::CFrustum::Planes aFrustumPlanes;
            };

        public:
            CCamera()
            {
                Reset();
            }

            void Update( float time );
            void Reset();
            void SetFOV( Math::Radians angle );
            void SetClippingPlanes( const ExtentF32& Planes );
            void SetViewport( const ExtentF32& Viewport );
            void SetPosition( const Math::CVector3& Position );
            void SetUp( const Math::CVector3& Up );
            void SetLookAt( const Math::CVector3& Position );
            void Move( const Math::CVector3& vecDistance );
            void Rotate( const Math::CVector3& vecAxis, Math::Radians angleRadians );
            void Rotate( Math::Radians pitch, Math::Radians yaw, Math::Radians roll );
            void SetAngleX( Math::Radians angleRadians );
            void SetAngleY( Math::Radians angleRadians );

            void SetYaw( Math::Radians angleRadians )
            {
                SetAngleY( angleRadians );
            }

            void SetPitch( Math::Radians angleRadians )
            {
                SetAngleX( angleRadians );
            }

            void RotateX( Math::Radians angleRadians )
            {
                SetAngleX( Math::Radians( m_rotateX + angleRadians ) );
            }

            void RotateY( Math::Radians angleRadians )
            {
                SetAngleY( Math::Radians( m_rotateY + angleRadians ) );
            }

            const Math::CVector3& GetPosition() const
            {
                return m_Desc.vecPosition;
            }

            const Math::CVector3& GetLookAt() const
            {
                return m_Desc.vecLookAt;
            }

            const Math::CVector3& GetUp() const
            {
                return m_Desc.vecUp;
            }

            const Math::CVector3& GetRight() const
            {
                return m_Desc.vecRight;
            }

            const Math::CVector3& GetDirection() const
            {
                return m_vecDirection;
            }

            const ExtentF32& GetClippingPlanes() const
            {
                return m_Desc.ClipPlanes;
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

            void CalcViewProjectionMatrix( Math::CMatrix4x4* pOut )
            {
                Math::CMatrix4x4::Mul( m_ViewMatrix, m_ProjMatrix, pOut );
            }

            Math::Radians GetFOV() const
            {
                return m_fovAngle;
            }

            const ExtentF32& GetViewport() const
            {
                return m_Desc.Viewport;
            }

            /// <summary>
            /// Gets min/max frustum widths
            /// </summary>
            /// <returns></returns>
            const ExtentF32& GetFrustumWidth() const
            {
                return m_FrustumWidth;
            }

            // Calculates frustum width at some distance
            float CalcFrustumWidth( const float distance ) const;

        protected:
            void _Init( const SCameraDesc& );
            void _UpdateViewMatrix();
            void _UpdateProjMatrix();
            void _ApplyRotation( const Math::CMatrix4x4& mtxTransform );

        protected:
            SCameraDesc       m_Desc;
            Math::CVector3    m_vecDirection;
            Math::CQuaternion m_quatOrientation = Math::CQuaternion::UNIT;
            Math::Radians     m_fovAngle        = Math::Radians( Math::Degrees( 45 ) );
            Math::Radians     m_rotateX         = Math::Radians( 0 );
            Math::Radians     m_rotateY         = Math::Radians( 0 );
            Math::Radians     m_rotateZ         = Math::Radians( 0 );
            /// <summary>
            /// Min frustum width at near distance and max at far distance
            /// </summary>
            ExtentF32         m_FrustumWidth;
            uint32_t          m_handle;
            uint32_t          m_hDbgView       = UNDEFINED_U32;
            bool              m_needProjUpdate = true;
            Math::CMatrix4x4  m_ViewMatrix;
            Math::CMatrix4x4  m_ProjMatrix;
            Math::CMatrix4x4  m_ViewProjMatrix;
            Math::CFrustum    m_Frustum;
        };

        using CameraPtr = CCamera*;
    } // namespace Scene
} // namespace VKE
