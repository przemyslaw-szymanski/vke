#include "Scene/CLight.h"
#include "Scene/CScene.h"

namespace VKE
{
    namespace Scene
    {
        Result CLight::_Create( const SLightDesc& Desc, SLights* pLights, uint32_t index )
        {
            Result ret = VKE_OK;
            m_Desc = Desc;
            m_pLights = pLights;
            m_index = index;
            return ret;
        }

        void CLight::GetData(SData* pOut) const
        {
            pOut->vecPos = m_Desc.vecPosition;
            pOut->vecDir = m_Desc.vecDirection;
            pOut->vecColor = { m_Desc.Color.r, m_Desc.Color.g, m_Desc.Color.b };
            pOut->attenuation = m_Desc.attenuation;
            pOut->radius = m_Desc.radius;
        }

        void CLight::_Destroy() {}

        void CLight::SetPosition( const Math::CVector3& vecPos)
        {
            m_Desc.vecPosition = vecPos;
            m_pLights->vPositions[ m_index ] = vecPos;
        }

        void CLight::_SetDebugView( uint32_t hView)
        {
            m_hDbgView = hView;
            m_pLights->vDbgViews[ m_index ] = hView;
        }

        void CLight::CalcMatrix( Math::CMatrix4x4* pOut) const
        {
            pOut->Transform( Math::CVector4::ONE, Math::CVector4::ZERO, Math::CQuaternion::UNIT, Math::CVector4( m_Desc.vecPosition ) );
        }

    } // Scene
} // VKE