#include "Scene/CScene.h"
#include "Scene/CCamera.h"

#include "RenderSystem/CGraphicsContext.h"

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

        handle_t CScene::AddObject( CDrawcall* pInOut )
        {
            auto handle2 = m_vDrawcalls.PushBack( *pInOut );
            auto handle = m_DrawData.Add( handle2 );
            UObjectHandle Handle;
            Handle.objDataIndex = handle;
            Handle.objTypeIndex = handle2;
            Handle.type = ObjectTypes::DRAWCALL;
            m_vDrawcalls[handle2].m_handle = Handle.handle;
            return Handle.handle;
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
                m_DrawData.vBits[ i ].visible = true;
            }
        }

        void CScene::_SortDrawcalls( const Math::CFrustum& Frustum )
        {

        }

        void CScene::_Draw( VKE::RenderSystem::CGraphicsContext* pCtx )
        {
            RenderSystem::CCommandBuffer* pCmdBuffer = pCtx->GetCommandBuffer();

            for( uint32_t i = 0; i < m_DrawData.vBits.GetCount(); ++i )
            {
                const auto& Bits = m_DrawData.vBits[i];
                if( Bits.value )
                {
                    // Load this drawcall == cache miss
                    const CDrawcall& Drawcall = m_vDrawcalls[ Bits.index ];
                    const auto& LOD = Drawcall.m_vLODs[Drawcall.m_currLOD];
                    
                    pCmdBuffer->Bind( LOD.hVertexBuffer, LOD.vertexBufferOffset );
                    pCmdBuffer->Bind( LOD.hIndexBuffer, LOD.indexBufferOffset );
                    pCmdBuffer->Bind( LOD.hDescSet );
                    pCmdBuffer->SetState( LOD.pVertexShader );
                    pCmdBuffer->SetState( LOD.pPixelShader );
                    pCmdBuffer->SetState( LOD.InputLayout );
                    pCmdBuffer->SetState( LOD.topology );
                    pCmdBuffer->DrawIndexed( LOD.DrawParams );
                }
            }
        }

        handle_t CScene::_CreateSceneNode(const uint32_t idx)
        {
            UObjectHandle Ret;
            auto handle = m_DrawData.Add( idx );
            Ret.type = ObjectTypes::SCENE_NODE;
            Ret.objDataIndex = handle;
            Ret.objTypeIndex = idx;
            return Ret.handle;
        }

    } // Scene
} // VKE
