#pragma once

#include "RenderSystem/Common.h"
#include "Core/Utils/TCSmartPtr.h"
#if VKE_VULKAN_RENDERER

#include "RenderSystem/Vulkan/Vulkan.h"

namespace VKE
{
    namespace RenderSystem
    {
        struct SDescriptorSetDesc
        {

        };

        class CDescriptorSet : public Core::CObject
        {
            friend class CDescriptorSetManager;
            friend class CDeviceContext;

            public:
        };
    } // RenderSystem
} // VKE

#endif // VKE_VULKAN_RENDERER