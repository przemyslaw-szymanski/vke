#include "Core/Memory/CMemoryPoolManager.h"

namespace VKE
{
    namespace Memory
    {
        uint32_t CalcAlignedSize( uint32_t size, uint32_t alignment )
        {
            return size + ( size % alignment );
        }

        Result CMemoryPoolManager::_FindFreeMemory(const SAllocateInfo& Info, SMemoryChunk** ppOut)
        {
            auto& Buffers = m_vmvMemoryBuffers[ Info.type ][ Info.index ];
            auto& vFreeChunkSizes = Buffers.FreeMemory.vChunkSizes;
            uint32_t minIdx = UINT32_MAX;
            uint32_t minDiff = UINT32_MAX;
            Result res = VKE_ENOTFOUND;
            const uint32_t alignedSize = CalcAlignedSize( Info.size, Info.alignment );

            const uint32_t count = vFreeChunkSizes.GetCount();
            for( uint32_t i = 0; i < count; ++i )
            {
                const uint32_t freeSize = vFreeChunkSizes[ i ];
                if( freeSize <= alignedSize )
                {
                    const uint32_t diff = alignedSize - Info.size;
                    if( diff < minDiff )
                    {
                        minDiff = diff;
                        minIdx = i;
                    }
                }
            }
            if( minIdx < UINT32_MAX )
            {
                *ppOut = &Buffers.FreeMemory.vChunks[ minIdx ];
                res = VKE_OK;
            }
            return res;
        }   

        Result CMemoryPoolManager::Allocate(const SAllocateInfo& Info, SAllocatedData* pOut)
        {
            SMemoryChunk* pMemChunk = nullptr;
            Result res = _FindFreeMemory( Info, &pMemChunk );
            if( VKE_SUCCEEDED( res ) )
            {
                const uint32_t alignedSize = CalcAlignedSize( Info.size, Info.alignment );
                pOut->pMemory = pMemChunk;
                pOut->type = Info.type;
                pOut->index = Info.index;

                uint64_t memPtr = pMemChunk->memory + pMemChunk->offset;
                m_mAllocatedData.insert( { memPtr, *pOut } );

                pMemChunk->offset += alignedSize;
                pMemChunk->size -= alignedSize;
            }
            else
            {
                res = VKE_ENOMEMORY;
            }
            return res;
        }

        Result CMemoryPoolManager::_AddFreeChunk(const SMemoryChunk& Chunk, SFreeMemoryData* pOut)
        {
            Result res = VKE_ENOMEMORY;
            if( pOut->vChunks.PushBack( Chunk ) != Utils::INVALID_POSITION )
            {
                if( pOut->vChunkSizes.PushBack( Chunk.size ) != Utils::INVALID_POSITION )
                {
                    res = VKE_OK;
                }
            }
            return res;
        }

        void CMemoryPoolManager::_MergeFreeChunks(SFreeMemoryData* pOut)
        {

        }

        Result CMemoryPoolManager::AllocatePool(const SPoolAllocateInfo& Info)
        {
            Result res = VKE_ENOMEMORY;
            auto& MemBuffers = m_vmvMemoryBuffers[ Info.type ][ Info.index ];
            MemBuffers.index = Info.index;
            {
                SMemoryBuffer Buffer;
                Buffer.memory = Info.memory;
                Buffer.size = Info.size;
                if( MemBuffers.vBuffers.PushBack( Buffer ) != Utils::INVALID_POSITION )
                {
                    SMemoryChunk FreeChunk;
                    FreeChunk.memory = Info.memory;
                    FreeChunk.offset = 0;
                    FreeChunk.size = Info.size;
                    res = _AddFreeChunk( FreeChunk, &MemBuffers.FreeMemory );
                }
            }
            return res;
        }

        Result CMemoryPoolManager::Free(uint64_t memory)
        {
            Result res = VKE_ENOTFOUND;
            auto Itr = m_mAllocatedData.find(memory);
            if( Itr != m_mAllocatedData.end() )
            {
                SAllocatedData& Data = Itr->second;
                auto& Buffers = m_vmvMemoryBuffers[ Data.type ][ Data.index ];
                res = _AddFreeChunk( *Data.pMemory, &Buffers.FreeMemory );
            }
            return res;
        }

    } // Memory
} // VKE