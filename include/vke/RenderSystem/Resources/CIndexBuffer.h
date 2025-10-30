#pragma once

#include "Core/CObject.h"

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
                //VkBuffer    m_vkBuffer = VK_NULL_HANDLE;
            };
        } // namespace Resources
    } // namespace RenderSystem
} // namespace VKE