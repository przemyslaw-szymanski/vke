#include "CForwardRenderer.h"
#include "Scene/CScene.h"
#include "RenderSystem/CGraphicsContext.h"

namespace VKE
{
    namespace RenderSystem
    {
        void CForwardRenderer::_Destroy()
        {

        }

        Result CForwardRenderer::_Create( const SFrameGraphDesc& Desc )
        {
            Result ret = VKE_OK;
            m_Desc = *reinterpret_cast<SForwardRendererDesc*>(Desc.pDesc);
            return ret;
        }

        void CForwardRenderer::Render( CGraphicsContext* pCtx )
        {
            auto& vpLayerDrawcalls = m_pScene->m_vpVisibleLayerDrawcalls;
            CCommandBuffer* pCmdBuffer = pCtx->GetCommandBuffer();
            {
                //pCmdBuffer->Bind( pCtx->GetSwapChain() );
                
                for( uint32_t layer = 0; layer < vpLayerDrawcalls.GetCount(); ++layer )
                {
                    auto& Layer = vpLayerDrawcalls[layer];
                    for( uint32_t d = 1; d < Layer.GetCount(); ++d )
                    {
                        _Draw( pCmdBuffer, Layer[d] );
                    }
                }
            }
        }

        void CForwardRenderer::_Draw( CCommandBuffer* pCmdBuffer, DrawcallPtr pDrawcall )
        {
            if( pDrawcall->IsFrameGrpahRenderingEnabled() )
            {
                auto& LOD = pDrawcall->GetLOD();
                //PipelinePtr pPipeline = PipelinePtr( LOD.ppPipeline->Get() );
                pCmdBuffer->Bind( LOD.vpPipelines[0] );
                pCmdBuffer->Bind( LOD.hIndexBuffer, LOD.indexBufferOffset );
                pCmdBuffer->Bind( LOD.hVertexBuffer, LOD.vertexBufferOffset );
                pCmdBuffer->Bind( LOD.hDescSet, LOD.descSetOffset );

                pCmdBuffer->DrawIndexed( LOD.DrawParams );
            }
        }

        void CForwardRenderer::_Sort()
        {

        }

    } // RenderSystem
} // VKE