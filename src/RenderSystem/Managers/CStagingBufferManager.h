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
            static const uint32_t   PAGE_SIZE = VKE_KILOBYTES( 64 );

            struct SBufferChunk
            {
                uint32_t            offset = 0; // offset in this chunk
                uint8_t             bufferRegion = 0;
            };

            // Memory page is a buffer chunk with fixed size
            struct SMemoryPage
            {

            };

            using BufferChunkArray          = Utils::TCDynamicArray< SBufferChunk >;
            using BufferChunkHandleArray    = Utils::TCDynamicArray< UStagingBufferHandle >;
            using BufferArray               = Utils::TCDynamicArray< BufferRefPtr >;
            using MemViewArray              = Utils::TCDynamicArray< CMemoryPoolView >;
            using ChunkBatchArray           = Utils::TCDynamicArray< BufferChunkHandleArray >;
            template<typename T>
            using Array                     = Utils::TCDynamicArray< T >;

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

                Result  GetBuffer( const SBufferRequirementInfo& Info, SBufferData** ppData );
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

            protected:

                SStagingBufferManagerDesc   m_Desc;
                Threads::SyncObject         m_MemViewSyncObj;
                MemViewArray                m_vMemViews;

                BufferChunkArray            m_vBufferChunks;
                BufferChunkHandleArray      m_vhBufferChunks;
                Threads::SyncObject         m_FreeChunkSyncObj;
                BufferChunkHandleArray      m_vhFreeChunks;
                Threads::SyncObject         m_BatchSyncObj;
                ChunkBatchArray             m_vChunkBatches;

                BufferArray                 m_vpBuffers;
                BufferDataArray             m_vUsedData;
        };
    }
}