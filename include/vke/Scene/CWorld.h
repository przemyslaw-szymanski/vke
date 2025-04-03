#pragma once

#include "Common.h"
#include "CCamera.h"
#include "Core/Utils/TCDynamicArray.h"
#include "Core/Memory/CFreeListPool.h"

namespace VKE
{
    namespace RenderSystem
    {
        class CDrawcall;
        using DrawcallPtr = CDrawcall*;
    }

    namespace Scene
    {
        class CScene;
        class CDrawcall;

        using ScenePtr = Utils::TCWeakPtr<class CScene>;
        using DrawcallPtr = CDrawcall*;

        class VKE_API CWorld
        {
            friend class CVkEngine;
            friend class CScene;

            struct SDesc
            {

            };

            using CameraArray = Utils::TCDynamicArray< CCamera, 8 >;
            using SceneArray = Utils::TCDynamicArray< CScene* >;
            using DrawcallMemMgr = Memory::CFreeListPool;

            public:

                Result      Init( RenderSystem::CommandBufferPtr );

                CameraPtr   GetCamera( uint32_t idx ) { return &m_vCameras[idx]; }

                ScenePtr    CreateScene( const SSceneDesc& Desc );
                void        SetScene(ScenePtr);
                ScenePtr    GetScene();
                void        DestroyScene( ScenePtr* pInOut );

                RenderSystem::DrawcallPtr CreateDrawcall(const SDrawcallDesc& Desc);
                
            protected:

                Result  _Create(const SDesc& Desc);
                void    _Destroy();

                void    _DestroyScene( CScene** ppInOut );

            protected:

              SDesc                         m_Desc;
              RenderSystem::CDeviceContext* m_pDevice = nullptr;
              CameraArray                   m_vCameras;
              SceneArray                    m_vpScenes;
              ScenePtr                      m_pCurrScene;
              DrawcallMemMgr                m_DrawcallMemMgr;
        };
    }
} // VKE
