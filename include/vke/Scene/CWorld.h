#pragma once

#include "Common.h"
#include "CCamera.h"
#include "Core/Utils/TCDynamicArray.h"

namespace VKE
{
    namespace RenderSystem
    {
        class CDrawcall;
    }

    namespace Scene
    {
        class CScene;
        using ScenePtr = Utils::TCWeakPtr<class CScene>;

        class VKE_API CWorld
        {
            friend class CVkEngine;
            friend class CScene;

            struct SDesc
            {

            };

            using CameraArray = Utils::TCDynamicArray< CCamera, 8 >;
            using SceneArray = Utils::TCDynamicArray< CScene* >;

            public:

                CameraPtr   GetCamera( uint32_t idx ) { return &m_vCameras[idx]; }

                ScenePtr    CreateScene( const SSceneDesc& Desc );
                void        DestroyScene( ScenePtr* pInOut );
                
            protected:

                Result  _Create(const SDesc& Desc);
                void    _Destroy();

                void    _DestroyScene( CScene** ppInOut );

            protected:

                SDesc       m_Desc;
                CameraArray m_vCameras;
                SceneArray  m_vpScenes;
        };
    }
} // VKE
