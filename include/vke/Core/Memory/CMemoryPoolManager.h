#pragma once

#include "Core/VKECommon.h"
#include "Core/Utils/TCDynamicArray.h"

namespace VKE
{
    namespace Memory
    {
        class CMemoryPoolManager
        {
            struct SMemoryChunk
            {
                uint64_t    memory;
                uint32_t    size;
                uint32_t    index;
            };
            using MemoryChunkVec = Utils::TCDynamicArray< SMemoryChunk >;
            using MemoryChunkVecMap = vke_hash_map< uint32_t, MemoryChunkVec >;

            struct SMemoryBuffer
            {
                uint64_t    memory;
                uint32_t    size;
            };
            using MemoryBufferVec = Utils::TCDynamicArray< SMemoryBuffer >;
            using MemoryBufferVecMap = vke_hash_map< uint32_t, MemoryBufferVec >;

            struct SMemoryBufferData
            {
                MemoryBufferVec     vBuffers;
                MemoryChunkVec      vFreeChunks;
                uint32_t            index;
            };
            using MemoryBufferDataVec = Utils::TCDynamicArray< SMemoryBufferData >;
            using MemoryBufferDataVecMap = vke_hash_map< uint32_t, MemoryBufferDataVec >;
            using MemoryBufferVec = Utils::TCDynamicArray< MemoryBufferDataVecMap >;

            public:

                struct SAllocateInfo
                {
                    uint32_t    size;
                    uint32_t    alignment;
                    uint16_t    type;
                };

                struct SAllocateData
                {
                    uint64_t    memory;
                    uint32_t    offset;
                };

            public:

                Result Allocate(const SAllocateInfo& Info, SAllocateData* pOut);

            protected:

            protected:

                MemoryBufferVec     m_vMemoryBuffers;
        };
    } // Memory
} // VKE