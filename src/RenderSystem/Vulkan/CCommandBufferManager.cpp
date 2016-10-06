#include "CCommandBufferManager.h"
#include "RenderSystem/Vulkan/CCommandBuffer.h"

#include "Core/Utils/CLogger.h"
#include "Core/Memory/Memory.h"

#include "RenderSystem/Vulkan/CDevice.h"
#include "RenderSystem/Vulkan/CDeviceContext.h"

namespace VKE
{
    namespace RenderSystem
    {
        CCommandBufferManager::CCommandBufferManager(CDevice* pDevice) :
            m_pDevice(pDevice),
            m_pDeviceCtx(pDevice->_GetDeviceContext())
        {}

        CCommandBufferManager::~CCommandBufferManager()
        {
            Destroy();
        }

        void CCommandBufferManager::Destroy()
        {
            m_FreeList.Destroy();
            m_vCmdBuffs.clear();
        }

        Result CCommandBufferManager::Create(uint32_t maxCmdBuffers)
        {
            /*if (VKE_FAILED(m_FreeList.Create(maxCmdBuffers, sizeof(CCommandBuffer))))
            {
                VKE_LOG_ERR("Unable to create memory for command buffer objects.");
                return VKE_ENOMEMORY;
            }*/

            m_vCmdBuffs.resize(maxCmdBuffers);
            m_pDevice->GetDeviceFunctions().vkCreateCommandPool()
            return VKE_OK;
        }

        CCommandBuffer* CCommandBufferManager::GetCommandBuffer(uint32_t id)
        {
            return &m_vCmdBuffs[ id ];
        }

        CCommandBuffer* CCommandBufferManager::GetCommandBuffer()
        {
            return &m_vCmdBuffs[ 0 ];
        }

        Resource::CManager::ResourceRawPtr CCommandBufferManager::_AllocateMemory(
            const Resource::SCreateInfo* const pInfo)
        {
            ResourceRawPtr pRes;
            const auto pCreateInfo = reinterpret_cast< const CCommandBuffer::SCreateInfo* >(pInfo);
            //Memory::CreateObject(&m_FreeList, &pRes, m_pDevice, this);
            return pRes;
        }
    } // RenderSystem
} // vke