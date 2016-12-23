#include "RenderSystem/CRenderPass.h"
#if VKE_VULKAN_RENDERER
#include "RenderSystem/CGraphicsContext.h"

namespace VKE
{
    namespace RenderSystem
    {
        CRenderPass::CRenderPass(CRenderingPipeline* pPipeline) :
            m_pPipeline(pPipeline)
        {}

        CRenderPass::~CRenderPass()
        {
            Destroy();
        }

        void CRenderPass::Destroy()
        {

        }

        Result CRenderPass::Create(const SRenderPassDesc& Desc)
        {
            return VKE_OK;
        }

        void CRenderPass::Begin()
        {
            //m_pSubmit = m_pPipeline->GetGraphicsContext()->GetNextSubmit(m_Desc.renderQueueCount);
        }

        void CRenderPass::End()
        {

        }

    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER