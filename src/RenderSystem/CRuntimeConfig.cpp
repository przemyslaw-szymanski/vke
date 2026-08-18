#include "RenderSystem/CRuntimeConfig.h"
#include "CCommandLineArgs.h"

namespace VKE::RenderSystem
{
    CRuntimeConfig::CRuntimeConfig()
    {
        DeferBarriers      = GetCommandLineParam< bool >( "rs.deferBarriers", false );
        ApiValidation      = GetCommandLineParam< bool >( "rs.apiValidation", false );
        ApiDebug           = GetCommandLineParam< bool >( "rs.apiDebug", false );
        VkDynamicRendering = GetCommandLineParam< bool >( "rs.vk.dynamicRendering", true );
        FeatureLevel       = GetCommandLineParam< FEATURE_LEVEL >( "rs.featureLevel", FeatureLevels::LEVEL_DEFAULT );
        RayTracing         = GetCommandLineParam< bool >( "rs.enableRayTracing", true );
        MeshShaders        = GetCommandLineParam< bool >( "rs.enableMeshShaders", true );
        Bindless           = GetCommandLineParam< bool >( "rs.bindless", true );
        VkTimelineSemaphore = GetCommandLineParam< bool >( "rs.vk.monitoredFence", true );
    }
}