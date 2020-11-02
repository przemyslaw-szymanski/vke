#pragma once

#include "Core/Utils/TCDynamicArray.h"
#include "RenderSystem/Resources/CBuffer.h"
#include "Core/VKEConfig.h"
#include "Core/Memory/CMemoryPoolManager.h"

namespace VKE
{
    namespace RenderSystem
    {
        class CDeviceContext;

        struct SStagingBufferManagerDesc
        {
            uint32_t    bufferSize = VKE_MEGABYTES( 1 );
        };

        union UStagingBufferHandle
        {
            struct
            {
                handle_t    bufferIndex : 8; // more than one staging buffer may be allocated
                handle_t    pageIndex   : 12; // buffer is split into pages
                handle_t    pageCount   : 12; // allocation size is aligned to page size
                handle_t    sizeLeft    : 32;
            };
            handle_t    handle = UNDEFINED_U64;
        };

#define VKE_CHUNKED_STAGING_BUFFER 1

        class CStagingBufferManager
        {
            friend class CBufferManager;

            using CommandBufferArray = Utils::TCDynamicArray< CCommandBuffer* >;

            static const uint8_t    MAX_BATCH_COUNT = std::numeric_limits<uint8_t>::max();
            static const uint8_t    MAX_BUFFER_COUNT = std::numeric_limits<uint8_t>::max();
            static const uint16_t   MAX_CHUNK_COUNT = std::numeric_limits<uint16_t>::max();
            static const uint32_t   MAX_CHUNK_SIZE = std::numeric_limits<uint32_t>::max();
            static const uint32_t   PAGE_SIZE = Config::RenderSystem::Buffer::STAGING_BUFFER_PAGE_SIZE;
            static const uint16_t   MAX_PAGE_COUNT = 0xFFF; // max number of pages in one buffer, 12 bits
            static const uint16_t   WHOLE_PAGE_BATCH_RESERVED = 0xFFFF;

            struct SBufferChunk
            {
                uint32_t            offset = 0; // offset in this chunk
                uint8_t             bufferRegion = 0;
            };

            // Memory page is a buffer chunk with fixed size
            struct SMemoryPage
            {

            };

            struct SAllocation
            {
                UStagingBufferHandle    Handle;
                uint32_t                offset = 0;
            };

            using BufferChunkArray          = Utils::TCDynamicArray< SBufferChunk >;
            using BufferChunkHandleArray    = Utils::TCDynamicArray< UStagingBufferHandle >;
            using BufferArray               = Utils::TCDynamicArray< BufferRefPtr >;
            using MemViewArray              = Utils::TCDynamicArray< CMemoryPoolView >;
            using ChunkBatchArray           = Utils::TCDynamicArray< BufferChunkHandleArray >;
            template<typename T>
            using Array                     = Utils::TCDynamicArray< T >;
            using PageBatchType             = uint16_t; // for performance pages are stored in batches of 16 - 1 bit for a page
            using PageBatch                 = Utils::TCBitset< PageBatchType >;
            using AllocatedPagesArray       = Utils::TCDynamicArray< PageBatch, 1 >;
            using BufferAllocatedPagesArray = Utils::TCDynamicArray< AllocatedPagesArray >;
            using AllocationArray           = Utils::TCDynamicArray< SAllocation >;
            using BufferAllocationArray     = Utils::TCDynamicArray< AllocationArray >;
            using AllocatedPagesArray2      = std::vector<bool>;
            using BufferAllocatedPagesArray2 = Utils::TCDynamicArray< AllocatedPagesArray2, 1 >;

            public:

                struct SBufferRequirementInfo
                {
                    CDeviceContext*                 pCtx;
                    SAllocationMemoryRequirementInfo   Requirements;
                };

                struct SMemoryRequirementInfo
                {
                    SAllocationMemoryRequirementInfo   Requirements;
                };

                struct SBufferData
                {
                    CommandBufferPtr    pCommandBuffer;
                    BufferPtr           pBuffer;
                    uint32_t            size;
                    uint32_t            offset;
                    uint32_t            handle;
                };

                using BufferDataArray = Utils::TCDynamicArray< SBufferData >;
                using UintArray = Utils::TCDynamicArray< uint32_t >;
                using IntArray = Utils::TCDynamicArray< int32_t >;

            public:

                Result  Create( const SStagingBufferManagerDesc& Desc );
                void    Destroy(CDeviceContext* pCtx);

                //Result  GetBuffer( const SBufferRequirementInfo& Info, SBufferData** ppData );
                Result  GetBuffer( const SBufferRequirementInfo& Info, const uint32_t& flags, handle_t* phBufferInOut, SStagingBufferInfo* pOut );

                void    GetBufferInfo( const handle_t& hStagingBuffer, SStagingBufferInfo* pOut );
                void    FreeBuffer( const handle_t& hStagingBuffer );

                void    FreeUnusedAllocations( CDeviceContext* pCtx );

                void    DefragmentMemory();

            protected:

                uint8_t        _CreateBuffer( const SBufferRequirementInfo& Info );
                UStagingBufferHandle    _GetNextChunk( const SBufferRequirementInfo& Info );
                UStagingBufferHandle    _FindFreeChunk( const uint32_t size );
                UStagingBufferHandle    _FindFreeChunk(const uint8_t bufferIdx, const uint32_t size);
                UStagingBufferHandle    _FindFreePages(const uint32_t size);
                UStagingBufferHandle    _FindFreePages(const uint8_t bufferIdx, const uint32_t size);
                void                    _UpdateBufferInfo(const handle_t& hStagingBuffer, const uint32_t dataWrittenSize);
                template<bool IsSet>
                void            _SetPageValues( UStagingBufferHandle hBuffer );
                uint32_t        _CalcOffsetInAllocation(UStagingBufferHandle hBuffer);
                uint16_t        _CalcPageCount(const uint32_t size) const;

            protected:

                SStagingBufferManagerDesc   m_Desc;
                Threads::SyncObject         m_MemViewSyncObj;
                MemViewArray                m_vMemViews;


                BufferArray                 m_vpBuffers;
                //BufferAllocatedPagesArray   m_vvAllocatedPages;
                BufferAllocatedPagesArray2  m_vvAllocatedPages;
                BufferAllocationArray       m_vvFreeAllocations;
                IntArray                    m_vvTotalFreeMem;
                uint32_t                    m_totalAllocatedMemory = 0;
        };

        template<bool IsSet>
        void CStagingBufferManager::_SetPageValues( UStagingBufferHandle hBuffer )
        {
            auto& vAllocatedPages = m_vvAllocatedPages[hBuffer.bufferIndex];
            static const uint8_t PageCountInBatch = std::numeric_limits< PageBatch::DataType >::digits;
            static_assert( PageCountInBatch > 0, "Bad data type" );
            //const uint8_t FirstBitIndex = PageCountInBatch - 1;
            //// Iterate for batches to indicate free

            //// Probably first allocated page is not aligned with a batch size
            //// e.g. if batch size is uint16 == 16 batch size then aligned page starts for every 16
            //// if not it is needed to iterate up to 16 bits

            //// then if allocation took more than one batch size (e.g. 16)
            //// we can iterate through all pages

            //// At the end allocation may not be aligned to batch size
            //// so iterate through single bits in last batch

            //// Assume page batch is 4 and allocated pages looks like:
            //// |0011|1111|1110|
            ////   #0   #1   #2
            //// pageIndex = 2, pageCount = 9
            //// 1. iterate for #0 batch to zero bits 0,1
            //// 2. iterate for #1 batch to zero whole batch at once
            //// 3. iterate for #3 batch to zero bits 1,2,3

            //// Select page batch
            //uint16_t batchIndex = (uint16_t)hBuffer.pageIndex / PageCountInBatch;
            //auto& FirstPage = vAllocatedPages[batchIndex];
            //// Calc how many pages are stored in a first page batch
            //// pageIndex - batchIndex * number of pages in one batch

            //// Calc first bit in first page batch
            //// Note bits are set in the opposite order, right to left so it is needed to inverse modulo
            //uint8_t startPageIndexInBatch = FirstBitIndex - ( hBuffer.pageIndex % PageCountInBatch );
            //// Check if requested number of pages fits into first page batch
            //int8_t endPageIndexInBatch = startPageIndexInBatch - (uint8_t)hBuffer.pageCount;

            //if (endPageIndexInBatch >= 0)
            //{
            //    // page count fits into first page batch
            //    for( uint8_t i = startPageIndexInBatch; i > endPageIndexInBatch; --i )
            //    {
            //        FirstPage.SetBit< IsSet >( i );
            //    }
            //}
            //else
            //{
            //    uint16_t totalPageCount = FirstBitIndex;

            //    for (uint8_t p = 0; p < hBuffer.pageCount; ++p)
            //    {
            //        uint16_t pageIndex = (uint16_t)hBuffer.pageIndex + p;
            //        batchIndex = (pageIndex) / PageCountInBatch;
            //        PageBatch* pCurrBatch = &vAllocatedPages[batchIndex];

            //        totalPageCount = FirstBitIndex + (batchIndex * PageCountInBatch);
            //        uint16_t bitIdx = (totalPageCount) - (uint16_t)pageIndex;
            //        pCurrBatch->SetBit< IsSet >( (uint8_t)bitIdx );
            //    }

            //}
            const auto count = hBuffer.pageIndex + hBuffer.pageCount;
            for (uint16_t i = hBuffer.pageIndex; i < count; ++i)
            {
                vAllocatedPages[i] = IsSet;
            }
        }
    } // RenderSystem
} // VKE