#pragma once

#include "Core/CObject.h"

namespace VKE::RenderSystem::Resources
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
    };
} // namespace VKE::RenderSystem::Resources