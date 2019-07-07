#include "Scene/CWorld.h"
#include "Scene/CScene.h"

namespace VKE
{
    namespace Scene
    {
        void CWorld::_Destroy()
        {
            for( uint32_t i = 0; i < m_vpScenes.GetCount(); ++i )
            {
                auto& pCurr = m_vpScenes[i];
                pCurr->_Destroy();
                Memory::DestroyObject( &HeapAllocator, &pCurr );
            }
            m_vpScenes.Clear();
        }

        Result CWorld::_Create( const SDesc& Desc )
        {
            Result ret = VKE_FAIL;
            m_Desc = Desc;
            m_vCameras.Resize( 8 );

            for( uint32_t i = 0; i < m_vCameras.GetCount(); ++i )
            {
                m_vCameras[i].Update(0.0f);
            }

            auto size = sizeof( RenderSystem::CDrawcall );
            const uint32_t maxDrawcallCountPerPool = 1000;
            const uint32_t poolCount = Config::Scene::MAX_DRAWCALL_COUNT / maxDrawcallCountPerPool;
            if( VKE_FAILED( m_DrawcallMemMgr.Create( maxDrawcallCountPerPool, size, poolCount ) ) )
            {
                goto ERR;
            }
            ret = VKE_OK;
            return ret;
        ERR:
            return ret;
        }

        ScenePtr CWorld::CreateScene( const SSceneDesc& Desc )
        {
            CScene* pScene;
            ScenePtr pRet;
            if( VKE_SUCCEEDED( Memory::CreateObject( &HeapAllocator, &pScene, this ) ) )
            {
                if( VKE_SUCCEEDED( pScene->_Create( Desc ) ) )
                {
                    m_vpScenes.PushBack( pScene );
                    pRet = ScenePtr{ pScene };
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

        void CWorld::_DestroyScene( CScene** ppInOut )
        {
            CScene* pScene = *ppInOut;
            pScene->_Destroy();
            Memory::DestroyObject( &HeapAllocator, ppInOut );
        }

        void CWorld::DestroyScene( ScenePtr* pInOut )
        {
            CScene* pScnee = (*pInOut).Release();
            _DestroyScene( &pScnee );
        }

        RenderSystem::DrawcallPtr CWorld::CreateDrawcall( const SDrawcallDesc& Desc )
        {
            RenderSystem::DrawcallPtr pRet;
            if( VKE_SUCCEEDED( Memory::CreateObject( &m_DrawcallMemMgr, &pRet ) ) )
            {

            }
            return pRet;
        }

    } // Scene
} // VKE
