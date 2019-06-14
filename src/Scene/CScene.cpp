#include "Scene/CScene.h"
#include "Scene/CCamera.h"

#include "Scene/COctree.h"
#include "Scene/CBVH.h"
#include "Scene/CQuadTree.h"

#include "RenderSystem/CGraphicsContext.h"

namespace VKE
{
    namespace Scene
    {
        Result CScene::_Create( const SSceneDesc& Desc)
        {
            Result ret = VKE_OK;
            switch( Desc.graphSystem )
            {
                case GraphSystems::OCTREE:
                {
                    ret = Memory::CreateObject( &HeapAllocator, &m_pOctree, this );
                    if( VKE_SUCCEEDED( ret ) )
                    {
                        ret = m_pOctree->_Create( Desc.OctreeDesc );
                    }
                }
                break;
            }
            return ret;
        }

        void CScene::_Destroy()
        {
            if( m_pOctree )
            {
                m_pOctree->_Destroy();
                Memory::DestroyObject( &HeapAllocator, &m_pOctree );
            }
            m_vpDrawcalls.Clear();
        }

        CameraPtr CScene::CreateCamera( cstr_t dbgName )
        {
            const uint32_t hCam = m_vCameras.PushBack( {} );
            CCamera* pCam = &m_vCameras[ hCam ];
 
            return CameraPtr{ pCam };
        }

        handle_t CScene::AddObject( DrawcallPtr pDrawcall, const SDrawcallDataInfo& Info )
        {
            auto handle2 = m_vpDrawcalls.PushBack( pDrawcall );
            auto handle = m_DrawData.Add( Info );
            VKE_ASSERT( handle == handle2, "" );
            UObjectHandle Handle;
            Handle.index = handle;
            Handle.type = ObjectTypes::DRAWCALL;
            
            //auto pBits = &m_DrawData.GetBits( handle );
            auto& AABB = m_DrawData.GetAABB( handle );
            Handle.graphIndex = m_pOctree->AddObject( AABB, Handle );
            pDrawcall->m_handle = Handle;

            return Handle.handle;
        }

        void CScene::Render( VKE::RenderSystem::CGraphicsContext* pCtx )
        {
            m_pCurrentCamera->Update(0);
            const Math::CFrustum& Frustum = m_pCurrentCamera->GetFrustum();
            _FrustumCullDrawcalls( Frustum );
            _SortDrawcalls( Frustum );
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
        }

        void CScene::_SortDrawcalls( const Math::CFrustum& Frustum )
        {

        }

        void CScene::_Draw( VKE::RenderSystem::CGraphicsContext* pCtx )
        {
            RenderSystem::CCommandBuffer* pCmdBuffer = pCtx->GetCommandBuffer();
            static uint32_t c = 0;
            c++;
            for( uint32_t i = 0; i < m_DrawData.vVisibles.GetCount(); ++i )
            {
                //const auto& Bits = m_DrawData.vBits[i];
                //if( Bits.visible )
                if( m_DrawData.vVisibles[ i ] )
                { 
                    // Load this drawcall == cache miss
                    const DrawcallPtr pDrawcall = m_vpDrawcalls[ i ];
                    const auto& LOD = pDrawcall->m_vLODs[ pDrawcall->m_currLOD ];
                    
                    pCmdBuffer->Bind( LOD.hVertexBuffer, LOD.vertexBufferOffset );
                    pCmdBuffer->Bind( LOD.hIndexBuffer, LOD.indexBufferOffset );
                    pCmdBuffer->Bind( LOD.hDescSet, LOD.descSetOffset );
                    /*pCmdBuffer->SetState( LOD.InputLayout );
                    pCmdBuffer->SetState( *(LOD.ppVertexShader) );
                    pCmdBuffer->SetState( *(LOD.ppPixelShader) );
                    pCmdBuffer->SetState( LOD.InputLayout.topology );*/
                    pCmdBuffer->Bind( *LOD.ppPipeline );
                    pCmdBuffer->DrawIndexed( LOD.DrawParams );
                }
            }
        }

        handle_t CScene::_CreateSceneNode(const uint32_t idx)
        {
            UObjectHandle Ret;
            auto handle = m_DrawData.Add();
            Ret.type = ObjectTypes::SCENE_NODE;
            Ret.index = handle;
            return Ret.handle;
        }

    } // Scene
} // VKE
