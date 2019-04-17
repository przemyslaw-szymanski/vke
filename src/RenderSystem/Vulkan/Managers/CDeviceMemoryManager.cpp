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

        Result CDeviceMemoryManager::_AllocateFromView( const SAllocateDesc& Desc,
            SAllocationHandle* pHandleOut, SBindMemoryInfo* pBindInfoOut )
        {
            SAllocateInfo Info;
            Info.hView = Desc.hView;
            Info.size = Desc.Memory.size;

            CMemoryPoolView::SAllocateData Data;
            Result ret = _AllocateSpace( Info, &Data );

            auto& BindInfo = *pBindInfoOut;
            BindInfo.hBuffer = Desc.Memory.hBuffer;
            BindInfo.hImage = Desc.Memory.hImage;
            BindInfo.hMemory = reinterpret_cast<DDIMemory>(Data.memory);
            BindInfo.offset = Data.offset;

            auto& Handle = *pHandleOut;
            Handle.hView.handle = Desc.hView;
            Handle.offset = Data.offset;

            return ret;
        }

        handle_t CDeviceMemoryManager::AllocateBuffer( const SAllocateDesc& Desc, SBindMemoryInfo* pBindInfoOut )
        {
            SAllocationHandle Handle;
            handle_t ret = NULL_HANDLE;
            Result res = VKE_FAIL;

            if( Desc.hView != NULL_HANDLE )
            {
                res = _AllocateFromView( Desc, &Handle, pBindInfoOut );
            }
            else
            {
                SMemoryAllocateData Data;
                res = m_pCtx->_GetDDI().Allocate< ResourceTypes::BUFFER >( Desc.Memory, nullptr, &Data );
                if( VKE_SUCCEEDED( res ) )
                {
                    Handle.handle = reinterpret_cast<handle_t>(Data.hMemory);

                    auto& BindInfo = *pBindInfoOut;
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
                    res = m_pCtx->_GetDDI().Bind< ResourceTypes::BUFFER >( *pBindInfoOut );
                }
            }
            if( VKE_SUCCEEDED( res ) )
            {
                ret = Handle.handle;
            }
            return ret;
        }

        handle_t CDeviceMemoryManager::AllocateTexture(const SAllocateDesc& Desc, SBindMemoryInfo* pBindInfoOut )
        {
            SAllocationHandle Handle;
            handle_t ret = NULL_HANDLE;
            Result res = VKE_FAIL;

            if( Desc.hView != NULL_HANDLE )
            {
                res = _AllocateFromView( Desc, &Handle, pBindInfoOut );
            }
            else
            {
                SMemoryAllocateData Data;
                res = m_pCtx->_GetDDI().Allocate< ResourceTypes::TEXTURE >( Desc.Memory, nullptr, &Data );
                if( VKE_SUCCEEDED( res ) )
                {
                    Handle.handle = reinterpret_cast< handle_t >( Data.hMemory );
                    auto& BindInfo = *pBindInfoOut;
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
                    res = m_pCtx->_GetDDI().Bind< ResourceTypes::TEXTURE >( *pBindInfoOut );
                }
            }
            if( VKE_SUCCEEDED( res ) )
            {
                ret = Handle.handle;
            }
            return ret;
        }

        Result CDeviceMemoryManager::BindMemory( const SBindMemoryInfo& Info )
        {
            Result ret = VKE_FAIL;
            if( Info.hBuffer != DDI_NULL_HANDLE )
            {
                ret = m_pCtx->DDI().Bind<ResourceTypes::BUFFER>( Info );
            }
            else if( Info.hImage != DDI_NULL_HANDLE )
            {
                ret = m_pCtx->DDI().Bind<ResourceTypes::TEXTURE>( Info );
            }
            return ret;
        }

        Result CDeviceMemoryManager::UpdateMemory( const SUpdateMemoryInfo& DataInfo, const SBindMemoryInfo& BindInfo )
        {
            Result ret = VKE_ENOMEMORY;
            SMapMemoryInfo MapInfo;
            MapInfo.hMemory = BindInfo.hMemory;
            MapInfo.offset = BindInfo.offset + DataInfo.offset;
            MapInfo.size = DataInfo.dataSize;
            void* pDst = m_pCtx->DDI().MapMemory( MapInfo );
            if( pDst != nullptr )
            {
                Memory::Copy( pDst, DataInfo.dataSize, DataInfo.pData, DataInfo.dataSize );
                ret = VKE_OK;
            }
            m_pCtx->DDI().UnmapMemory( BindInfo.hMemory );
            return ret;
        }

    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER