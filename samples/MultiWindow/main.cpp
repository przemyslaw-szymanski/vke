

#include "../CSampleFramework.h"

struct SGfxContextListener : public VKE::RenderSystem::EventListeners::IGraphicsContext
{
    VKE::RenderSystem::ShaderRefPtr pVs, pPs;
    VKE::RenderSystem::VertexBufferRefPtr pVb;
    VKE::RenderSystem::SVertexInputLayoutDesc Layout;
    VKE::RenderSystem::BufferRefPtr pUBO;
    VKE::RenderSystem::DescriptorSetLayoutRefPtr pSetLayout;
    VKE::RenderSystem::DescriptorSetRefPtr pDescSet;
    VKE::RenderSystem::DescriptorSetLayoutHandle hSetLayout;
    VKE::RenderSystem::DescriptorSetHandle hDescSet = VKE::NULL_HANDLE;
    VKE::WindowPtr pWnd;
    uint32_t idx;
    SFpsCounter m_Fps;
    bool doInit = true;

    struct SUbo
    {
        VKE::RenderSystem::SColor   Color;
    };

    SUbo UBO;

    SGfxContextListener(uint32_t id) : idx{id}
    {

    }

    virtual ~SGfxContextListener()
    {

    }

    bool AutoDestroy() override { return false; }

    bool OnRenderFrame(VKE::RenderSystem::CGraphicsContext* pCtx) override
    {
        char buff[128];
        vke_sprintf( buff, 128, "Window %d - %d fps", idx, m_Fps.GetFps() );
        pCtx->GetSwapChain()->GetWindow()->SetText( buff );

        using namespace VKE::RenderSystem;
        if( doInit )
        {
            doInit = false;
            VKE::RenderSystem::CDeviceContext* pDeviceCtx = pCtx->GetDeviceContext();
            SLoadShaderData LoadData;
            LoadData.apShaderFiles[VKE::RenderSystem::ShaderTypes::VERTEX] = "data/samples/shaders/simple.vs";
            LoadData.apShaderFiles[VKE::RenderSystem::ShaderTypes::PIXEL] = "data/samples/shaders/simple-with-bindings.ps";
            LoadSimpleShaders( pDeviceCtx, LoadData, pVs, pPs );
            CreateSimpleTriangle( pCtx, pVb, &Layout );

            SCreateBufferDesc Desc;
            Desc.Create.async = false;
            Desc.Create.pOutput = &pUBO;
            Desc.Buffer.memoryUsage = MemoryUsages::CPU_ACCESS;
            Desc.Buffer.size = sizeof( SUbo );
            Desc.Buffer.usage = BufferUsages::UNIFORM_BUFFER;
            pUBO = pDeviceCtx->CreateBuffer( Desc );
            if( pUBO.IsValid() )
            {
                UBO.Color = VKE::RenderSystem::SColor( 0u );

                SUpdateMemoryInfo Info;
                Info.dataSize = sizeof( UBO );
                Info.dstDataOffset = 0;
                Info.pData = &UBO;
                pCtx->UpdateBuffer( Info, &pUBO );
                SCreateBindingDesc Desc;
                Desc.AddBinding( { 0, PipelineStages::VERTEX | PipelineStages::PIXEL }, pUBO );
                hDescSet = pCtx->CreateResourceBindings( Desc );
                pCtx->UpdateDescriptorSet( pUBO, &hDescSet );
            }
        }
        //if( hDescSet != VKE::NULL_HANDLE )
        {
            uint32_t offset = pUBO->SetNextChunk();
            if( idx == 0 )
            {
                UBO.Color.r += 0.0001f;
                UBO.Color.r = std::min( UBO.Color.r, 1.0f );
                if( UBO.Color.r >= 1.0f )
                {
                    UBO.Color.r -= 0.0001f;
                    UBO.Color.r = std::max( UBO.Color.r, 0.0f );

                    UBO.Color.g += 0.0001f;
                    UBO.Color.g = std::min( UBO.Color.g, 1.0f );
                    if( UBO.Color.g >= 1.0f )
                    {
                        UBO.Color.g -= 0.0001f;
                        UBO.Color.g = std::max( UBO.Color.g, 0.0f );

                        UBO.Color.b += 0.0001f;
                        UBO.Color.b = std::min( UBO.Color.b, 1.0f );
                    }
                }
                if( UBO.Color.b >= 1.0f )
                {
                    UBO.Color = 0.0f;
                }
            }
            else
            {
                UBO.Color.b += 0.0001f;
                UBO.Color.b = std::min( UBO.Color.b, 1.0f );
                if( UBO.Color.b >= 1.0f )
                {
                    UBO.Color.g += 0.0001f;
                    UBO.Color.g = std::min( UBO.Color.g, 1.0f );
                    if( UBO.Color.g >= 1.0f )
                    {
                        UBO.Color.r += 0.0001f;
                        UBO.Color.r = std::min( UBO.Color.r, 1.0f );
                    }
                }
                if( UBO.Color.r >= 1.0f )
                {
                    UBO.Color = 0.0f;
                }
            }
            SUpdateMemoryInfo Info;
            Info.dataSize = sizeof( UBO );
            Info.dstDataOffset = offset;
            Info.pData = &UBO;
            pCtx->UpdateBuffer( Info, &pUBO );
        }
        

        SSimpleDrawData Data;
        Data.pLayout = &Layout;
        Data.pVertexBuffer = pVb;
        Data.pPixelShader = pPs;
        Data.pVertexShader = pVs;
        Data.hDescSet = hDescSet;
        DrawSimpleFrame( pCtx, Data );
        return true;
    }
};

int main()
{
    VKE_DETECT_MEMORY_LEAKS();
    //VKE::Platform::Debug::BreakAtAllocation( 275 );
    CSampleFramework Sample;
    SSampleCreateDesc Desc;
    VKE::RenderSystem::EventListeners::IGraphicsContext* apListeners[2] =
    {
        VKE_NEW SGfxContextListener(0),
        VKE_NEW SGfxContextListener(1)
    };

    VKE::SWindowDesc aWndDescs[ 2 ] = {};
    aWndDescs[ 0 ].mode = VKE::WindowModes::WINDOW;
    aWndDescs[ 0 ].Size = { 800, 600 };
    aWndDescs[ 0 ].pTitle = "Window 1";
    aWndDescs[0].Position = { 100, 200 };
    aWndDescs[ 1 ] = aWndDescs[ 0 ];
    aWndDescs[ 1 ].pTitle = "Window 2";
    aWndDescs[1].Position = { 1000, 200 };

    Desc.ppGfxListeners = apListeners;
    Desc.gfxListenerCount = 1;
    Desc.customWindowCount = 1;
    Desc.pCustomWindows = aWndDescs;

    if( Sample.Create( Desc ) )
    {
        Sample.Start();
    }
    VKE_DELETE( apListeners[0] );
    VKE_DELETE( apListeners[1] );
    Sample.Destroy();
    return 0;
}