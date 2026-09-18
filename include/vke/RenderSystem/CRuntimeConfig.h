#pragma once

#include "RenderSystem/Common.h"

namespace VKE::RenderSystem
{
    class VKE_API CRuntimeConfig
    {
    public:

        static const CRuntimeConfig& GetInstance()
        {
            static CRuntimeConfig Instance;
            return Instance;
        }

        CRuntimeConfig();

        uint32_t DeferBarriers : 1;
        uint32_t ApiValidation : 1;
        uint32_t ApiDebug : 1;
        uint32_t VkDynamicRendering : 1;
        uint32_t RayTracing : 1;
        uint32_t Bindless : 1;
        uint32_t VkTimelineSemaphore : 1;
        uint32_t MeshShaders : 1;

        FEATURE_LEVEL FeatureLevel;
    };

    static const CRuntimeConfig& GetRuntimeConfig()
    {
        return CRuntimeConfig::GetInstance();
    }
} // namespace VKE::RenderSystem