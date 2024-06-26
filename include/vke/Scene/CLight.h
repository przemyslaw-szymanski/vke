#pragma once
#include "Core/CObject.h"
#include "Scene/Common.h"
namespace VKE
{
    namespace Scene
    {
        class CScene;
        struct SLights;
        class VKE_API CLight
        {
            friend class CSceneManager;
            friend class CScene;
            friend struct SLights;
            VKE_DECL_BASE_OBJECT( LightHandle );
            VKE_DECL_BASE_RESOURCE();

          public:
            struct SData
            {
                Math::CVector3 vecPos;
                float radius;
                Math::CVector3 vecDir;
                float attenuation;
                Math::CVector3 vecColor;
                float pad1[ 1 ];
            };

          public:
            CLight()
            {
            }
            ~CLight()
            {
            }
            void IsEnabled( bool );
            bool IsEnabled() const;
            const SLightDesc& GetDesc() const
            {
                return m_Desc;
            }

            SData GetData() const
            {
                SData Ret;
                GetData( &Ret );
                return Ret;
            }

            void SetPosition( const Math::CVector3& );

            void CalcMatrix( Math::CMatrix4x4* ) const;

            void GetData(SData*) const;
            uint32_t WriteConstantBufferData( uint8_t** ) const;

          protected:
            Result _Create( const SLightDesc&, SLights*, uint32_t );
            void _Destroy();
            void _SetDebugView( uint32_t );

          protected:
            SLightDesc m_Desc;
            SLights* m_pLights;
            uint32_t m_index;
            uint32_t m_hDbgView = UNDEFINED_U32;
        };
        using LightRefPtr = Utils::TCObjectSmartPtr<CLight>;
        using LightPtr = Utils::TCWeakPtr<CLight>;
    } // namespace Scene
} // namespace VKE