#pragma once

#include "Common.h"
#include "Core/Utils/TCDynamicArray.h"
#include "Core/Threads/Common.h"

namespace VKE
{
    namespace Memory
    {
        //class VKE_API CMemoryPoolManager
        //{
        //    public:

        //        struct SMemoryChunk
        //        {
        //            uint32_t    offset;
        //            uint32_t    size;
        //        };

        //    protected:

        //        using MemoryChunkVec = Utils::TCDynamicArray< SMemoryChunk >;
        //        using MemoryChunkVecMap = vke_hash_map< uint32_t, MemoryChunkVec >;
        //        using MemoryChunkMap = vke_hash_map< uint64_t, SMemoryChunk* >;
        //        using UintVec = Utils::TCDynamicArray< uint32_t >;

        //        struct SFreeMemoryData
        //        {
        //            MemoryChunkVec  vChunks;
        //            UintVec         vChunkSizes;
        //        };

        //        struct SMemoryPool
        //        {
        //            SFreeMemoryData     FreeMemory;
        //            uint64_t            memory;
        //            uint32_t            size;
        //            uint32_t            allocationAlignment;
        //        };
        //        using MemoryBufferMap = vke_hash_map< handle_t, SMemoryPool >;

        //        struct SAllocatedChunk
        //        {
        //            SMemoryPool*    pPool;
        //        };
        //        using AllocatedChunkMap = vke_hash_map< uint64_t, SAllocatedChunk >;

        //        enum FIND_FREE_METHOD
        //        {
        //            BEST_FIT,
        //            FIRST_FREE
        //        };

        //        struct SFindInfo
        //        {
        //            SMemoryPool*        pPool;
        //            uint32_t            size;
        //            FIND_FREE_METHOD    findMethod;
        //        };

        //    public:

        //        struct SAllocateInfo
        //        {
        //            handle_t    hPool;
        //            uint32_t    size;
        //        };

        //        struct SAllocatedData
        //        {
        //            SMemoryChunk        Memory;
        //            uint32_t            type;
        //            uint32_t            index;
        //        };
        //        using AllocatedMemoryMap = vke_hash_map< uint64_t, SAllocatedData >;
        //        using AllocatedMemoryVec = Utils::TCDynamicArray< SAllocatedData, 1 >;

        //        struct SPoolAllocateInfo
        //        {
        //            uint64_t memory = 0;
        //            uint32_t type = 0;
        //            uint32_t size = 0;
        //            uint32_t alignment = 0;           
        //        };

        //        struct SCache
        //        {
        //            SMemoryPool*    pPool; // last used pool
        //            handle_t        hPool; // last used pool handle
        //        };
        //        using CacheVec = Utils::TCDynamicArray< SCache >;

        //    public:

        //        CMemoryPoolManager();
        //        ~CMemoryPoolManager();

        //        Result      Create(const SMemoryPoolManagerDesc& Desc);
        //        void        Destroy();

        //        uint64_t    Allocate(const SAllocateInfo& Info, SAllocatedData* pOut);
        //        Result      Free(uint64_t memory);
        //        Result      CreatePool(const SPoolAllocateInfo& Info);

        //    protected:

        //        uint32_t    _FindFreeMemory(const SFindInfo& Info);
        //        Result      _AddFreeChunk(const SMemoryChunk& Chunk, SFreeMemoryData* pOut);
        //        void        _MergeFreeChunks(SFreeMemoryData* pOut);

        //    protected:

        //        MemoryBufferMap     m_mMemoryPools;
        //};
    } // Memory

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

            uint64_t    Allocate( uint32_t size, SAllocateData* pOut );

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