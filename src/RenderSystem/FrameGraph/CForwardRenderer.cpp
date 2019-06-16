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

        }

        void CForwardRenderer::_Sort()
        {

        }

    } // RenderSystem
} // VKE