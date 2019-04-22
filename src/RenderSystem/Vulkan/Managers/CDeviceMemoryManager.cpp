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

        handle_t CDeviceMemoryManager::_CreatePool( const SCreateMemoryPoolDesc& Desc )
        {
            handle_t ret = NULL_HANDLE;
            SAllocateMemoryDesc AllocDesc;
            AllocDesc.size = Desc.Memory.size;
            AllocDesc.usage = Desc.Memory.memoryUsages;
            SAllocateMemoryData MemData;
            Result res = m_pCtx->DDI().Allocate( AllocDesc, &MemData );
            if( VKE_SUCCEEDED( res ) )
            {
                CMemoryPoolView::SInitInfo Info;
                Info.memory = reinterpret_cast< uint64_t >( MemData.hDDIMemory );
                Info.offset = 0;
                Info.size = Desc.Memory.size;

                SPool Pool;
                Pool.Data = MemData;
                Pool.View.Init( Info );
                ret = m_vPools.PushBack( Pool );
            }
            return ret;
        }

        Result CDeviceMemoryManager::_AllocateFromPool( const SAllocateDesc& Desc,
            SAllocationHandle* pHandleOut, SBindMemoryInfo* pBindInfoOut )
        {
            Result ret = VKE_FAIL;
            SPool* pPool = nullptr;

            auto Itr = m_mPoolIndices.find( Desc.Memory.memoryUsages );
            // If no pool is created for such memory usage create a new one
            // and call this function again
            if( Itr == m_mPoolIndices.end() )
            {
                SCreateMemoryPoolDesc PoolDesc;
                PoolDesc.bind = false;
                PoolDesc.Memory.memoryUsages = Desc.Memory.memoryUsages;
                PoolDesc.Memory.size = std::max<uint32_t>( Desc.poolSize, Desc.Memory.size );
                handle_t hPool = _CreatePool( PoolDesc );
                m_mPoolIndices[ Desc.Memory.memoryUsages ].PushBack( hPool );
                return _AllocateFromPool( Desc, pHandleOut, pBindInfoOut );
            }
            else
            {
                const HandleVec& vHandles = Itr->second;
                SAllocationMemoryRequirements MemReq;
                if( Desc.Memory.hDDIBuffer != DDI_NULL_HANDLE )
                {
                    m_pCtx->DDI().GetMemoryRequirements( Desc.Memory.hDDIBuffer, &MemReq );
                }
                else if( Desc.Memory.hDDITexture != DDI_NULL_HANDLE )
                {
                    m_pCtx->DDI().GetMemoryRequirements( Desc.Memory.hDDITexture, &MemReq );
                }
                SAllocateMemoryInfo Info;
                Info.alignment = MemReq.alignment;
                Info.size = MemReq.size;
                // Find firt pool with enough memory
                for( uint32_t i = 0; i < vHandles.GetCount(); ++i )
                {
                    const auto poolIdx = vHandles[ i ];
                    auto& Pool = m_vPools[ poolIdx ];
                    
                    CMemoryPoolView::SAllocateData Data;
                    uint64_t memory = Pool.View.Allocate( Info, &Data );
                    if( memory != CMemoryPoolView::INVALID_ALLOCATION )
                    {
                        ret = VKE_OK;
                        pBindInfoOut->hDDIBuffer = Desc.Memory.hDDIBuffer;
                        pBindInfoOut->hDDITexture = Desc.Memory.hDDITexture;
                        pBindInfoOut->hDDIMemory = reinterpret_cast<DDIMemory>( Data.memory );
                        pBindInfoOut->offset = Data.offset;
                        pBindInfoOut->hMemory = poolIdx;
                        pHandleOut->handle = poolIdx;
                        break;
                    }
                }
            }

            return ret;
        }

        handle_t CDeviceMemoryManager::_AllocateMemory( const SAllocateDesc& Desc, SBindMemoryInfo* pOut )
        {
            SAllocationHandle hAlloc = {};
            const auto dedicatedAllocation = Desc.Memory.memoryUsages & MemoryUsages::SEPARATE_ALLOCATION;
            if( !dedicatedAllocation )
            {
                Result res =_AllocateFromPool( Desc, &hAlloc, pOut );
            }
            else
            {
                SAllocateMemoryData Data;
                SAllocateMemoryDesc AllocDesc;
                AllocDesc.size = Desc.Memory.size;
                AllocDesc.usage = Desc.Memory.memoryUsages;
                Result res = m_pCtx->_GetDDI().Allocate( AllocDesc, &Data );
                if( VKE_SUCCEEDED( res ) )
                {
                    hAlloc.handle = reinterpret_cast< handle_t >( Data.hDDIMemory );

                    auto& BindInfo = *pOut;
                    BindInfo.hDDITexture = Desc.Memory.hDDITexture;
                    BindInfo.hDDIBuffer = Desc.Memory.hDDIBuffer;
                    BindInfo.hDDIMemory = Data.hDDIMemory;
                    BindInfo.hMemory = NULL_HANDLE;
                    BindInfo.offset = 0;
                }
            }
            return hAlloc.handle;
        }

        handle_t CDeviceMemoryManager::AllocateBuffer( const SAllocateDesc& Desc, SBindMemoryInfo* pBindInfoOut )
        {
            handle_t ret = _AllocateMemory( Desc, pBindInfoOut );

            if( ret != NULL_HANDLE )
            {
                {
                    m_pCtx->_GetDDI().Bind< ResourceTypes::BUFFER >( *pBindInfoOut );
                }
            }
            return ret;
        }

        handle_t CDeviceMemoryManager::AllocateTexture(const SAllocateDesc& Desc, SBindMemoryInfo* pBindInfoOut )
        {
            handle_t ret = _AllocateMemory( Desc, pBindInfoOut );

            if( ret != NULL_HANDLE )
            {
                {
                    m_pCtx->_GetDDI().Bind< ResourceTypes::TEXTURE >( *pBindInfoOut );
                }
            }
            return ret;
        }

        Result CDeviceMemoryManager::UpdateMemory( const SUpdateMemoryInfo& DataInfo, const SBindMemoryInfo& BindInfo )
        {
            Result ret = VKE_ENOMEMORY;
            SMapMemoryInfo MapInfo;
            MapInfo.hMemory = BindInfo.hDDIMemory;
            MapInfo.offset = BindInfo.offset + DataInfo.offset;
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

    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER