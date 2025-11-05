#include "CRenderSystem.h"

#if VKE_D3D12_RENDER_SYSTEM

namespace VKE::RenderSystem
{
    CRenderSystem::CRenderSystem( CVkEngine* pEngine ) : m_pEngine( pEngine )
    {
    }

    CRenderSystem::~CRenderSystem()
    {
        Destroy();
    }

    void CRenderSystem::Destroy() {};

    Result CRenderSystem::Create( const SRenderSystemDesc& Info )
    {
        return VKE_OK;
    }

    Result CRenderSystem::_CreateFreeListMemory( uint32_t id, uint16_t* pElemCountOut, uint16_t defaultElemCount,
                                                 size_t memSize )
    {
        return VKE_OK;
    };

    Result CRenderSystem::_AllocMemory( SRenderSystemDesc* /*pInfoOut*/ )
    {
        return VKE_OK;
    };

    Result CRenderSystem::_InitAPI()
    {
        return VKE_OK;
    };

    const CRenderSystem::AdapterVec& CRenderSystem::GetAdapters() const
    {
        return m_vAdapterInfos;
    };

    CDeviceContext* CRenderSystem::CreateDeviceContext( const SDeviceContextDesc& Desc )
    {
        return nullptr;
    };

    CDeviceContext* CRenderSystem::GetDeviceContext() const
    {
        return nullptr;
    };

    void CRenderSystem::DestroyDeviceContext( CDeviceContext** ppOut ) {};

    Result CRenderSystem::MakeCurrent( RenderSystem::CGraphicsContext* pCtx, CONTEXT_SCOPE scope )
    {
        return VKE_OK;
    };

    CGraphicsContext* CRenderSystem::GetCurrentContext( CONTEXT_SCOPE scope )
    {
        return nullptr;
    };

    void CRenderSystem::RenderFrame( const WindowPtr pWnd ) {};

    handle_t CRenderSystem::CreateFramebuffer( const RenderSystem::SFramebufferDesc& /*Info*/ )
    {
        return 0;
    };

    CFrameGraph* CRenderSystem::CreateFrameGraph( const SFrameGraphDesc& Desc )
    {
        return nullptr;
    };

    CFrameGraph* CRenderSystem::GetFrameGraph()
    {
        return nullptr;
    };

    void SetResourceTypes() {};

} // namespace VKE::RenderSystem
#endif