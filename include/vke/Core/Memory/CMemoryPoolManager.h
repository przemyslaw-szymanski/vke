#pragma once

#include "Common.h"
#include "Core/Utils/TCDynamicArray.h"
#include "Core/Threads/Common.h"

namespace VKE
{
    class VKE_API CMemoryPoolView
    {
        protected:

            struct SChunk
            {
                uint32_t offset;
                uint32_t size;
            };
            using ChunkVec = Utils::TCDynamicArray< SChunk >;
            using UintVec = Utils::TCDynamicArray< uint32_t >;

        public:

            static const uint64_t   INVALID_ALLOCATION = UNDEFINED_U64;

            struct SAllocateData
            {
                uint64_t    memory;
                uint32_t    offset;
                uint32_t    size;
            };

            struct SInitInfo
            {
                uint64_t    memory;
                uint32_t    offset;
                uint32_t    size;
                uint16_t    allocationAlignment;
            };

        public:

            Result      Init(const SInitInfo& Info);

            uint64_t    Allocate( const SAllocateMemoryInfo& Info, SAllocateData* pOut );
            void        Free( const SAllocateData& Data );

        protected:

            uint32_t    _FindFree( uint32_t size );

        protected:

            SInitInfo   m_InitInfo;
            SChunk      m_MainChunk;
            ChunkVec    m_vFreeChunks;
            UintVec     m_vFreeChunkSizes;
            UintVec     m_vFreeChunkOffsets;
    };
} // VKE