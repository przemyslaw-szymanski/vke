#include "RenderSystem/Vulkan/CCommandBufferManager.h"
#if VKE_VULKAN_RENDERER
#include "Core/Utils/CLogger.h"
#include "Core/Memory/Memory.h"

#include "RenderSystem/Vulkan/CVkDeviceWrapper.h"
#include "RenderSystem/CGraphicsContext.h"

namespace VKE
{
    namespace RenderSystem
    {
        CCommandBufferManager::CCommandBufferManager(CGraphicsContext* pCtx) :
            m_pCtx(pCtx),
            m_VkDevice(pCtx->_GetDevice())
        {}

        CCommandBufferManager::~CCommandBufferManager()
        {
            Destroy();
        }

        void CCommandBufferManager::Destroy()
        {
            for( uint32_t i = 1; i < m_vpPools.GetCount(); ++i )
            {
                DestroyPool(i);
            }
            m_vpPools.FastClear();
        }

        Result CCommandBufferManager::Create(const SCommandBufferManagerDesc& Desc)
        {
            m_Desc = Desc;
            m_vpPools.PushBack(nullptr);
            return VKE_OK;
        }

        handle_t CCommandBufferManager::CreatePool(const SCommandPoolDesc& Desc)
        {
            SCommandPool* pPool;
            if( VKE_FAILED(Memory::CreateObject(&HeapAllocator, &pPool)) )
            {
                VKE_LOG_ERR("Unable to create command pool object. No memory.");
                return NULL_HANDLE;
            }

            if( !pPool->vCommandBuffers.Resize(Desc.commandBufferCount) )
            {
                VKE_LOG_ERR("Unable to resize vCommandBuffers. No memory.");
                return NULL_HANDLE;
            }
            if( !pPool->vFreeCommandBuffers.Reserve(Desc.commandBufferCount) )
            {
                VKE_LOG_ERR("Unable to resize vFreeCommandBuffers. No memory.");
                return NULL_HANDLE;
            }

            const auto& ICD = m_VkDevice.GetICD();
            
            VkCommandPoolCreateInfo ci;
            Vulkan::InitInfo(&ci, VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO);
            ci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            ci.queueFamilyIndex = m_pCtx->_GetQueue()->familyIndex;
            VK_ERR(m_VkDevice.CreateObject(ci, nullptr, &pPool->vkPool));

            VkCommandBufferAllocateInfo ai;
            Vulkan::InitInfo(&ai, VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO);
            ai.commandBufferCount = Desc.commandBufferCount;
            ai.commandPool = pPool->vkPool;
            ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            VK_ERR(ICD.vkAllocateCommandBuffers(m_VkDevice.GetHandle(), &ai, &pPool->vCommandBuffers[ 0 ]));
            pPool->vFreeCommandBuffers = pPool->vCommandBuffers;
            auto pCbs = &pPool->vCommandBuffers[ 0 ];
            // $TID AllocCmdBuffers: mgr={(void*)this}, pool={(void*)pPool}, cbs={pCbs, 64}
            return m_vpPools.PushBack(pPool);
        }

        void CCommandBufferManager::DestroyPool(const handle_t& hPool)
        {
            auto pPool = _GetPool(hPool);
            m_VkDevice.DestroyObject(nullptr, &pPool->vkPool);
            Memory::DestroyObject(&HeapAllocator, &pPool);
        }

        void CCommandBufferManager::FreeCommandBuffers(const handle_t& hPool)
        {
            auto pPool = _GetPool(hPool);
            // All command buffers must be freed
            assert(pPool->vCommandBuffers.GetCount() == pPool->vFreeCommandBuffers.GetCount());
            const auto& ICD = m_VkDevice.GetICD();
            const auto count = pPool->vCommandBuffers.GetCount();
            ICD.vkFreeCommandBuffers(m_VkDevice.GetHandle(), pPool->vkPool, count, &pPool->vCommandBuffers[ 0 ]);
            pPool->vCommandBuffers.FastClear();
            pPool->vFreeCommandBuffers.FastClear();
        }

        VkCommandBuffer CCommandBufferManager::_GetNextCommandBuffer(SCommandPool* pPool)
        {
            auto& vFreeCbs = pPool->vFreeCommandBuffers;
            VkCommandBuffer vkCb;
            if( vFreeCbs.PopBack(&vkCb) )
            {
                return vkCb;
            }
            else
            {
                VKE_LOG_ERR("Max command buffer for pool:" << pPool->vkPool << " reached.");
                assert(0 && "Command buffer resize is not supported now.");
                return VK_NULL_HANDLE;
            }
        }

        void CCommandBufferManager::_FreeCommandBuffers(uint32_t count, const VkCommandBuffer* pArray,
                                                        SCommandPool* pPool)
        {
            auto& vFreeCbs = pPool->vFreeCommandBuffers;
            for( uint32_t i = count; i-- > 0; )
            {
                vFreeCbs.PushBack(pArray[ i ]);
            }
        }

        void CCommandBufferManager::_CreateCommandBuffers(uint32_t count, VkCommandBuffer* pArray, SCommandPool* pPool)
        {
            assert(pPool);
            auto& vFreeCbs = pPool->vFreeCommandBuffers;
            if( vFreeCbs.GetCount() < count )
            {
                Utils::TCDynamicArray< VkCommandBuffer > vTmps;
                vTmps.Resize(count);

                VkCommandBufferAllocateInfo ai;
                Vulkan::InitInfo(&ai, VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO);
                ai.commandBufferCount = count;
                ai.commandPool = pPool->vkPool;
                ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
                const auto& ICD = m_pCtx->_GetDevice().GetICD();
                
                VK_ERR(ICD.vkAllocateCommandBuffers(m_VkDevice.GetHandle(), &ai, &vTmps[ 0 ]));
                // $TID CreateCommandBuffers: cbmgr={(void*)this}, pool={pPool->vkPool}, cbs={vTmps}
                pPool->vCommandBuffers.Append(0, vTmps.GetCount(), &vTmps[0]);
                vFreeCbs.Append(vTmps.GetCount(), &vTmps[0]);
                _CreateCommandBuffers(count, pArray, pPool);
            }

            for( uint32_t i = 0; i < count; ++i )
            {
                vFreeCbs.PopBack(&pArray[ i ]);
            }
        }
       
    } // RenderSystem
} // vke
#endif // VKE_VULKAN_RENDERER