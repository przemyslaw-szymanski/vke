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
            Result ret = VKE_OK;
            m_Desc = Desc;
            m_vCameras.Resize( 8 );

            for( uint32_t i = 0; i < m_vCameras.GetCount(); ++i )
            {
                m_vCameras[i].Update();
            }
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

    } // Scene
} // VKE
