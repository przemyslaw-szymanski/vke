#include "RenderSystem/Vulkan/Managers/CDeviceMemoryManager.h"
#if VKE_VULKAN_RENDERER
#include "RenderSystem/CDeviceContext.h"
#include "RenderSystem/Vulkan/Vulkan.h"
namespace VKE
{
    namespace RenderSystem
    {
        CDeviceMemoryManager::CDeviceMemoryManager(CDeviceContext* pCtx) :
            m_pCtx{ pCtx }
        {

        }

        CDeviceMemoryManager::~CDeviceMemoryManager()
        {
            
        }

        Result CDeviceMemoryManager::Create(const SDeviceMemoryManagerDesc& Desc)
        {
            m_pVkMemProperties = &m_pCtx->GetDeviceInfo().MemoryProperties;
            m_Desc = Desc;
            m_Desc.MemoryPoolDesc.poolTypeCount = ResourceTypes::_MAX_COUNT;
            m_Desc.MemoryPoolDesc.indexTypeCount = static_cast< uint32_t >( m_pVkMemProperties->memoryTypeCount );

            Result res = Memory::CMemoryPoolManager::Create( m_Desc.MemoryPoolDesc );
            if( VKE_SUCCEEDED( res ) )
            {
                if( m_vVkCaches.Resize( ResourceTypes::_MAX_COUNT ) )
                {
                    res = VKE_OK;
                }
            }
            
            return res;
        }

        void CDeviceMemoryManager::Destroy()
        {
            Memory::CMemoryPoolManager::Destroy();
        }

        int32_t FindMemoryTypeIndex(const VkPhysicalDeviceMemoryProperties* pMemProps,
                                    uint32_t requiredMemBits,
                                    VkMemoryPropertyFlags requiredProperties)
        {
            const uint32_t memCount = pMemProps->memoryTypeCount;
            for( uint32_t memIdx = 0; memIdx < memCount; ++memIdx )
            {
                const uint32_t memTypeBits = ( 1 << memIdx );
                const bool isRequiredMemType = requiredMemBits & memTypeBits;
                const VkMemoryPropertyFlags props = pMemProps->memoryTypes[ memIdx ].propertyFlags;
                const bool hasRequiredProps = ( props & requiredProperties ) == requiredProperties;
                if( isRequiredMemType && hasRequiredProps )
                    return static_cast< int32_t >( memIdx );
            }
            return -1;
        }

        uint64_t CDeviceMemoryManager::Allocate(const VkImage& vkImg, const VkMemoryPropertyFlags& vkProperties)
        {
            uint64_t ret = 0;
            VkMemoryRequirements vkMemReq;
            auto& Device = m_pCtx->_GetDevice();
            Device.GetMemoryRequirements( vkImg, &vkMemReq );

            SVkCache& VkCache = m_vVkCaches[ static_cast< uint32_t >( ResourceTypes::TEXTURE ) ];
            int32_t idx;
            if( VkCache.memoryBits == vkMemReq.memoryTypeBits && VkCache.vkPropertyFlags == vkProperties )
            {
                idx = VkCache.memoryTypeIndex;
            }
            else
            {
                idx = FindMemoryTypeIndex( m_pVkMemProperties, vkMemReq.memoryTypeBits, vkProperties );
                VkCache.memoryBits = vkMemReq.memoryTypeBits;
                VkCache.vkPropertyFlags = vkProperties;
                VkCache.memoryTypeIndex = idx;
            }

            if( idx >= 0 )
            {
                SAllocateInfo AllocInfo;
                AllocInfo.alignment = static_cast< uint32_t >( vkMemReq.alignment );
                AllocInfo.index = static_cast< uint32_t >( idx );
                AllocInfo.size = static_cast< uint32_t >( vkMemReq.size );
                AllocInfo.type = ResourceTypes::TEXTURE;
                SAllocatedData AllocData;
                ret = Allocate( AllocInfo, &AllocData );
                if( ret > 0 )
                {
                    VkDeviceMemory vkMemory = reinterpret_cast< VkDeviceMemory >( AllocData.Memory.memory );
                    VK_ERR( Device.BindMemory( vkImg, vkMemory, AllocData.Memory.offset ) );
                }
            }
            else
            {
                VKE_LOG_ERR("Unable to find proper memory type index.");
            }
            return ret;
        }

        uint64_t CDeviceMemoryManager::Allocate(const SAllocateInfo& Info, SAllocatedData* pOut)
        {
            uint64_t ptr = Memory::CMemoryPoolManager::Allocate( Info, pOut );
            if( ptr == 0 )
            {
                Memory::CMemoryPoolManager::SPoolAllocateInfo PoolInfo;
                PoolInfo.alignment = Info.alignment;
                PoolInfo.index = Info.index;
                PoolInfo.size = 1024*1024*10;
                PoolInfo.type = Info.type;
                if( VKE_SUCCEEDED( AllocatePool( PoolInfo ) ) )
                {
                    return Allocate( Info, pOut );
                }
            }
            return ptr;
        }

        Result CDeviceMemoryManager::AllocatePool(const SPoolAllocateInfo& Info)
        {
            Result res = VKE_ENOMEMORY;
            auto& Device = m_pCtx->_GetDevice();
            VkDeviceMemory vkMemory;
            VkMemoryAllocateInfo AllocateInfo;
            Vulkan::InitInfo( &AllocateInfo, VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO );
            AllocateInfo.allocationSize = Info.size;
            AllocateInfo.memoryTypeIndex = Info.index;
            VkResult vkRes = Device.AllocateMemory( AllocateInfo, nullptr, &vkMemory );
            VK_ERR( vkRes );
            if( vkRes == VK_ERROR_OUT_OF_HOST_MEMORY )
            {
                VKE_LOG_ERR( "Unable to allocate memory on the host. Memory size: " << Info.size );
                return res;
            }
            else if( vkRes == VK_ERROR_OUT_OF_DEVICE_MEMORY )
            {
                VKE_LOG_ERR( "Unable to allocate memory on the device. Memory size: " << Info.size );
                return res;
            }

            Memory::CMemoryPoolManager::SPoolAllocateInfo PoolInfo = Info;
            PoolInfo.memory = reinterpret_cast< uint64_t >( vkMemory );
            if( VKE_SUCCEEDED( Memory::CMemoryPoolManager::AllocatePool( PoolInfo ) ) )
            {
                res = VKE_OK;
            }

            return res;
        }

    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER