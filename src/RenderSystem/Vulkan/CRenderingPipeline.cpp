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

        CRenderPass* CRenderingPipeline::CreatePass(const SRenderPassDesc& Desc)
        {
            CRenderPass* pPass;
            /*if( VKE_FAILED(Memory::CreateObject(&HeapAllocator, &pPass, this)) )
            {
                VKE_LOG_ERR("Unable to create CRenderPass object. No memory.");
                return nullptr;
            }
            if( VKE_FAILED(pPass->Create(Desc)) )
            {
                Memory::DestroyObject(&HeapAllocator, &pPass);
                return nullptr;
            }
            m_vpRenderPasses.PushBack(pPass);*/
            return pPass;
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