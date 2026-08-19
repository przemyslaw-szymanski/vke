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
    } // namespace RenderSystem

    namespace Scene
    {
        class CScene;
        class CDrawcall;

        using ScenePtr    = Utils::TCWeakPtr< class CScene >;
        using DrawcallPtr = CDrawcall*;
    }

    namespace World
    {
        class VKE_API CWorld
        {
            friend class CVkEngine;
            friend class CScene;

            struct SDesc
            {
            };

            using CameraArray    = Utils::TCDynamicArray< Scene::CCamera, 8 >;
            using SceneArray     = Utils::TCDynamicArray< Scene::CScene* >;
            using DrawcallMemMgr = Memory::CFreeListPool;

        public:

            vke_force_inline static CWorld& GetInstance()
            {
                static CWorld World;
                return World;
            }

            Result Create( RenderSystem::CommandBufferPtr );

            Scene::CameraPtr GetCamera( uint32_t idx )
            {
                return &m_vCameras[ idx ];
            }

            Scene::ScenePtr CreateScene( const Scene::SSceneDesc& Desc );
            void     SetScene( Scene::ScenePtr );
            Scene::ScenePtr GetScene();
            void     DestroyScene( Scene::ScenePtr* pInOut );

            RenderSystem::DrawcallPtr CreateDrawcall( const Scene::SDrawcallDesc& Desc );

        protected:
            Result _Create( const SDesc& Desc );
            void   _Destroy();

            void _DestroyScene( Scene::CScene** ppInOut );

        protected:
            SDesc                         m_Desc;
            RenderSystem::CDeviceContext* m_pDevice = nullptr;
            CameraArray                   m_vCameras;
            SceneArray                    m_vpScenes;
            Scene::ScenePtr               m_pCurrScene;
            DrawcallMemMgr                m_DrawcallMemMgr;
        };

        vke_force_inline static CWorld& GetInstance()
        {
            return CWorld::GetInstance();
        }
    } // namespace World
} // namespace VKE
