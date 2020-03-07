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

            public:

                struct SBufferRequirementInfo
                {
                    CDeviceContext*                 pCtx;
                    SAllocationMemoryRequirements   Requirements;
                };

                struct SBufferData
                {
                    CommandBufferPtr    pCommandBuffer;
                    BufferPtr           pBuffer;
                    uint32_t            size;
                    uint32_t            offset;
                    uint32_t            handle;
                };

                struct SBufferInfo
                {
                    handle_t    hMemory;
                    DDIBuffer   hDDIBuffer;
                    uint32_t    sizeLeft;
                    uint32_t    offset;
                };

                

                using BufferDataArray = Utils::TCDynamicArray< SBufferData >;

            public:

                Result  Create( const SStagingBufferManagerDesc& Desc );
                void    Destroy(CDeviceContext* pCtx);

                //Result  GetBuffer( const SBufferRequirementInfo& Info, SBufferData** ppData );
                Result  GetBuffer( const SBufferRequirementInfo& Info, handle_t* phBufferInOut, SBufferInfo* pOut );

                void    GetBufferInfo( const handle_t& hStagingBuffer, SBufferInfo* pOut );
                void    FreeBuffer( SBufferData** ppInOut );
                void    FreeBuffer( const handle_t& hStagingBuffer );

                void    FreeUnusedAllocations( CDeviceContext* pCtx );

                void    DefragmentMemory();

            protected:

                uint8_t        _CreateBuffer( const SBufferRequirementInfo& Info );
                UStagingBufferHandle    _GetNextChunk( const SBufferRequirementInfo& Info );
                UStagingBufferHandle    _FindFreeChunk( const uint32_t size );
                void                    _UpdateBufferInfo(const handle_t& hStagingBuffer, const uint32_t dataWrittenSize);
                template<bool IsSet>
                void            _SetPageValues( UStagingBufferHandle hBuffer );
                uint32_t        _CalcOffsetInAllocation(UStagingBufferHandle hBuffer);

            protected:

                SStagingBufferManagerDesc   m_Desc;
                Threads::SyncObject         m_MemViewSyncObj;
                MemViewArray                m_vMemViews;


                BufferArray                 m_vpBuffers;
                BufferAllocatedPagesArray   m_vvAllocatedPages;
                BufferAllocationArray       m_vvFreeAllocations;
        };

        template<bool IsSet>
        void CStagingBufferManager::_SetPageValues( UStagingBufferHandle hBuffer )
        {
            AllocatedPagesArray& vAllocatedPages = m_vvAllocatedPages[hBuffer.bufferIndex];
            static const auto PageCountInBatch = std::numeric_limits< AllocatedPagesArray::DataType >::digits;
            // Iterate for batches to indicate free

            // Probably first allocated page is not aligned with a batch size
            // e.g. if batch size is uint16 == 16 batch size then aligned page starts for every 16
            // if not it is needed to iterate up to 16 bits
            const uint16_t firstPageBitCount = hBuffer.pageIndex % PageCountInBatch;
            // then if allocation took more than one batch size (e.g. 16)
            // we can iterate through all pages
            const uint16_t pageBatchCount = (hBuffer.pageCount) / PageCountInBatch;
            // At the end allocation may not be aligned to batch size
            // so iterate through single bits in last batch
            const uint16_t pageBatchCountReminder = hBuffer.pageCount % PageCountInBatch;
            // Assume page batch is 4 and allocated pages looks like:
            // |0011|1111|1110|
            //   #0   #1   #2
            // pageIndex = 2, pageCount = 9
            // 1. iterate for #0 batch to zero bits 0,1
            // 2. iterate for #1 batch to zero whole batch at once
            // 3. iterate for #3 batch to zero bits 1,2,3

            // Select page batch
            // Calc num of bits in a whole array
            const uint16_t batchIndex = hBuffer.pageIndex / PageCountInBatch;
            const uint16_t reminder = hBuffer.pageIndex % PageCountInBatch;
            auto& FirstPage = vAllocatedPages[batchIndex];

            const AllocatedPagesArray::DataType value = IsSet ? WHOLE_PAGE_BATCH_RESERVED : 0;

            // Set last from the last to first bit to emulate regular array elements
            const uint8_t lastBit = PageCountInBatch - reminder;
            for (uint16_t i = lastBit; i > 0; --i)
            {
                FirstPage.SetBit< IsSet >(i);
            }

            for (uint16_t i = 0; i < pageBatchCount; ++i)
            {
                vAllocatedPages[i] = value;
            }
            // For the rest of single bits iterate through bits
            // From the oldest one to emulate a regular array
            for (uint16_t i = pageBatchCountReminder; i > 0; --i)
            {
                // Clear single bits
                FirstPage.SetBit< IsSet >(i);
            }
        }
    } // RenderSystem
} // VKE