

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
    VKE::RenderSystem::DescriptorSetHandle hDescSet;
    bool doInit = true;

    struct SUbo
    {
        VKE::RenderSystem::SColor   Color;
    };

    SUbo UBO;

    SGfxContextListener()
    {

    }

    virtual ~SGfxContextListener()
    {

    }

    bool OnRenderFrame(VKE::RenderSystem::CGraphicsContext* pCtx) override
    {
        using namespace VKE::RenderSystem;
        if( doInit )
        {
            doInit = false;
            VKE::RenderSystem::CDeviceContext* pDeviceCtx = pCtx->GetDeviceContext();
            LoadSimpleShaders( pDeviceCtx, pVs, pPs );
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
                UBO.Color = SColor( 0.3f, 0.6f, 0.9f, 1.0f );
                SUpdateMemoryInfo UpdateInfo;
                UpdateInfo.pData = &UBO;
                UpdateInfo.dataSize = sizeof( SUbo );
                UpdateInfo.offset = 0;
                pCtx->UpdateBuffer( UpdateInfo, &pUBO );

                SDescriptorSetLayoutDesc SetLayoutDesc;
                SDescriptorSetLayoutDesc::SBinding Binding;
                Binding.count = 1;
                Binding.idx = 0;
                Binding.stages = PipelineStages::VERTEX | PipelineStages::PIXEL;
                Binding.type = BindingTypes::UNIFORM_BUFFER_DYNAMIC;
                SetLayoutDesc.vBindings.PushBack( Binding );
                hSetLayout = pDeviceCtx->CreateDescriptorSetLayout( SetLayoutDesc );
                if( hSetLayout != VKE::NULL_HANDLE )
                {
                    SDescriptorSetDesc SetDesc;
                    SetDesc.vLayouts.PushBack( hSetLayout );
                    hDescSet = pCtx->CreateDescriptorSet( SetDesc );
                }
            }
        }
        SSimpleDrawData Data;
        Data.pLayout = &Layout;
        Data.pVertexBuffer = pVb;
        Data.pPixelShader = pPs;
        Data.pVertexShader = pVs;
        DrawSimpleFrame( pCtx, Data );
        return true;
    }
};

int main()
{
    VKE_DETECT_MEMORY_LEAKS();
    CSampleFramework Sample;
    SSampleCreateDesc Desc;
    VKE::RenderSystem::EventListeners::IGraphicsContext* apListeners[2] =
    {
        VKE_NEW SGfxContextListener(),
        VKE_NEW SGfxContextListener()
    };

    VKE::SWindowDesc aWndDescs[ 2 ] = {};
    aWndDescs[ 0 ].mode = VKE::WindowModes::WINDOW;
    aWndDescs[ 0 ].Size = { 800, 600 };
    aWndDescs[ 0 ].pTitle = "Window 1";
    aWndDescs[ 1 ] = aWndDescs[ 0 ];
    aWndDescs[ 1 ].pTitle = "Window 2";

    Desc.ppGfxListeners = apListeners;
    Desc.gfxListenerCount = 2;
    Desc.customWindowCount = 2;
    Desc.pCustomWindows = aWndDescs;

    if( Sample.Create( Desc ) )
    {
        Sample.Start();
    }
    Sample.Destroy();
    return 0;
}