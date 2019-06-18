#include "Scene/CScene.h"
#include "Scene/CCamera.h"

#include "Scene/COctree.h"
#include "Scene/CBVH.h"
#include "Scene/CQuadTree.h"

#include "RenderSystem/CGraphicsContext.h"
#include "RenderSystem/FrameGraph/CForwardRenderer.h"

#include "Scene/CWorld.h"

namespace VKE
{
    namespace Scene
    {
        Result CScene::_Create( const SSceneDesc& Desc)
        {
            Result ret = VKE_FAIL;
            SOctreeDesc OctDesc;
            SSceneGraphDesc SceneGraphDesc = Desc.SceneGraphDesc;
            RenderSystem::SFrameGraphDesc FrameGraphDesc = Desc.FrameGraphDesc;

            if( SceneGraphDesc.pDesc == nullptr )
            {
                SceneGraphDesc.pDesc = &OctDesc;
                SceneGraphDesc.pName = SCENE_GRAPH_OCTREE_NAME;
            }
            
            if( strcmp( SceneGraphDesc.pName, SCENE_GRAPH_OCTREE_NAME ) == 0 )
            {
                ret = Memory::CreateObject( &HeapAllocator, &m_pOctree, this );
                if( VKE_SUCCEEDED( ret ) )
                {
                    SOctreeDesc* pDesc = reinterpret_cast< SOctreeDesc* >( SceneGraphDesc.pDesc );
                    ret = m_pOctree->_Create( *pDesc );
                }
            }

            RenderSystem::SForwardRendererDesc FwdDesc;
            if( FrameGraphDesc.pDesc == nullptr )
            {
                FrameGraphDesc.pDesc = &FwdDesc;
                FrameGraphDesc.pName = RenderSystem::FRAME_GRAPH_FORWARD_RENDERER_NAME;
            }

            if( strcmp( FrameGraphDesc.pName, RenderSystem::FRAME_GRAPH_FORWARD_RENDERER_NAME ) == 0 )
            {
                m_pFrameGraph = VKE_NEW RenderSystem::CForwardRenderer();
            }
            VKE_ASSERT( m_pFrameGraph != nullptr, "" );
            m_pFrameGraph->SetScene( this );
            return ret;
        }

        void CScene::_Destroy()
        {
            if( m_pOctree )
            {
                m_pOctree->_Destroy();
                Memory::DestroyObject( &HeapAllocator, &m_pOctree );
            }
            if( m_pFrameGraph )
            {
                m_pFrameGraph->_Destroy();
                VKE_DELETE( m_pFrameGraph );
            }

            m_vpDrawcalls.Clear();
        }

        RenderSystem::DrawcallPtr CScene::CreateDrawcall( const Scene::SDrawcallDesc& Desc )
        {
            auto pDrawcall = m_pWorld->CreateDrawcall( Desc );

            return pDrawcall;
        }

        CameraPtr CScene::CreateCamera( cstr_t dbgName )
        {
            const uint32_t hCam = m_vCameras.PushBack( {} );
            CCamera* pCam = &m_vCameras[ hCam ];
 
            return CameraPtr{ pCam };
        }

        handle_t CScene::AddObject( RenderSystem::DrawcallPtr pDrawcall, const SDrawcallDataInfo& Info )
        {
            auto handle2 = m_vpDrawcalls.PushBack( pDrawcall );
            auto handle = m_DrawData.Add( Info );
            VKE_ASSERT( handle == handle2, "" );
            RenderSystem::UObjectHandle Handle;
            RenderSystem::UDrawcallHandle hDrawcall;
            hDrawcall.reserved1 = handle;
            Handle.index = handle;
            Handle.type = Scene::ObjectTypes::DRAWCALL;
            
            //auto pBits = &m_DrawData.GetBits( handle );
            auto& AABB = m_DrawData.GetAABB( handle );
            COctree::UObjectHandle hNodeObj = m_pOctree->AddObject( AABB, Handle );
            pDrawcall->m_hObj = Handle;
            pDrawcall->m_hSceneGraph = hNodeObj.handle;

            return Handle.handle;
        }

        void CScene::Render( VKE::RenderSystem::CGraphicsContext* pCtx )
        {
            m_pCurrentCamera->Update(0);
            const Math::CFrustum& Frustum = m_pCurrentCamera->GetFrustum();
            _FrustumCullDrawcalls( Frustum );
            _Draw( pCtx );
        }

        void CScene::_FrustumCullDrawcalls( const Math::CFrustum& Frustum )
        {
            /*const auto& vAABBs = m_DrawData.vAABBs;
            const uint32_t count = vAABBs.GetCount();
            for( uint32_t i = 0; i < count; ++i )
            {
                m_DrawData.vBits[ i ].visible = true;
            }*/
            if( m_pOctree )
            {
                m_pOctree->FrustumCull( m_pCurrentCamera->GetFrustum() );
            }
            for( uint32_t i = 0; i < m_DrawData.vVisibles.GetCount(); ++i )
            {
                if( m_DrawData.vVisibles[i] )
                {
                    m_vpVisibleDrawcalls.PushBack( m_vpDrawcalls[i] );
                }
            }
        }

        void CScene::_SortDrawcalls( const Math::CFrustum& Frustum )
        {

        }

        void CScene::_Draw( VKE::RenderSystem::CGraphicsContext* pCtx )
        {
            m_pFrameGraph->Render( pCtx );
        }

        handle_t CScene::_CreateSceneNode(const uint32_t idx)
        {
            UObjectHandle Ret;
            Ret.handle = 0;
            return Ret.handle;
        }

    } // Scene
} // VKE
