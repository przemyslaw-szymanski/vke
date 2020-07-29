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

        uint16_t CStagingBufferManager::_CalcPageCount(const uint32_t size) const
        {
            const uint16_t ret = (uint16_t)( size / PAGE_SIZE ) + 1;
            VKE_ASSERT(ret < MAX_PAGE_COUNT, "Max page count limit reached.");
            return ret;
        }

        void LogPageValues(std::vector<bool>& v)
        {
            char tmp[2048];
            for (uint32_t i = 0; i < v.size(); ++i)
            {
                tmp[i] = v[i] ? '1' : '0';
            }
            tmp[v.size()] = 0;
            VKE_LOG(tmp);
        }

        Result CStagingBufferManager::GetBuffer( const SBufferRequirementInfo& Info, handle_t* phInOut,
                                                 SStagingBufferInfo* pOut )
        {
            VKE_ASSERT( phInOut != nullptr, "" );
            Result ret = VKE_OK;
            UStagingBufferHandle hAllocation, hInOut;
            hInOut.handle = *phInOut;
            hAllocation.handle = *phInOut;

            const auto alignedSize = Memory::CalcAlignedSize( Info.Requirements.size, (uint32_t)Info.Requirements.alignment );
            // New allocation is needed if no allocation was created or no space left in a current one
            bool needNewAllocation = (hAllocation.handle == UNDEFINED_U64) || (hAllocation.sizeLeft < alignedSize);

            if( needNewAllocation )
            {
                //hAllocation = _FindFreeChunk( alignedSize );
                hAllocation = _FindFreePages(alignedSize);
                // No free allocations left
                if (hAllocation.handle == UNDEFINED_U64)
                {
                    const uint8_t bufferIdx = _CreateBuffer( Info );
                    //hAllocation = _FindFreeChunk( bufferIdx, alignedSize );
                    hAllocation = _FindFreePages(bufferIdx, alignedSize);
                }

                _SetPageValues< true >(hAllocation);

            }

            VKE_ASSERT(hAllocation.handle != UNDEFINED_U64, "");
            VKE_ASSERT(hAllocation.sizeLeft >= alignedSize, "");
            // Calc allocation size
            const uint32_t allocationSize = (uint16_t)hAllocation.pageCount * PAGE_SIZE;
            // Calc start offset of the allocation in the buffer
            const uint32_t allocationOffset = (uint16_t)hAllocation.pageIndex * PAGE_SIZE;
            // Calc local offset in the allocation
            VKE_ASSERT(  allocationSize >= (uint32_t)hAllocation.sizeLeft, "" );
            const uint32_t currentOffset = allocationSize - (uint32_t)hAllocation.sizeLeft;
            // Calc total offset in the buffer
            const uint32_t totalOffset = allocationOffset + currentOffset;
            hAllocation.sizeLeft -= alignedSize;

            const auto pBuffer = m_vpBuffers[hAllocation.bufferIndex];
            pOut->hDDIBuffer = pBuffer->GetDDIObject();

            pOut->hMemory = pBuffer->GetMemory();
            pOut->offset = totalOffset;
            pOut->sizeLeft = hAllocation.sizeLeft;

            *phInOut = hAllocation.handle;

            m_vvTotalFreeMem[hAllocation.bufferIndex] += alignedSize;
            /*VKE_LOG("Alloc staging: buffIdx: " << hAllocation.bufferIndex <<
                " size: " << hAllocation.pageCount * PAGE_SIZE << " total size: " << m_vvTotalFreeMem[hAllocation.bufferIndex]);
            LogPageValues(m_vvAllocatedPages[hAllocation.bufferIndex]);*/

            return ret;
        }

        void CStagingBufferManager::GetBufferInfo( const handle_t& hStagingBuffer, SStagingBufferInfo* pOut )
        {

        }

        void CStagingBufferManager::_UpdateBufferInfo(const handle_t& hStagingBuffer, const uint32_t dataWrittenSize)
        {
            UStagingBufferHandle Handle;
            Handle.handle = hStagingBuffer;
            Handle.sizeLeft -= dataWrittenSize;
        }

        void CStagingBufferManager::FreeBuffer( const handle_t& hStagingBuffer )
        {
            if( hStagingBuffer != UNDEFINED_U64 )
            {
                UStagingBufferHandle Handle;
                Handle.handle = hStagingBuffer;
                _SetPageValues< false >( Handle );
                auto& vFreeAllocations = m_vvFreeAllocations[ Handle.bufferIndex ];
                SAllocation Allocation;
                Allocation.Handle = Handle;
                Allocation.offset = (uint32_t)Handle.pageIndex * PAGE_SIZE;
                vFreeAllocations.PushBack( Allocation );

                m_vvTotalFreeMem[Handle.bufferIndex] -= (uint32_t)Handle.pageCount * PAGE_SIZE;
                /*VKE_LOG("Free staging memory: buffIdx: " << Handle.bufferIndex <<
                    " size: " << Handle.pageCount * PAGE_SIZE << " total size: " << m_vvTotalFreeMem[Handle.bufferIndex]);
                LogPageValues(m_vvAllocatedPages[Handle.bufferIndex]);*/
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

            const uint32_t pageCount = bufferSize / PAGE_SIZE;
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
                const uint32_t idx = m_vpBuffers.PushBack( pBuffer );
                VKE_ASSERT( idx < MAX_BUFFER_COUNT, "" );

                // Create page array to indicate which pages are free to use
                // This is a bool/bit array. 0 means a page is not used.
                //const uint32_t tmp = m_vvAllocatedPages.PushBack( AllocatedPagesArray( pageCount, PageBatch( 0 ) ) );
                AllocatedPagesArray2 vPageArray( pageCount, 0 );
                const auto tmp = m_vvAllocatedPages.PushBack( vPageArray );

                // Newly created buffer has one free allocation
                SAllocation FreeAllocation;
                FreeAllocation.Handle.bufferIndex = tmp;
                FreeAllocation.Handle.pageCount = pageCount;
                FreeAllocation.Handle.pageIndex = 0;
                FreeAllocation.Handle.sizeLeft = pageCount * PAGE_SIZE;
                const uint32_t tmp2 = m_vvFreeAllocations.PushBack( AllocationArray{ FreeAllocation } );
                m_vvTotalFreeMem.PushBack(0);
                VKE_ASSERT( tmp == tmp2, "" );
                VKE_ASSERT( tmp == idx, "" );
                ret = (uint8_t)idx;
            }
            return ret;
        }

        UStagingBufferHandle CStagingBufferManager::_FindFreeChunk( const uint32_t size )
        {
            UStagingBufferHandle hRet;
            // Find minimum requirements
            const uint32_t requiredPageCount = size / PAGE_SIZE + 1; // round up
            const uint32_t requiredPageBatchCount = PageBatch::GetBitCount() / requiredPageCount + 1;

            for (uint8_t b = 0; b < m_vvFreeAllocations.GetCount(); ++b)
            {
                hRet = _FindFreeChunk( b, size );
                if (hRet.handle != UNDEFINED_U64)
                {
                    break;
                }
            }
            return hRet;
        }

        UStagingBufferHandle CStagingBufferManager::_FindFreeChunk( const uint8_t bufferIdx, const uint32_t size )
        {
            UStagingBufferHandle hRet;
            const uint32_t requiredPageCount = size / PAGE_SIZE + 1; // round up
            const auto& vAllocations = m_vvFreeAllocations[bufferIdx];
            for (uint32_t a = 0; a < vAllocations.GetCount(); ++a)
            {
                auto& Allocation = vAllocations[a];
                const int32_t pageCountLeft = (int32_t)Allocation.Handle.pageCount - requiredPageCount;
                // This allocation contains enough pages
                if (pageCountLeft >= 0)
                {
                    hRet = Allocation.Handle;
                    hRet.pageCount = requiredPageCount;
                    hRet.sizeLeft = requiredPageCount * PAGE_SIZE;
                    // Shrink it
                    Allocation.Handle.pageCount = pageCountLeft;
                    Allocation.Handle.pageIndex += requiredPageCount;
                    Allocation.Handle.sizeLeft = pageCountLeft * PAGE_SIZE;
                    /// TODO: remove empty allocations: pageCountLeft == 0
                    break;
                }
            }
            return hRet;
        }

        UStagingBufferHandle CStagingBufferManager::_FindFreePages(const uint32_t size)
        {
            const uint16_t pageCount = (uint16_t)( size / PAGE_SIZE + 1 );
            UStagingBufferHandle hRet;
            for (uint8_t i = 0; i < m_vvAllocatedPages.GetCount(); ++i)
            {
                hRet = _FindFreePages(i, size);
                if (hRet.handle != UNDEFINED_U64)
                {
                    break;
                }
            }
            return hRet;
        }

        UStagingBufferHandle CStagingBufferManager::_FindFreePages(const uint8_t bufferIdx, const uint32_t size)
        {
            auto& vAllocatedPages = m_vvAllocatedPages[bufferIdx];
            const uint8_t pageCount = (uint8_t)(size / PAGE_SIZE + 1);
            const uint8_t lastPageIndex = pageCount - 1;
            const uint32_t count = (uint32_t)vAllocatedPages.size() - pageCount;
            UStagingBufferHandle hRet;
            if (pageCount == 1)
            {
                for (uint32_t i = 0; i < vAllocatedPages.size(); ++i)
                {
                    if (vAllocatedPages[i] == false)
                    {
                        hRet.bufferIndex = bufferIdx;
                        hRet.pageCount = pageCount;
                        hRet.pageIndex = i;
                        hRet.sizeLeft = pageCount * PAGE_SIZE;
                        break;
                    }
                }
            }
            else
            {
                for (uint32_t i = 0; i < count; ++i)
                {
                    // Check if current and last page are free
                    const uint32_t lastIndexToCheck = i + lastPageIndex;
                    const bool isLastIndexAllocated = vAllocatedPages[lastIndexToCheck];
                    const bool isFirstIndexAllocated = vAllocatedPages[i];

                    if (isFirstIndexAllocated == false && isLastIndexAllocated == false)
                    {
                        // Check range (i, i+lastPageIndex)
                        uint8_t allocatedCount = 0;
                        for (uint32_t p = i + 1; p < lastIndexToCheck - 1; ++p)
                        {
                            allocatedCount += vAllocatedPages[p];
                        }
                        // No allocated pages within this range
                        if (allocatedCount == 0)
                        {
                            hRet.bufferIndex = bufferIdx;
                            hRet.pageCount = pageCount;
                            hRet.pageIndex = i;
                            hRet.sizeLeft = pageCount * PAGE_SIZE;
                            break;
                        }
                    }
                    else if (isLastIndexAllocated == true)
                    {
                        // if last index is allocated skip this whole range
                        i += pageCount;
                    }
                }
            }
            return hRet;
        }

        void CStagingBufferManager::FreeUnusedAllocations( CDeviceContext* )
        {

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