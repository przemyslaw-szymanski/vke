#pragma once

#include "Core/VKECommon.h"
#include "Core/Utils/TCSmartPtr.h"
#include "Core/Resource/TCManager.h"
#include "Core/Memory/CFreeList.h"
#include "RenderSystem/Vulkan/CCommandBuffer.h"

namespace VKE
{
    namespace RenderSystem
    {
        class CCommandBuffer;
        class CDevice;

        class VKE_API CCommandBufferManager : public Resource::TCManager< CCommandBuffer >
        {            
            public:
            CCommandBufferManager(CDevice*);
            ~CCommandBufferManager();

            Result Create(uint32_t maxCmdBuffers);
            void Destroy();

            protected:

            virtual CManager::ResourceRawPtr _AllocateMemory(const Resource::SCreateInfo* const) override;

            protected:

            Memory::CFreeList   m_FreeList;
            CDevice*            m_pDevice;
        };
    } // RenderSystem
} // vke