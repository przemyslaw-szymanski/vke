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

                    CBuffer(CGraphicsContext* pCtx);
                    ~CBuffer();

                    Result Create(const SBufferDesc& Desc);
                    void Destroy();

                protected:

                    void _Destroy();

                protected:

                    SBufferDesc         m_Desc;
                    CGraphicsContext*   m_pCtx;
                    VkBuffer            m_vkBuffer = VK_NULL_HANDLE;
            };
        } // Resources
        using BufferPtr = Utils::TCWeakPtr< Resources::CBuffer >;
        using BufferRefPtr = Utils::TCObjectSmartPtr< Resources::CBuffer >;
    } // RenderSystem
} // VKE