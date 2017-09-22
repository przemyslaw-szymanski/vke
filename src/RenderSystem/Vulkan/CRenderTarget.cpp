#include "RenderSystem/CRenderTarget.h"
#if VKE_VULKAN_RENDERER
#include "RenderSystem/CDeviceContext.h"
#include "RenderSystem/Vulkan/CResourceManager.h"

namespace VKE
{
    namespace RenderSystem
    {
        CRenderTarget::CRenderTarget(CDeviceContext* pCtx)
        {}

        CRenderTarget::~CRenderTarget()
        {}

    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER