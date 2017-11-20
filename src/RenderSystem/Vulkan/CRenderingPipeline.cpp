#include "RenderSystem/CRenderingPipeline.h"
#if VKE_VULKAN_RENDERER
#include "RenderSystem/CGraphicsContext.h"
#include "RenderSystem/CDeviceContext.h"
#include "RenderSystem/Vulkan/CRenderPass.h"
#include "RenderSystem/Managers/CAPIResourceManager.h"
#include "RenderSystem/CRenderTarget.h"
namespace VKE
{
    namespace RenderSystem
    {
        CRenderingPipeline::CRenderingPipeline(CGraphicsContext* pCtx) :
            m_pCtx(pCtx)
        {}

        CRenderingPipeline::~CRenderingPipeline()
        {
            Destroy();
        }

        void CRenderingPipeline::Destroy()
        {
            for( auto& pPass : m_vpRenderPasses )
            {
                Memory::DestroyObject(&HeapAllocator, &pPass);
            }
            m_vpRenderPasses.Clear();
        }  

        Result CRenderingPipeline::Create(const SRenderingPipelineDesc& Desc)
        {
            m_Desc = Desc;
            return VKE_OK;
        }


        void CRenderingPipeline::Begin()
        {

        }

        void CRenderingPipeline::End()
        {

        }

        void CRenderingPipeline::Render()
        {
            for( uint32_t i = 0; i < m_Desc.vRenderPassHandles.GetCount(); ++i )
            {
                const auto& Pass = m_Desc.vRenderPassHandles[ i ];
                Pass.OnRender( Pass );
            }
        }

    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER