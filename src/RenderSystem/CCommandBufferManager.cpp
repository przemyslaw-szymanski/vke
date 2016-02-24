#include "RenderSystem/CCommandBufferManager.h"
#include "RenderSystem/CCommandBuffer.h"

#include "Utils/CLogger.h"
#include "Memory/Memory.h"

namespace VKE
{
    namespace RenderSystem
    {
        CCommandBufferManager::CCommandBufferManager(CDevice* pDevice) :
            m_pDevice(pDevice)
        {}

        CCommandBufferManager::~CCommandBufferManager()
        {
            Destroy();
        }

        void CCommandBufferManager::Destroy()
        {
            m_FreeList.Destroy();
        }

        Result CCommandBufferManager::Create(uint32_t maxCmdBuffers)
        {
            if (VKE_FAILED(m_FreeList.Create(maxCmdBuffers, sizeof(CCommandBuffer))))
            {
                VKE_LOG_ERR("Unable to create memory for command buffer objects.");
                return VKE_ENOMEMORY;
            }
            return VKE_OK;
        }

        Resource::CManager::ResourceRawPtr CCommandBufferManager::_AllocateMemory(
            const Resource::SCreateInfo* const pInfo)
        {
            ResourceRawPtr pRes;
            const auto pCreateInfo = reinterpret_cast< const CCommandBuffer::SCreateInfo* >(pInfo);
            Memory::CreateObject(&m_FreeList, &pRes, m_pDevice, this);
            return pRes;
        }
    } // RenderSystem
} // vke