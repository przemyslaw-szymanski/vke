#pragma once

#include "Core/VKECommon.h"
#include "Core/Utils/TCSmartPtr.h"
#include "Core/Resources/TCManager.h"
#include "Core/Memory/CFreeList.h"
#include "RenderSystem/Vulkan/CCommandBuffer.h"
#include "RenderSystem/Vulkan/Vulkan.h"

namespace VKE
{
    namespace Vulkan
    {
        class CDeviceWrapper;
    }

    namespace RenderSystem
    {
        class CCommandBuffer;
        class CGraphicsContext;

        class VKE_API CCommandBufferManager : public Resources::TCManager< CCommandBuffer >
        {            
            using CmdBuffVec = vke_vector< CCommandBuffer >;

            public:

                CCommandBufferManager(CGraphicsContext*);
                ~CCommandBufferManager();

                Result Create(uint32_t maxCmdBuffers);
                void Destroy();

                CCommandBuffer* GetCommandBuffer(uint32_t id);
                CCommandBuffer* GetCommandBuffer();

                protected:

                virtual CManager::ResourceRawPtr _AllocateMemory(const Resources::SCreateDesc* const) override;

            protected:

                Memory::CFreeList   m_FreeList;
                CDevice*            m_pDevice;
                Vulkan::CDeviceWrapper*     m_pDeviceCtx;
                CmdBuffVec          m_vCmdBuffs;
                VkCommandPool       m_vkCmdPool = VK_NULL_HANDLE;
        };
    } // RenderSystem
} // vke