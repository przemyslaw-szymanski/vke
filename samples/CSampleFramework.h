#pragma once

#include "VKE.h"

struct SSampleCreateDesc
{
    VKE::SWindowDesc**                                      ppCustomWindows = nullptr;
    uint32_t                                                customWindowCount = 0;
    VKE::cstr_t                                             pWndName = "sample";
    VKE::cstr_t                                             pAdapterName = "";
    VKE::RenderSystem::EventListeners::IGraphicsContext**   ppGfxListeners = nullptr;
    uint32_t                                                gfxListenerCount = 0;

};

class CSampleFramework
{
    using WindowArray = VKE::Utils::TCDynamicArray< VKE::WindowPtr >;
    using DeviceContextArray = VKE::Utils::TCDynamicArray< VKE::RenderSystem::CDeviceContext* >;
    using GraphicsContextArray = VKE::Utils::TCDynamicArray< VKE::RenderSystem::CGraphicsContext* >;

    public:

        bool    Create(const SSampleCreateDesc& Desc);

        void    Destroy();

        void    Start();

    public:

        VKE::CVkEngine*         m_pEngine = nullptr;
        VKE::CRenderSystem*     m_pRenderSystem = nullptr;
        WindowArray             m_vpWindows;
        DeviceContextArray      m_vpDeviceContexts;
        GraphicsContextArray    m_vpGraphicsContexts;
};

bool CSampleFramework::Create(const SSampleCreateDesc& Desc)
{
    m_pEngine = VKECreate();
    if( !m_pEngine )
    {
        goto ERR;
    }

    VKE::SEngineInfo EngineInfo;
    EngineInfo.thread.threadCount = VKE::Constants::Threads::COUNT_OPTIMAL;
    EngineInfo.thread.taskMemSize = 1024; // 1kb per task
    EngineInfo.thread.maxTaskCount = 1024;

    auto err = m_pEngine->Init( EngineInfo );
    if( VKE_FAILED( err ) )
    {
        goto ERR;
    }

    VKE::SWindowDesc WndInfos[1];
    if( Desc.ppCustomWindows )
    {
        for( uint32_t i = 0; i < Desc.customWindowCount; ++i )
        {
            auto pWnd = m_pEngine->CreateRenderWindow( *Desc.ppCustomWindows[ i ] );
            if( pWnd.IsNull() )
            {
                goto ERR;
            }
            m_vpWindows.PushBack( pWnd );
        }
    }
    else
    {
        WndInfos[0].mode = VKE::WindowModes::WINDOW;
        WndInfos[0].vSync = false;
        WndInfos[0].hWnd = 0;
        WndInfos[0].pTitle = Desc.pWndName;
        WndInfos[0].Size = { 800, 600 };
        auto pWnd1 = m_pEngine->CreateRenderWindow( WndInfos[0] );
        if( pWnd1.IsNull() )
        {
            goto ERR;
        }
        m_vpWindows.PushBack( pWnd1 );
    }

    VKE::RenderSystem::SRenderSystemDesc RenderSysDesc;
    auto pRenderSys = m_pEngine->CreateRenderSystem( RenderSysDesc );
    if( !pRenderSys )
    {
        goto ERR;
    }

    const auto& vAdapters = pRenderSys->GetAdapters();
    VKE::RenderSystem::SAdapterInfo* pAdapterInfo = nullptr;
    for( uint32_t i = 0; i < vAdapters.GetCount(); ++i )
    {
        if( strcmp( vAdapters[i].name, Desc.pWndName ) == 0 )
        {
            pAdapterInfo = &vAdapters[ i ];
            break;
        }
    }
    if( pAdapterInfo == nullptr )
    {
        pAdapterInfo = &vAdapters[0];
    }

    VKE::RenderSystem::SDeviceContextDesc DevCtxDesc;
    DevCtxDesc.pAdapterInfo = pAdapterInfo;
    auto pDevCtx = pRenderSys->CreateDeviceContext( DevCtxDesc );
    if( !pDevCtx )
    {
        goto ERR;
    }
    m_vpDeviceContexts.PushBack( pDevCtx );

    for( uint32_t i = 0; i < m_vpWindows.GetCount(); ++i )
    {
        VKE::RenderSystem::SGraphicsContextDesc GraphicsDesc;
        GraphicsDesc.SwapChainDesc.pWindow = m_vpWindows[ i ];
        auto pGraphicsCtx = pDevCtx->CreateGraphicsContext( GraphicsDesc );
        if( Desc.gfxListenerCount )
        {
            pGraphicsCtx->SetEventListener( Desc.ppGfxListeners[i] );
        }
        m_vpGraphicsContexts.PushBack( pGraphicsCtx );
    }

    return true;

ERR:
    VKEDestroy( &m_pEngine );
    return false;
}

void CSampleFramework::Destroy()
{
    VKEDestroy( &m_pEngine );
}

void CSampleFramework::Start()
{
    for( uint32_t i = 0; i < m_vpWindows.GetCount(); ++i )
    {
        m_vpWindows[ i ]->IsVisible( true );
    }
    m_pEngine->StartRendering();
}