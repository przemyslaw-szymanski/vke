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
            m_lastPoolSize = Desc.defaultPoolSize;
            return ret;
        }

        void CDeviceMemoryManager::Destroy()
        {

        }

        handle_t CDeviceMemoryManager::_CreatePool( const SCreateMemoryPoolDesc& Desc )
        {
            handle_t ret = INVALID_HANDLE;
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
                    Info.memory = (uint64_t)(MemData.hDDIMemory);
                    Info.offset = 0;
                    Info.size = Desc.size;
                    Info.allocationAlignment = Desc.alignment;
                    CMemoryPoolView View;
                    handle_t viewIdx = ret;
                    // There is a new pool to be added
                    if( ret >= m_vPoolViews.GetCount() )
                    {
                        viewIdx = m_vPoolViews.PushBack( View );
                        m_vSyncObjects.PushBack( {} );
                    }
                    {
                        m_vPoolViews[ viewIdx ].Init( Info );
                    }

                    m_totalMemAllocated += AllocDesc.size;
                    VKE_LOG_WARN("Created new device memory pool with size: " << VKE_LOG_MEM_SIZE(AllocDesc.size) << ".");
                    VKE_LOG("Total device memory allocated: " << VKE_LOG_MEM_SIZE(m_totalMemAllocated) << ".");
                }
            }
            return ret;
        }

        uint32_t CalculateNewPoolSize(const uint32_t& newPoolSize, const uint32_t& lastPoolSize,
            const SDeviceMemoryManagerDesc& Desc)
        {
            uint32_t ret = newPoolSize;
            if (newPoolSize == 0)
            {
                switch (Desc.resizePolicy)
                {
                    case DeviceMemoryResizePolicies::DEFAULT_SIZE:
                    {
                        ret = Desc.defaultPoolSize;
                        break;
                    }
                    case DeviceMemoryResizePolicies::TWO_TIMES_DEFAULT_SIZE:
                    {
                        ret = Desc.defaultPoolSize * 2;
                        break;
                    }
                    case DeviceMemoryResizePolicies::FOUR_TIMES_DEFAULT_SIZE:
                    {
                        ret = Desc.defaultPoolSize * 4;
                        break;
                    }
                    case DeviceMemoryResizePolicies::TWO_TIMES_LAST_SIZE:
                    {
                        ret = lastPoolSize * 2;
                        break;
                    }
                    case DeviceMemoryResizePolicies::FOUR_TIMES_LAST_SIZE:
                    {
                        ret = lastPoolSize * 4;
                        break;
                    }
                }
            }
            return ret;
        }

        handle_t CDeviceMemoryManager::_CreatePool(const SAllocateDesc& Desc,
            const SAllocationMemoryRequirementInfo& MemReq)
        {
            auto poolSize = std::max<uint32_t>(m_lastPoolSize, MemReq.size);
            poolSize = std::max<uint32_t>(poolSize, Desc.poolSize);

            SCreateMemoryPoolDesc PoolDesc;
            PoolDesc.usage = Desc.Memory.memoryUsages;
            PoolDesc.size = poolSize;
            PoolDesc.alignment = MemReq.alignment;
            handle_t hPool = _CreatePool(PoolDesc);
            m_mPoolIndices[Desc.Memory.memoryUsages].PushBack(hPool);

            m_lastPoolSize = poolSize;
            return hPool;
        }

        handle_t CDeviceMemoryManager::_AllocateFromPool( const SAllocateDesc& Desc,
            const SAllocationMemoryRequirementInfo& MemReq, SBindMemoryInfo* pBindInfoOut )
        {
            handle_t ret = INVALID_HANDLE;
            //SPool* pPool = nullptr;

            auto Itr = m_mPoolIndices.find( Desc.Memory.memoryUsages );
            // If no pool is created for such memory usage create a new one
            // and call this function again
            if( Itr == m_mPoolIndices.end() )
            {
                const handle_t hPool = _CreatePool(Desc, MemReq);
                VKE_ASSERT(hPool != INVALID_HANDLE, "");
                ret = _AllocateFromPool( Desc, MemReq, pBindInfoOut );
            }
            else
            {
                const HandleVec& vHandles = Itr->second;

                SAllocateMemoryInfo Info;
                Info.alignment = MemReq.alignment;
                Info.size = MemReq.size;
                uint64_t memory = CMemoryPoolView::INVALID_ALLOCATION;
                // Find firt pool with enough memory
                for( uint32_t i = 0; i < vHandles.GetCount(); ++i )
                {
                    const auto poolIdx = vHandles[ i ];
                    //auto& Pool = m_PoolBuffer[ poolIdx ];
                    auto& View = m_vPoolViews[ poolIdx ];

                    CMemoryPoolView::SAllocateData Data;
                    memory = View.Allocate( Info, &Data );
                    if( memory != CMemoryPoolView::INVALID_ALLOCATION )
                    {
                        pBindInfoOut->hDDIBuffer = Desc.Memory.hDDIBuffer;
                        pBindInfoOut->hDDITexture = Desc.Memory.hDDITexture;
                        pBindInfoOut->hDDIMemory = (DDIMemory)( Data.memory );
                        pBindInfoOut->offset = Data.offset;
                        pBindInfoOut->hMemory = poolIdx;

                        UAllocationHandle Handle;
                        SMemoryAllocationInfo AllocInfo;
                        AllocInfo.hMemory = Data.memory;
                        AllocInfo.offset = Data.offset;
                        AllocInfo.size = Info.size;
                        Handle.hAllocInfo = m_AllocBuffer.Add( AllocInfo );
                        Handle.hPool = static_cast< uint16_t >( poolIdx );
                        Handle.dedicated = false;
                        ret = Handle.handle;
                        break;
                    }
                }
                // If there is no free space in any of currently allocated pools
                if (memory == CMemoryPoolView::INVALID_ALLOCATION)
                {
                    // Create new memory pool
                    SAllocateDesc NewDesc = Desc;
                    NewDesc.poolSize = CalculateNewPoolSize(Desc.poolSize, m_lastPoolSize, m_Desc);
                    //const float sizeMB = NewDesc.poolSize / 1024.0f / 1024.0f;
                    VKE_LOG_WARN("No device memory for allocation with requirements: " << VKE_LOG_MEM_SIZE(MemReq.size) << ", " << MemReq.alignment << " bytes alignment.");
                    //VKE_LOG_WARN("Create new device memory pool with size: " << VKE_LOG_MEM_SIZE(NewDesc.poolSize) << ".");
                    const handle_t hPool = _CreatePool(NewDesc, MemReq);
                    //VKE_LOG_WARN("Total device memory allocated: " << VKE_LOG_MEM_SIZE(m_totalMemAllocated) << "." );
                    VKE_ASSERT(hPool != INVALID_HANDLE, "");
                    ret = _AllocateFromPool(Desc, MemReq, pBindInfoOut);
                }
            }

            m_totalMemUsed += MemReq.size;

            return ret;
        }

        handle_t CDeviceMemoryManager::_AllocateMemory( const SAllocateDesc& Desc, SBindMemoryInfo* pOut )
        {
            handle_t ret = INVALID_HANDLE;
            const auto dedicatedAllocation = Desc.Memory.memoryUsages & MemoryUsages::DEDICATED_ALLOCATION;

            SAllocationMemoryRequirementInfo MemReq = {};
            if( Desc.Memory.hDDIBuffer != DDI_NULL_HANDLE )
            {
                m_pCtx->DDI().GetBufferMemoryRequirements( Desc.Memory.hDDIBuffer, &MemReq );
            }
            else if( Desc.Memory.hDDITexture != DDI_NULL_HANDLE )
            {
                m_pCtx->DDI().GetTextureMemoryRequirements( Desc.Memory.hDDITexture, &MemReq );
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
                    BindInfo.hMemory = INVALID_HANDLE;
                    BindInfo.offset = 0;

                    SMemoryAllocationInfo AllocInfo;
                    AllocInfo.hMemory = ( handle_t )( Data.hDDIMemory );
                    AllocInfo.offset = 0;
                    AllocInfo.size = AllocDesc.size;
                    UAllocationHandle Handle;
                    Handle.dedicated = true;
                    Handle.hAllocInfo = m_AllocBuffer.Add( AllocInfo );
                    Handle.hPool = 0;
                    ret = Handle.handle;

                    VKE_LOG_WARN("Allocate new device memory with size: " << VKE_LOG_MEM_SIZE(AllocDesc.size) << ".");

                    m_totalMemAllocated += AllocDesc.size;
                    m_totalMemUsed += AllocDesc.size;

                    VKE_LOG_WARN("Total device memory allocated: " << VKE_LOG_MEM_SIZE(m_totalMemAllocated) << ".");
                }
            }
            return ret;
        }

        handle_t CDeviceMemoryManager::AllocateBuffer( const SAllocateDesc& Desc )
        {
            SBindMemoryInfo BindInfo;
            handle_t ret = _AllocateMemory( Desc, &BindInfo );

            if( ret != INVALID_HANDLE )
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

            if( ret != INVALID_HANDLE )
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
            MapInfo.hMemory = ( DDIMemory )( AllocInfo.hMemory );
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

        void* CDeviceMemoryManager::MapMemory(const SUpdateMemoryInfo& DataInfo, const handle_t& hMemory)
        {
            UAllocationHandle Handle = hMemory;
            const auto& AllocInfo = m_AllocBuffer[Handle.hAllocInfo];
            SMapMemoryInfo MapInfo;
            MapInfo.hMemory = (DDIMemory)AllocInfo.hMemory;
            MapInfo.offset = AllocInfo.offset + DataInfo.dstDataOffset;
            MapInfo.size = DataInfo.dataSize;
            Threads::ScopedLock l(m_vSyncObjects[Handle.hPool]);
            void* pRet = m_pCtx->DDI().MapMemory(MapInfo);
            return pRet;
        }

        void CDeviceMemoryManager::UnmapMemory(const handle_t& hMemory)
        {
            UAllocationHandle Handle = hMemory;
            const auto& AllocInfo = m_AllocBuffer[Handle.hAllocInfo];
            Threads::ScopedLock l(m_vSyncObjects[Handle.hPool]);
            m_pCtx->DDI().UnmapMemory((DDIMemory)AllocInfo.hMemory);
        }

        const SMemoryAllocationInfo& CDeviceMemoryManager::GetAllocationInfo( const handle_t& hMemory )
        {
            UAllocationHandle Handle = hMemory;
            return m_AllocBuffer[Handle.hAllocInfo];
        }

    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER