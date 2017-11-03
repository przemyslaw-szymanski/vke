#include "RenderSystem/CRenderingPipeline.h"
#if VKE_VULKAN_RENDERER
#include "RenderSystem/CGraphicsContext.h"
#include "RenderSystem/CDeviceContext.h"
#include "RenderSystem/Vulkan/CRenderPass.h"
#include "RenderSystem/Vulkan/CResourceManager.h"
#include "RenderSystem/CRenderTarget.h"
namespace VKE
{
    namespace RenderSystem
    {
        CRenderingPipeline::CRenderingPipeline(CDeviceContext* pCtx) :
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
            m_vpRenderPasses.FastClear();
        }  

        Result CRenderingPipeline::Create(const SRenderingPipelineDesc& /*Desc*/)
        {
            
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

        }

    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER