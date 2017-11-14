#pragma once

#include "Common.h"
#include "Core/Utils/TCDynamicArray.h"
#include "Core/Threads/Common.h"

namespace VKE
{
    namespace Memory
    {
        class VKE_API CMemoryPoolManager
        {
            public:

                struct SMemoryChunk
                {
                    uint64_t    memory;
                    uint32_t    offset;
                    uint32_t    size;
                };

            protected:

                using MemoryChunkVec = Utils::TCDynamicArray< SMemoryChunk >;
                using MemoryChunkVecMap = vke_hash_map< uint32_t, MemoryChunkVec >;
                using MemoryChunkMap = vke_hash_map< uint64_t, SMemoryChunk* >;

                struct SMemoryBuffer
                {
                    uint64_t    memory;
                    uint32_t    size;
                };
                using MemoryBufferVec = Utils::TCDynamicArray< SMemoryBuffer >;
                using MemoryBufferVecMap = vke_hash_map< uint32_t, MemoryBufferVec >;
                using UintVec = Utils::TCDynamicArray< uint32_t >;

                struct SFreeMemoryData
                {
                    MemoryChunkVec  vChunks;
                    UintVec         vChunkSizes;
                };

                struct SMemoryBufferData
                {
                    SFreeMemoryData     FreeMemory;
                    MemoryBufferVec     vBuffers;
                    uint32_t            index;
                };
                using MemoryBufferDataVec = Utils::TCDynamicArray< SMemoryBufferData, 16 >;
                using MemoryBufferDataVecVec = Utils::TCDynamicArray< MemoryBufferDataVec, 16 >;     

                struct SFindMemoryData
                {
                    SMemoryBufferData*  pMemoryData;
                    uint32_t            freeChunkIdx;
                };

            public:

                struct SAllocateInfo
                {
                    uint32_t    size;
                    uint32_t    alignment;
                    uint32_t    index;
                    uint16_t    type;
                };

                struct SAllocatedData
                {
                    SMemoryChunk        Memory;
                    uint32_t            type;
                    uint32_t            index;
                };
                using AllocatedMemoryMap = vke_hash_map< uint64_t, SAllocatedData >;
                using AllocatedMemoryVec = Utils::TCDynamicArray< SAllocatedData, 1 >;

                struct SPoolAllocateInfo
                {
                    uint32_t type = 0;
                    uint32_t index = 0;
                    uint32_t size = 0;
                    uint32_t alignment = 0;
                    uint64_t memory = 0;
                };

                struct STemporaryData
                {
                    MemoryChunkVec      vChunks;
                    Threads::SyncObject SyncObj;
                };
                using TmpDataVec = Utils::TCDynamicArray< STemporaryData >;

                struct SCache
                {
                    uint32_t            lastUsedIndex;
                    SMemoryBufferData*  pLastUsedMemoryData = nullptr;
                };
                using CacheVec = Utils::TCDynamicArray< SCache >;

            public:

                CMemoryPoolManager();
                virtual ~CMemoryPoolManager();

                virtual Result      Create(const SMemoryPoolManagerDesc& Desc);
                virtual void        Destroy();

                virtual uint64_t    Allocate(const SAllocateInfo& Info, SAllocatedData* pOut);
                virtual Result      Free(uint64_t memory);
                virtual Result      AllocatePool(const SPoolAllocateInfo& Info);

            protected:

                uint32_t    _FindFreeMemory(const SAllocateInfo& Info, SFindMemoryData* pOut);
                Result      _AddFreeChunk(const SMemoryChunk& Chunk, SFreeMemoryData* pOut);
                void        _MergeFreeChunks(SFreeMemoryData* pOut);

            protected:

                MemoryBufferDataVecVec      m_vvMemoryBuffers;
                CacheVec                    m_vCaches;
                AllocatedMemoryMap          m_mAllocatedData;
                AllocatedMemoryVec          m_vAllocatedData;
                TmpDataVec                  m_vTmpData;
        };
    } // Memory
} // VKE