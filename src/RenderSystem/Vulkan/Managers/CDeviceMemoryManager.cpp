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
            m_PoolBuffer.Add( {} );
            m_vPoolViews.PushBack( {} );
            m_vSyncObjects.PushBack( {} );
            return ret;
        }

        void CDeviceMemoryManager::Destroy()
        {

        }

        handle_t CDeviceMemoryManager::_CreatePool( const SCreateMemoryPoolDesc& Desc )
        {
            handle_t ret = NULL_HANDLE;
            SAllocateMemoryDesc AllocDesc;
            AllocDesc.size = Desc.size;
            AllocDesc.usage = Desc.usage;

            SAllocateMemoryData MemData;
            Result res = m_pCtx->DDI().Allocate( AllocDesc, &MemData );
            if( VKE_SUCCEEDED( res ) )
            {
                SPool Pool;
                Pool.Data = MemData;
                ret = m_PoolBuffer.Add( Pool );
                if( ret != UNDEFINED_U32 )
                {
                    CMemoryPoolView::SInitInfo Info;
                    Info.memory = reinterpret_cast<uint64_t>(MemData.hDDIMemory);
                    Info.offset = 0;
                    Info.size = Desc.size;
                    Info.allocationAlignment = Desc.alignment;
                    CMemoryPoolView View;
                    uint32_t viewIdx = ret;
                    // There is a new pool to be added
                    if( ret >= m_vPoolViews.GetCount() )
                    {
                        viewIdx = m_vPoolViews.PushBack( View );
                        m_vSyncObjects.PushBack( {} );
                    }
                    {
                        m_vPoolViews[ viewIdx ].Init( Info );
                    }
                }
            }
            return ret;
        }

        handle_t CDeviceMemoryManager::_AllocateFromPool( const SAllocateDesc& Desc,
            const SAllocationMemoryRequirements& MemReq, SBindMemoryInfo* pBindInfoOut )
        {
            handle_t ret = NULL_HANDLE;
            SPool* pPool = nullptr;

            auto Itr = m_mPoolIndices.find( Desc.Memory.memoryUsages );
            // If no pool is created for such memory usage create a new one
            // and call this function again
            if( Itr == m_mPoolIndices.end() )
            {
                SCreateMemoryPoolDesc PoolDesc;
                PoolDesc.usage = Desc.Memory.memoryUsages;
                PoolDesc.size = std::max<uint32_t>( Desc.poolSize, MemReq.size );
                PoolDesc.alignment = MemReq.alignment;
                handle_t hPool = _CreatePool( PoolDesc );
                m_mPoolIndices[ Desc.Memory.memoryUsages ].PushBack( hPool );
                ret = _AllocateFromPool( Desc, MemReq, pBindInfoOut );
            }
            else
            {
                const HandleVec& vHandles = Itr->second;
                
                SAllocateMemoryInfo Info;
                Info.alignment = MemReq.alignment;
                Info.size = MemReq.size;
                // Find firt pool with enough memory
                for( uint32_t i = 0; i < vHandles.GetCount(); ++i )
                {
                    const auto poolIdx = vHandles[ i ];
                    //auto& Pool = m_PoolBuffer[ poolIdx ];
                    auto& View = m_vPoolViews[ poolIdx ];
                    
                    CMemoryPoolView::SAllocateData Data;
                    uint64_t memory = View.Allocate( Info, &Data );
                    if( memory != CMemoryPoolView::INVALID_ALLOCATION )
                    {
                        pBindInfoOut->hDDIBuffer = Desc.Memory.hDDIBuffer;
                        pBindInfoOut->hDDITexture = Desc.Memory.hDDITexture;
                        pBindInfoOut->hDDIMemory = reinterpret_cast<DDIMemory>( Data.memory );
                        pBindInfoOut->offset = Data.offset;
                        pBindInfoOut->hMemory = poolIdx;

                        UAllocationHandle Handle;
                        SMemoryAllocationInfo AllocInfo;
                        AllocInfo.hMemory = Data.memory;
                        AllocInfo.offset = Data.offset;
                        AllocInfo.size = Info.size;
                        Handle.hAllocInfo = m_AllocBuffer.Add( AllocInfo );
                        Handle.hPool = poolIdx;
                        Handle.dedicated = false;
                        ret = Handle.handle;
                        break;
                    }
                }
            }

            return ret;
        }

        handle_t CDeviceMemoryManager::_AllocateMemory( const SAllocateDesc& Desc, SBindMemoryInfo* pOut )
        {
            handle_t ret = NULL_HANDLE;
            const auto dedicatedAllocation = Desc.Memory.memoryUsages & MemoryUsages::DEDICATED_ALLOCATION;

            SAllocationMemoryRequirements MemReq;
            if( Desc.Memory.hDDIBuffer != DDI_NULL_HANDLE )
            {
                m_pCtx->DDI().GetMemoryRequirements( Desc.Memory.hDDIBuffer, &MemReq );
            }
            else if( Desc.Memory.hDDITexture != DDI_NULL_HANDLE )
            {
                m_pCtx->DDI().GetMemoryRequirements( Desc.Memory.hDDITexture, &MemReq );
            }

            if( !dedicatedAllocation )
            {
                ret =_AllocateFromPool( Desc, MemReq, pOut );
            }
            else
            {
                SAllocateMemoryData Data;
                SAllocateMemoryDesc AllocDesc;
                AllocDesc.size = MemReq.size;
                AllocDesc.usage = Desc.Memory.memoryUsages;
                Result res = m_pCtx->_GetDDI().Allocate( AllocDesc, &Data );
                if( VKE_SUCCEEDED( res ) )
                {
                    auto& BindInfo = *pOut;
                    BindInfo.hDDITexture = Desc.Memory.hDDITexture;
                    BindInfo.hDDIBuffer = Desc.Memory.hDDIBuffer;
                    BindInfo.hDDIMemory = Data.hDDIMemory;
                    BindInfo.hMemory = NULL_HANDLE;
                    BindInfo.offset = 0;

                    SMemoryAllocationInfo AllocInfo;
                    AllocInfo.hMemory = reinterpret_cast< handle_t >( Data.hDDIMemory );
                    AllocInfo.offset = 0;
                    AllocInfo.size = AllocDesc.size;
                    UAllocationHandle Handle;
                    Handle.dedicated = true;
                    Handle.hAllocInfo = m_AllocBuffer.Add( AllocInfo );
                    Handle.hPool = 0;
                    ret = Handle.handle;
                }
            }
            return ret;
        }

        handle_t CDeviceMemoryManager::AllocateBuffer( const SAllocateDesc& Desc )
        {
            SBindMemoryInfo BindInfo;
            handle_t ret = _AllocateMemory( Desc, &BindInfo );

            if( ret != NULL_HANDLE )
            {
                {
                    m_pCtx->_GetDDI().Bind< ResourceTypes::BUFFER >( BindInfo );
                }
            }
            return ret;
        }

        handle_t CDeviceMemoryManager::AllocateTexture(const SAllocateDesc& Desc )
        {
            SBindMemoryInfo BindInfo;
            handle_t ret = _AllocateMemory( Desc, &BindInfo );

            if( ret != NULL_HANDLE )
            {
                {
                    m_pCtx->_GetDDI().Bind< ResourceTypes::TEXTURE >( BindInfo );
                }
            }
            return ret;
        }

        Result CDeviceMemoryManager::UpdateMemory( const SUpdateMemoryInfo& DataInfo, const SBindMemoryInfo& BindInfo )
        {
            Result ret = VKE_ENOMEMORY;
            SMapMemoryInfo MapInfo;
            MapInfo.hMemory = BindInfo.hDDIMemory;
            MapInfo.offset = BindInfo.offset + DataInfo.dstDataOffset;
            MapInfo.size = DataInfo.dataSize;
            void* pDst = m_pCtx->DDI().MapMemory( MapInfo );
            if( pDst != nullptr )
            {
                Memory::Copy( pDst, DataInfo.dataSize, DataInfo.pData, DataInfo.dataSize );
                ret = VKE_OK;
            }
            m_pCtx->DDI().UnmapMemory( BindInfo.hDDIMemory );
            return ret;
        }

        Result CDeviceMemoryManager::UpdateMemory( const SUpdateMemoryInfo& DataInfo, const handle_t& hMemory )
        {
            UAllocationHandle Handle = hMemory;

            const auto& AllocInfo = m_AllocBuffer[ Handle.hAllocInfo ];
            Result ret = VKE_ENOMEMORY;
            SMapMemoryInfo MapInfo;
            MapInfo.hMemory = reinterpret_cast< DDIMemory >( AllocInfo.hMemory );
            MapInfo.offset = AllocInfo.offset + DataInfo.dstDataOffset;
            MapInfo.size = DataInfo.dataSize;
            
            {
                Threads::ScopedLock l( m_vSyncObjects[Handle.hPool] );
                void* pDst = m_pCtx->DDI().MapMemory( MapInfo );
                if( pDst != nullptr )
                {
                    Memory::Copy( pDst, DataInfo.dataSize, DataInfo.pData, DataInfo.dataSize );
                    ret = VKE_OK;
                }
                m_pCtx->DDI().UnmapMemory( MapInfo.hMemory );
            }
            return ret;
        }

        const SMemoryAllocationInfo& CDeviceMemoryManager::GetAllocationInfo( const handle_t& hMemory )
        {
            UAllocationHandle Handle = hMemory;
            return m_AllocBuffer[Handle.hAllocInfo];
        }

    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER