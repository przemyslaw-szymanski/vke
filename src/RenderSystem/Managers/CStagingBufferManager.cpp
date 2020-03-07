#include "CStagingBufferManager.h"
#include "RenderSystem/CDeviceContext.h"
#include "RenderSystem/Vulkan/Managers/CDeviceMemoryManager.h"

namespace VKE
{
    namespace RenderSystem
    {
        Result CStagingBufferManager::Create( const SStagingBufferManagerDesc& Desc )
        {
            Result ret = VKE_OK;
            m_Desc = Desc;
            return ret;
        }

        void CStagingBufferManager::Destroy(CDeviceContext*)
        {
            for( uint32_t i = 0; i < m_vpBuffers.GetCount(); ++i )
            {
                //pCtx->DestroyBuffer( &m_vpBuffers[i] );
                m_vpBuffers[i] = nullptr;
            }
            m_vpBuffers.Clear();
            m_vMemViews.Clear();
        }

        //Result CStagingBufferManager::GetBuffer( const SBufferRequirementInfo& Info, SBufferData** ppData )
        //{
        //    Result ret = VKE_ENOTFOUND;
        //    SBindMemoryInfo BindInfo;
        //    //auto& DeviceMemMgr = Info.pCtx->_GetDeviceMemoryManager();
        //    const uint32_t bufferSize = std::max( m_Desc.bufferSize, Info.Requirements.size );

        //    SAllocateMemoryInfo AllocInfo;
        //    CMemoryPoolView::SAllocateData AllocData;
        //    AllocInfo.alignment = Info.Requirements.alignment;
        //    AllocInfo.size = Info.Requirements.size;

        //    Threads::ScopedLock l( m_MemViewSyncObj );
        //    for( uint32_t i = 0; i < m_vMemViews.GetCount(); ++i )
        //    {
        //        CMemoryPoolView& View = m_vMemViews[ i ];
        //        uint64_t memory = View.Allocate( AllocInfo, &AllocData );
        //        if( memory != 0 )
        //        {
        //            SBufferData Data;
        //            Data.pBuffer = m_vpBuffers[i];
        //            Data.offset = AllocData.offset;
        //            Data.size = AllocInfo.size;
        //            Data.handle = i;
        //            const auto dataIdx = m_vUsedData.PushBack( Data );
        //            *ppData = &m_vUsedData[dataIdx];
        //            ret = VKE_OK;
        //            break;
        //        }
        //    }
        //    if( ret == VKE_ENOTFOUND )
        //    {
        //        SCreateBufferDesc BufferDesc;
        //        BufferDesc.Create.async = false;
        //        BufferDesc.Create.stages = Core::ResourceStages::FULL_LOAD;
        //        BufferDesc.Buffer.memoryUsage = MemoryUsages::STAGING;
        //        BufferDesc.Buffer.size = bufferSize;
        //        BufferDesc.Buffer.usage = BufferUsages::TRANSFER_SRC;
        //        BufferHandle hBuffer = Info.pCtx->CreateBuffer( BufferDesc );
        //        if( hBuffer != INVALID_HANDLE )
        //        {
        //            auto pBuffer = Info.pCtx->GetBuffer( hBuffer );
        //            CMemoryPoolView::SInitInfo ViewInfo;
        //            ViewInfo.allocationAlignment = 0;
        //            ViewInfo.memory = ( pBuffer->m_hMemory );
        //            ViewInfo.size = bufferSize;
        //            ViewInfo.offset = 0;
        //            CMemoryPoolView View;
        //            View.Init( ViewInfo );
        //            m_vMemViews.PushBack( View );
        //            m_vpBuffers.PushBack( pBuffer );
        //            return GetBuffer( Info, ppData );
        //        }
        //    }
        //    return ret;
        //}

        Result CStagingBufferManager::GetBuffer( const SBufferRequirementInfo& Info, handle_t* phInOut,
                                                 CStagingBufferManager::SBufferInfo* pOut )
        {
            VKE_ASSERT( phInOut != nullptr, "" );
            Result ret = VKE_OK;
            UStagingBufferHandle hStagingBuffer, hInOut;
            hInOut.handle = *phInOut;
            hStagingBuffer.handle = *phInOut;

            const auto alignedSize = Memory::CalcAlignedSize( Info.Requirements.size, (uint32_t)Info.Requirements.alignment );
            // New allocation is needed if no allocation was created or no space left in a current one
            bool needNewAllocation = (hStagingBuffer.handle == UNDEFINED_U64) || (hStagingBuffer.sizeLeft < alignedSize);
            
            if( needNewAllocation )
            {

            }
            // Calc allocation size
            const uint32_t allocationSize = hStagingBuffer.pageCount * PAGE_SIZE;
            // Calc start offset of the allocation in the buffer
            const uint32_t allocationOffset = hStagingBuffer.pageIndex * PAGE_SIZE;
            // Calc offset in the allocation
            const uint32_t currentOffset = allocationSize - hStagingBuffer.sizeLeft;
            // Calc total offset in the buffer
            const uint32_t totalOffset = allocationOffset + currentOffset;
            
            return ret;
        }

        void CStagingBufferManager::GetBufferInfo( const handle_t& hStagingBuffer, SBufferInfo* pOut )
        {
            UStagingBufferHandle Handle;
            Handle.handle = hStagingBuffer;
            const auto pBuffer = m_vpBuffers[ Handle.bufferIndex ];
            const auto& Chunk = m_vBufferChunks[ Handle.index ];
            pOut->hMemory = pBuffer->m_hMemory;
            pOut->hDDIBuffer = pBuffer->GetDDIObject();
            pOut->offset = pBuffer->CalcOffset( Chunk.bufferRegion, 0 ) + Chunk.offset;
        }

        void CStagingBufferManager::_UpdateBufferInfo(const handle_t& hStagingBuffer, const uint32_t dataWrittenSize)
        {
            UStagingBufferHandle Handle;
            Handle.handle = hStagingBuffer;
            auto& Chunk = m_vBufferChunks[Handle.index];
            Chunk.offset += dataWrittenSize;
        }

        

        void CStagingBufferManager::FreeBuffer( const handle_t& hStagingBuffer )
        {
            if( hStagingBuffer != UNDEFINED_U64 )
            {
                UStagingBufferHandle Handle;
                Handle.handle = hStagingBuffer;
                _SetPageValues< false >( Handle );
            }
        }

        uint8_t CStagingBufferManager::_CreateBuffer( const SBufferRequirementInfo& Info )
        {
            uint8_t ret = UNDEFINED_U8;
            // Align requested size to minimal allocation uint
            uint32_t requestedSize = Memory::CalcAlignedSize( Info.Requirements.size, PAGE_SIZE );
            // Align again to required alignment
            requestedSize = Memory::CalcAlignedSize( requestedSize, (uint32_t)Info.Requirements.alignment );
            // Do not allow to small allocations
            const uint32_t bufferSize = std::max( requestedSize, Config::RenderSystem::Buffer::STAGING_BUFFER_SIZE );
            const uint32_t regionCount = 1;

            const uint16_t pageCount = bufferSize / PAGE_SIZE;
            VKE_ASSERT( pageCount <= MAX_PAGE_COUNT, "Staging buffer requested size is too big. Increase buffer page size." );
            VKE_ASSERT( m_vpBuffers.GetCount() < MAX_BUFFER_COUNT, "" );

            if( m_vpBuffers.GetCount() + 1 < MAX_BUFFER_COUNT )
            {
                SCreateBufferDesc BufferDesc;
                BufferDesc.Create.async = false;
                BufferDesc.Create.stages = Core::ResourceStages::FULL_LOAD;
                BufferDesc.Buffer.memoryUsage = MemoryUsages::STAGING;
                BufferDesc.Buffer.size = 0; // Config::RenderSystem::Buffer::STAGING_BUFFER_SIZE;
                BufferDesc.Buffer.usage = BufferUsages::TRANSFER_SRC;
                BufferDesc.Buffer.vRegions =
                {
                    SBufferRegion( regionCount, bufferSize )
                };
                BufferHandle hBuffer = Info.pCtx->CreateBuffer( BufferDesc );
                auto pBuffer = Info.pCtx->GetBuffer( hBuffer );
                auto idx = m_vpBuffers.PushBack( pBuffer );
                VKE_ASSERT( idx < MAX_BUFFER_COUNT, "" );

                // Create page array to indicate which pages are free to use
                // This is a bool/bit array. 0 means a page is not used.
                const auto tmp = m_vvAllocatedPages.PushBack( AllocatedPagesArray( pageCount, 0 ) );
                // Newly created buffer has one free allocation
                SAllocation FreeAllocation;
                FreeAllocation.Handle.bufferIndex = tmp;
                FreeAllocation.Handle.pageCount = pageCount;
                FreeAllocation.Handle.pageIndex = 0;
                FreeAllocation.Handle.sizeLeft = pageCount * PAGE_SIZE;
                const auto tmp2 = m_vvFreeAllocations.PushBack( AllocationArray{ FreeAllocation } );
                VKE_ASSERT( tmp == tmp2 == idx, "");
                ret = (uint8_t)idx;
            }
            return ret;
        }

        UStagingBufferHandle CStagingBufferManager::_FindFreeChunk( const uint32_t size )
        {
            UStagingBufferHandle hRet;
            // Find minimum requirements
            uint32_t minSize = UNDEFINED_U32;
            const uint32_t requiredPageCount = size / PAGE_SIZE + 1; // round up
            const uint32_t requiredPageBatchCount = PageBatch::GetBitCount() / requiredPageCount + 1;

            for (uint32_t b = 0; b < m_vvFreeAllocations.GetCount(); ++b)
            {
                const auto& vAllocations = m_vvFreeAllocations[b];
                for (uint32_t a = 0; a < vAllocations.GetCount(); ++a)
                {
                    auto& Allocation = vAllocations[a];
                }
            }
            
            return hRet;
        }

        void CStagingBufferManager::FreeBuffer( SBufferData** ppData )
        {
            auto pData = *ppData;
            auto& View = m_vMemViews[ pData->handle ];
            CMemoryPoolView::SAllocateData AllocData;
            AllocData.memory = pData->pBuffer->m_hMemory;
            AllocData.offset = pData->offset;
            AllocData.size = pData->size;
            View.Free( AllocData );
        }

        void CStagingBufferManager::FreeUnusedAllocations( CDeviceContext* )
        {
            uint32_t count = m_vUsedData.GetCount();
            uint32_t currEl = 0;
            for( uint32_t i = 0; i < count; ++i )
            {
                auto& Curr = m_vUsedData[ currEl ];
                if( Curr.pCommandBuffer.IsValid() && Curr.pCommandBuffer->IsExecuted() )
                {
                    auto pData = &Curr;
                    FreeBuffer( &pData );
                    m_vUsedData.RemoveFast( currEl );
                }
                else
                {
                    currEl++;
                }
            }

            DefragmentMemory();
        }

        void CStagingBufferManager::DefragmentMemory()
        {
            for( uint32_t i = 0; i < m_vMemViews.GetCount(); ++i )
            {
                if( m_vMemViews[ i ].CanDefragment() )
                {
                    m_vMemViews[ i ].Defragment();
                }
            }
        }

    } // RenderSystem
} // VKE