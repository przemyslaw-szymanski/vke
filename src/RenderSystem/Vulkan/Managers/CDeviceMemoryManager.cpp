#include "RenderSystem/Vulkan/Managers/CDeviceMemoryManager.h"
#if VKE_VULKAN_RENDERER
#include "RenderSystem/CDeviceContext.h"
#include "RenderSystem/CDDI.h"
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
            // Add empty pool
            m_vPools.PushBack( {} );
            return ret;
        }

        void CDeviceMemoryManager::Destroy()
        {

        }

        handle_t CDeviceMemoryManager::CreateView( const SViewDesc& Desc )
        {
            SViewHandle ret;
            ret.handle = Desc.hPool;
            SPool& Pool = m_vPools[ret.poolIdx];
            if( Pool.sizeUsed >= Desc.size )
            {
                CMemoryPoolView View;
                CMemoryPoolView::SInitInfo Info;
                Info.allocationAlignment = Pool.Data.alignment;
                Info.memory = 0;
                Info.offset = Pool.sizeUsed;
                Info.size = Desc.size;
                Pool.sizeUsed += Desc.size;

                if( VKE_SUCCEEDED( View.Init( Info ) ) )
                {
                    ret.viewIdx = Pool.vViews.PushBack( View );
                }
                else
                {
                    ret.handle = NULL_HANDLE;
                }
            }
            else
            {
                VKE_LOG_ERR( "Memory pool has no free memory left (size left: " << Pool.sizeUsed << ") for requested view (size: " << Desc.size << "." );
                ret.handle = NULL_HANDLE;
            }
            return ret.handle;
        }

        Result CDeviceMemoryManager::_AllocateSpace( const SAllocateInfo& Info, CMemoryPoolView::SAllocateData* pOut )
        {
            Result ret = VKE_ENOMEMORY;
            SViewHandle hView;
            hView.handle = Info.hView;
            auto& Pool = m_vPools[hView.poolIdx];
            CMemoryPoolView& View = Pool.vViews[hView.viewIdx];
            uint64_t mem = View.Allocate( Info.size, pOut );
            if( mem != CMemoryPoolView::INVALID_ALLOCATION )
            {             
                ret = VKE_OK;
            }
            return ret;
        }

        handle_t CDeviceMemoryManager::AllocateTexture(const SAllocateDesc& Desc)
        {
            SBindMemoryInfo BindInfo;
            handle_t ret = NULL_HANDLE;
            Result res = VKE_FAIL;
            SAllocationHandle Handle;

            if( Desc.hView != NULL_HANDLE )
            {
                               /*Memory::CMemoryPoolManager::SAllocateInfo Info;
                Memory::CMemoryPoolManager::SAllocatedData Data;
                Info.hPool = Desc.hPool;
                Info.size = Desc.Memory.size;
                uint64_t res = m_MemoryPoolMgr.Allocate( Info, &Data );
                ret = res != 0 ? VKE_OK : VKE_FAIL;
                BindInfo.hBuffer = Desc.Memory.hBuffer;
                BindInfo.hImage = Desc.Memory.hImage;
                BindInfo.hMemory = reinterpret_cast< DDIMemory >( Data.Memory.memory );
                BindInfo.offset = Data.Memory.offset;*/
                SAllocateInfo Info;
                Info.hView = Desc.hView;
                Info.size = Desc.Memory.size;
                CMemoryPoolView::SAllocateData Data;
                res = _AllocateSpace( Info, &Data );
                BindInfo.hBuffer = Desc.Memory.hBuffer;
                BindInfo.hImage = Desc.Memory.hImage;
                BindInfo.hMemory = reinterpret_cast<DDIMemory>(Data.memory);
                BindInfo.offset = Data.offset;
                
                Handle.hView.handle = Desc.hView;
                Handle.offset = Data.offset;
            }
            else
            {
                SMemoryAllocateData Data;
                res = m_pCtx->_GetDDI().Allocate< ResourceTypes::TEXTURE >( Desc.Memory, nullptr, &Data );
                if( VKE_SUCCEEDED( res ) )
                {
                    Handle.handle = reinterpret_cast< handle_t >( Data.hMemory );

                    BindInfo.hImage = Desc.Memory.hImage;
                    BindInfo.hBuffer = Desc.Memory.hBuffer;
                    BindInfo.hMemory = Data.hMemory;
                    BindInfo.offset = 0;
                }
            }
            if( VKE_SUCCEEDED( res ) )
            {
                if( Desc.bind )
                {   
                    res = m_pCtx->_GetDDI().Bind< ResourceTypes::TEXTURE >( BindInfo );
                }
            }
            if( VKE_SUCCEEDED( res ) )
            {
                ret = Handle.handle;
            }
            return ret;
        }

    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER