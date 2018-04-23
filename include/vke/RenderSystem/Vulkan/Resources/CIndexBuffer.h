#pragma once
#include "Core/CObject.h"
#include "RenderSystem/Vulkan/Vulkan.h"

namespace VKE
{
    namespace RenderSystem
    {
        namespace Resources
        {
            struct SBufferDesc
            {

            };

            class VKE_API CBuffer : public VKE::Core::CObject
            {
                VKE_ADD_OBJECT_MEMBERS

                public:

                protected:

                    SBufferDesc m_Desc;
                    VkBuffer    m_vkBuffer = VK_NULL_HANDLE;
            };
        } // Resources
    } // RenderSystem
} // VKE