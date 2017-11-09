#pragma once

#include "Core/VKECommon.h"
#include "Core/Utils/TCDynamicArray.h"

namespace VKE
{
    namespace Memory
    {
        class CMemoryPoolManager
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
                using MemoryBufferDataVec = Utils::TCDynamicArray< SMemoryBufferData >;
                using MemoryBufferDataVecMap = vke_hash_map< uint32_t, SMemoryBufferData >;
                using MemoryBufferDataVecMapVec = Utils::TCDynamicArray< MemoryBufferDataVecMap >;       

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
                    const SMemoryChunk* pMemory;
                    uint32_t            type;
                    uint32_t            index;
                };
                using AllocatedMemoryMap = vke_hash_map< uint64_t, SAllocatedData >;

                struct SPoolAllocateInfo
                {
                    uint32_t type = 0;
                    uint32_t index = 0;
                    uint32_t size = 0;
                    uint32_t alignment = 0;
                    uint64_t memory = 0;
                };

            public:

                Result Allocate(const SAllocateInfo& Info, SAllocatedData* pOut);
                Result Free(uint64_t memory);
                Result AllocatePool(const SPoolAllocateInfo& Info);

            protected:

                Result  _FindFreeMemory(const SAllocateInfo& Info, SMemoryChunk** ppOut);
                Result  _AddFreeChunk(const SMemoryChunk& Chunk, SFreeMemoryData* pOut);
                void    _MergeFreeChunks(SFreeMemoryData* pOut);

            protected:

                MemoryBufferDataVecMapVec   m_vmvMemoryBuffers;
                AllocatedMemoryMap          m_mAllocatedData;
        };
    } // Memory
} // VKE