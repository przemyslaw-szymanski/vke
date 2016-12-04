#include "RenderSystem/CRenderingPipeline.h"
#if VKE_VULKAN_RENDERER
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

        }

        uint32_t CRenderingPipeline::_IsTextureUsedInNextPass(CRenderPass* pPass, const handle_t& hTex)
        {
            // Recursively check if texture is used in any next render pass
            for( uint32_t i = 0; i < pPass->m_Desc.vNextPasses.GetCount(); ++i )
            {
                CRenderPass* pNextPass = m_pCtx->GetRenderPass(pPass->m_Desc.vNextPasses[ i ]);
                for( uint32_t sp = 0; sp < pNextPass->m_Desc.vSubpassDescs.GetCount(); ++sp )
                {
                    const auto& vWrites = pNextPass->m_Desc.vSubpassDescs[ sp ].vWriteTargets;
                    const auto& vReads = pNextPass->m_Desc.vSubpassDescs[ sp ].vReadTargets;

                    for( uint32_t rt = 0; rt < vReads.GetCount(); ++rt )
                    {
                        if( vReads[ rt ] == hTex )
                            return 2;
                    }

                    for( uint32_t rt = 0; rt < vWrites.GetCount(); ++rt )
                    {
                        if( vWrites[ rt ] == hTex )
                            return 1; // wirte == 1, read == 2, not used == 0
                    }
                }
                return _IsTextureUsedInNextPass(pNextPass, hTex);
            }
            return 0; // not used
        }

        Result CRenderingPipeline::Create(const SRenderingPipelineDesc& Desc)
        {
            
            return VKE_OK;
        }

    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER