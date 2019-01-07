#include "RenderSystem/Vulkan/Managers/CDeviceMemoryManager.h"
#if VKE_VULKAN_RENDERER
#include "RenderSystem/CDeviceContext.h"
#include "RenderSystem/CDeviceDriverInterface.h"
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
            m_Desc = Desc;
            Result ret = VKE_OK;

            return ret;
        }

        void CDeviceMemoryManager::Destroy()
        {

        }

        Result CDeviceMemoryManager::AllocateTexture(const SAllocateDesc& Desc)
        {
            SBindMemoryInfo BindInfo;
            Result ret;

            if( Desc.hPool )
            {
                Memory::CMemoryPoolManager::SAllocateInfo Info;
                Memory::CMemoryPoolManager::SAllocatedData Data;
                Info.hPool = Desc.hPool;
                Info.size = Desc.Memory.size;
                uint64_t res = m_MemoryPoolMgr.Allocate( Info, &Data );
                ret = res != 0 ? VKE_OK : VKE_FAIL;
                BindInfo.hBuffer = Desc.Memory.hBuffer;
                BindInfo.hImage = Desc.Memory.hImage;
                BindInfo.hMemory = reinterpret_cast< DDIMemory >( Data.Memory.memory );
                BindInfo.offset = Data.Memory.offset;
            }
            else
            {
                SMemoryAllocateData Data;
                ret = m_pCtx->_GetDDI().Allocate< ResourceTypes::TEXTURE >( Desc.Memory, nullptr, &Data );
                BindInfo.hImage = Desc.Memory.hImage;
                BindInfo.hBuffer = Desc.Memory.hBuffer;
                BindInfo.hMemory = Data.hMemory;
                BindInfo.offset = 0;
            }
            if( VKE_SUCCEEDED( ret ) )
            {
                if( Desc.bind )
                {   
                    ret = m_pCtx->_GetDDI().Bind< ResourceTypes::TEXTURE >( BindInfo );
                }
            }
            return ret;
        }

    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER