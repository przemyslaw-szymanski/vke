#pragma once
#include "Core/Memory/CMemoryPoolManager.h"
#include "Core/Utils/TCDynamicArray.h"
#include "Core/VKEConfig.h"
#include "RenderSystem/Resources/CBuffer.h"
namespace VKE
{
    namespace RenderSystem
    {
        class CDeviceContext;
        struct SStagingBufferManagerDesc
        {
            uint32_t bufferSize = VKE_MEGABYTES( 1 );
        };
        union UStagingBufferHandle
        {
            struct
            {
                handle_t bufferIndex : 8; // more than one staging buffer may be
                                          // allocated
                handle_t pageIndex : 12;  // buffer is split into pages
                handle_t
                    pageCount : 12; // allocation size is aligned to page size
                handle_t sizeLeft : 32;
            };
            handle_t handle = UNDEFINED_U64;
        };
#define VKE_CHUNKED_STAGING_BUFFER 1
        class CStagingBufferManager
        {
            friend class CBufferManager;
            using CommandBufferArray = Utils::TCDynamicArray<CCommandBuffer*>;
            static const uint8_t MAX_BATCH_COUNT =
                std::numeric_limits<uint8_t>::max();
            static const uint8_t MAX_BUFFER_COUNT =
                std::numeric_limits<uint8_t>::max();
            static const uint16_t MAX_CHUNK_COUNT =
                std::numeric_limits<uint16_t>::max();
            static const uint32_t MAX_CHUNK_SIZE =
                std::numeric_limits<uint32_t>::max();
            static const uint32_t PAGE_SIZE =
                Config::RenderSystem::Buffer::STAGING_BUFFER_PAGE_SIZE;
            static const uint16_t MAX_PAGE_COUNT =
                0xFFF; // max number of pages in one buffer, 12 bits
            static const uint16_t WHOLE_PAGE_BATCH_RESERVED = 0xFFFF;
            struct SBufferChunk
            {
                uint32_t offset = 0; // offset in this chunk
                uint8_t bufferRegion = 0;
            };
            // Memory page is a buffer chunk with fixed size
            struct SMemoryPage
            {
            };
            struct SAllocation
            {
                UStagingBufferHandle Handle;
                uint32_t offset = 0;
            };
            using BufferChunkArray = Utils::TCDynamicArray<SBufferChunk>;
            using BufferChunkHandleArray =
                Utils::TCDynamicArray<UStagingBufferHandle>;
            using BufferArray = Utils::TCDynamicArray<BufferRefPtr>;
            using MemViewArray = Utils::TCDynamicArray<CMemoryPoolView>;
            using ChunkBatchArray =
                Utils::TCDynamicArray<BufferChunkHandleArray>;
            template<typename T> using Array = Utils::TCDynamicArray<T>;
            using PageBatchType =
                uint16_t; // for performance pages are stored in batches of 16 -
                          // 1 bit for a page
            using PageBatch = Utils::TCBitset<PageBatchType>;
            using AllocatedPagesArray = Utils::TCDynamicArray<PageBatch, 1>;
            using BufferAllocatedPagesArray =
                Utils::TCDynamicArray<AllocatedPagesArray>;
            using AllocationArray = Utils::TCDynamicArray<SAllocation>;
            using BufferAllocationArray =
                Utils::TCDynamicArray<AllocationArray>;
            using AllocatedPagesArray2 = std::vector<bool>;
            using BufferAllocatedPagesArray2 =
                Utils::TCDynamicArray<AllocatedPagesArray2, 1>;
          public:
            struct PageStates
            {
                enum STATE
                {
                    FREE = 0,
                    ALLOCATED = 1,
                    _MAX_COUNT
                };
            };
            using PAGE_STATE = PageStates::STATE;
            struct SBufferRequirementInfo
            {
                CDeviceContext* pCtx;
                SAllocationMemoryRequirementInfo Requirements;
            };
            struct SMemoryRequirementInfo
            {
                SAllocationMemoryRequirementInfo Requirements;
            };
            struct SBufferData
            {
                CommandBufferPtr pCommandBuffer;
                BufferPtr pBuffer;
                uint32_t size;
                uint32_t offset;
                uint32_t handle;
            };
            using BufferDataArray = Utils::TCDynamicArray<SBufferData>;
            using UintArray = Utils::TCDynamicArray<uint32_t>;
            using IntArray = Utils::TCDynamicArray<int32_t>;
          public:
            Result Create( const SStagingBufferManagerDesc& Desc );
            void Destroy( CDeviceContext* pCtx );
            // Result  GetBuffer( const SBufferRequirementInfo& Info,
            // SBufferData** ppData );
            Result GetBuffer( const SBufferRequirementInfo& Info,
                              const uint32_t& flags, handle_t* phBufferInOut,
                              SStagingBufferInfo* pOut );
            void GetBufferInfo( const handle_t& hStagingBuffer,
                                SStagingBufferInfo* pOut );
            void FreeBuffer( const handle_t& hStagingBuffer );
            void FreeUnusedAllocations( CDeviceContext* pCtx );
            void DefragmentMemory();
            void LogStagingBuffer( handle_t hStagingBuffer, cstr_t pMsg );
          protected:
            uint8_t _CreateBuffer( const SBufferRequirementInfo& Info );
            UStagingBufferHandle
            _GetNextChunk( const SBufferRequirementInfo& Info );
            UStagingBufferHandle _FindFreeChunk( const uint32_t size );
            UStagingBufferHandle _FindFreeChunk( const uint8_t bufferIdx,
                                                 const uint32_t size );
            UStagingBufferHandle _FindFreePages( const uint32_t size );
            UStagingBufferHandle _FindFreePages( const uint8_t bufferIdx,
                                                 const uint32_t size );
            void _UpdateBufferInfo( const handle_t& hStagingBuffer,
                                    const uint32_t dataWrittenSize );
            template<PAGE_STATE State>
            void _SetPageValues( UStagingBufferHandle hBuffer );
            uint32_t _CalcOffsetInAllocation( UStagingBufferHandle hBuffer );
            uint16_t _CalcPageCount( const uint32_t size ) const;
          protected:
            SStagingBufferManagerDesc m_Desc;
            Threads::SyncObject m_MemViewSyncObj;
            MemViewArray m_vMemViews;
            BufferArray m_vpBuffers;
            // BufferAllocatedPagesArray   m_vvAllocatedPages;
            BufferAllocatedPagesArray2 m_vvAllocatedPages;
            BufferAllocationArray m_vvFreeAllocations;
            IntArray m_vvTotalFreeMem;
            uint32_t m_totalAllocatedMemory = 0;
        };
        template<CStagingBufferManager::PAGE_STATE State>
        void
        CStagingBufferManager::_SetPageValues( UStagingBufferHandle hBuffer )
        {
            auto& vAllocatedPages = m_vvAllocatedPages[ hBuffer.bufferIndex ];
            static const uint8_t PageCountInBatch =
                std::numeric_limits<PageBatch::DataType>::digits;
            static_assert( PageCountInBatch > 0, "Bad data type" );

            const auto count = hBuffer.pageIndex + hBuffer.pageCount;
            for( uint16_t i = hBuffer.pageIndex; i < count; ++i )
            {
                vAllocatedPages[ i ] = State;
            }
        }
    } // namespace RenderSystem
} // namespace VKE