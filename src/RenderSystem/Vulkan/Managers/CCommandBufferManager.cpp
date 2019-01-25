#include "RenderSystem/Vulkan/Managers/CCommandBufferManager.h"
#if VKE_VULKAN_RENDERER
#include "Core/Utils/CLogger.h"
#include "Core/Memory/Memory.h"

#include "RenderSystem/Vulkan/CVkDeviceWrapper.h"
#include "RenderSystem/CDeviceContext.h"

namespace VKE
{
    namespace RenderSystem
    {
        CCommandBufferManager::CCommandBufferManager(CDeviceContext* pCtx) :
            m_pCtx(pCtx)
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
            m_vpPools.Clear();
        }

        Result CCommandBufferManager::Create(const SCommandBufferManagerDesc& Desc)
        {
            Result ret = VKE_FAIL;
            m_Desc = Desc;
            m_vpPools.PushBack(nullptr); // add null pool
            ret = VKE_OK;
            return ret;
        }

        handle_t CCommandBufferManager::CreatePool(const SCommandPoolDesc& Desc)
        {
            SCommandPool* pPool;
            if( VKE_FAILED(Memory::CreateObject(&HeapAllocator, &pPool)) )
            {
                VKE_LOG_ERR("Unable to create command pool object. No memory.");
                return NULL_HANDLE;
            }

            if( !pPool->vCommandBuffers.Resize( Desc.commandBufferCount ) )
            {
                VKE_LOG_ERR("Unable to resize vCommandBuffers. No memory.");
                return NULL_HANDLE;
            }
            if( !pPool->vDDICommandBuffers.Resize( Desc.commandBufferCount ) )
            {
                VKE_LOG_ERR( "Unable to resize vCommandBuffers. No memory." );
                return NULL_HANDLE;
            }
            if( !pPool->vpFreeCommandBuffers.Resize(Desc.commandBufferCount ) )
            {
                VKE_LOG_ERR("Unable to resize vFreeCommandBuffers. No memory.");
                return NULL_HANDLE;
            }

           /* const auto& ICD = m_VkDevice.GetICD();
            
            VkCommandPoolCreateInfo ci;
            Vulkan::InitInfo(&ci, VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO);
            ci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            ci.queueFamilyIndex = m_pCtx->_GetQueue()->familyIndex;
            VK_ERR(m_VkDevice.CreateObject(ci, nullptr, &pPool->m_hPool));*/

            SCommandBufferPoolDesc PoolDesc;
            PoolDesc.queueFamilyIndex = 0;
            pPool->hDDIPool = m_pCtx->_GetDDI().CreateObject( PoolDesc, nullptr );

            /*VkCommandBufferAllocateInfo ai;
            Vulkan::InitInfo( &ai, VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO );
            ai.commandBufferCount = Desc.commandBufferCount;
            ai.commandPool = pPool->hPool;
            ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;*/
            //VK_ERR( ICD.vkAllocateCommandBuffers( m_VkDevice.GetHandle(), &ai, &pPool->vVkCommandBuffers[ 0 ] ) );
            SAllocateCommandBufferInfo Info;
            Info.count = Desc.commandBufferCount;
            Info.hDDIPool = pPool->hDDIPool;
            Info.level = CommandBufferLevels::PRIMARY;
            m_pCtx->_GetDDI().AllocateObjects( Info, &pPool->vDDICommandBuffers[0] );
            for( uint32_t i = 0; i < Desc.commandBufferCount; ++i )
            {
                VkCommandBuffer vkCb = pPool->vDDICommandBuffers[ i ];
                CCommandBuffer& CmdBuffer = pPool->vCommandBuffers[ i ];
                CmdBuffer.Init( m_pCtx, vkCb );
                pPool->vpFreeCommandBuffers[ i ] = &pPool->vCommandBuffers[ i ];
            }
            //auto pCbs = &pPool->vCommandBuffers[ 0 ];
            // $TID AllocCmdBuffers: mgr={(void*)this}, pool={(void*)pPool}, cbs={pCbs, 64}
            return m_vpPools.PushBack(pPool);
        }

        void CCommandBufferManager::DestroyPool(const handle_t& hPool)
        {
            auto pPool = _GetPool(hPool);
            //m_VkDevice.DestroyObject(nullptr, &pPool->m_hPool);
            m_pCtx->_GetDDI().DestroyObject( &pPool->hDDIPool, nullptr );
            Memory::DestroyObject(&HeapAllocator, &pPool);
        }

        void CCommandBufferManager::FreeCommandBuffers(const handle_t& hPool)
        {
            auto pPool = _GetPool(hPool);
            // All command buffers must be freed
            VKE_ASSERT(pPool->vDDICommandBuffers.GetCount() == pPool->vpFreeCommandBuffers.GetCount(),
                "All command buffers must be freed" );
            //const auto& ICD = m_VkDevice.GetICD();
            const auto count = pPool->vCommandBuffers.GetCount();
            SFreeCommandBufferInfo Info;
            Info.hDDIPool = pPool->hDDIPool;
            Info.pDDICommandBuffers = &pPool->vDDICommandBuffers[0];
            Info.count = count;
            m_pCtx->_GetDDI().FreeObjects( Info );
            pPool->vDDICommandBuffers.Clear();
            pPool->vpFreeCommandBuffers.Clear();
            pPool->vCommandBuffers.Clear();
        }

        CommandBufferPtr CCommandBufferManager::_GetNextCommandBuffer(SCommandPool* pPool)
        {
            auto& vpFrees = pPool->vpFreeCommandBuffers;
            CommandBufferPtr pCb;
            if( vpFrees.PopBack( &pCb ) )
            {
                return pCb;
            }
            else
            {
                VKE_LOG_ERR("Max command buffer for pool:" << pPool->hDDIPool << " reached.");
                assert(0 && "Command buffer resize is not supported now.");
                return CommandBufferPtr();
            }
        }

        void CCommandBufferManager::_FreeCommandBuffers(uint32_t count, CommandBufferPtr* ppArray,
                                                        SCommandPool* pPool)
        {
            auto& vFreeCbs = pPool->vpFreeCommandBuffers;
            for( uint32_t i = count; i-- > 0; )
            {
                vFreeCbs.PushBack( ppArray[ i ] );
            }
        }

        Result CCommandBufferManager::_CreateCommandBuffers(uint32_t count, CommandBufferPtr* ppArray, SCommandPool* pPool)
        {
            assert(pPool);
            auto& vFreeCbs = pPool->vpFreeCommandBuffers;
            Result ret = VKE_OK;
            if( vFreeCbs.GetCount() < count )
            {
                Utils::TCDynamicArray< DDICommandBuffer, DEFAULT_COMMAND_BUFFER_COUNT > vTmps;
                vTmps.Resize(count);
                auto& DDI = m_pCtx->_GetDDI();
                /*VkCommandBufferAllocateInfo ai;
                Vulkan::InitInfo(&ai, VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO);
                ai.commandBufferCount = count;
                ai.commandPool = pPool->m_hPool;
                ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
                
                DDI.vkAllocateCommandBuffers(m_VkDevice.GetHandle(), &ai, &vTmps[ 0 ]);*/
                SAllocateCommandBufferInfo Info;
                Info.count = count;
                Info.hDDIPool = pPool->hDDIPool;
                Info.level = CommandBufferLevels::PRIMARY;
                ret = DDI.AllocateObjects( Info, &vTmps[0] );
                if( VKE_SUCCEEDED( ret ) )
                {
                    // $TID CreateCommandBuffers: cbmgr={(void*)this}, pool={pPool->m_hPool}, cbs={vTmps}
                    pPool->vDDICommandBuffers.Append( vTmps.GetCount(), &vTmps[0] );
                    for( uint32_t i = 0; i < count; ++i )
                    {
                        CCommandBuffer Cb;
                        Cb.Init( m_pCtx, vTmps[i] );
                        pPool->vCommandBuffers.PushBack( Cb );
                        pPool->vpFreeCommandBuffers.PushBack( CommandBufferPtr( &pPool->vCommandBuffers.Back() ) );
                    }
                    ret = _CreateCommandBuffers( count, ppArray, pPool );
                }
            }

            for( uint32_t i = 0; i < count; ++i )
            {
                vFreeCbs.PopBack( &ppArray[ i ] );
            }
            return ret;
        }
       
    } // RenderSystem
} // vke
#endif // VKE_VULKAN_RENDERER