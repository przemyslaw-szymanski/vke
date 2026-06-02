#pragma once

#include "RenderSystem/Common.h"

namespace VKE
{
    namespace RenderSystem
    {
        class CGraphicsContext;
        class CDeviceContext;

        class CRenderingPipeline final
        {

        public:
            CRenderingPipeline( CGraphicsContext* pCtx );
            ~CRenderingPipeline();

            Result Create( const SRenderingPipelineDesc& Desc );
            void   Destroy();
            void   Begin();
            void   Render();
            void   End();

        protected:
            SRenderingPipelineDesc m_Desc;
            CGraphicsContext*      m_pCtx;
        };
    } // namespace RenderSystem
} // namespace VKE
