#include "Scene/CWorld.h"
#include "Scene/CScene.h"
#include "Scene/CResourceManager.h"

namespace VKE
{
    namespace World
    {
        void CWorld::_Destroy()
        {
            for( uint32_t i = 0; i < m_vpScenes.GetCount(); ++i )
            {
                auto& pCurr = m_vpScenes[ i ];
                pCurr->_Destroy();
                Memory::DestroyObject( &HeapAllocator, &pCurr );
            }
            m_vpScenes.Clear();
            CResourceManager::GetInstance()._Destroy();
        }

        Result CWorld::_Create( const SDesc& Desc )
        {
            Result ret = VKE_FAIL;
            m_Desc     = Desc;
            m_vCameras.Resize( 8 );

            for( uint32_t i = 0; i < m_vCameras.GetCount(); ++i )
            {
                m_vCameras[ i ].Update( 0.0f );
            }

            auto           size                    = sizeof( RenderSystem::CDrawcall );
            const uint32_t maxDrawcallCountPerPool = 1000;
            const uint32_t poolCount               = Config::Scene::MAX_DRAWCALL_COUNT / maxDrawcallCountPerPool;
            if( VKE_SUCCEEDED( m_DrawcallMemMgr.Create( maxDrawcallCountPerPool, size, poolCount ) ) )
            {
                // Create default scene
                Scene::SSceneDesc Scene;
                auto       pScene = CreateScene( Scene );
                if( pScene!= nullptr )
                {
                    SetScene( pScene );
                    ret = VKE_OK;
                }
            }

            return ret;
        }

        Result CWorld::Create( RenderSystem::CommandBufferPtr pCmdBuffer )
        {
            Result ret = VKE_FAIL;
            if( m_pDevice == nullptr )
            {
                m_pDevice = pCmdBuffer->GetContext()->GetDeviceContext();
                {
                    SResourceManagerDesc Desc;
                    Desc.pDevice = m_pDevice;
                    ret = CResourceManager::GetInstance()._Create( Desc );
                }
                //ret       = m_pCurrScene->Init( pCmdBuffer );
            }
            return ret;
        }

        Scene::ScenePtr CWorld::CreateScene( const Scene::SSceneDesc& Desc )
        {
            Scene::CScene* pScene;
            Scene::ScenePtr pRet;
            if( VKE_SUCCEEDED( Memory::CreateObject( &HeapAllocator, &pScene, this ) ) )
            {
                if( VKE_SUCCEEDED( pScene->_Create( Desc ) ) )
                {
                    m_vpScenes.PushBack( pScene );
                    pRet = Scene::ScenePtr{ pScene };
                    m_pCurrScene = pScene;
                }
            }
            else
            {
                VKE_LOG_ERR( "Unable to create memory for CScene." );
                goto ERR;
            }
            return pRet;

        ERR:
            _DestroyScene( &pScene );
            return pRet;
        }

        void CWorld::_DestroyScene( Scene::CScene** ppInOut )
        {
            Scene::CScene* pScene = *ppInOut;
            pScene->_Destroy();
            Memory::DestroyObject( &HeapAllocator, ppInOut );
        }

        void CWorld::DestroyScene( Scene::ScenePtr* pInOut )
        {
            Scene::CScene* pScnee = ( *pInOut ).Release();
            _DestroyScene( &pScnee );
        }

        RenderSystem::DrawcallPtr CWorld::CreateDrawcall( const Scene::SDrawcallDesc& Desc )
        {
            RenderSystem::DrawcallPtr pRet;
            if( VKE_SUCCEEDED( Memory::CreateObject( &m_DrawcallMemMgr, &pRet ) ) )
            {
            }
            return pRet;
        }

        void CWorld::SetScene( Scene::ScenePtr pScene )
        {
            m_pCurrScene = pScene;
        }

        Scene::ScenePtr CWorld::GetScene()
        {
            return m_pCurrScene;
        }

    } // namespace Scene
} // namespace VKE
