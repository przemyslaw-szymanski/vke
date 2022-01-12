#pragma once

#include "VKE.h"

struct SSampleCreateDesc
{
    VKE::SWindowDesc*                                       pCustomWindows = nullptr;
    uint32_t                                                customWindowCount = 0;
    VKE::cstr_t                                             pWndName = "sample";
    VKE::cstr_t                                             pAdapterName = "";
    VKE::RenderSystem::EventListeners::IGraphicsContext**   ppGfxListeners = nullptr;
    uint32_t                                                gfxListenerCount = 0;

};

struct SFpsCounter
{
    VKE::Utils::CTimer  Timer;
    uint32_t            fps = 0;
    uint32_t            lastFps = 0;

    SFpsCounter()
    {
        Timer.Start();
    }

    uint32_t GetFps()
    {
        static const uint32_t secUS = 1000 * 1000;
        if( Timer.GetElapsedTime() >= secUS )
        {
            lastFps = fps;
            fps = 0;
            Timer.Start();
        }
        ++fps;
        return lastFps;
    }
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
    VKE::Result err;
    VKE::SEngineInfo EngineInfo;
    m_pEngine = VKECreate();
    if( !m_pEngine )
    {
        goto ERR;
    }

    
    EngineInfo.thread.threadCount = VKE::Constants::Threads::COUNT_OPTIMAL;
    EngineInfo.thread.taskMemSize = 1024; // 1kb per task
    EngineInfo.thread.maxTaskCount = 1024;

    err = m_pEngine->Init( EngineInfo );
    if( VKE_FAILED( err ) )
    {
        goto ERR;
    }

    if( Desc.pCustomWindows )
    {
        for( uint32_t i = 0; i < Desc.customWindowCount; ++i )
        {
            auto pWnd = m_pEngine->CreateRenderWindow( Desc.pCustomWindows[ i ] );
            if( pWnd.IsNull() )
            {
                goto ERR;
            }
            m_vpWindows.PushBack( pWnd );
        }
    }
    else
    {
        VKE::SWindowDesc WndInfos[1];
        WndInfos[0].mode = VKE::WindowModes::WINDOW;
        WndInfos[0].vSync = false;
        WndInfos[0].hWnd = 0;
        WndInfos[0].pTitle = Desc.pWndName;
        WndInfos[0].Size = { 1024, 768 };
        auto pWnd1 = m_pEngine->CreateRenderWindow( WndInfos[0] );
        if( pWnd1.IsNull() )
        {
            goto ERR;
        }
        m_vpWindows.PushBack( pWnd1 );
    }
    {
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
            if( strcmp( vAdapters[ i ].name, Desc.pWndName ) == 0 )
            {
                pAdapterInfo = &vAdapters[ i ];
                break;
            }
        }
        if( pAdapterInfo == nullptr )
        {
            pAdapterInfo = &vAdapters[ 0 ];
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
            // GraphicsDesc.SwapChainDesc.Size = m_vpWindows[i]->GetSize();
            auto pGraphicsCtx = pDevCtx->CreateGraphicsContext( GraphicsDesc );
            if( Desc.gfxListenerCount )
            {
                pGraphicsCtx->SetEventListener( Desc.ppGfxListeners[ i ] );
            }
            m_vpGraphicsContexts.PushBack( pGraphicsCtx );
            GraphicsDesc.SwapChainDesc.pWindow->IsVisible( true );
        }
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

struct SLoadShaderData
{
    const char* apShaderFiles[VKE::RenderSystem::ShaderTypes::_MAX_COUNT] = {};
};

void LoadSimpleShaders( VKE::RenderSystem::CDeviceContext* pCtx,
    const SLoadShaderData& Data,
    VKE::RenderSystem::ShaderRefPtr& pVertexShader,
    VKE::RenderSystem::ShaderRefPtr& pPixelShader )
{
    VKE::RenderSystem::SCreateShaderDesc VsDesc, PsDesc;

    VsDesc.Create.async = true;
    VsDesc.Create.stages = VKE::Core::ResourceStages::FULL_LOAD;
    VsDesc.Create.pOutput = &pVertexShader;
    VsDesc.Shader.FileInfo.pFileName = Data.apShaderFiles[VKE::RenderSystem::ShaderTypes::VERTEX];
    /*VsDesc.Create.pfnCallback = [&](const void* pShaderDesc, void* pShader)
    {
        using namespace VKE::RenderSystem;
        ShaderPtr pTmp = *reinterpret_cast<ShaderPtr*>(pShader);

        if( pTmp->GetDesc().type == VKE::RenderSystem::ShaderTypes::VERTEX )
        {
            pVertexShader = *reinterpret_cast<VKE::RenderSystem::ShaderPtr*>(pShader);
        }
        else if( pTmp->GetDesc().type == VKE::RenderSystem::ShaderTypes::PIXEL )
        {
            pPixelShader = *reinterpret_cast<VKE::RenderSystem::ShaderPtr*>(pShader);
        }
    };*/

    PsDesc = VsDesc;
    PsDesc.Create.pOutput = &pPixelShader;
    PsDesc.Shader.FileInfo.pFileName = Data.apShaderFiles[VKE::RenderSystem::ShaderTypes::PIXEL];

    pCtx->CreateShader( VsDesc );
    pCtx->CreateShader( PsDesc );
}

template<class ContextType>
bool CreateSimpleTriangle( ContextType* pCtx,
                           VKE::RenderSystem::VertexBufferPtr& pVb,
                           VKE::RenderSystem::SVertexInputLayoutDesc* pLayout )
{
    VKE::RenderSystem::SCreateBufferDesc BuffDesc;
    BuffDesc.Create.async = false;
    BuffDesc.Buffer.usage = VKE::RenderSystem::BufferUsages::VERTEX_BUFFER;
    BuffDesc.Buffer.memoryUsage = VKE::RenderSystem::MemoryUsages::GPU_ACCESS;
    BuffDesc.Buffer.size = ( sizeof( float ) * 4 ) * 3;
    auto hVb = pCtx->GetDeviceContext()->CreateBuffer( BuffDesc );
    pVb = pCtx->GetDeviceContext()->GetBuffer( hVb );
    const float vb[ 4 * 3 ] =
    {
        0.0f,   0.5f,   0.0f,   1.0f,
        -0.5f, -0.5f,   0.0f,   1.0f,
        0.5f,  -0.5f,   0.0f,   1.0f
    };
    VKE::RenderSystem::SUpdateMemoryInfo Info;
    Info.pData = vb;
    Info.dataSize = sizeof( vb );
    Info.dstDataOffset = 0;
    pCtx->UpdateBuffer( Info, &pVb );

    pLayout->vAttributes =
    {
        { "Position", VKE::RenderSystem::VertexAttributeTypes::POSITION }
    };

    return pVb.IsValid();
}

struct SSimpleDrawData
{
    VKE::RenderSystem::SVertexInputLayoutDesc* pLayout;
    VKE::RenderSystem::VertexBufferPtr pVertexBuffer;
    VKE::RenderSystem::ShaderPtr pVertexShader;
    VKE::RenderSystem::ShaderPtr pPixelShader;
    VKE::RenderSystem::DescriptorSetHandle hDescSet = nullptr;
};

void DrawSimpleFrame( VKE::RenderSystem::CGraphicsContext* pCtx, const SSimpleDrawData& Data )
{
    auto pCmdBuffer = pCtx->BeginFrame();
    pCmdBuffer->SetState( *Data.pLayout );
    pCmdBuffer->Bind( Data.pVertexBuffer );
    pCmdBuffer->SetState( Data.pVertexShader );
    pCmdBuffer->SetState( Data.pPixelShader );
    if( Data.hDescSet != nullptr )
    {
        pCmdBuffer->Bind( Data.hDescSet, 0 );
    }
    pCmdBuffer->Draw( 3 );
    pCtx->EndFrame();
}