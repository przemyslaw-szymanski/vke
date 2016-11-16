#include "RenderSystem/CRenderQueue.h"
#if VKE_VULKAN_RENDERER
#include "RenderSystem/CGraphicsContext.h"
#include "Core/VKEForwardDeclarations.h"

namespace VKE
{
    namespace RenderSystem
    {
        CRenderQueue::CRenderQueue(CGraphicsContext* pCtx) :
            m_pCtx(pCtx)
        {
            assert(pCtx);
        }

        CRenderQueue::~CRenderQueue()
        {
            Destroy();
        }

        void CRenderQueue::Destroy()
        {

        }

        Result CRenderQueue::Create(const SRenderQueueDesc& Desc)
        {
            assert(m_pCtx);
            m_Desc = Desc;
            
            return VKE_OK;
        }

        Result CRenderQueue::Begin()
        {
            assert(m_pCtx);
            m_vkCmdBuffer = m_pCtx->_GetNextCommandBuffer(m_Desc.usage);
            return VKE_OK;
        }

        Result CRenderQueue::End()
        {
            return VKE_OK;
        }

        Result CRenderQueue::Execute()
        {
            assert(m_pCtx);
            return m_pCtx->ExecuteRenderQueue(this);
        }

        void CRenderQueue::IsEnabled(bool enabled)
        {
            if( m_enabled != enabled )
            {
                m_enabled = enabled;
                assert(m_pCtx);
                m_pCtx->_EnableRenderQueue(this, enabled);
            }
        }

    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER