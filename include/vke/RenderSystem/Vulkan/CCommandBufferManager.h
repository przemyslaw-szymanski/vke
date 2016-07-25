#pragma once

#include "Core/Resource/CResource.h"
#include "Core/Resource/CManager.h"
#include "Core/Memory/CFreeList.h"

namespace VKE
{
    namespace RenderSystem
    {
        class CDevice;

        class CCommandBufferManager : public Resource::CManager
        {
            public:

            CCommandBufferManager(CDevice* pDevice);
            virtual ~CCommandBufferManager();

            Result Create(uint32_t);
            void Destroy();
            protected:

            Resource::CManager::ResourceRawPtr _AllocateMemory(const Resource::SCreateInfo* const pInfo);

            protected:
            CDevice* m_pDevice;
            Memory::CFreeList m_FreeList;
        };
    }
}