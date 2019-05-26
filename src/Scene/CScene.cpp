#include "Scene/CScene.h"
#include "Scene/CCamera.h"

namespace VKE
{
    namespace Scene
    {
        Result CScene::_Create( const SSceneDesc& )
        {
            Result ret = VKE_OK;
            return ret;
        }

        void CScene::_Destroy()
        {
            m_vDrawcalls.Clear();
        }

        CameraPtr CScene::CreateCamera( cstr_t dbgName )
        {
            const uint32_t hCam = m_vCameras.PushBack( {} );
            CCamera* pCam = &m_vCameras[ hCam ];
 
            return CameraPtr{ pCam };
        }

        uint32_t CScene::AddObject( CDrawcall* pInOut )
        {
            auto handle = m_DrawData.Add();
            CDrawcall Drawcall;
            Drawcall.m_handle = handle;
            auto handle2 = m_vDrawcalls.PushBack( Drawcall );
            VKE_ASSERT( handle == handle2, "" );
            pInOut = &m_vDrawcalls[ handle ];
            return handle;
        }

        void CScene::Render( VKE::RenderSystem::CGraphicsContext* pCtx )
        {
            const Math::CFrustum& Frustum = m_pCurrentCamera->GetFrustum();
            _FrustumCullDrawcalls( Frustum );
            _SortDrawcalls( Frustum );
            _Draw( pCtx );
        }

        void CScene::_FrustumCullDrawcalls( const Math::CFrustum& Frustum )
        {
            const auto& vAABBs = m_DrawData.vAABBs;
            const uint32_t count = vAABBs.GetCount();
            for( uint32_t i = 0; i < count; ++i )
            {
                m_DrawData.vVisibles[ i ] = true;
            }
        }

        void CScene::_SortDrawcalls( const Math::CFrustum& Frustum )
        {

        }

        void CScene::_Draw( VKE::RenderSystem::CGraphicsContext* pCtx )
        {

        }

    } // Scene
} // VKE
